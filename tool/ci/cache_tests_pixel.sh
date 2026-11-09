#!/usr/bin/env bash
# Producer/self-heal: ensures tests/pixel (the image-compared internal tests)
# is populated, and publishes it as its OWN BART cache tarball -- same split
# as cache_webgl.sh: only the internal_test jobs need it (~100MB), so it stays
# out of cache_starfish_thirdparty.sh's shared tarball.
#
# Run with cwd = a checkout of this repo. Self-contained: safe to call from
# any job that needs it -- whichever job hits this first for a given pin
# builds and publishes it, everyone else after that just fetches it. On
# success, prints the cache id to stdout.
set -euo pipefail
LIB_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=lib_submodule_cache.sh
source "$LIB_DIR/lib_submodule_cache.sh"

cache_public_submodule tests-pixel tests/pixel
