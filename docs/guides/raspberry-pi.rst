Raspberry Pi Build Notes
========================

These notes cover building the modern ``goestools`` branch directly on a
Raspberry Pi running Raspberry Pi OS or another Debian-based ARM Linux.

Recommended Native Build
------------------------

On a Raspberry Pi, build natively. Do not run ``scripts/setup_raspbian.sh`` for
a normal Pi checkout.

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

Install SDR development packages if this Pi will build ``goesrecv`` with
Airspy or RTL-SDR support:

.. code-block:: sh

   sudo apt install -y libairspy-dev librtlsdr-dev

Clone the modern branch:

.. code-block:: sh

   git clone --recursive --branch modern-wsl-build https://github.com/lxdengar/goestools
   cd goestools

If you already cloned ``main`` and do not see ``scripts/modern_build.sh``, switch
to the modern branch:

.. code-block:: sh

   git fetch origin
   git switch modern-wsl-build
   git submodule update --init --recursive

Build:

.. code-block:: sh

   scripts/modern_build.sh

``scripts/modern_build.sh`` is a generic native Linux build helper. It checks
for required tools, configures ``build/``, and runs the CMake build with the
detected CPU count.

CMake And libaec
----------------

Raspberry Pi OS Bullseye ships CMake 3.18.4. The vendored ``libaec`` submodule
currently requires CMake 3.26 or newer. The build helper detects older CMake
versions and automatically configures ``goestools`` with system ``libaec``
instead:

.. code-block:: text

   -DGOESTOOLS_USE_SYSTEM_LIBAEC=ON

Make sure ``libaec-dev`` is installed. If configure reports that system
``libaec`` development files are missing, run:

.. code-block:: sh

   sudo apt install -y libaec-dev

Then rerun:

.. code-block:: sh

   scripts/modern_build.sh

Manual Build
------------

The equivalent manual build is:

.. code-block:: sh

   git submodule update --init --recursive
   mkdir -p build
   cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DGOESTOOLS_USE_SYSTEM_LIBAEC=ON
   cmake --build . -j"$(nproc)"

Legacy Cross-Compilation Helper
-------------------------------

``scripts/setup_raspbian.sh`` is a legacy cross-compilation helper. It is not
the normal Raspberry Pi build path.

That script targets old Raspbian Stretch-era packages and Ubuntu 18.04 cross
compiler packages such as ``gcc-5-arm-linux-gnueabihf``. Modern Raspberry Pi OS
and current Ubuntu releases generally do not provide those packages, so running
the script can fail with:

.. code-block:: text

   E: Unable to locate package gcc-5-arm-linux-gnueabihf

If you are on the Pi itself, ignore that script and build natively as shown
above.
