.. _installation:

Installation
============

Building ``goestools`` requires Linux. For current Ubuntu and WSL setup, start
with :doc:`quickstart`; that page includes the modern fork URL, dependency
packages, helper scripts, smoke build, receiver configuration, services, and
the optional station GUI.

It can be built for x86 and ARM (with NEON).

.. note::

  Other operating systems may be fine as well but have not been
  confirmed to work. If you manage to get goestools to work on macOS,
  Windows, or something else, please reach out and share instructions,
  so they can be added to this page.

Dependencies
------------

System dependencies:

* CMake
* C++14 compiler
* libaec
* OpenCV (for image processing in goesproc)
* zlib (for decompressing EMWIN data)

Bundled dependencies (see vendor directory in repository):

* libcorrect (currently a fork with CMake related fixes)
* libaec
* nanomsg
* json
* tinytoml

Modern Linux Build
------------------

Use this repository for the modernized Ubuntu, WSL, and native Raspberry Pi
build path:

.. code-block:: sh

  git clone --recursive --branch modern-wsl-build https://github.com/lxdengar/goestools
  cd goestools

The modern helper scripts are on the ``modern-wsl-build`` branch. If you
already cloned ``main`` and do not see ``scripts/modern_build.sh``, switch
branches:

.. code-block:: sh

  git fetch origin
  git switch modern-wsl-build
  git submodule update --init --recursive

Install dependencies as described in :doc:`quickstart`, then run:

.. code-block:: sh

  scripts/modern_build.sh
  scripts/smoke_build.sh

``scripts/modern_build.sh`` performs the normal out-of-tree CMake build in
``build/``. ``scripts/smoke_build.sh`` performs a separate non-destructive
verification build in a temporary directory.

For native Raspberry Pi builds, use the same modern branch and see
:doc:`guides/raspberry-pi`. The legacy ``scripts/setup_raspbian.sh`` helper is
only for old cross-compilation workflows and is not the normal Pi build path.

Manual Build
------------

These instructions should work for both Ubuntu and Raspbian.

Install system dependencies:

.. code-block:: text

  sudo apt-get install -y \
    build-essential \
    cmake \
    git-core \
    libaec-dev \
    libopencv-dev \
    libproj-dev \
    zlib1g-dev

If you want to run goesrecv on this machine, you also have to install
the development packages of the drivers the SDRs you want to use;
``librtlsdr-dev`` for an RTL-SDR, ``libairspy-dev`` for an Airspy.

Now you can build ``goestools`` manually:

.. code-block:: text

  mkdir build
  cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
  make

Install system-wide only when you intentionally want root-managed binaries:

.. code-block:: text

  make install

The goestools executables are now available in /usr/local/bin.

Original Upstream
-----------------

The original upstream repository is:

.. code-block:: text

  https://github.com/pietern/goestools

Use the upstream repository if you specifically need the original project
state. Use this fork for the WSL/Ubuntu helper scripts and current docs.
