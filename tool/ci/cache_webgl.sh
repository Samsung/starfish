#!/usr/bin/env bash
# Producer/self-heal: ensures third_party/webgl (the Khronos WebGL
# conformance suites, see docs/khronos_webgl.md) is populated, and publishes
# it as its OWN BART cache tarball -- same split as cache_wpt.sh, for the
# same reason: only the khronos_test job needs it (~150MB), so it must not
# ride along in cache_starfish_thirdparty.sh's shared tarball that every
# build job restores (that script excludes it explicitly).
#
# Run with cwd = a checkout of this repo. Self-contained: safe to call from
# the khronos_test job directly -- whichever job hits this first for a given
# pin builds and publishes it, everyone else after that just fetches it. On
# success, prints the cache id to stdout.
set -euo pipefail
LIB_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=lib_submodule_cache.sh
source "$LIB_DIR/lib_submodule_cache.sh"

cache_public_submodule webgl third_party/webgl
