# Station Monitoring Design

This document describes how the Qt `goestools-station` app should monitor a
running `goesrecv` process. It is a design note only; no station monitoring code
is implemented here.

## Existing Stats Interface

`goesrecv` publishes runtime stats over nanomsg PUB/SUB sockets. The sample
configuration exposes two TCP endpoints:

```toml
[demodulator.stats_publisher]
bind = "tcp://0.0.0.0:6001"

[decoder.stats_publisher]
bind = "tcp://0.0.0.0:6002"
```

An external station app should subscribe to:

```text
tcp://127.0.0.1:6001  demodulator stats
tcp://127.0.0.1:6002  decoder stats
```

when `goesrecv` runs on the same host. For remote receivers, replace
`127.0.0.1` with the receiver host address.

The publisher implementation is `src/goesrecv/stats_publisher.cc`. It sends one
complete JSON object per nanomsg message. The current strings include a trailing
newline, but subscribers should treat nanomsg message boundaries as framing and
not depend on the newline.

`StatsPublisher::publish` checks for connected subscribers before sending, so
messages are skipped when no station subscriber is connected. The station should
expect gaps during startup, reconnects, or receiver restarts.

## Payloads

### Demodulator

Produced by `Demodulator::publishStats` in `src/goesrecv/demodulator.cc`.

Example:

```json
{"timestamp": "2018-04-18T04:52:58.357Z","gain": 2.2494e+01,"frequency": 2.9813e+03,"omega": 1.6181e+00}
```

Fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `timestamp` | string | UTC timestamp from `util::stringTime()`. |
| `gain` | number | AGC gain multiplier. |
| `frequency` | number | Costas-loop frequency correction in Hz. Can be negative. |
| `omega` | number | Clock recovery samples-per-symbol estimate. |

### Decoder

Produced by `Decoder::publishStats` in `src/goesrecv/decoder.cc`.

Example:

```json
{"timestamp": "2018-04-18T04:54:22.974Z","skipped_symbols": 0,"viterbi_errors": 44,"reed_solomon_errors": 0,"ok": 1}
```

Fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `timestamp` | string | UTC timestamp from `util::stringTime()`. |
| `skipped_symbols` | integer | Symbols skipped while acquiring frame sync. |
| `viterbi_errors` | integer | Estimated corrected Viterbi bit errors. |
| `reed_solomon_errors` | integer | Corrected Reed-Solomon bytes, or `-1` when uncorrectable. |
| `ok` | integer/bool | Nonzero when a packet was correctable and published. |

## Proposed Qt Architecture

Keep monitoring separate from `ManagedProcess` and the product catalog:

```text
MonitorTab
  -> StatsSubscriber demodulator endpoint
  -> StatsSubscriber decoder endpoint
  -> StatsHistory rolling window
  -> HealthSummary / table / charts
```

Recommended classes:

- `StatsSubscriber`: owns one nanomsg subscriber socket and reads messages from
  a worker thread.
- `StatsMessage`: normalized record containing source, receive time, optional
  payload timestamp, and parsed numeric fields.
- `StatsHistory`: bounded in-memory rolling history for recent demodulator and
  decoder samples.
- `HealthSummary`: computes display metrics from `StatsHistory`.
- `MonitorTab`: Qt Widgets UI that renders current health, recent rates, and
  small trend views.

Do not read nanomsg sockets on the GUI thread. The subscriber should emit Qt
signals with parsed messages and let Qt deliver them to the UI thread using
queued connections.

## Subscription Behavior

Each stats stream should use one subscriber:

1. Create an `NN_SUB` socket.
2. `nn_connect` to the configured endpoint.
3. Subscribe to the empty topic with `NN_SUB_SUBSCRIBE`, matching the existing
   in-process monitor.
4. Read one nanomsg message at a time.
5. Parse the message as JSON.
6. Attach a source label: `demodulator` or `decoder`.
7. Emit a parsed message to the station app.

Reconnect behavior should be tolerant:

- Show `Disconnected` when socket setup or receive fails.
- Retry with backoff, such as 1s, 2s, 5s, then 10s.
- Keep the last known health values visible but mark them stale.
- Avoid blocking app shutdown; worker threads must stop promptly.

## JSON Parsing Rules

The station should be permissive:

- Ignore unknown fields.
- Treat missing fields as unavailable.
- Accept `ok` as either integer or boolean.
- Parse timestamps when present, but use receive time for rolling-window
  retention if parsing fails.
- Never crash the UI on malformed JSON; increment a parse-error counter and
  show a warning state if parse errors continue.

Recommended normalized message shape:

```text
source: demodulator | decoder
received_at: station UTC time
payload_timestamp: optional UTC time from JSON
values: map<string, double/int/bool>
raw_json: optional, for diagnostics only
```

## Rolling History

The first station version should keep only short in-memory history. Suggested
retention:

- 5 minutes by default.
- Configurable retention later, with an upper bound to avoid unbounded memory.
- Store raw samples in ring buffers or deques ordered by receive time.

Suggested buffers:

- Demodulator samples: timestamp, gain, frequency, omega.
- Decoder samples: timestamp, skipped symbols, Viterbi errors,
  Reed-Solomon errors, ok flag.

On every insert, drop entries older than the retention cutoff.

Use receive time for window membership because PUB/SUB delivery gaps and
payload timestamp parse failures should not break retention. Payload timestamps
can still be displayed for diagnostics.

## Health Metrics

The station should compute health over a short display window, such as the last
10 seconds, and optionally show longer trends over the retained 5-minute window.

### Demodulator Metrics

Display:

- Latest gain.
- Average gain over the display window.
- Latest frequency correction.
- Average frequency correction over the display window.
- Latest omega.
- Average omega over the display window.
- Time since last demodulator stats message.

Health hints:

- Mark stale if no demodulator message arrived for more than a few seconds.
- Frequency correction can be negative; chart and gauge ranges must allow that.
- Sudden jumps in `omega` or sustained high frequency correction may indicate
  receiver instability, but thresholds should be user-configurable later.

### Decoder Metrics

Display:

- Packet rate: count of decoder messages with `ok != 0` per second.
- Drop rate: count of decoder messages with `ok == 0` per second.
- Drop percentage over the display window.
- Average Viterbi errors.
- Reed-Solomon corrected-byte sum or average, excluding `-1`.
- Count of uncorrectable packets, where `reed_solomon_errors == -1`.
- Max or sum of skipped symbols over the display window.
- Time since last decoder stats message.

Health hints:

- `ok == 0` should count as a dropped/uncorrectable packet.
- `reed_solomon_errors == -1` should not be included in corrected-byte
  averages; count it separately as uncorrectable.
- Sustained nonzero `skipped_symbols` suggests lock acquisition trouble.

## UI Design For The Monitor Tab

Keep the first Qt implementation simple:

- Endpoint fields for demodulator and decoder stats.
- Connect/disconnect button.
- Connection state badges for each stream.
- A compact summary grid:
  - Gain
  - Frequency correction
  - Omega
  - Packet rate
  - Drop rate
  - Viterbi average
  - Reed-Solomon average/sum
  - Skipped symbols
- A small text area for recent parse/socket warnings.

Optional next step after the summary grid:

- Lightweight Qt charts or custom sparkline widgets for gain, frequency,
  omega, packet rate, drop rate, and Viterbi errors.

Avoid large historical persistence in the first pass. The catalog and logs are
separate concerns, and receiver health should remain responsive even on a small
station host.

## Relationship To Existing Monitor

`src/goesrecv/monitor.cc` already subscribes to internal `inproc://` stats
endpoints and aggregates values for stdout and statsd. The station app should
reuse the same field semantics, not the internal `inproc://` endpoints.

Existing aggregation behavior to mirror:

- `gain`: average over interval.
- `frequency`: average over interval.
- `omega`: average over interval.
- `viterbi_errors`: average over decoder messages.
- `reed_solomon_errors`: sum or average only nonnegative values.
- `ok`: count success and drop totals separately.

The station can improve on this by retaining a rolling history and showing stale
states, rates, and trends.

## Configuration

Initial defaults:

```text
demodulator endpoint: tcp://127.0.0.1:6001
decoder endpoint:     tcp://127.0.0.1:6002
retention:            5 minutes
display window:       10 seconds
stale timeout:        5 seconds
```

Future settings can live in the station Settings tab and be persisted with
`QSettings`.

## Failure Modes

The Monitor tab should explicitly handle:

- `goesrecv` not running.
- Stats endpoints not configured.
- Subscriber connected after messages have already been skipped.
- Malformed JSON.
- One stream present while the other is absent.
- Receiver restart causing temporary receive errors.

In all cases, the station should keep the UI responsive and show the last known
values with a clear stale/disconnected state.
