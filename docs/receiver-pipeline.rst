Receiver Pipeline
=================

The live receive pipeline is:

.. code-block:: text

   radio or sample source
     -> goesrecv demodulator
     -> goesrecv decoder
     -> 892-byte VCDU packet stream
     -> goesproc, goeslrit, goesemwin, or goespackets
     -> LRIT files, EMWIN files, images, text, or recorded packet files

Major Executables
-----------------

``goesrecv``
  Demodulates a configured input source and decodes valid packets. The main
  ownership files are ``src/goesrecv/goesrecv.cc``,
  ``src/goesrecv/demodulator.cc``, ``src/goesrecv/decoder.cc``, and
  ``src/decoder/packetizer.cc``.

``goespackets``
  Relays, filters, republishes, and records decoded VCDU packets.

``goeslrit``
  Assembles decoded VCDU packets into LRIT files through the assembler code
  under ``src/assembler``.

``goesemwin``
  Extracts EMWIN products from decoded packets.

``goesproc``
  Processes packet streams or LRIT files into user-facing images and text.

Shared Boundaries
-----------------

The most important external boundary is the 892-byte VCDU packet. It is
published by ``goesrecv`` and consumed by ``goesproc``, ``goeslrit``,
``goesemwin``, and ``goespackets``.

For wrapper and dashboard work, prefer integrating at existing packet,
stats-publisher, product-output, and process-log boundaries.
