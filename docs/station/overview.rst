Overview
========

``goestools-station`` is an optional Qt 6 Widgets application for operating a
GOES receive station around the existing command-line tools.

Goals:

- Start and supervise external ``goestools`` commands.
- Display receiver and decoder health.
- Show live process logs.
- Browse generated products.
- Keep settings in user-space paths.

The GUI is intentionally a wrapper/dashboard. It should not reimplement or
modify core DSP, demodulation, Viterbi, Reed-Solomon, packet assembly, LRIT, or
EMWIN behavior.

Current tabs:

- Status
- Processes
- Monitor
- Catalog
- Logs
- Settings

The broader plan is captured in :doc:`../station-app`.
