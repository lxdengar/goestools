Product Catalog Prototype
=========================

``tools/catalog/catalog_products.py`` scans a ``goesproc`` or ``goeslrit``
output directory and writes product metadata into SQLite.

The prototype records:

- Product path and filename.
- Inferred product type.
- Parsed timestamp when common GOES filename patterns are recognized.
- File size.
- File modification time.

Example
-------

.. code-block:: sh

   python3 tools/catalog/catalog_products.py ~/goes-data --database ~/goes-data/catalog.sqlite --summary

The script upserts rows by absolute path, so repeated scans refresh existing
metadata.

Relationship To Station App
---------------------------

The current Qt station app has an in-memory ``ProductScanner`` and
``ProductCatalogModel``. This Python SQLite tool is a prototype for future
persistent catalog/search work.
