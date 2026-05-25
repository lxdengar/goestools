# AGENTS.md

## Project goals

This repository is being used as the foundation for a GOES receiver/processing system.

Prefer incremental, reviewable changes. Do not rewrite core DSP, demodulation, Viterbi, Reed-Solomon, or packet processing code unless specifically asked.

## Build

Use an out-of-tree build:

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j$(nproc)
