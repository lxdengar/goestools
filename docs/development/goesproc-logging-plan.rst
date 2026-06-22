goesproc Logging Improvement Plan
=================================

This document describes the design and phased implementation of more useful
``goesproc`` logs for receiver operators and future monitoring tools.

Implementation Status
---------------------

The initial implementation now includes:

- text and JSON Lines event formatting;
- log-level, format, summary-interval, and progress command-line controls;
- startup, first-input, shutdown, and periodic summary events;
- one-time GOES-R spacecraft detection;
- product start, segment, completion, and incomplete-product events;
- false-color waiting, duplicate, time-mismatch, and completion events;
- checked output writes with written, skipped, and failed events;
- structured, rate-limited VCDU counter-gap events through an optional
  assembler callback;
- a logging smoke test under ``test/goesproc-logging``.

The remaining shared-assembler diagnostics (CRC, TP_PDU, and S_PDU events) are
still emitted through their established stderr messages. Converting those
messages should remain a later, focused change because the assembler is shared
by multiple executables.

Goals
-----

The improved logs should let an operator answer these questions quickly:

- Is ``goesproc`` running with the expected configuration and input source?
- Which spacecraft is supplying the products being processed?
- Which product, region, and channel is currently being assembled?
- Was an image written, skipped, incomplete, or rejected?
- Are VCDU gaps or LRIT assembly failures preventing complete images?
- Is false-color output waiting for or missing one of its source channels?
- How many packets, products, and files have been processed recently?
- Can the station app or another monitor consume the same events reliably?

Non-Goals
---------

The first implementation should not:

- change receiver, demodulator, Viterbi, Reed-Solomon, VCDU, LRIT, or image
  processing behavior;
- add a large third-party logging framework;
- log every handler mismatch or every incoming VCDU at the default level;
- hard-code operational role names such as ``GOES-West`` into product identity;
- turn normal packet progress into an unbounded systemd journal stream;
- make JSON logging mandatory for ordinary command-line use.

Current Behavior
----------------

Logging is currently distributed across several components:

``src/goesproc/goesproc.cc``
  Reports configuration and option errors, but does not print a startup
  summary.

``src/goesproc/packet_processor.cc``
  Shows a single updating VCDU line when standard output is a terminal. The
  line contains SCID, VCID, and the VCDU counter, but not the product
  spacecraft name.

``src/goesproc/handler_goesr.cc``
  Parses the ancillary ``Satellite`` value, product, region, channel, frame
  time, segmentation details, and image identifier. These fields are used for
  filtering and assembly but are not logged. Replacement of an incomplete
  segmented image currently has a logging TODO.

``src/goesproc/file_writer.cc``
  Prints ``Writing:`` or ``Skipping (file exists):`` followed by a path and
  optional elapsed time. The message does not identify the product separately,
  and some write results are not checked explicitly.

``src/assembler/virtual_channel.cc``
  Reports VCDU counter gaps, CRC failures, sequence gaps, and incomplete
  transport/session data. These messages have useful details but no timestamp,
  severity, or common event shape. This assembler is shared with other tools,
  so it should not be refactored as part of the first logging change.

Logging Model
-------------

Use a small ``goesproc``-owned logging helper rather than a new dependency. A
log event should have:

- UTC timestamp;
- severity: ``debug``, ``info``, ``warning``, or ``error``;
- stable event name;
- concise human-readable message;
- optional structured fields.

Example text event:

.. code-block:: text

   2026-06-21T18:42:10Z INFO product_written spacecraft=G18 product=CMIP region=FD channel=CH13 path=/home/pi/goes-data/goes18/fd/... duration_ms=421

The event name and field names should remain stable even if the prose changes.
Values containing whitespace must be quoted or escaped consistently.

Log Levels
~~~~~~~~~~

``error``
  A requested operation failed, such as invalid configuration, an unreadable
  input, or a failed output write.

``warning``
  Processing continues, but data was lost or a product could not complete.
  Examples include VCDU gaps, CRC failures, incomplete image replacement, and
  false-color source mismatch.

``info``
  Low-volume lifecycle events: startup, first spacecraft detection, completed
  products, files written or skipped, and periodic summaries.

``debug``
  High-volume diagnostic events: individual VCDUs, segment arrival, handler
  filtering, false-color waiting state, and detailed assembly progress.

Default output should be ``info``. Routine non-matches must not be logged at
``info`` because every assembled LRIT file is offered to multiple handlers.

Command-Line Interface
----------------------

Add backward-compatible options to ``goesproc``:

.. code-block:: text

   --log-level quiet|error|warning|info|debug
   --log-format text|json
   --summary-interval SEC
   --no-progress

Recommended defaults:

- ``--log-level info``;
- ``--log-format text``;
- ``--summary-interval 60``;
- terminal packet progress enabled only when standard output is a TTY.

``quiet`` should suppress routine output while preserving fatal errors. A
summary interval of ``0`` should disable periodic summaries. Existing commands
must continue to work without specifying any new option.

Startup Events
--------------

After configuration validation, print one startup event containing:

- process version;
- mode: ``packet`` or ``lrit``;
- configuration path;
- subscription endpoint or input paths;
- output directory;
- overwrite setting;
- number of configured handlers;
- configured image origins, such as ``G16`` and ``G18``;
- log level, format, and summary interval.

Example:

.. code-block:: text

   2026-06-21T18:40:00Z INFO started mode=packet source=tcp://127.0.0.1:5004 output=/home/pi/goes-data handlers=12 spacecraft=G16,G18

Logging a subscription endpoint means that ``goesproc`` attempted to subscribe;
it must not claim that packets are flowing until the first packet arrives.
Emit a separate ``input_active`` event when the first VCDU is read.

Spacecraft Detection
--------------------

GOES-R LRIT ancillary text includes a ``Satellite`` value such as ``G18``.
Expose this existing value through a read-only ``GOESRProduct`` accessor. Emit
``spacecraft_detected`` once per spacecraft per process run when a product
passes the spacecraft filter:

.. code-block:: text

   2026-06-21T18:40:07Z INFO spacecraft_detected spacecraft=G18

Use ``G18`` as the stable identity. Do not encode ``GOES-West`` into the event,
because operational roles can move to another spacecraft. User documentation
or a dashboard may display the current role separately.

Product Lifecycle Events
------------------------

For GOES-R image products, make the following fields available to logging:

- spacecraft string and numeric ID;
- product short name;
- region;
- channel;
- frame start time;
- segmented flag;
- image identifier;
- expected segment count;
- received segment count.

Emit these events:

``product_started`` (debug)
  First accepted segment for an image identifier.

``segment_received`` (debug)
  Segment number and current/expected segment counts. This is intentionally
  debug-only.

``product_complete`` (info)
  All required segments are present, or an unsegmented product is ready.

``product_incomplete`` (warning)
  A new image identifier replaced an incomplete image. Include received and
  expected segment counts so the operator can distinguish one missing segment
  from severe loss.

Example:

.. code-block:: text

   2026-06-21T18:42:09Z WARNING product_incomplete spacecraft=G18 product=CMIP region=FD channel=CH13 image_id=381 received_segments=9 expected_segments=10

Do not emit a warning merely because a product does not match a handler. Track
non-matches in debug counters if they are useful during development.

False-Color Events
------------------

False-color output depends on two complete source channels from the same frame
time. Add events for:

``false_color_waiting`` (debug)
  One source channel is complete and is waiting for its partner.

``false_color_duplicate`` (debug)
  The same channel arrived twice for one pending pair.

``false_color_time_mismatch`` (warning)
  The second channel belongs to a different frame time, so the older pending
  pair cannot complete.

``false_color_complete`` (info)
  Both channels were paired and the synthesized image is ready to write.

Include spacecraft, region, both channel names, and both frame times where
applicable.

Output Events And Error Checks
------------------------------

Replace ambiguous writer messages with stable events while preserving the
path-first usefulness of current output:

``output_written`` (info)
  Include path, file type, dimensions when applicable, elapsed time, and final
  file size when inexpensive to obtain.

``output_skipped`` (info)
  File already exists and ``--force`` was not selected.

``output_failed`` (error)
  Directory creation, image encoding, file opening, writing, flushing, or
  closing failed.

Check and report the return value of ``cv::imwrite``. Check stream state after
binary/text/JSON writes. A failed write must not be reported as successful.
Whether a failed output should stop the process or continue should be decided
explicitly; the recommended first behavior is to log an error and continue
with later products unless the output directory itself is unusable.

Product metadata should be passed to the logging call by the handler. Avoid
making the generic ``FileWriter`` parse paths to infer spacecraft or product
identity.

VCDU And Assembly Diagnostics
-----------------------------

Retain the existing VCDU counter-gap calculation: the 24-bit counter is tracked
per virtual channel and wraparound is supported. Improve the message shape to
include:

- VCID;
- previous and current counters;
- inferred number of missing VCDUs;
- cumulative missing count;
- timestamp and warning severity.

Example:

.. code-block:: text

   2026-06-21T18:41:03Z WARNING vcdu_gap vcid=1 lost=2 previous=100 current=103 total_lost=2

Assembler diagnostics also need stable events for CRC failure, transport PDU
sequence gaps, session PDU loss, and discarded partial data.

Because the assembler is shared, implement this in two steps:

1. Leave existing assembler messages unchanged during the initial
   ``goesproc`` logging work.
2. Later add an optional event callback or diagnostics sink to the assembler.
   Preserve existing stderr messages when no sink is supplied so ``goeslrit``
   and other callers do not change unexpectedly.

Do not make the shared assembler depend on a ``goesproc`` logger.

Periodic Summaries
------------------

Emit a low-volume summary at the configured interval. Suggested counters:

- VCDUs read;
- VCDUs inferred missing;
- CRC failures;
- LRIT files assembled;
- products matched;
- products completed;
- products abandoned incomplete;
- false-color products completed or abandoned;
- outputs written, skipped, and failed;
- bytes written;
- spacecraft observed during the interval.

Example:

.. code-block:: text

   2026-06-21T18:42:00Z INFO summary interval_s=60 vcdu=3420 lost=0 lrit=84 matched=26 complete=18 incomplete=0 written=18 skipped=0 failed=0 spacecraft=G18

Counters in each summary should cover the preceding interval. If lifetime
totals are also included, distinguish them with a ``total_`` prefix.

Structured JSON Output
----------------------

``--log-format json`` should produce one JSON object per line. Every object
should include:

.. code-block:: json

   {
     "timestamp": "2026-06-21T18:42:10Z",
     "level": "info",
     "event": "product_written",
     "spacecraft": "G18",
     "product": "CMIP",
     "region": "FD",
     "channel": "CH13",
     "path": "/home/pi/goes-data/goes18/fd/example.png",
     "duration_ms": 421
   }

Requirements:

- one complete object per line;
- no ANSI control sequences or carriage-return progress lines;
- numeric values encoded as numbers rather than strings;
- absent fields omitted rather than populated with misleading defaults;
- event and field names documented and treated as a compatibility surface;
- human text format and JSON format generated from the same event data.

This format can feed the station app, journald collectors, or lightweight
external monitoring without scraping human prose.

Terminal And Service Behavior
-----------------------------

Interactive packet progress should remain a single updating line when standard
output is a TTY and text logging is selected. It should be disabled when:

- output is redirected;
- running under a systemd service without a TTY;
- JSON logging is selected;
- ``--no-progress`` is supplied.

Before printing a normal event while the progress line is active, clear or end
the progress line cleanly. Never emit ANSI erase sequences into files,
journald, or JSON output.

Send informational events to standard output and warnings/errors to standard
error in text mode. Document this behavior so service and shell users know how
to capture both streams.

Rate Limiting
-------------

Repeated assembly warnings can overwhelm useful information during poor
reception. Add conservative rate limiting for repeated events with identical
keys, such as the same VCID and failure type. When suppression ends, report the
number of suppressed events:

.. code-block:: text

   2026-06-21T18:43:00Z WARNING vcdu_gap vcid=1 suppressed=37

Never suppress the first occurrence, fatal errors, startup events, output
failures, or periodic summary counters.

Implementation Phases
---------------------

Phase 1: Product identity and startup
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Add the small ``goesproc`` logging helper and text formatter.
- Add CLI log-level and progress controls.
- Print startup and first-input events.
- Expose read-only GOES-R spacecraft metadata.
- Emit one-time spacecraft detection, product completion, output written, and
  output skipped events.
- Preserve existing processing and filenames.

Phase 2: Product-loss diagnostics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Log incomplete segmented-image replacement.
- Add false-color waiting, mismatch, duplicate, and completion events.
- Check all output-write results and emit ``output_failed``.
- Add product and writer counters.

Phase 3: Summaries and structured output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Add periodic interval summaries.
- Add JSON Lines formatting from the same event objects.
- Document the event schema.
- Add rate limiting for repetitive warnings.

Phase 4: Shared assembler integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Define an optional assembler diagnostics callback or sink.
- Convert VCDU gap, CRC, TP_PDU, and S_PDU diagnostics into structured events
  when a sink is present.
- Preserve legacy stderr behavior for callers that do not provide a sink.
- Feed assembler counters into ``goesproc`` summaries.

Likely Files
------------

The implementation is expected to remain primarily in:

``src/goesproc/options.h`` and ``src/goesproc/options.cc``
  New command-line logging options.

``src/goesproc/goesproc.cc``
  Logger initialization and startup events.

``src/goesproc/packet_processor.h`` and ``packet_processor.cc``
  First-input event, progress coordination, and interval counters.

``src/goesproc/handler_goesr.h`` and ``handler_goesr.cc``
  Read-only spacecraft/segment metadata and product lifecycle events.

``src/goesproc/file_writer.h`` and ``file_writer.cc``
  Write-result checking and output events.

New ``src/goesproc/log.h`` and ``log.cc`` files
  Small event model plus text/JSON formatting, if direct local helpers become
  too repetitive. Avoid a larger abstraction until the event list proves it
  useful.

``src/assembler/virtual_channel.*``
  Optional diagnostics sink only in the later shared-assembler phase.

Tests And Verification
----------------------

Add focused tests or fixtures for:

- text event formatting and escaping;
- JSON output parsing, types, and one-object-per-line behavior;
- log-level filtering;
- one-time spacecraft detection for G16, G18, and G19 metadata;
- segmented product completion and incomplete replacement;
- false-color pairing, duplicate channels, and time mismatch;
- successful, skipped, and failed file writes;
- periodic versus lifetime counters;
- VCDU counter gaps including 24-bit wraparound;
- rate-limiter suppression counts;
- no ANSI sequences in redirected or JSON output.

Run regression checks with recorded VCDUs or LRIT fixtures and confirm that:

- output files and names are unchanged;
- the same products complete as before;
- default text output remains readable in a terminal;
- systemd logs contain one event per line;
- JSON logs can be consumed by a simple parser and the station-app prototype;
- receiver and decoder targets are untouched.

Documentation Work
------------------

When implementation begins, update:

- ``docs/commands/goesproc.rst`` with new options and examples;
- ``docs/configuration.rst`` with spacecraft/product field explanations;
- ``docs/guides/services.rst`` with journald and JSON logging examples;
- station-app logging documentation with the JSON event contract;
- development notes with the completed phase and verification results.

Open Decisions
--------------

Resolve these points before implementing later phases:

- Should output-write failure continue processing or terminate after the first
  failure?
- Should summaries default to interval counters only, or include lifetime
  totals as well?
- Should ``warning`` be named ``warn`` in text while remaining ``warning`` in
  JSON?
- Which event fields form a stable external interface for the station app?
- Should the assembler diagnostics sink be a callback, a small interface, or
  a counter object plus existing messages?
- Should non-GOES-R handlers expose a generic source name so EMWIN, NWS, GOES-N,
  and Himawari events share the same field shape?

Acceptance Criteria
-------------------

The logging work is complete when:

- an operator can identify the active input, configured origins, and detected
  spacecraft from logs;
- every completed or abandoned GOES-R image has an understandable event;
- output success, skip, and failure are distinguishable;
- repetitive failures remain visible without overwhelming the log;
- periodic summaries reveal whether products are completing;
- JSON output is valid, documented, and usable by the station app;
- existing product output and decoder behavior remain unchanged;
- focused tests cover event formatting and the highest-risk loss paths.
