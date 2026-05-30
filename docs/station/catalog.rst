Catalog
=======

The Catalog tab scans a generated output directory and displays products in a
Qt table model.

Current behavior:

- Recursively scan a configured directory.
- Display filename.
- Display relative path.
- Display modified time.
- Display size.
- Display inferred file type.

The current station catalog is in-memory only and does not require SQLite.
Future work can add persistence, filtering, thumbnails, and integration with
``tools/catalog/catalog_products.py``.
