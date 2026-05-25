# goestools Station

`goestools-station` is an optional Qt 6 Widgets prototype for operator-facing
station control. It is separate from the existing command-line receiver and
processing tools.

Enable it explicitly when configuring CMake:

```sh
cmake -S . -B build-station \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DBUILD_GOESTOOLS_STATION=ON
cmake --build build-station --target goestools-station
```

Qt 6 Widgets development files must be installed for this target. Normal builds
leave `BUILD_GOESTOOLS_STATION` off, so Qt is not required unless this app is
enabled.

The first version creates tabs for:

- Status
- Processes
- Monitor
- Catalog
- Logs
- Settings

The Processes tab accepts an external command, optional arguments, and an
optional working directory, then uses a `ManagedProcess` wrapper around
`QProcess` to start, stop, and restart the command. The tab shows status, PID,
uptime, exit code, and stdout/stderr output.

Captured stdout and stderr are also appended to per-process log files under the
application data directory, usually:

```text
~/.local/share/goestools-station/logs
```

For each process name, the app writes:

- `<process>-stdout.log`
- `<process>-stderr.log`
- `<process>-combined.log`

The Logs tab shows live combined stdout/stderr output and includes a simple
process-name filter.

The Catalog tab scans a configured output directory recursively and displays the
results in a Qt table model. It currently records filename, relative path,
modified time, size, and inferred file type. It does not require SQLite yet.
