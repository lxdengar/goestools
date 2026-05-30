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

See :doc:`stats-interface` and :doc:`station-monitoring` for payload details.
