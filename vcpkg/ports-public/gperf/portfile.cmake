# OVERLAY DEVIATION from this directory's usual "patch on top of upstream"
# convention (see ../README.md) -- this portfile is not a patched copy of
# upstream gperf's, it's a full replacement, but only on Windows (see below).
#
# Upstream gperf builds via autotools (AUTORECONF + vcpkg_make_configure),
# which on Windows means bootstrapping the full msys2/autoconf/automake/
# libtool toolchain (dozens of extra downloads, see fetch_vcpkg_asset.ps1/
# AGENTS.md) and running `./configure` under MSYS2's POSIX-emulation
# process-spawn overhead -- by a wide margin the single slowest step in
# this repo's entire vcpkg install, just to produce one small host tool
# that only fontconfig needs (to generate its perfect-hash lookup tables
# at build time; gperf itself never ships in the final product).
#
# gperf's generated-code format has been stable since well before 3.0,
# and fontconfig only invokes gperf's basic ANSI-C table generation (none
# of the 3.1+-only flags), so a much older prebuilt binary is safe here.
# GnuWin32's gperf 3.0.1 is a standalone PE32 executable depending on
# nothing but kernel32.dll/msvcrt.dll -- verified via objdump, no mingw or
# msys runtime DLLs needed -- so it just runs, no extra packaging required.
#
# gperf.exe/COPYING are vendored directly in this port directory rather
# than fetched via vcpkg_download_distfile -- this one small (~100 KB)
# binary is exactly the thing the rest of this whole self-heal effort
# (BART asset mirror, bulk prefetch) exists to work around a flaky/
# firewalled network for, so skip that entirely: no origin URL, no BART,
# no network call of any kind, ever, for this port. Source: GnuWin32
# gperf 3.0.1 bin.zip, sha512
# 3f2d3418304390ecd729b85f65240a9e4d204b218345f82ea466ca3d7467789f43d0d2129fcffc18eaad3513f49963e79775b10cc223979540fa2e502fe7d4d9
# (also mirrored in BART's vcpkg-assets/gperf-3.0.1-bin.zip for provenance).
#
# Non-Windows falls straight through to upstream's real recipe, unmodified
# (gperf 3.3, autotools) -- msys2/autoconf's overhead is a Windows-only
# problem, and this path matters: gperf is a *host* tool (fontconfig looks
# it up via CURRENT_HOST_INSTALLED_DIR), so a Linux machine cross-compiling
# Starfish for Windows (e.g. the MSVC-on-Wine Docker rig) needs a real
# x64-linux gperf here, not the Windows-only vendored binary. An overlay
# port replaces the named port for every triplet that requests it, so this
# portfile has to keep handling that case rather than refusing it.
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_BUILD_TYPE release) # tool only

if(VCPKG_TARGET_IS_WINDOWS)
    # Matches upstream gperf's own bindir override (portfile.cmake:
    # "--bindir=${prefix}/tools/${PORT}") so fontconfig's portfile finds it
    # at the exact same relative path regardless of which gperf port
    # produced it.
    file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/gperf.exe" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
    vcpkg_install_copyright(FILE_LIST "${CMAKE_CURRENT_LIST_DIR}/COPYING")
else()
    vcpkg_download_distfile(ARCHIVE
        URLS
            "https://ftpmirror.gnu.org/gnu/gperf/gperf-${VERSION}.tar.gz"
            "https://ftp.gnu.org/pub/gnu/gperf/gperf-${VERSION}.tar.gz"
        FILENAME "gperf-${VERSION}.tar.gz"
        SHA512 246b75b8ce7d77d6a8725cd15f1cf2e68da404812573af1d5bf32dbe6ad4228f48757baefc77bcb1f5597c2397043c04d31d8a04ab507bfa7a80f85e1ab6045f
    )

    vcpkg_extract_source_archive(
        SOURCE_PATH
        ARCHIVE ${ARCHIVE}
    )

    vcpkg_make_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        AUTORECONF
        OPTIONS_RELEASE
            "--bindir=\\\${prefix}/tools/${PORT}" # legacy from vendored CMake build
    )

    vcpkg_make_install()

    vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
endif()
