# goestools Station App

This document describes the planned Qt GUI for this repository. The app is
intended to provide an operator-facing station dashboard around the existing
`goestools` command-line tools without changing the core receiver, decoder, or
packet-processing behavior.

## Goals

- Provide a single desktop UI for running and observing a GOES receive station.
- Keep `goesrecv`, `goesproc`, `goeslrit`, and related tools as external
  processes rather than rewriting their internals.
- Prefer process supervision, monitoring, logging, catalog browsing, and
  configuration helpers before any receiver/decoder refactors.
- Make common station workflows visible: start receiver, watch health, inspect
  logs, browse products, and adjust paths/settings.
- Keep the GUI optional. Normal command-line builds should not require Qt.

## Non-Goals

- Do not reimplement DSP, demodulation, Viterbi, Reed-Solomon, packet assembly,
  LRIT decoding, EMWIN extraction, or product generation in the GUI.
- Do not replace the existing CLI tools or service-based operation.
- Do not require SQLite for the first product catalog pass.
- Do not require root install paths.

## Application Structure

The app lives under `apps/goestools-station` and builds as the optional CMake
target `goestools-station` when `BUILD_GOESTOOLS_STATION=ON`.

Planned top-level tabs:

- Status
- Processes
- Monitor
- Catalog
- Logs
- Settings

## Status

The Status tab should be the quick station overview. It should summarize:

- Receiver process state.
- Processor process state.
- Packet health.
- Last received stats time.
- Product output activity.
- Current warning/error state.

The first version can use simple labels and state indicators. Later versions can
add compact trend charts or a station timeline.

## Processes

The Processes tab supervises external commands using `ManagedProcess`, a Qt
wrapper around `QProcess`.

Initial responsibilities:

- Configure command path, arguments, and working directory.
- Start, stop, and restart a process.
- Show status, PID, uptime, exit code, stdout, and stderr.
- Append stdout/stderr to per-process log files.

Planned process profiles:

- `goesrecv`
- `goesproc --mode packet`
- `goeslrit`
- Optional helper scripts such as smoke builds or catalog scans.

The GUI should treat these as external tools. It should not link against or
call receiver/decoder internals.

## Monitor

The Monitor tab should subscribe to `goesrecv` runtime stats. The existing
stats interface is documented in `docs/station-monitoring.md` and
`docs/stats-interface.md`.

Default endpoints:

```text
tcp://127.0.0.1:6001  demodulator stats
tcp://127.0.0.1:6002  decoder stats
```

The station app should:

- Use one subscriber per stats endpoint.
- Parse one JSON object per nanomsg message.
- Keep a short rolling in-memory history.
- Mark data stale when messages stop arriving.
- Show parse/socket warnings without freezing the UI.

Suggested health metrics:

- AGC gain.
- Frequency correction.
- Clock recovery omega.
- Packet rate.
- Drop rate.
- Viterbi error average.
- Reed-Solomon correction summary.
- Skipped-symbol count.
- Time since last demodulator/decoder message.

The first Monitor tab can be a summary grid. Charts can come later.

## Catalog

The Catalog tab browses generated products in an output directory.

The first implementation should remain SQLite-free and use:

- `ProductScanner` to recursively scan a configured directory.
- `ProductCatalogModel` to expose results through a Qt table model.
- `QTableView` to show filename, relative path, modified time, size, and file
  type.

Future catalog features:

- Persistent SQLite index.
- Incremental rescan.
- File-open actions.
- Product thumbnails.
- Filters by type, time, satellite, channel, or region.
- Integration with `tools/catalog/catalog_products.py` or a later native
  catalog backend.

## Logs

The Logs tab should show live combined output from supervised processes.

Current direction:

- `ManagedProcess` captures stdout and stderr.
- Per-process files are appended under the app data log directory.
- The Logs tab displays live output.
- A process-name filter controls which live entries are shown.

Future logging work:

- Load historical log files on startup.
- Add severity detection for common warning/error lines.
- Add search.
- Add log rotation or size limits.
- Link log entries to process restarts and exit codes.

## Settings

The Settings tab should eventually persist station configuration with
`QSettings`.

Candidate settings:

- Paths to `goesrecv`, `goesproc`, and helper scripts.
- Config file paths.
- Output directory.
- Working directory.
- Stats endpoints.
- Rolling history duration.
- Stale timeout.
- Log directory.
- Product catalog scan interval.
- Whether processes should restart automatically.

Settings should default to per-user paths and should not assume binaries or
configs were installed by root.

## Phased Implementation Plan

### Phase 1: App Skeleton

- Add optional Qt 6 Widgets target.
- Create main window and top-level tabs.
- Keep normal CLI builds independent of Qt.

### Phase 2: Process Supervision

- Add `ManagedProcess`.
- Start/stop/restart one configured external command.
- Show PID, status, uptime, exit code, stdout, and stderr.
- Keep receiver/decoder source untouched.

### Phase 3: Logs

- Append stdout/stderr to per-process log files.
- Add live Logs tab.
- Add simple process filter.

### Phase 4: Product Catalog

- Add `ProductScanner`.
- Add `ProductCatalogModel`.
- Display recursive output-directory scan in a `QTableView`.
- Infer file type from extension/path.

### Phase 5: Runtime Monitoring

- Add nanomsg stats subscribers.
- Parse demodulator and decoder JSON stats.
- Keep short rolling history.
- Display health metrics and stale/disconnected states.

### Phase 6: Settings And Profiles

- Add persistent `QSettings`.
- Add reusable process profiles for common `goesrecv` and `goesproc` setups.
- Allow user-configurable paths, endpoints, output directories, and log
  locations.

### Phase 7: Operator Polish

- Add product filters and file-open actions.
- Add small trend charts.
- Add log search.
- Add restart policy controls.
- Add clearer error states for missing commands, missing configs, and broken
  stats subscriptions.

## Integration Boundaries

Preferred integration points:

- External process execution via `QProcess`.
- Existing stats nanomsg publishers.
- Existing packet/product output directories.
- Existing logs/stdout/stderr.
- Existing config files.

Avoid changing:

- Core DSP and demodulation.
- Decoder packetizer, Viterbi, and Reed-Solomon behavior.
- Packet assembly and LRIT/EMWIN/DCS parsing.
- Existing CLI behavior unless specifically requested.
