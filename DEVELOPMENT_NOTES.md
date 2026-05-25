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

## Issues Observed During Repository Inspection

- The working tree already had uncommitted changes before these notes were
  added:
  - `CMakeLists.txt`
  - `src/goesrecv/packet_publisher.h`
  - `vendor/libaec`
  - `vendor/sanitizers-cmake`
  - `vendor/tinytoml`
- No source files were changed while preparing these notes.
- The build was not run as part of writing this file.
