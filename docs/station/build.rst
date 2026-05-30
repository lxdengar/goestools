Build
=====

The station app is optional and requires Qt 6 Widgets development files.

Install Qt 6 development files on Ubuntu:

.. code-block:: sh

   sudo apt install -y qt6-base-dev

Configure and build:

.. code-block:: sh

   cmake -S . -B build-station \
     -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
     -DBUILD_GOESTOOLS_STATION=ON
   cmake --build build-station --target goestools-station

Run:

.. code-block:: sh

   build-station/apps/goestools-station/goestools-station

Normal command-line builds leave the station app off and do not require Qt.
