# goestools Station

Canonical station app documentation lives in:

```text
docs/station.rst
docs/station/
```

Build the optional Qt 6 app with:

```sh
cmake -S . -B build-station \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DBUILD_GOESTOOLS_STATION=ON
cmake --build build-station --target goestools-station
```
