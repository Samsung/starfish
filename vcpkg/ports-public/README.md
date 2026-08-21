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
