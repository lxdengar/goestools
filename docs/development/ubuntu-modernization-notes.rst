Ubuntu Modernization Notes
==========================

These notes were migrated from the former root-level Ubuntu changes text file.
They record the Ubuntu/CMake modernization work, follow-on documentation and
tooling additions, and the build errors encountered while updating the project.

Summary
-------

Implemented a minimal CMake modernization and verified after each step.

Changed:

- ``CMakeLists.txt``: added C/C++ standard properties, Threads, ZLIB, and
  libaec generated/source include paths.
- ``src/goesrecv/CMakeLists.txt``: replaced raw pthread linkage with
  ``Threads::Threads``.
- ``src/lib/CMakeLists.txt``: replaced raw z linkage with ``ZLIB::ZLIB``.
- ``src/assembler/CMakeLists.txt``: replaced raw ``aec``/``sz`` linkage with
  ``libaec::aec``/``libaec::sz``.
- ``src/goesrecv/packet_publisher.h``: fixed the pre-existing bad include by
  using ``<cstdint>``, which was necessary to compile.
- ``scripts/build_wsl.sh``: added a repeatable WSL/Ubuntu build helper that
  checks for required tools and runs the out-of-tree CMake build without
  installing packages.
- ``docker/Dockerfile.ubuntu-dev``: added a minimal Ubuntu development image
  that can build the project from a mounted source checkout.

Verification:

- Step 1 configure/build passed after resolving pre-existing blockers from the
  dirty tree: bad ``cstdinit``, uninitialized ``vendor/tinytoml``, and updated
  libaec include/link behavior.
- Step 2 ``Threads::Threads`` configure/build passed.
- Step 3 ``ZLIB::ZLIB`` configure/build passed.
- Final executable check passed for ``goesrecv``, ``goeslrit``, ``goesproc``,
  and ``goespackets``.
- ``scripts/build_wsl.sh`` configure/build passed using the existing build
  tree.
- Dockerfile syntax/configuration was reviewed; build verification depends on
  Docker daemon availability.
- ``scripts/smoke_build.sh`` fresh temporary configure/build passed to 100%.
- ``tools/monitor/stats_subscriber.py --help`` ran successfully; Python syntax
  compile check passed.
- ``tools/catalog/catalog_products.py --help`` ran successfully.
- ``tools/catalog/catalog_products.py`` was smoke-tested against a scratch
  output tree with sample image, LRIT, text, and packet files; it created a
  SQLite catalog and inferred the expected product types/timestamps.

Notes:

- CMake still emits deprecation warnings for old minimums in top-level/vendor
  CMake files.
- OpenCV emits system-header warnings during compilation.
- Neither warning class blocks the build.

WSL Build Helper
----------------

Use the helper from the repository root:

.. code-block:: sh

   scripts/build_wsl.sh

The script installs no packages. It checks for ``cmake``, ``git``,
``pkg-config``, ``make``, a C compiler, and a C++ compiler. It then creates or
reuses the out-of-tree build directory, configures with
``CMAKE_INSTALL_PREFIX`` set to ``/usr/local`` by default, and builds with the
detected CPU count.

Optional overrides:

.. code-block:: sh

   BUILD_DIR=build-debug CMAKE_BUILD_TYPE=Debug scripts/build_wsl.sh
   CMAKE_INSTALL_PREFIX=/opt/goestools scripts/build_wsl.sh

The script is safe to run repeatedly after source changes or after a successful
build.

Docker Ubuntu Build
-------------------

Build the minimal Ubuntu development image from the repository root:

.. code-block:: sh

   docker build -f docker/Dockerfile.ubuntu-dev -t goestools-ubuntu-dev .

Run the project build by mounting the checkout:

.. code-block:: sh

   docker run --rm -v "$PWD":/workspace -w /workspace goestools-ubuntu-dev

The image default command runs ``scripts/build_wsl.sh``, which checks
prerequisites, creates or reuses ``build/``, configures CMake, and builds with
the detected CPU count. Initialize submodules in the checkout before running
the container if they are not already populated:

.. code-block:: sh

   git submodule update --init --recursive

Recent Documentation And Tooling
--------------------------------

The following changes were added after the Ubuntu/Docker build helper notes.
They are documentation, wrapper, service, and prototype-tooling additions; they
do not change the core C++ receiver/decoder path.

``AGENTS.md``
  Added repository guidance to preserve receiver/decoder behavior, prioritize
  small build and portability fixes, and prefer wrapper/dashboard/monitoring
  work before refactoring DSP, demodulation, Viterbi, Reed-Solomon, packet
  processing, or LRIT/EMWIN/DCS decoding internals.

``docs/receiver-pipeline.md``
  Added a source-tree map of the major executables, pipeline stages, packet and
  LRIT data boundaries, and the files that appear to own each stage.

``docs/stats-interface.md``
  Documented the ``goesrecv`` demodulator and decoder stats publishers, JSON
  message fields, nanomsg subscription behavior, current monitor aggregation
  behavior, and recommendations for a future monitor/dashboard.

``systemd/examples/``
  Added user-service examples for ``goesrecv`` and ``goesproc`` plus a README.
  The units use per-user paths such as ``%h/.local/bin``, ``%E/goestools``,
  and ``%h/goes-data``, and do not assume binaries or configs were installed by
  root.

``tools/monitor/``
  Added ``stats_subscriber.py`` and ``README.md``. The Python prototype
  subscribes to a ``goesrecv`` stats endpoint, parses JSON messages, and prints
  them. It is separate from the C++ build and uses ``ctypes`` with the system
  ``libnanomsg``.

``tools/catalog/``
  Added ``catalog_products.py`` and ``README.md``. The Python prototype scans a
  generated product directory, infers product type and timestamps from common
  GOES filenames, and upserts filename, path, timestamp, type, size, mtime, and
  scan time into SQLite.

``scripts/smoke_build.sh``
  Added a non-destructive smoke build helper that checks submodules, configures
  in a fresh temporary build directory, and builds without installing or
  touching the repository's normal build directory. It passes
  ``-DCMAKE_POLICY_VERSION_MINIMUM=3.5`` by default so fresh configures work
  with newer CMake versions and older vendored CMake files without editing
  vendor source.

Errors Encountered And Fixes
----------------------------

Missing ``cstdinit`` Header
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Error:

.. code-block:: text

   src/goesrecv/packet_publisher.h:4:10: fatal error: cstdinit: No such file or directory

Cause:
  The dirty worktree already had an invalid include in
  ``packet_publisher.h``.

Fix:
  Replaced ``<cstdinit>`` with ``<cstdint>``.

Missing ``szlib.h``
~~~~~~~~~~~~~~~~~~~

Error:

.. code-block:: text

   src/assembler/session_pdu.h:8:10: fatal error: szlib.h: No such file or directory

Cause:
  The updated ``vendor/libaec`` layout places ``szlib.h`` under
  ``vendor/libaec/include``, but the project only added ``vendor/libaec/src``
  to the include path.

Fix:
  Added ``vendor/libaec/include`` to the project include paths.

Missing ``toml/toml.h``
~~~~~~~~~~~~~~~~~~~~~~~

Error:

.. code-block:: text

   src/goesrecv/config.cc:7:10: fatal error: toml/toml.h: No such file or directory

Cause:
  ``vendor/tinytoml`` was present as a submodule directory but its contents
  were not checked out.

Fix:

.. code-block:: sh

   git submodule update --init vendor/tinytoml

Missing ``libaec.h`` From ``szlib.h``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Error:

.. code-block:: text

   vendor/libaec/include/szlib.h:41:10: fatal error: libaec.h: No such file or directory

Cause:
  Current libaec generates ``libaec.h`` into the build tree at
  ``build/vendor/libaec/include/libaec.h``.

Fix:
  Added ``${PROJECT_BINARY_DIR}/vendor/libaec/include`` to the project include
  paths.

Raw libaec Linker Names
~~~~~~~~~~~~~~~~~~~~~~~

Error:

.. code-block:: text

   /usr/bin/x86_64-linux-gnu-ld.bfd: cannot find -laec: No such file or directory
   /usr/bin/x86_64-linux-gnu-ld.bfd: cannot find -lsz: No such file or directory

Cause:
  The updated vendored libaec CMake exports targets instead of relying on raw
  library names being discoverable through the link path.

Fix:
  Replaced assembler linkage from raw ``aec sz`` to
  ``libaec::aec libaec::sz``.

Verification Commands
---------------------

Configured and built out of tree:

.. code-block:: sh

   mkdir -p build
   cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
   cmake --build . -j$(nproc)

Checked representative executables:

.. code-block:: text

   build/src/goesrecv/goesrecv
   build/src/goeslrit/goeslrit
   build/src/goesproc/goesproc
   build/src/goespackets/goespackets
