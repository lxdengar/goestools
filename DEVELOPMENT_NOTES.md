# Development Notes

## Ubuntu on WSL2 Build

These notes cover building `goestools` on Ubuntu under WSL2. The project is a
CMake-based C/C++ build with vendored dependencies and optional SDR hardware
support.

## Dependencies

Install the general build and runtime development packages:

```sh
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  git-core \
  pkg-config \
  libopencv-dev \
  libproj-dev \
  zlib1g-dev
```

Install SDR development packages if you want to build `goesrecv` with hardware
source support:

```sh
sudo apt install -y \
  libairspy-dev \
  librtlsdr-dev
```

Without `libairspy-dev` or `librtlsdr-dev`, CMake can still configure the
project, but `goesrecv` will be built without the corresponding source support.

The repository also uses bundled dependencies from `vendor/`:

- `libcorrect` for Viterbi/Reed-Solomon correction.
- `libaec` for AEC/SZIP decompression.
- `nanomsg` for packet and stats pub/sub.
- `nlohmann_json`.
- `tinytoml`.
- `sanitizers-cmake` when sanitizer options are enabled.

If cloning fresh, include submodules:

```sh
git clone --recursive https://github.com/pietern/goestools
```

For an existing checkout, initialize/update submodules:

```sh
git submodule update --init --recursive
```

## Build Commands

From the repository root:

```sh
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j"$(nproc)"
```

The repeatable WSL/Ubuntu helper runs the same out-of-tree CMake build without
installing packages:

```sh
scripts/build_wsl.sh
```

The script checks for required build tools, creates or reuses `build/`, runs
CMake configure, and then builds with the detected CPU count. It is safe to run
again after source changes or after a successful build.

Optional environment overrides:

```sh
BUILD_DIR=build-debug CMAKE_BUILD_TYPE=Debug scripts/build_wsl.sh
CMAKE_INSTALL_PREFIX=/opt/goestools scripts/build_wsl.sh
```

## Docker Ubuntu Build

The minimal Ubuntu development image installs only the packages needed to build
the default project from source. Build it from the repository root:

```sh
docker build -f docker/Dockerfile.ubuntu-dev -t goestools-ubuntu-dev .
```

Run the build by mounting the checkout into the container:

```sh
docker run --rm -v "$PWD":/workspace -w /workspace goestools-ubuntu-dev
```

The container default command runs `scripts/build_wsl.sh`, so it creates or
reuses `build/` and performs the normal out-of-tree CMake build. Initialize
submodules in the checkout before using the container if needed:

```sh
git submodule update --init --recursive
```

Optional install:

```sh
sudo cmake --install .
```

The main expected executables are built under `build/src/...`, including:

- `build/src/goesrecv/goesrecv`
- `build/src/goeslrit/goeslrit`
- `build/src/goesproc/goesproc`
- `build/src/goespackets/goespackets`

## Useful Build Options

The top-level CMake build exposes these options:

```sh
-DBUILD_GOESRECV=ON
-DBUILD_GOESLRIT=ON
-DBUILD_GOESPROC=ON
-DBUILD_GOESPACKETS=ON
-DBUILD_GOESEMWIN=OFF
```

Example: build only the LRIT/proc path without `goesrecv`:

```sh
cmake .. \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DBUILD_GOESRECV=OFF \
  -DBUILD_GOESLRIT=ON \
  -DBUILD_GOESPROC=ON
```

## WSL2 Notes

- The project documentation says building requires Linux; Ubuntu on WSL2 fits
  that requirement for normal compilation.
- Direct USB SDR access from WSL2 may require additional host-side USB
  forwarding setup. Compilation can succeed without testing hardware capture.
- `goesproc` requires OpenCV. On newer Ubuntu releases, the CMake file first
  checks `opencv` and then falls back to required `opencv4`.
- `libproj-dev` is optional for map overlay/projection support in `goesproc`.
  If PROJ is not found, `goesproc` still builds without `HAS_PROJ`.
- `goesrecv` detects Airspy and RTL-SDR support through pkg-config modules
  `libairspy` and `librtlsdr`. Missing packages produce CMake warnings and
  disable those source backends.
- The build creates a generated include directory and symlink for vendored
  `nanomsg` headers under the build tree.

## Data Flow Reference

The normal live pipeline is:

```text
SDR samples
  -> goesrecv demodulator
  -> decoder Packetizer
  -> 892-byte VCDU packets
  -> nanomsg packet publisher
  -> goeslrit or goesproc --mode packet
```

`goeslrit` assembles VCDU packets into `.lrit` files. `goesproc` can either
consume packets directly or process existing `.lrit` files:

```text
goesrecv -> goeslrit -> .lrit files -> goesproc --mode lrit
goesrecv -> goesproc --mode packet
```

## Recent Documentation And Tooling Additions

These additions are outside the core receiver/decoder path. They are intended
to make build verification, service operation, monitoring, and future dashboard
work easier without changing C++ receiver behavior.

### Repository Agent Guidance

`AGENTS.md` now documents repository working rules:

- Preserve existing receiver and decoder behavior.
- Prefer small build, packaging, documentation, and portability fixes.
- Prefer wrapper scripts, services, dashboards, monitoring, and operator tools
  before refactoring DSP, demodulation, Viterbi, Reed-Solomon, packet
  processing, or LRIT/EMWIN/DCS decoding internals.

### Receiver Pipeline Documentation

`docs/receiver-pipeline.md` maps the major executables and data flow:

- `goesrecv` owns RF/sample input, demodulation, decoding, and packet
  publication.
- `goespackets` relays, filters, records, and republishes 892-byte VCDU packet
  streams.
- `goeslrit` assembles decoded VCDU packets into LRIT files.
- `goesemwin` extracts EMWIN fragments, QBT packets, and EMWIN files.
- `goesproc` processes packet streams or LRIT files into image/text products.

The document also identifies the source files that appear to own each stage and
calls out stable integration points for wrapper/dashboard work.

### Receiver Stats Interface Documentation

`docs/stats-interface.md` describes the existing `goesrecv` stats streams:

- Demodulator stats publisher, commonly `tcp://0.0.0.0:6001`.
- Decoder stats publisher, commonly `tcp://0.0.0.0:6002`.
- JSON message fields for gain, frequency correction, clock recovery,
  Viterbi/Reed-Solomon correction counts, skipped symbols, and packet success.
- Aggregation behavior used by the existing in-process `Monitor`.

This is intended as the interface reference for future monitoring and dashboard
prototypes.

### systemd User Service Examples

`systemd/examples/` contains user-service examples that do not assume a root
install:

- `goesrecv.service`
- `goesproc.service`
- `README.md`

The examples use per-user paths such as `%h/.local/bin`, `%E/goestools`, and
`%h/goes-data`, and the README explains how to customize binary paths, config
paths, output directories, and packet subscribe addresses.

### Python Monitor Prototype

`tools/monitor/` contains a standalone Python prototype:

- `stats_subscriber.py` subscribes to a `goesrecv` nanomsg stats endpoint,
  parses each received JSON object, and prints it.
- `README.md` documents usage for demodulator and decoder stats endpoints.

The prototype is intentionally separate from the C++/CMake build. It uses
Python `ctypes` with the system `libnanomsg`, so it does not add a Python
package dependency.

### Python Product Catalog Prototype

`tools/catalog/` contains a standalone SQLite catalog prototype:

- `catalog_products.py` scans a `goesproc` or `goeslrit` output directory and
  records filename, inferred timestamp, inferred product type, file size,
  modification time, and scan time.
- `README.md` documents usage, product type heuristics, timestamp parsing, and
  the SQLite schema.

The script is designed for future dashboard/search work and is not connected to
the C++ build.

### Smoke Build Script

`scripts/smoke_build.sh` is a non-destructive build verification helper:

- Verifies recursive submodules are checked out.
- Configures CMake in a fresh temporary directory under `${TMPDIR:-/tmp}`.
- Builds from that temporary tree.
- Does not install anything and does not touch the repository's normal `build/`
  directory.
- Leaves the temporary build directory in place for inspection.

The script passes `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` by default because newer
CMake versions reject older vendored `cmake_minimum_required` declarations in a
fresh configure otherwise. This avoids modifying vendor source code.

Verification performed:

- `scripts/smoke_build.sh` completed a fresh configure/build successfully to
  `100%`.
- `tools/monitor/stats_subscriber.py --help` ran successfully, and a syntax
  compile check passed.
- `tools/catalog/catalog_products.py --help` ran successfully.
- `tools/catalog/catalog_products.py` was smoke-tested against a scratch output
  tree with sample image, LRIT, text, and packet files; it created a SQLite
  catalog and inferred the expected product types/timestamps.

## Historical Inspection Notes

Earlier WSL/Ubuntu inspection found a dirty worktree with CMake, vendored
dependency, and include-path issues. Those details are preserved in
`ubuntu_changes.txt` under "Errors Encountered And Fixes"; the current build
verification status is the smoke-build result above.
