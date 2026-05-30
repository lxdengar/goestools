# systemd user service examples

Canonical service documentation lives in:

```text
docs/guides/services.rst
```

This directory contains example user units:

- `goesrecv.service`
- `goesproc.service`

They are intended for per-user installs and build-tree testing, and do not
assume binaries or configs were installed by root.
