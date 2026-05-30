Smoke Build
===========

``scripts/smoke_build.sh`` is a non-destructive verification helper.

It:

- Checks that recursive submodules are checked out.
- Configures CMake in a fresh temporary directory under ``${TMPDIR:-/tmp}``.
- Builds from that temporary tree.
- Does not install anything.
- Does not touch the repository's normal ``build/`` directory.
- Leaves the temporary build directory in place for inspection.

Run it from the repository root:

.. code-block:: sh

   scripts/smoke_build.sh

Useful overrides:

.. code-block:: sh

   JOBS=4 scripts/smoke_build.sh
   CMAKE_BUILD_TYPE=Debug scripts/smoke_build.sh

The script passes ``-DCMAKE_POLICY_VERSION_MINIMUM=3.5`` by default so fresh
configures work with newer CMake versions and older vendored CMake files
without editing vendor source.
