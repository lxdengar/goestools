Station Monitoring Design
=========================

The Qt station app should monitor ``goesrecv`` by subscribing to the existing
demodulator and decoder stats publishers.

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

Rolling History
---------------

Keep short in-memory history, such as five minutes, and drop samples older than
the retention cutoff. Use station receive time for window membership and retain
payload timestamps for diagnostics.

Health Metrics
--------------

Display at least:

- Gain.
- Frequency correction.
- Omega.
- Packet rate.
- Drop rate.
- Drop percentage.
- Viterbi error average.
- Reed-Solomon corrected-byte summary, excluding ``-1``.
- Uncorrectable packet count.
- Skipped-symbol count.
- Time since last demodulator and decoder messages.

Failure Modes
-------------

The UI should handle ``goesrecv`` not running, missing endpoints, malformed
JSON, one stream present without the other, receiver restarts, and stale data.
It should keep the last known values visible while clearly marking stale or
disconnected states.
