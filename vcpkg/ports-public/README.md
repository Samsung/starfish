# Patched public vcpkg ports

`vcpkg-configuration.json` registers this directory as `overlay-ports`, so
every build -- Docker, CI, or a native Windows developer machine -- picks these
up with no command line flag to forget.

vcpkg has no way to patch a port that ships with the registry: an overlay
replaces a port wholesale. So each directory here is a **verbatim copy of the
port at the baseline commit pinned in `vcpkg-configuration.json`**, plus our
patch, plus one line adding it to the portfile's `PATCHES` list. Everything
else must stay byte-identical to upstream, and the copies have to be re-synced
whenever that baseline moves -- a newer port with our stale copy on top of it
is the failure mode to watch for.

Upstream is https://github.com/microsoft/vcpkg (MIT).

## angle -- from-scratch port, not a patch

Unlike every other port here, `angle` has no upstream vcpkg counterpart at
all: Google does not publish one, and ANGLE's own build is GN/Chromium-based,
not CMake. This port is a hand-written `CMakeLists.txt` (globbing ANGLE's own
source layout for `libEGL`/`libGLESv2`/`libANGLE`) pinned at commit
`aa292a59f9f222535c2ff34d8eecbe3cce039664` (2019-07-19), the same commit the
DALi-era build (`tool/windows/dali-vcpkg`) uses -- it also references this
directory as a second overlay path, so both graphs share one copy instead of
drifting. The current (non-DALi) Windows graph has not yet been moved to a
newer ANGLE; that is separate follow-up work, not a DALi-only limitation.

## gperf -- full replacement, not a patch

Unlike every other port here, `gperf`'s `portfile.cmake` and `vcpkg.json` are
**not** a patched copy of upstream's -- see the portfile's own comment for
why. Short version: upstream gperf builds via autotools, which on Windows
means bootstrapping the entire msys2/autoconf/automake/libtool toolchain and
running `./configure` under MSYS2's process-spawn overhead -- by far the
slowest step in this repo's vcpkg install, just to produce a small host tool
fontconfig needs at build time (to generate its perfect-hash lookup tables;
gperf itself never ships in the final product). This overlay instead installs
GnuWin32's prebuilt gperf 3.0.1 binary (a standalone PE32 executable, no mingw/
msys runtime DLLs required) to the exact same `tools/gperf/gperf.exe` path
fontconfig's portfile looks for. gperf's generated-code format has been stable
since well before 3.0, and fontconfig only uses gperf's basic ANSI-C table
generation, so the older prebuilt is safe here. `gperf.exe`/`COPYING` are
vendored directly in this directory (not fetched at build time) -- one small
~100 KB binary is exactly what the network self-heal effort elsewhere in this
repo (BART asset mirror, bulk prefetch) exists to work around, so this port
just has no network dependency at all instead. If a future fontconfig port
update ever needs a newer gperf feature, this port needs to be revisited (or
dropped back to upstream's autotools build).

This only applies on a Windows target, though. gperf is a *host* tool
(fontconfig looks it up via `CURRENT_HOST_INSTALLED_DIR`), so anything
cross-compiling Starfish for Windows from a non-Windows machine -- e.g. the
MSVC-on-Wine Docker rig -- needs a real gperf built for its own (non-Windows)
host triplet, and an overlay port replaces the named port for every triplet
that asks for it, not just Windows ones. `portfile.cmake`'s `else()` branch is
therefore upstream's actual 3.3 autotools recipe, byte-for-byte, gated on
`VCPKG_TARGET_IS_WINDOWS`; only the Windows branch is the fast vendored-binary
deviation. Hence the two license identifiers and the "3.3" version in
`vcpkg.json`, even though the Windows binary itself is the older 3.0.1.

## cairo -- `float-word-order-msvc.patch`

cairo's `meson.build` decides the float word order from `__FLOAT_WORD_ORDER__`
and `__ORDER_BIG_ENDIAN__`, which only GCC and Clang define. Under MSVC the
probe cannot tell "both undefined" from "both big endian", so it set
`FLOAT_WORDS_BIGENDIAN` on little-endian x86 and x64.

`_cairo_fixed_from_double()` then returns the *high* word of its magic-number
union -- the constant `0x42B80000` -- for every coordinate it converts. Paths
collapse to a single point, clips become empty, every fill is clipped away, and
cairo reports no error at any point. The visible symptom is a blank page while
the GL compositor still draws its own quads, which is why this took a long time
to find. The patch uses the machine endianness meson already knows.

Drop this when the fix is available upstream.

## libwebsockets -- `export-include-path.patch`, `msvc-warnings.patch`

Pinned at 5.0.0 (bumped from 4.5.8 so the port can build with
`LWS_WITH_SCHANNEL`, native Windows SChannel/SSPI TLS -- 4.5.8 only offered
OpenSSL-family backends or mbedTLS on Windows). The portfile forces
`-DLWS_WITH_SCHANNEL=ON` explicitly on Windows rather than relying on
upstream's default so a future upstream default change fails configure
loudly instead of silently trying to link OpenSSL, which this port's
`vcpkg.json` no longer depends on there (`"platform": "!windows"` on the
`openssl` dependency). Known gap: the schannel TLS backend only implements
`LCCSCF_ALLOW_SELFSIGNED` of the flags Starfish's `WebSecurityMode::Disable`
path sets (see `SocketLWS.cpp`) and ignores `client_ssl_ca_filepath`, so that
dev-only insecure mode is weaker on Windows than elsewhere.

`export-include-path` makes the installed CMake config point at its own
`../include` instead of a relative path that does not survive vcpkg's layout.
`msvc-warnings` keeps libwebsockets' `/W3 /WX` but suppresses C4018, C4133,
C4142, C4267 and C4996, which it otherwise fails the build on (C4018/C4133 only
fire on `x86-windows`, in the schannel backend files, which upstream evidently
never build-tested under `/WX` on 32-bit). The portfile also passes
`-DLWS_WITH_HTTP3=OFF -DLWS_ROLE_QUIC=OFF`: QUIC defaults on together with
schannel, its role files are riddled with more `x86-windows`-only `/WX` trips
(`C4244`, `uint64_t` truncating to a 32-bit `size_t`) than are worth
individually suppressing, and Starfish only ever speaks plain `ws(s)://`
(the H1/WS roles), never HTTP/3; and `-DLWS_WITH_LIBUV=OFF
-DLWS_WITH_EXTERNAL_POLL=OFF`: Starfish already calls `lws_service()` from
its own service thread, and taking the dependency would put a second `uv_*`
implementation in the process next to libtuv.
