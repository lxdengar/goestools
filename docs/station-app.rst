goestools Station App Plan
==========================

``goestools-station`` is the planned Qt GUI for this repository. It should wrap
the existing command-line tools rather than replacing receiver or decoder
internals.

Goals
-----

- Provide one desktop UI for common station workflows.
- Supervise external ``goestools`` processes.
- Display receiver health.
- Browse generated products.
- Show logs.
- Keep normal command-line builds independent of Qt.

Tabs
----

- Status
- Processes
- Monitor
- Catalog
- Logs
- Settings

Phases
------

1. App skeleton and optional CMake target.
2. Process supervision.
3. Live and file-backed logs.
4. Product catalog table.
5. Runtime stats monitoring.
6. Persistent settings and process profiles.
7. Operator polish such as filters, charts, and stronger error states.

Integration Boundaries
----------------------

Preferred integration points are external processes, nanomsg stats publishers,
packet/product output directories, logs, and config files. Avoid changing core
DSP, decoder, packetizer, Viterbi, Reed-Solomon, LRIT, EMWIN, and DCS behavior
for station UI work.
