#!/bin/sh
#
# Configure and build goestools on modern native Linux systems without
# installing packages.
#

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "${script_dir}/.." && pwd)

build_dir=${BUILD_DIR:-"${repo_dir}/build"}
install_prefix=${CMAKE_INSTALL_PREFIX:-/usr/local}
build_type=${CMAKE_BUILD_TYPE:-}
use_system_libaec=${GOESTOOLS_USE_SYSTEM_LIBAEC:-}
policy_minimum=${CMAKE_POLICY_VERSION_MINIMUM:-3.5}

missing=

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        missing="${missing} $1"
    fi
}

require_command cmake
require_command git
require_command pkg-config
require_command make

if ! command -v cc >/dev/null 2>&1 && ! command -v gcc >/dev/null 2>&1; then
    missing="${missing} cc"
fi

if ! command -v c++ >/dev/null 2>&1 && ! command -v g++ >/dev/null 2>&1; then
    missing="${missing} c++"
fi

if [ -n "${missing}" ]; then
    echo "Missing required build tools:${missing}" >&2
    echo "Install the missing tools/packages, then rerun this script." >&2
    exit 1
fi

if command -v nproc >/dev/null 2>&1; then
    jobs=$(nproc)
else
    jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
fi

echo "Repository: ${repo_dir}"
echo "Build dir:  ${build_dir}"
echo "Jobs:       ${jobs}"

mkdir -p "${build_dir}"

cmake_version=$(cmake --version | awk 'NR == 1 {print $3}')
if [ -z "${use_system_libaec}" ]; then
    case "$(printf '%s\n%s\n' "3.26" "${cmake_version}" | sort -V | head -n 1)" in
        "${cmake_version}")
            if [ "${cmake_version}" != "3.26" ]; then
                use_system_libaec=ON
                echo "CMake ${cmake_version} is older than vendored libaec requires; using system libaec."
                echo "If configure fails, install libaec-dev and rerun this script."
            else
                use_system_libaec=OFF
            fi
            ;;
        *)
            use_system_libaec=OFF
            ;;
    esac
fi

if [ -n "${build_type}" ]; then
    cmake -S "${repo_dir}" -B "${build_dir}" \
        -DCMAKE_INSTALL_PREFIX="${install_prefix}" \
        -DCMAKE_BUILD_TYPE="${build_type}" \
        -DCMAKE_POLICY_VERSION_MINIMUM="${policy_minimum}" \
        -DGOESTOOLS_USE_SYSTEM_LIBAEC="${use_system_libaec}"
else
    cmake -S "${repo_dir}" -B "${build_dir}" \
        -DCMAKE_INSTALL_PREFIX="${install_prefix}" \
        -DCMAKE_POLICY_VERSION_MINIMUM="${policy_minimum}" \
        -DGOESTOOLS_USE_SYSTEM_LIBAEC="${use_system_libaec}"
fi

cmake --build "${build_dir}" -j"${jobs}"
