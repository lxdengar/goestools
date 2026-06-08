Roadmap
=======

``goestools-station`` is being built in small phases so it remains a wrapper
around the existing command-line receiver and processor tools.

Phases
------

1. App skeleton and optional CMake target.
2. Process supervision.
3. Live and file-backed logs.
4. Product catalog table.
5. Runtime stats monitoring.
6. Persistent settings and process profiles.
7. Operator polish such as filters, charts, and stronger error states.

Development Priorities
----------------------

- Keep normal command-line builds independent of Qt.
- Prefer external-process supervision over embedding decoder behavior in the
  GUI.
- Use nanomsg stats publishers for receiver health instead of scraping console
  output when possible.
- Use generated product directories and logs as station app inputs.
- Keep changes outside the core DSP, demodulator, Viterbi, Reed-Solomon, packet
  assembly, LRIT, EMWIN, and DCS paths unless a core change is explicitly
  requested.
