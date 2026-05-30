Build Notes
===========

Canonical build instructions live in :doc:`../quickstart`,
:doc:`../guides/wsl-ubuntu`, :doc:`../guides/docker-ubuntu`, and
:doc:`../guides/smoke-build`.

The root ``DEVELOPMENT_NOTES.md`` file is retained as a working note, but new
user-facing build documentation should be added under ``docs/``.

Current build guidance:

- Use out-of-tree builds.
- Keep submodules initialized.
- Prefer small, reviewable portability fixes.
- Use ``scripts/smoke_build.sh`` for non-destructive verification.
