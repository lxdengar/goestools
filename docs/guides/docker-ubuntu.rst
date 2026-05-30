Docker Ubuntu Build
===================

The repository includes a minimal Ubuntu development Dockerfile:

.. code-block:: sh

   docker build -f docker/Dockerfile.ubuntu-dev -t goestools-ubuntu-dev .

Run a build by mounting the checkout:

.. code-block:: sh

   docker run --rm -v "$PWD":/workspace -w /workspace goestools-ubuntu-dev

The container default command runs ``scripts/build_wsl.sh``. Initialize
submodules in the checkout before using the container:

.. code-block:: sh

   git submodule update --init --recursive

Scope
-----

The Docker image is for repeatable build verification. It does not replace
receiver-host setup, SDR permissions, systemd services, or station-specific
configuration.
