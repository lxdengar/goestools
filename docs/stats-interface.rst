Receiver Stats Interface
========================

``goesrecv`` publishes runtime stats through nanomsg PUB/SUB sockets.

Default sample endpoints:

.. code-block:: text

   tcp://127.0.0.1:6001  demodulator stats
   tcp://127.0.0.1:6002  decoder stats

Demodulator Messages
--------------------

Produced by ``src/goesrecv/demodulator.cc``:

.. code-block:: json

   {"timestamp": "2018-04-18T04:52:58.357Z","gain": 2.2494e+01,"frequency": 2.9813e+03,"omega": 1.6181e+00}

Fields:

- ``timestamp``: UTC timestamp.
- ``gain``: AGC gain multiplier.
- ``frequency``: Costas-loop frequency correction in Hz.
- ``omega``: clock recovery samples-per-symbol estimate.

Decoder Messages
----------------

Produced by ``src/goesrecv/decoder.cc``:

.. code-block:: json

   {"timestamp": "2018-04-18T04:54:22.974Z","skipped_symbols": 0,"viterbi_errors": 44,"reed_solomon_errors": 0,"ok": 1}

Fields:

- ``timestamp``: UTC timestamp.
- ``skipped_symbols``: symbols skipped during frame sync acquisition.
- ``viterbi_errors``: estimated corrected Viterbi bit errors.
- ``reed_solomon_errors``: corrected Reed-Solomon bytes, or ``-1`` when not
  correctable.
- ``ok``: nonzero when the packet was correctable and published.

Subscriber Guidance
-------------------

Subscribe to the empty nanomsg topic and parse each received nanomsg message as
one complete JSON object. Expect gaps when no subscriber is connected, during
receiver restarts, or during network reconnects.
