Settings
========

The Settings tab should eventually persist station configuration with
``QSettings``.

Candidate settings:

- Paths to ``goesrecv``, ``goesproc``, and helper scripts.
- Config file paths.
- Output directory.
- Working directory.
- Stats endpoints.
- Rolling history duration.
- Stale timeout.
- Log directory.
- Product catalog scan interval.
- Restart behavior.

Settings should default to per-user paths and should not assume root-installed
binaries or config files.
