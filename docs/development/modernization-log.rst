Modernization Log
=================

The Ubuntu/CMake modernization work and the errors encountered while updating
the build are preserved in :doc:`ubuntu-modernization-notes`.

Summary:

- Added C/C++ standard settings.
- Switched pthread linkage to ``Threads::Threads``.
- Switched zlib linkage to ``ZLIB::ZLIB``.
- Updated libaec include/link behavior for the vendored CMake targets.
- Added WSL/Ubuntu and Docker build helpers.
- Added the non-destructive smoke build helper.

Future modernization notes should be summarized here and linked to specific
commits, pull requests, or detailed development pages when available.
