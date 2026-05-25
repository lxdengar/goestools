# goesrecv Stats Monitor Prototype

This is a small Python prototype for subscribing to a `goesrecv` stats
publisher and printing parsed JSON messages. It is intentionally separate from
the C++ build and is not referenced by CMake.

The prototype uses Python's standard `ctypes` module to call `libnanomsg`
directly. It does not require a Python nanomsg package, but the nanomsg shared
library must be installed on the system.

## Examples

Subscribe to the demodulator stats endpoint from the sample config:

```sh
python3 tools/monitor/stats_subscriber.py tcp://127.0.0.1:6001 --source demodulator
```

Subscribe to the decoder stats endpoint:

```sh
python3 tools/monitor/stats_subscriber.py tcp://127.0.0.1:6002 --source decoder
```

Pretty-print each JSON object:

```sh
python3 tools/monitor/stats_subscriber.py tcp://127.0.0.1:6001 --pretty
```

The script subscribes to the empty nanomsg topic, matching the existing
`goesrecv` monitor behavior.
