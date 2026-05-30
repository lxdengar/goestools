Quickstart
==========

This page is the canonical path for getting ``goestools`` installed, built,
verified, and running on a Linux receiver host.

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

   git clone --recursive https://github.com/pietern/goestools
   cd goestools

For an existing checkout:

.. code-block:: sh

   git submodule update --init --recursive

Build
-----

Use an out-of-tree build:

.. code-block:: sh

   mkdir -p build
   cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
   cmake --build . -j"$(nproc)"

From the repository root, the WSL/Ubuntu helper runs the same normal build:

.. code-block:: sh

   scripts/build_wsl.sh

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

   mkdir -p ~/goes-data
   build/src/goesproc/goesproc \
     -c ~/goesproc.conf \
     -m packet \
     --subscribe tcp://127.0.0.1:5004 \
     --out ~/goes-data

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
