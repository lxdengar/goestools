Monitoring
==========

The Monitor tab should subscribe to ``goesrecv`` runtime stats and display
receiver health.

Architecture
------------

Recommended components:

- ``StatsSubscriber``: one worker-thread subscriber per endpoint.
- ``StatsMessage``: normalized parsed message with source, receive time, and
  numeric fields.
- ``StatsHistory``: bounded rolling in-memory history.
- ``HealthSummary``: display metrics computed from history.
- ``MonitorTab``: Qt Widgets UI for health state and trends.

Do not read nanomsg sockets on the GUI thread.

Default endpoints:

.. code-block:: text

   tcp://127.0.0.1:6001  demodulator stats
   tcp://127.0.0.1:6002  decoder stats

Rolling History
---------------

Keep short in-memory history, such as five minutes, and drop samples older than
the retention cutoff. Use station receive time for window membership and retain
payload timestamps for diagnostics.

Expected metrics:

- AGC gain.
- Frequency correction.
- Clock recovery omega.
- Packet rate.
- Drop rate.
- Viterbi error average.
- Reed-Solomon correction summary.
- Drop percentage.
- Uncorrectable packet count.
- Skipped-symbol count.
- Time since last demodulator and decoder stats messages.

Failure Modes
-------------

The UI should handle ``goesrecv`` not running, missing endpoints, malformed
JSON, one stream present without the other, receiver restarts, and stale data.
It should keep the last known values visible while clearly marking stale or
disconnected states.
