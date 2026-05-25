#!/bin/sh
#
# Non-destructive smoke build for goestools.
#
# This script verifies that submodules are checked out, configures CMake in a
# fresh temporary build directory, and builds the project there. It does not
# install anything and does not touch the repository's normal build directory.
#

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "${script_dir}/.." && pwd)

install_prefix=${CMAKE_INSTALL_PREFIX:-/usr/local}
build_type=${CMAKE_BUILD_TYPE:-}
jobs=${JOBS:-}

missing=

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        missing="${missing} $1"
    fi
}

require_command cmake
require_command git
require_command mktemp

if [ -z "${jobs}" ]; then
    if command -v nproc >/dev/null 2>&1; then
        jobs=$(nproc)
    else
        jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
    fi
fi

if [ -n "${missing}" ]; then
    echo "Missing required tools:${missing}" >&2
    exit 1
fi

if [ ! -f "${repo_dir}/.gitmodules" ]; then
    echo "No .gitmodules file found; cannot verify submodules." >&2
    exit 1
fi

echo "Checking submodules..."
missing_submodules=$(
    git -C "${repo_dir}" submodule status --recursive |
    while IFS= read -r line; do
        case "${line}" in
            -*)
                echo "${line}"
                ;;
            U*)
                echo "${line}"
                ;;
        esac
    done
)

if [ -n "${missing_submodules}" ]; then
    echo "Submodules are not fully checked out:" >&2
    echo "${missing_submodules}" >&2
    echo "Run: git submodule update --init --recursive" >&2
    exit 1
fi

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/goestools-smoke-build.XXXXXX")

echo "Repository: ${repo_dir}"
echo "Build dir:  ${build_dir}"
echo "Jobs:       ${jobs}"

policy_minimum=${CMAKE_POLICY_VERSION_MINIMUM:-3.5}

configure_args="-DCMAKE_INSTALL_PREFIX=${install_prefix}"
configure_args="${configure_args} -DCMAKE_POLICY_VERSION_MINIMUM=${policy_minimum}"
if [ -n "${build_type}" ]; then
    configure_args="${configure_args} -DCMAKE_BUILD_TYPE=${build_type}"
fi

# shellcheck disable=SC2086
cmake -S "${repo_dir}" -B "${build_dir}" ${configure_args}
cmake --build "${build_dir}" -j"${jobs}"

echo "Smoke build passed."
echo "Temporary build directory was left in place: ${build_dir}"
