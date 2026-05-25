#!/usr/bin/env python3
"""Prototype SQLite cataloger for generated GOES products.

This script is intentionally standalone and separate from the C++ build.
"""

import argparse
import re
import sqlite3
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".gif", ".tif", ".tiff"}
TEXT_EXTENSIONS = {".txt", ".text", ".md", ".html", ".xml", ".json", ".csv"}
ARCHIVE_EXTENSIONS = {".zip", ".gz", ".bz2", ".xz"}

TIMESTAMP_PATTERNS = [
    re.compile(r"(?P<value>\d{8}T\d{6}Z)"),
    re.compile(r"(?P<value>\d{8}-\d{6})"),
    re.compile(r"(?P<value>\d{14})"),
    re.compile(r"[_-]s(?P<value>\d{13})"),
]


@dataclass
class Product:
    path: Path
    root: Path
    relative_path: str
    filename: str
    product_type: str
    timestamp_utc: Optional[str]
    size_bytes: int
    mtime_utc: str
    scanned_at_utc: str


def utc_iso(dt):
    return dt.astimezone(timezone.utc).replace(microsecond=0).isoformat()


def parse_timestamp_from_name(name):
    for pattern in TIMESTAMP_PATTERNS:
        match = pattern.search(name)
        if not match:
            continue

        value = match.group("value")
        parsers = [
            ("%Y%m%dT%H%M%SZ", value),
            ("%Y%m%d-%H%M%S", value),
            ("%Y%m%d%H%M%S", value),
        ]
        if len(value) == 13:
            year = int(value[0:4])
            day_of_year = int(value[4:7])
            hour = int(value[7:9])
            minute = int(value[9:11])
            second = int(value[11:13])
            try:
                dt = datetime.strptime(
                    f"{year:04d}{day_of_year:03d}{hour:02d}{minute:02d}{second:02d}",
                    "%Y%j%H%M%S",
                )
                return utc_iso(dt.replace(tzinfo=timezone.utc))
            except ValueError:
                continue

        for fmt, candidate in parsers:
            try:
                dt = datetime.strptime(candidate, fmt)
                return utc_iso(dt.replace(tzinfo=timezone.utc))
            except ValueError:
                continue

    return None


def infer_product_type(path):
    lower_parts = [part.lower() for part in path.parts]
    lower_name = path.name.lower()
    suffix = path.suffix.lower()

    if "emwin" in lower_name or "emwin" in lower_parts:
        return "emwin"
    if "dcs" in lower_name or "dcs" in lower_parts:
        return "dcs"
    if suffix in IMAGE_EXTENSIONS:
        return "image"
    if suffix in TEXT_EXTENSIONS:
        return "text"
    if suffix == ".raw":
        return "packet-capture"
    if suffix.startswith(".lrit") or ".lrit" in lower_name:
        return "lrit"
    if suffix in ARCHIVE_EXTENSIONS:
        return "archive"
    return "unknown"


def iter_files(root, database):
    root = root.resolve()
    database = database.resolve()
    database_sidecars = {
        database,
        Path(f"{database}-journal"),
        Path(f"{database}-shm"),
        Path(f"{database}-wal"),
    }
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.resolve() in database_sidecars:
            continue
        yield path


def product_from_path(root, path, scanned_at_utc):
    stat = path.stat()
    return Product(
        path=path.resolve(),
        root=root.resolve(),
        relative_path=str(path.resolve().relative_to(root.resolve())),
        filename=path.name,
        product_type=infer_product_type(path),
        timestamp_utc=parse_timestamp_from_name(path.name),
        size_bytes=stat.st_size,
        mtime_utc=utc_iso(datetime.fromtimestamp(stat.st_mtime, timezone.utc)),
        scanned_at_utc=scanned_at_utc,
    )


def connect_database(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(path)
    db.execute("pragma journal_mode = wal")
    db.execute(
        """
        create table if not exists products (
          path text primary key,
          root text not null,
          relative_path text not null,
          filename text not null,
          product_type text not null,
          timestamp_utc text,
          size_bytes integer not null,
          mtime_utc text not null,
          scanned_at_utc text not null
        )
        """
    )
    db.execute(
        "create index if not exists idx_products_timestamp on products(timestamp_utc)"
    )
    db.execute(
        "create index if not exists idx_products_type on products(product_type)"
    )
    db.execute(
        "create index if not exists idx_products_relative_path on products(relative_path)"
    )
    return db


def upsert_product(db, product):
    db.execute(
        """
        insert into products (
          path,
          root,
          relative_path,
          filename,
          product_type,
          timestamp_utc,
          size_bytes,
          mtime_utc,
          scanned_at_utc
        ) values (?, ?, ?, ?, ?, ?, ?, ?, ?)
        on conflict(path) do update set
          root = excluded.root,
          relative_path = excluded.relative_path,
          filename = excluded.filename,
          product_type = excluded.product_type,
          timestamp_utc = excluded.timestamp_utc,
          size_bytes = excluded.size_bytes,
          mtime_utc = excluded.mtime_utc,
          scanned_at_utc = excluded.scanned_at_utc
        """,
        (
            str(product.path),
            str(product.root),
            product.relative_path,
            product.filename,
            product.product_type,
            product.timestamp_utc,
            product.size_bytes,
            product.mtime_utc,
            product.scanned_at_utc,
        ),
    )


def print_summary(db):
    print("products by type:")
    for product_type, count, total_size in db.execute(
        """
        select product_type, count(*), coalesce(sum(size_bytes), 0)
        from products
        group by product_type
        order by product_type
        """
    ):
        print(f"  {product_type:15s} {count:8d} {total_size:12d} bytes")

    latest = db.execute(
        """
        select coalesce(timestamp_utc, mtime_utc), product_type, relative_path
        from products
        order by coalesce(timestamp_utc, mtime_utc) desc
        limit 5
        """
    ).fetchall()
    if latest:
        print("\nlatest products:")
        for timestamp, product_type, relative_path in latest:
            print(f"  {timestamp} {product_type:15s} {relative_path}")


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Scan generated GOES products into a SQLite catalog."
    )
    parser.add_argument("output_dir", help="directory containing generated products")
    parser.add_argument(
        "--database",
        "-d",
        default="product-catalog.sqlite",
        help="SQLite database path (default: product-catalog.sqlite)",
    )
    parser.add_argument(
        "--summary",
        action="store_true",
        help="print a small summary after scanning",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv or sys.argv[1:])
    root = Path(args.output_dir).expanduser()
    database = Path(args.database).expanduser()

    if not root.is_dir():
        print(f"catalog_products.py: not a directory: {root}", file=sys.stderr)
        return 1

    scanned_at_utc = utc_iso(datetime.now(timezone.utc))
    db = connect_database(database)
    count = 0

    with db:
        for path in iter_files(root, database):
            product = product_from_path(root, path, scanned_at_utc)
            upsert_product(db, product)
            count += 1

    print(f"cataloged {count} files into {database}")
    if args.summary:
        print_summary(db)

    db.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
