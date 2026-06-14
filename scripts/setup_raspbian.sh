#!/bin/bash
#
# Setup environment for cross-compilation for Raspbian.
#

set -e

if [ "${GOESTOOLS_LEGACY_RASPBIAN_XCOMPILE:-}" != "1" ]; then
    cat >&2 <<'EOF'
scripts/setup_raspbian.sh is a legacy cross-compilation helper.

It targets old Raspbian Stretch-era packages and Ubuntu 18.04 cross compiler
packages such as gcc-5-arm-linux-gnueabihf. Those packages are not available
on modern Raspberry Pi OS or current Ubuntu releases.

If you are building directly on a Raspberry Pi, do not run this script. Use:

  scripts/modern_build.sh

or the normal out-of-tree CMake build documented in docs/guides/raspberry-pi.rst.

To run the legacy cross-compilation setup anyway, set:

  GOESTOOLS_LEGACY_RASPBIAN_XCOMPILE=1 scripts/setup_raspbian.sh
EOF
    exit 1
fi

target_dir="xcompile/raspbian"
mkdir -p "${target_dir}"
cp -f "$(dirname "$0")/files/raspberrypi.cmake" "${target_dir}"

install_if_needed() {
    if ! dpkg -l "$1" | grep -q '^ii'; then
        sudo apt-get install -y "$1"
    fi
}

# These resolve to 5.5.0-12ubuntu1cross1 on Ubuntu 18.04
install_if_needed gcc-5-arm-linux-gnueabihf
install_if_needed g++-5-arm-linux-gnueabihf

urls() {
    scripts/list_raspbian_urls.py \
        librtlsdr-dev \
        libairspy-dev \
        libusb-1.0-0-dev \
        libudev1 \
        zlib1g-dev \
        libopencv-dev \
        libopencv-highgui-dev \
        libproj-dev \
        gcc-5 \
        g++-5
}

tmp="$target_dir/tmp"
mkdir -p "$tmp"

# Download and extract packages of interest
for url in $(urls); do
    deb=$(basename "${url}")
    if [ ! -f "${tmp}/${deb}" ]; then
        echo "Downloading ${url}..."
        ( cd "$tmp" && curl -LOs "${url}" )
    fi
    echo "Extracting ${deb}..."
    dpkg-deb -x "${tmp}/${deb}" "${target_dir}/sysroot"
done
