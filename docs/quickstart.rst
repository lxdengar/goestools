Quickstart
==========

This page is the canonical path for getting ``goestools`` installed, built,
verified, and running on a Linux receiver host.

Which Repository?
-----------------

Use this modernized fork for the native Linux build helper, smoke build,
consolidated docs, prototype tools, and optional station GUI:

.. code-block:: text

   https://github.com/lxdengar/goestools

The original upstream project is:

.. code-block:: text

   https://github.com/pietern/goestools

Use upstream if you specifically need the original project state. Use this fork
for the build flow documented here.

.. important::

   The modernized build helpers currently live on the ``modern-wsl-build``
   branch. If you clone the repository's default ``main`` branch, files such as
   ``scripts/modern_build.sh`` and ``scripts/smoke_build.sh`` may be missing.

Install Dependencies
--------------------

Install the common build dependencies:

.. code-block:: sh

   sudo apt update
   sudo apt install -y \
     build-essential \
     cmake \
     git-core \
     pkg-config \
     libaec-dev \
     libopencv-dev \
     libproj-dev \
     zlib1g-dev

Install SDR development packages if this machine will build ``goesrecv`` with
hardware source support:

.. code-block:: sh

   sudo apt install -y \
     libairspy-dev \
     librtlsdr-dev

Qt is optional and is only needed for the ``goestools-station`` GUI:

.. code-block:: sh

   sudo apt install -y qt6-base-dev

Clone And Initialize Submodules
-------------------------------

For a fresh checkout:

.. code-block:: sh

   git clone --recursive --branch modern-wsl-build https://github.com/lxdengar/goestools
   cd goestools

For an existing checkout:

.. code-block:: sh

   git fetch origin
   git switch modern-wsl-build
   git submodule update --init --recursive

Verify that you are on the modern branch and have the helper scripts:

.. code-block:: sh

   git branch --show-current
   ls -la scripts/modern_build.sh scripts/smoke_build.sh

If ``git branch --show-current`` prints ``main`` or the scripts are missing,
switch to ``modern-wsl-build`` before following the rest of this guide.

Build
-----

For Ubuntu, WSL, and native Raspberry Pi builds, the recommended build entry
point is the helper script from the repository root:

.. code-block:: sh

   scripts/modern_build.sh

The helper checks required tools, creates or reuses ``build/``, configures
CMake, and builds with the detected CPU count. It does not install packages.
On systems with CMake older than 3.26, such as Raspberry Pi OS Bullseye, it
automatically uses the distro ``libaec-dev`` package instead of the vendored
``libaec`` submodule.

For Raspberry Pi-specific notes, including why not to run the legacy
``scripts/setup_raspbian.sh`` cross-compilation helper, see
:doc:`guides/raspberry-pi`.

Use an out-of-tree build:

.. code-block:: sh

   mkdir -p build
   cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
   cmake --build . -j"$(nproc)"

Verify A Clean Build
--------------------

Run the non-destructive smoke build from the repository root:

.. code-block:: sh

   scripts/smoke_build.sh

The smoke build checks that submodules are present, configures in a temporary
directory, and builds there. It does not install anything and does not touch the
normal ``build/`` directory.

Configure The Receiver
----------------------

Copy and edit the sample configs:

.. code-block:: sh

   cp etc/goesrecv.conf ~/goesrecv.conf
   cp etc/goesproc.conf ~/goesproc.conf

Edit ``~/goesrecv.conf`` for the receiver hardware, downlink mode, frequency,
gain, and packet/stats publisher addresses.

The sample config publishes decoded packets on:

.. code-block:: text

   tcp://0.0.0.0:5004

and publishes runtime stats on:

.. code-block:: text

   tcp://0.0.0.0:6001  demodulator stats
   tcp://0.0.0.0:6002  decoder stats

Run The Receiver
----------------

Start ``goesrecv``:

.. code-block:: sh

   build/src/goesrecv/goesrecv -v -i 10 -c ~/goesrecv.conf

Process live packets with ``goesproc``:

.. code-block:: sh

   cd /path/to/goestools
   mkdir -p ~/goes-data
   build/src/goesproc/goesproc \
     -c ~/goesproc.conf \
     -m packet \
     --subscribe tcp://127.0.0.1:5004 \
     --out ~/goes-data

The sample ``goesproc.conf`` uses bundled contrast-curve and lookup-table
images under ``share/wxstar``. Relative resource paths are resolved from the
directory where ``goesproc`` is started, not from the configuration file's
directory. Run the command from the repository root as shown above, or replace
those resource paths with absolute paths for services and other working
directories.

Alternatively, assemble LRIT files first:

.. code-block:: sh

   build/src/goeslrit/goeslrit \
     --all \
     --subscribe tcp://127.0.0.1:5004 \
     --out ~/goes-lrit

Optional User Services
----------------------

User-service examples are provided under ``systemd/examples``. The canonical
setup guide is :doc:`guides/services`.

Optional Station GUI
--------------------

Build the Qt station GUI only when Qt 6 development files are installed:

.. code-block:: sh

   cmake -S . -B build-station \
     -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
     -DBUILD_GOESTOOLS_STATION=ON
   cmake --build build-station --target goestools-station

Run it:

.. code-block:: sh

   build-station/apps/goestools-station/goestools-station

See :doc:`station/overview` for the station app plan and current features.
