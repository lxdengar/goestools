Stats Monitor Prototype
=======================

``tools/monitor/stats_subscriber.py`` is a small Python prototype that
subscribes to a ``goesrecv`` stats endpoint and prints parsed JSON messages.

It is separate from the C++ build and uses Python ``ctypes`` with the system
``libnanomsg`` shared library.

Examples
--------

Demodulator stats:

.. code-block:: sh

   python3 tools/monitor/stats_subscriber.py tcp://127.0.0.1:6001 --source demodulator

Decoder stats:

.. code-block:: sh

   python3 tools/monitor/stats_subscriber.py tcp://127.0.0.1:6002 --source decoder

Pretty output:

.. code-block:: sh

   python3 tools/monitor/stats_subscriber.py tcp://127.0.0.1:6001 --pretty

See also :doc:`../stats-interface` and :doc:`../station/monitoring`.
