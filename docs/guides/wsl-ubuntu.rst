WSL And Ubuntu Build Notes
==========================

These notes cover building ``goestools`` on Ubuntu and Ubuntu under WSL2.

Recommended Build Helper
------------------------

From the repository root:

.. code-block:: sh

   scripts/build_wsl.sh

The helper:

- Checks for required build tools.
- Creates or reuses ``build/``.
- Runs CMake configure.
- Builds with the detected CPU count.
- Installs no packages.

Optional overrides:

.. code-block:: sh

   BUILD_DIR=build-debug CMAKE_BUILD_TYPE=Debug scripts/build_wsl.sh
   CMAKE_INSTALL_PREFIX=/opt/goestools scripts/build_wsl.sh

WSL2 Notes
----------

- Ubuntu on WSL2 is suitable for normal compilation.
- Direct USB SDR access from WSL2 may need host-side USB forwarding.
- Building can succeed without testing hardware capture.
- ``goesrecv`` detects Airspy and RTL-SDR support through pkg-config modules
  ``libairspy`` and ``librtlsdr``.
- Missing SDR packages disable those source backends but do not prevent other
  tools from building.

Common Warnings
---------------

CMake may emit deprecation warnings for old top-level and vendored CMake
minimums. OpenCV may emit system-header warnings while compiling ``goesproc``.
Those warnings do not necessarily block the build.
