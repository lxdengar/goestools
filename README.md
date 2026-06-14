# goestools

Tools to receive, decode, assemble, and process GOES satellite data.

This fork is the modernized build and operator-tooling branch of the original
`pietern/goestools` project. It keeps the receiver and decoder behavior intact
while adding current Ubuntu/WSL/Raspberry Pi build guidance, helper scripts,
consolidated Sphinx documentation, service examples, prototype monitoring and
catalog tools, and the optional `goestools-station` Qt dashboard. Use the
original upstream repository if you specifically need the historical project
state; use this fork for the documented modern Linux build flow.

The original project was created by Pieter Noordhuis. This fork claims no
ownership or additional rights over the original project; it is made available
to all under the repository's existing license.

The main executables are:

- `goesrecv`: demodulate and decode a signal into a packet stream.
- `goeslrit`: assemble LRIT files from decoded packets.
- `goesproc`: process LRIT files or packet streams into images and text.
- `goespackets`: record, filter, relay, and republish packet streams.
- `goestools-station`: optional Qt 6 station dashboard prototype.

## Documentation

The canonical documentation lives in [docs/](docs/).

Start here:

- [Quickstart](docs/quickstart.rst)
- [Configuration](docs/configuration.rst)
- [Commands](docs/commands.rst)
- [Services](docs/guides/services.rst)
- [Station App](docs/station.rst)
- [Tools](docs/tools.rst)
- [Development Notes](docs/development.rst)

## Minimal Build

```sh
git submodule update --init --recursive
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j"$(nproc)"
```

For a non-destructive verification build:

```sh
scripts/smoke_build.sh
```
