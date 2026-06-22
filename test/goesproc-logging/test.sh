#!/bin/sh

set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
goesproc=${GOESPROC:-"${repo_dir}/build/src/goesproc/goesproc"}

if [ ! -x "${goesproc}" ]; then
  echo "goesproc executable not found: ${goesproc}" >&2
  echo "Build the goesproc target or set GOESPROC to its path." >&2
  exit 1
fi

tmp=$(mktemp -d)
trap 'rm -rf "${tmp}"' EXIT INT TERM

mkdir -p "${tmp}/input" "${tmp}/output"
printf '%s\n' \
  '[[handler]]' \
  'type = "text"' \
  'origin = "other"' \
  'directory = "./text"' \
  > "${tmp}/goesproc.conf"

"${goesproc}" \
  -c "${tmp}/goesproc.conf" \
  -m lrit \
  --out "${tmp}/output" \
  --summary-interval 0 \
  "${tmp}/input" \
  > "${tmp}/text.log"

grep -q ' INFO started ' "${tmp}/text.log"
grep -q ' INFO stopped' "${tmp}/text.log"

"${goesproc}" \
  -c "${tmp}/goesproc.conf" \
  -m lrit \
  --out "${tmp}/output" \
  --log-format json \
  --summary-interval 0 \
  "${tmp}/input" \
  > "${tmp}/json.log"

python3 - "${tmp}/json.log" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as stream:
    events = [json.loads(line) for line in stream if line.strip()]

assert [event["event"] for event in events] == ["started", "stopped"]
assert all("timestamp" in event and "level" in event for event in events)
PY

"${goesproc}" \
  -c "${tmp}/goesproc.conf" \
  -m lrit \
  --out "${tmp}/output" \
  --log-level quiet \
  --summary-interval 0 \
  "${tmp}/input" \
  > "${tmp}/quiet.log"

test ! -s "${tmp}/quiet.log"

echo "goesproc logging smoke test passed"
