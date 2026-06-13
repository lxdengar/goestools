WSL And Ubuntu Build Notes
==========================

These notes cover building ``goestools`` on Ubuntu and Ubuntu under WSL2.

Fresh WSL/Ubuntu Checkout
-------------------------

Install the common build dependencies:

.. code-block:: sh

   sudo apt update
   sudo apt install -y \
     build-essential \
     cmake \
     git-core \
     pkg-config \
     libopencv-dev \
     libproj-dev \
     zlib1g-dev

Install SDR development packages if this machine will build ``goesrecv`` with
Airspy or RTL-SDR support:

.. code-block:: sh

   sudo apt install -y libairspy-dev librtlsdr-dev

Clone the modern WSL/Ubuntu fork:

.. code-block:: sh

   git clone --recursive --branch modern-wsl-build https://github.com/lxdengar/goestools
   cd goestools

If you already cloned without ``--recursive``, initialize submodules before
building:

.. code-block:: sh

   git submodule update --init --recursive

Branch Check
------------

The modern WSL/Ubuntu helper scripts are on the ``modern-wsl-build`` branch.
If a checkout is on ``main``, ``scripts/build_wsl.sh`` may not exist.

Check the current branch and scripts:

.. code-block:: sh

   git branch --show-current
   ls -la scripts/build_wsl.sh scripts/smoke_build.sh

If needed, switch branches and initialize submodules:

.. code-block:: sh

   git fetch origin
   git switch modern-wsl-build
   git submodule update --init --recursive

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
