#!/bin/bash

#   __|    \    _ \  |      _ \   __| __ __| __ __|
#  (      _ \     /  |     (   | (_ |    |      |
# \___| _/  _\ _|_\ ____| \___/ \___|   _|     _|

# scripts/build_envroot_binary.sh
# Created 1/21/26 - 3:37 PM UK Time (London) by carlogtt

# Script Options
set -o errexit  # Exit immediately if a command exits with a non-zero status
set -o pipefail # Exit status of a pipeline is the status of the last cmd to exit with non-zero

script_dir_abs="$(realpath -- "$(dirname -- "${BASH_SOURCE[0]}")")"
declare -r script_dir_abs
project_root_dir_abs="$(realpath -- "${script_dir_abs}/..")"
declare -r project_root_dir_abs
libexec_dir="${project_root_dir_abs}/libexec/envroot"
declare -r libexec_dir

host_os="$(uname -s)"
host_arch="$(uname -m | tr '[:upper:]' '[:lower:]')"

case "${host_arch}" in
arm64 | aarch64)
    host_arch_id="arm64"
    ;;
x86_64 | amd64)
    host_arch_id="x86_64"
    ;;
*)
    host_arch_id="${host_arch}"
    ;;
esac

src="${project_root_dir_abs}/src/envroot.c"

build_macos() {
    local arch="$1"
    local output_dir="${libexec_dir}/macos/${arch}"
    local output="${output_dir}/envroot"
    local cc="${CC_MACOS:-clang}"

    if [[ "${host_os}" != "Darwin" ]]; then
        echo "macOS launcher build requires a macOS host."
        return 1
    fi

    if ! command -v "${cc}" >/dev/null 2>&1; then
        echo "Missing compiler for macOS (${cc})."
        return 1
    fi

    rm -rf "${output_dir}"
    mkdir -p "${output_dir}"
    "${cc}" -O2 -Wall -Wextra -arch "${arch}" -mmacosx-version-min=11.0 \
        -o "${output}" "${src}"
    chmod 755 "${output}"
    echo "Built ${output}"
}

build_linux() {
    local arch="$1"
    local output_dir="${libexec_dir}/linux/${arch}"
    local output="${output_dir}/envroot"
    local cc=""
    local cc_label=""
    local needs_cross=0

    if [[ "${host_os}" != "Linux" ]]; then
        echo "Linux launcher build requires a Linux host."
        return 1
    fi

    if [[ "${arch}" == "x86_64" ]]; then
        cc_label="CC_LINUX_X86_64"
        if [[ "${host_arch_id}" == "x86_64" ]]; then
            cc="${CC_LINUX_X86_64:-cc}"
        else
            cc="${CC_LINUX_X86_64:-x86_64-linux-gnu-gcc}"
            needs_cross=1
        fi
    else
        cc_label="CC_LINUX_ARM64"
        if [[ "${host_arch_id}" == "arm64" ]]; then
            cc="${CC_LINUX_ARM64:-cc}"
        else
            cc="${CC_LINUX_ARM64:-aarch64-linux-gnu-gcc}"
            needs_cross=1
        fi
    fi

    if ! command -v "${cc}" >/dev/null 2>&1; then
        if [[ "${needs_cross}" -eq 1 ]]; then
            echo "Missing cross-compiler for Linux ${arch} (${cc}). Set ${cc_label} to a cross compiler."
        else
            echo "Missing compiler for Linux ${arch} (${cc})."
        fi
        return 1
    fi

    rm -rf "${output_dir}"
    mkdir -p "${output_dir}"
    "${cc}" -O2 -Wall -Wextra -o "${output}" "${src}"
    chmod 755 "${output}"
    echo "Built ${output}"
}

build_all() {
    local failed=0

    case "${host_os}" in
    Darwin)
        build_macos "arm64" || failed=1
        build_macos "x86_64" || failed=1
        ;;
    Linux)
        build_linux "arm64" || failed=1
        build_linux "x86_64" || failed=1
        ;;
    *)
        echo "Unsupported host OS: ${host_os}"
        return 1
        ;;
    esac

    if [[ "${failed}" -ne 0 ]]; then
        echo "One or more launcher builds failed."
        return 1
    fi
}

build_all
