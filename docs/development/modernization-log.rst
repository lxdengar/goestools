Modernization Log
=================

The older ``ubuntu_changes.txt`` file records the Ubuntu/CMake modernization
work and the errors encountered while updating the build.

Summary:

- Added C/C++ standard settings.
- Switched pthread linkage to ``Threads::Threads``.
- Switched zlib linkage to ``ZLIB::ZLIB``.
- Updated libaec include/link behavior for the vendored CMake targets.
- Added WSL/Ubuntu and Docker build helpers.
- Added the non-destructive smoke build helper.

The historical text remains in ``ubuntu_changes.txt`` for now. Future
modernization notes should be summarized here and linked to specific commits or
pull requests when available.
