# GOES Product Catalog Prototype

This prototype scans a `goesproc` or `goeslrit` output directory and records a
small product catalog in SQLite. It is intentionally separate from the C++
build and is not referenced by CMake.

The first version stores:

- product path and filename
- inferred product type
- timestamp, when it can be parsed from common GOES filenames
- file size
- file modification time

The catalog is meant as a starting point for dashboard/search work. It does not
modify generated products.

## Usage

Scan an output directory into a catalog database:

```sh
python3 tools/catalog/catalog_products.py ~/goes-data --database ~/goes-data/catalog.sqlite
```

Scan again after new products arrive:

```sh
python3 tools/catalog/catalog_products.py ~/goes-data --database ~/goes-data/catalog.sqlite
```

Rows are upserted by absolute path, so repeated scans refresh size, timestamp,
and type metadata for existing files.

Print a summary after scanning:

```sh
python3 tools/catalog/catalog_products.py ~/goes-data --database ~/goes-data/catalog.sqlite --summary
```

Skip the catalog database itself when it lives under the scanned output tree:

```sh
python3 tools/catalog/catalog_products.py ~/goes-data --database ~/goes-data/catalog.sqlite
```

The script already excludes the selected database path automatically.

## Product Type Inference

This is deliberately heuristic. The script currently classifies files by
extension and path/name hints:

- image: `.png`, `.jpg`, `.jpeg`, `.gif`, `.tif`, `.tiff`
- text: `.txt`, `.text`, `.md`, `.html`, `.xml`, `.json`, `.csv`
- LRIT: `.lrit` and `.lrit*`
- packet capture: `.raw`
- compressed/archive products: `.zip`, `.gz`, `.bz2`, `.xz`
- EMWIN/DCS when those strings appear in the path or filename
- unknown otherwise

Future versions can replace this with parsing of product headers or sidecar
metadata if a dashboard needs stronger classification.

## Timestamp Inference

The script tries several common timestamp forms seen in GOES product names,
including:

- `YYYYMMDDTHHMMSSZ`
- `YYYYMMDD-HHMMSS`
- `YYYYMMDDHHMMSS`
- GOES-R ABI segments such as `_s20180582358300_`

When no timestamp is found, the `timestamp_utc` column is left empty. The file's
modification time is always recorded separately as `mtime_utc`.

## SQLite Schema

The prototype creates one table:

```sql
CREATE TABLE products (
  path TEXT PRIMARY KEY,
  root TEXT NOT NULL,
  relative_path TEXT NOT NULL,
  filename TEXT NOT NULL,
  product_type TEXT NOT NULL,
  timestamp_utc TEXT,
  size_bytes INTEGER NOT NULL,
  mtime_utc TEXT NOT NULL,
  scanned_at_utc TEXT NOT NULL
);
```

Indexes are added for `timestamp_utc`, `product_type`, and `relative_path`.

Inspect the catalog with the `sqlite3` CLI:

```sh
sqlite3 ~/goes-data/catalog.sqlite \
  "select timestamp_utc, product_type, size_bytes, relative_path from products order by coalesce(timestamp_utc, mtime_utc) desc limit 20;"
```
