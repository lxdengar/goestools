# goestools

Tools to receive, decode, assemble, and process GOES satellite data.

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
