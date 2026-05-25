# AGENTS.md

## Project goals

This repository is being used as the foundation for a GOES receiver/processing system.

Prefer incremental, reviewable changes that keep the existing receiver and
decoder behavior stable.

## Change priorities

- Prioritize small build, packaging, documentation, and portability fixes.
- Prefer wrapper scripts, service files, dashboards, monitoring, and operator
  tooling before changing receiver or decoder internals.
- Keep changes scoped and easy to review. Avoid broad cleanups that are not
  needed for the requested task.
- Preserve the behavior of core DSP, demodulation, Viterbi, Reed-Solomon,
  packet processing, and LRIT/EMWIN/DCS decoding code unless the user
  specifically asks to modify those paths.
- If a core-path change is unavoidable, explain why, keep it minimal, and add
  focused verification that demonstrates the existing behavior is preserved.

## Build

Use an out-of-tree build:

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j$(nproc)
```
