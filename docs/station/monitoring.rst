Monitoring
==========

The Monitor tab should subscribe to ``goesrecv`` runtime stats and display
receiver health.

Default endpoints:

.. code-block:: text

   tcp://127.0.0.1:6001  demodulator stats
   tcp://127.0.0.1:6002  decoder stats

Expected metrics:

- AGC gain.
- Frequency correction.
- Clock recovery omega.
- Packet rate.
- Drop rate.
- Viterbi error average.
- Reed-Solomon correction summary.
- Skipped-symbol count.
- Time since last demodulator and decoder stats messages.

The detailed design is in :doc:`../station-monitoring`.
