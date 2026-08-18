#!/usr/bin/env bash
# Windows-specific SUBSET of cache_starfish_thirdparty.sh's cache.
#
# build/windows.cmake only ever reads a fraction of what that cache
# contains -- confirmed by grepping its own THIRD_PARTY_ROOT references:
# binding_generator, MP4Parse, clipper, rapidxml, skia_matrix, webm,
# earcut.hpp, all of third_party/windows/*, and escargot (recursively, for
# its own third_party + walrus's sljit -- same as the general cache).
# webrtc, openssl/libpng/libjpeg-turbo/giflib/libtuv (non-windows builds of
# those -- third_party/windows/* has its own separate copies),
# nanomsg(cpp), httplib, googletest (non-escargot), deviceapi, libwebp, and
# libwebsockets (non-windows) are never read by this build at all -- this
# repo's own pre-cache-era windows.yml (see git history) used to `git
# submodule update --init` this exact same curated set directly on the
# Windows runner itself, before the general cache existed. Packing the
# FULL general-purpose cache into windows.yml wasted both fetch and
# (worse, interpreter-driven Windows-side extraction) time on content it
# never touches.
#
# MUST run after cache_starfish_thirdparty.sh in the SAME checkout: that
# script already fetches/self-heals + standalone-ifies (see
# lib_submodule_cache.sh's submodule_make_standalone) the full
# third_party/binding_generator tree this is a subset of. This script only
# ever reads what's already on disk -- no fetching of its own, no proxy.
set -euo pipefail
LIB_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=lib_submodule_cache.sh
source "$LIB_DIR/lib_submodule_cache.sh"

PATHS=(binding_generator third_party/MP4Parse third_party/clipper third_party/rapidxml third_party/skia_matrix third_party/webm third_party/earcut.hpp third_party/windows third_party/escargot)

for p in "${PATHS[@]}"; do
  [ -e "$p" ] || { echo "::error::$p missing -- cache_starfish_thirdparty_windows.sh must run after cache_starfish_thirdparty.sh in the same checkout" >&2; exit 1; }
done

key=$(submodule_cache_key "${PATHS[@]}")
id=$(submodule_cache_id starfish-thirdparty-windows "$key")

if submodule_cache_exists "$id" cache.zip; then
  echo "$id"
  exit 0
fi

# Miss: zip up this subset of what cache_starfish_thirdparty.sh already
# populated (and made standalone) in this same checkout -- see above, no
# fetching here.
#
# zip, not tar.gz: windows.yml extracts with PowerShell's native
# Expand-Archive (.NET's System.IO.Compression) instead of Python's
# tarfile module. Both can handle non-ASCII filenames correctly, but
# Expand-Archive runs compiled .NET code per entry instead of an
# interpreted Python loop, which matters across the thousands of small
# files here. Python's zipfile (unlike Info-ZIP's `zip` CLI by default)
# automatically sets the UTF-8 flag bit on any entry name that isn't
# plain ASCII, which .NET's ZipArchive honors on read -- this pairing is
# what actually avoids the bsdtar-with-tar.gz crash this repo hit before
# on Korean-named files (see cache_starfish_thirdparty.sh's
# --exclude=third_party/wpt comment) without needing Python on the
# extraction side at all.
#
# Symlinks aren't preserved (zipfile.write() dereferences them, copying
# the target's content instead) -- a deliberate side effect, not an
# oversight: it's what actually rules out a repeat of that same
# tarfile.LinkOutsideDestinationError class of bug for this cache, not
# just the exclude below (third_party/escargot/test/* is pruned purely
# because it's leftover contamination from other jobs reusing this same
# workspace, never actually inited by this script or
# cache_starfish_thirdparty.sh -- see there).
python3 - "${PATHS[@]}" <<'PYEOF'
import sys, zipfile, os

paths = sys.argv[1:]
prune = "third_party/escargot/test"

with zipfile.ZipFile("cache.zip", "w", zipfile.ZIP_DEFLATED) as zf:
    for base in paths:
        if os.path.isfile(base):
            zf.write(base)
            continue
        for root, dirs, files in os.walk(base):
            if root == prune or root.startswith(prune + os.sep):
                dirs[:] = []
                continue
            for f in files:
                p = os.path.join(root, f)
                if os.path.islink(p) and not os.path.exists(p):
                    continue  # broken symlink -- nothing to embed
                zf.write(p)
PYEOF

submodule_cache_push "$id" cache.zip cache.zip
rm -f cache.zip
echo "$id"
