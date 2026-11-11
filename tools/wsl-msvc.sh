#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
#
# Build and test the library with the MSVC toolchain of the Windows host, from WSL.
# The build tree (Ninja, single configuration) is on the Windows file system, in
# %LOCALAPPDATA%\bitfilled\build-msvc-<repo path hash>, as the MSVC build steps run through
# cmd.exe, which can't use a WSL (UNC) path as its working directory. The sources are compiled from
# the WSL file system directly.
#
# usage: tools/wsl-msvc.sh [--fresh] [--build-type <Release|Debug|...>] [-- <ctest args>]
#   --fresh       reconfigure the build tree from scratch
#   --build-type  set the CMAKE_BUILD_TYPE (default for a new build tree: Release, as in CI)
#
# Requirements: Visual Studio (Build Tools) 2022 or later on the Windows host, with the
# "Desktop development with C++" workload (MSVC, CMake and Ninja). The native toolset of the host is
# used: x64 on x64 hosts, ARM64 on Windows on ARM hosts (Visual Studio 2022 17.4 or later).
# Windows needs no git: the test dependency (boost.ut) is fetched in WSL, and passed to CPM as a
# local source directory.
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fresh=0
build_type=Release
build_type_set=0
ctest_args=()

while (($#)); do
    case "$1" in
    --fresh) fresh=1 ;;
    --build-type)
        build_type="$2"
        build_type_set=1
        shift
        ;;
    --)
        shift
        ctest_args=("$@")
        break
        ;;
    -h | --help)
        # the header comment, after the license
        awk 'NR > 3 && !/^#/ { exit } NR > 3 { sub(/^# ?/, ""); print }' "${BASH_SOURCE[0]}"
        exit 0
        ;;
    *)
        echo "unknown argument: $1" >&2
        exit 2
        ;;
    esac
    shift
done

command -v wslpath >/dev/null || {
    echo "this script must be run from WSL" >&2
    exit 1
}
vswhere="$(wslpath -u 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe')"
[[ -x "${vswhere}" ]] || {
    echo "vswhere.exe not found, is Visual Studio installed on the Windows host?" >&2
    exit 1
}

# Windows environment of the host
cd "$(wslpath -u 'C:\')" # avoids cmd.exe complaining about a UNC working directory
local_app_data="$(cmd.exe /c 'echo %LOCALAPPDATA%' | tr -d '\r')"
host_arch="$(cmd.exe /c 'echo %PROCESSOR_ARCHITECTURE%' | tr -d '\r')"
cd - >/dev/null

# the native MSVC toolset of the host
case "${host_arch}" in
AMD64)
    vcvars_arch=x64
    vs_component=Microsoft.VisualStudio.Component.VC.Tools.x86.x64
    ;;
ARM64)
    vcvars_arch=arm64
    vs_component=Microsoft.VisualStudio.Component.VC.Tools.ARM64
    ;;
*)
    echo "unsupported Windows host architecture: ${host_arch}" >&2
    exit 1
    ;;
esac
vs_dir="$("${vswhere}" -latest -products '*' -requires "${vs_component}" \
    -property installationPath | tr -d '\r')"
[[ -n "${vs_dir}" ]] || {
    echo "no Visual Studio installation with the MSVC ${vcvars_arch} toolset found" >&2
    exit 1
}

# the build tree on the Windows file system
repo_hash="$(printf '%s' "${repo_dir}" | sha1sum | cut -c1-8)"
build_dir="$(wslpath -u "${local_app_data}")/bitfilled/build-msvc-${repo_hash}"
mkdir -p "${build_dir}"
echo "build tree: ${build_dir}"

# boost.ut, at the version that the tests pin
ut_version="$(sed -n 's/.*CPMAddPackage("gh:boost-ext\/ut@\([^"]*\)").*/\1/p' \
    "${repo_dir}/test/CMakeLists.txt")"
[[ -n "${ut_version}" ]] || {
    echo "boost.ut version not found in test/CMakeLists.txt" >&2
    exit 1
}
ut_dir="${build_dir}/_wsl_deps/ut-${ut_version}"
if [[ ! -d "${ut_dir}" ]]; then
    git clone --quiet --depth 1 --branch "v${ut_version}" https://github.com/boost-ext/ut.git \
        "${ut_dir}"
fi

if ((fresh)); then
    find "${build_dir}" -mindepth 1 -maxdepth 1 ! -name _wsl_deps -exec rm -rf {} +
fi

# the sources are accessed by their UNC path (\\wsl.localhost\<distro>\...)
win_repo="$(wslpath -w "${repo_dir}")"
win_build="$(wslpath -w "${build_dir}")"
win_ut="$(wslpath -m "${ut_dir}")" # forward slashes, as CMake parses it

{
    printf '@echo off\r\n'
    printf 'call "%s\\VC\\Auxiliary\\Build\\vcvarsall.bat" %s >nul || exit /b 1\r\n' \
        "${vs_dir}" "${vcvars_arch}"
    if ((build_type_set)); then
        printf 'if 1==1 (\r\n'
    else
        printf 'if not exist "%s\\CMakeCache.txt" (\r\n' "${win_build}"
    fi
    printf '  cmake -S "%s" -B "%s" -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl' \
        "${win_repo}" "${win_build}"
    printf ' -DCMAKE_BUILD_TYPE=%s -DCPM_ut_SOURCE="%s" || exit /b 1\r\n' \
        "${build_type}" "${win_ut}"
    printf ')\r\n'
    printf 'cmake --build "%s" || exit /b 1\r\n' "${win_build}"
    printf 'ctest --test-dir "%s" --output-on-failure %s || exit /b 1\r\n' \
        "${win_build}" "${ctest_args[*]-}"
} >"${build_dir}/wsl-build.bat"

cd "${build_dir}"
cmd.exe /c wsl-build.bat 2>&1 | tr -d '\r'
exit "${PIPESTATUS[0]}"
