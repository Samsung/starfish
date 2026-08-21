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
