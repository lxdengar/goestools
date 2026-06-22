Configuration
=============

The primary runtime configuration files are:

``etc/goesrecv.conf``
  Example receiver, demodulator, decoder, packet publisher, and stats publisher
  configuration.

``etc/goesproc.conf``
  Example product processing handlers for images, EMWIN, text, and related
  products.

Receiver Configuration
----------------------

``goesrecv`` reads TOML configuration. Important sections include:

``[demodulator]``
  Selects downlink mode and sample source.

``[airspy]`` / ``[rtlsdr]`` / ``[nanomsg]``
  Configures the selected sample source.

``[decoder.packet_publisher]``
  Configures where decoded 892-byte VCDU packets are published.

``[demodulator.stats_publisher]`` and ``[decoder.stats_publisher]``
  Configure runtime stats endpoints for monitoring and dashboards.

Product Processing Configuration
--------------------------------

``goesproc`` uses handler sections to decide which LRIT products to process and
where to write output. The sample file includes examples for:

- GOES-R images.
- GOES-N images.
- EMWIN products.
- NWS images.
- NWS text.
- Generic text.

Image remap, lookup-table, and map paths are resolved relative to the process
working directory, not relative to the configuration file. The repository
sample references bundled files under ``./share/wxstar`` and therefore works
when ``goesproc`` is started from the repository root. Use absolute paths when
running it from another directory or as a service.

GOES-West Satellite Naming
~~~~~~~~~~~~~~~~~~~~~~~~~~

``GOES-West`` is an operational role rather than a permanent spacecraft name.
GOES-17 previously served in that role, but NOAA replaced it with GOES-18.
Current GOES-West ABI imagery therefore identifies itself as GOES-18, and its
image handlers must use:

.. code-block:: toml

   origin = "goes18"

A handler configured with ``origin = "goes17"`` only matches historical
GOES-17 products; it does not process current GOES-18 imagery. The default
``etc/goesproc.conf`` includes GOES-18 handlers and writes their output beneath
``./goes18``.

Typical Live Processing
-----------------------

``goesrecv`` publishes packets:

.. code-block:: toml

   [decoder.packet_publisher]
   bind = "tcp://0.0.0.0:5004"

``goesproc`` subscribes locally:

.. code-block:: sh

   goesproc -c ~/goesproc.conf -m packet --subscribe tcp://127.0.0.1:5004

Monitoring Endpoints
--------------------

The default stats endpoints are:

.. code-block:: toml

   [demodulator.stats_publisher]
   bind = "tcp://0.0.0.0:6001"

   [decoder.stats_publisher]
   bind = "tcp://0.0.0.0:6002"

See :doc:`stats-interface` and :doc:`station/monitoring` for payload details.
