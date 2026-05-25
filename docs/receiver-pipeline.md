# Receiver Pipeline

This document summarizes how the receiver and processing tools are organized in
the source tree. It is intended as a map for build, wrapper, dashboard, and
operator tooling work; the core DSP and decoder path should be treated as
existing behavior unless a task explicitly calls for changing it.

## High-Level Flow

The normal live pipeline is:

```text
radio or sample source
  -> goesrecv demodulator
  -> goesrecv decoder
  -> 892-byte VCDU packet stream
  -> goesproc, goeslrit, goesemwin, or goespackets
  -> LRIT files, EMWIN files, images, text, or recorded packet files
```

`goesrecv` owns the RF/sample-to-packet path. Downstream tools consume the
decoded packet stream either from a nanomsg publisher or from packet files on
disk. The shared packet unit at that boundary is a 892-byte VCDU, represented
throughout the tree as `std::array<uint8_t, 892>`.

The most important handoff is therefore:

```text
soft bits -> decoder::Packetizer -> VCDU packets -> PacketReader/PacketWriter
```

## Major Executables

### `goesrecv`

`goesrecv` demodulates a configured input source and decodes valid packets. Its
entry point is `src/goesrecv/goesrecv.cc`.

The executable constructs three long-running components:

- `Demodulator`, implemented in `src/goesrecv/demodulator.cc`
- `Decoder`, implemented in `src/goesrecv/decoder.cc`
- `Monitor`, implemented in `src/goesrecv/monitor.cc`

`Demodulator` builds the configured source, runs the DSP chain, and pushes soft
bits into an internal queue. The DSP stage ownership appears to be:

- Source selection: `src/goesrecv/source.cc`
- Hardware or stream sources: `airspy_source.cc`, `rtlsdr_source.cc`,
  `nanomsg_source.cc`
- AGC: `src/goesrecv/agc.cc`
- Costas loop: `src/goesrecv/costas.cc`
- Root-raised-cosine filtering: `src/goesrecv/rrc.cc`
- Clock recovery: `src/goesrecv/clock_recovery.cc`
- Soft-bit quantization: `src/goesrecv/quantize.cc`

`Decoder` bridges the demodulator soft-bit queue into `decoder::Packetizer`.
`Packetizer`, implemented in `src/decoder/packetizer.cc`, owns frame sync,
Viterbi decode, phase/NRZ-M handling, derandomization, and Reed-Solomon
correction. Valid packets are published through `PacketPublisher` in
`src/goesrecv/packet_publisher.cc`.

### `goespackets`

`goespackets` relays, filters, republishes, and records decoded VCDU packets.
Its entry point is `src/goespackets/goespackets.cc`.

It reads packets from either:

- nanomsg, through `src/lib/nanomsg_reader.cc`
- packet files, through `src/lib/file_reader.cc`

It can filter by VCID using `src/assembler/vcdu.h`, then write packets to:

- nanomsg, through `src/lib/nanomsg_writer.cc`
- packet files, through `src/lib/file_writer.cc`

This is the main utility-shaped executable for packet stream fanout and
recording.

### `goeslrit`

`goeslrit` assembles decoded VCDU packets into LRIT files. Its entry point is
`src/goeslrit/goeslrit.cc`.

It consumes packets from nanomsg or packet files, optionally filters by VCID,
then feeds packets into `assembler::Assembler` from
`src/assembler/assembler.cc`. The assembler path appears to be owned by:

- VCDU view and header fields: `src/assembler/vcdu.h`
- Virtual-channel state and drop handling: `src/assembler/virtual_channel.cc`
- Transport PDU parsing and CRC: `src/assembler/transport_pdu.cc`,
  `src/assembler/crc.cc`
- Session PDU/LRIT file assembly: `src/assembler/session_pdu.cc`

`goeslrit` filters assembled files by LRIT file type and NOAA LRIT product ID,
then writes `.lrit` files to the selected output directory.

### `goesemwin`

`goesemwin` extracts EMWIN products from the decoded packet stream. Its entry
point is `src/goesemwin/goesemwin.cc`.

It reads the same 892-byte packet stream as `goeslrit`, then uses
`assembler::Assembler` to find EMWIN-over-LRIT products. In the current source,
the EMWIN-specific reader filters for GOES-N VCID `0`, LRIT file type `214`,
and NOAA LRIT product ID `42`.

After that, ownership moves into:

- QBT fragment and packet assembly: `src/goesemwin/qbt.cc`
- EMWIN file assembly: `src/goesemwin/emwin.cc`

The executable can write raw EMWIN-over-LRIT fragments, QBT packets, or final
EMWIN files depending on `--mode`.

### `goesproc`

`goesproc` converts packets or LRIT files into user-facing products such as
images and text files. Its entry point is `src/goesproc/goesproc.cc`.

It has two modes:

- Packet mode: reads 892-byte VCDU packets from nanomsg, stdin, packet files, or
  directories of `*.raw` files.
- LRIT mode: reads `.lrit` files or directories containing `*.lrit*` files.

Packet mode is owned by `src/goesproc/packet_processor.cc`. It assembles VCDUs
into LRIT files in memory using `assembler::Assembler`, wraps each result as an
`lrit::File`, and sends it to configured handlers.

LRIT mode is owned by `src/goesproc/lrit_processor.cc`. It loads LRIT files,
sorts them by LRIT timestamp header, and sends them to the same handler layer.

Handler ownership appears to be:

- Handler interface: `src/goesproc/handler.h`
- GOES-R images: `src/goesproc/handler_goesr.cc`
- GOES-N images: `src/goesproc/handler_goesn.cc`
- Himawari-8 images: `src/goesproc/handler_himawari8.cc`
- NWS images: `src/goesproc/handler_nws_image.cc`
- NWS text: `src/goesproc/handler_nws_text.cc`
- Generic text: `src/goesproc/handler_text.cc`
- EMWIN products: `src/goesproc/handler_emwin.cc`
- Shared output writing: `src/goesproc/file_writer.cc`

Image and product helpers live nearby, including `image.cc`, `area.cc`,
`map_drawer.cc`, `proj.cc`, `filename.cc`, and `gradient.cc`.

## Supporting Tools

The tree also contains smaller inspection and debug executables:

- `packetdump`, from `src/decoder/packetdump.cc`, exercises the decoder
  packetizer against a reader.
- `compute_sync_words`, from `src/decoder/compute_sync_words.cc`, supports
  decoder sync word work.
- `packetinfo`, from `src/assembler/packetinfo.cc`, inspects packet/VCDU data.
- `lritdump`, from `src/lrit/lritdump.cc`, inspects LRIT files.
- `areadump`, from `src/lrit/areadump.cc`, inspects AREA/image-related output.
- `dcsdump`, from `src/dcs/dcsdump.cc`, inspects DCS content.
- `unzip`, from `src/lib/unzip.cc`, is a small zip helper.
- `benchmark`, from `src/goesrecv/benchmark.cc`, benchmarks DSP components.

These are useful for diagnostics, but they are not the main live receiver
pipeline.

## Shared Data Boundaries

### Sample and Soft-Bit Streams

Inside `goesrecv`, data moves through bounded queues defined by
`src/goesrecv/queue.h`. The source produces complex samples, the DSP chain
transforms them, and quantization emits `std::vector<int8_t>` soft bits.

Optional publishers can expose intermediate samples, soft bits, and stats:

- `src/goesrecv/sample_publisher.cc`
- `src/goesrecv/soft_bit_publisher.cc`
- `src/goesrecv/stats_publisher.cc`

Those publishers are observability hooks. The main packet handoff remains the
decoder packet publisher.

### VCDU Packets

The external packet boundary is the 892-byte VCDU:

- Produced by `goesrecv` through `PacketPublisher`
- Read by `goespackets`, `goeslrit`, `goesemwin`, and `goesproc`
- Stored as raw packet files by `FileWriter`
- Replayed by `FileReader`
- Published and subscribed over nanomsg by `NanomsgWriter` and `NanomsgReader`

`src/assembler/vcdu.h` owns the lightweight view of VCDU header fields such as
SCID, VCID, and counter.

### LRIT Files

The LRIT file model is owned by `src/lrit/`:

- Core LRIT parsing: `src/lrit/lrit.cc`, `src/lrit/lrit.h`
- File wrapper: `src/lrit/file.cc`, `src/lrit/file.h`
- JSON helpers: `src/lrit/json.cc`

`goeslrit` writes assembled LRIT files to disk. `goesproc` can either assemble
LRIT files from packets internally or read existing LRIT files from disk.

## Practical Pipeline Shapes

Live processing directly into products:

```text
goesrecv --config goesrecv.conf
  -> nanomsg packet publisher
  -> goesproc --mode packet --subscribe ADDR --config goesproc.conf
  -> images/text/output products
```

Live packet recording and later replay:

```text
goesrecv
  -> goespackets --subscribe ADDR --record
  -> packets-*.raw
  -> goesproc --mode packet packets-*.raw
```

Assemble LRIT first, then process:

```text
goesrecv
  -> goeslrit --subscribe ADDR --all --out DIR
  -> *.lrit files
  -> goesproc --mode lrit DIR
```

EMWIN extraction:

```text
goesrecv
  -> goesemwin --subscribe ADDR --mode emwin --out DIR
  -> EMWIN files
```

## Ownership Map

| Stage | Main files |
| --- | --- |
| Command orchestration | `src/goesrecv/goesrecv.cc`, `src/goesproc/goesproc.cc`, `src/goeslrit/goeslrit.cc`, `src/goesemwin/goesemwin.cc`, `src/goespackets/goespackets.cc` |
| Configuration and options | `src/goesrecv/config.cc`, `src/goesrecv/options.cc`, `src/goesproc/config.cc`, `src/goesproc/options.cc`, per-command `options.cc` files |
| RF/sample sources | `src/goesrecv/source.cc`, `airspy_source.cc`, `rtlsdr_source.cc`, `nanomsg_source.cc` |
| DSP/demodulation | `src/goesrecv/demodulator.cc`, `agc.cc`, `costas.cc`, `rrc.cc`, `clock_recovery.cc`, `quantize.cc` |
| Frame decoding | `src/goesrecv/decoder.cc`, `src/decoder/packetizer.cc`, `correlator.cc`, `derandomizer.cc`, `reed_solomon.cc`, `viterbi.h` |
| Packet publication and replay | `src/goesrecv/packet_publisher.cc`, `src/lib/packet_reader.cc`, `packet_writer.cc`, `nanomsg_reader.cc`, `nanomsg_writer.cc`, `file_reader.cc`, `file_writer.cc` |
| VCDU/TPDU/SPDU assembly | `src/assembler/assembler.cc`, `vcdu.h`, `virtual_channel.cc`, `transport_pdu.cc`, `session_pdu.cc`, `crc.cc` |
| LRIT parsing | `src/lrit/lrit.cc`, `src/lrit/file.cc`, `src/lrit/json.cc` |
| Product processing | `src/goesproc/packet_processor.cc`, `lrit_processor.cc`, handler files under `src/goesproc/` |
| EMWIN extraction | `src/goesemwin/goesemwin.cc`, `qbt.cc`, `emwin.cc` |
| DCS support | `src/dcs/dcs.cc`, `src/dcs/dcsdump.cc` |

## Notes for Future Work

For wrapper, service, dashboard, and monitoring work, the safest integration
points are the existing nanomsg packet publisher, stats publishers, packet
record/replay files, and command-line tools. Those boundaries allow new
operator-facing behavior without changing the demodulator, decoder,
Reed-Solomon, Viterbi, packetizer, or assembler internals.

When a change needs to touch core receiver or decoder behavior, first identify
which boundary it affects: samples, soft bits, 892-byte VCDUs, LRIT files, or
final products. Keep the change scoped to that boundary and verify with the
smallest command that exercises the affected stage.
