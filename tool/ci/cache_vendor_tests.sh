#!/usr/bin/env bash
# usage: cache_vendor_tests.sh <webkit|blink|gecko>
#
# Producer/self-heal: ensures third_party/<vendor> (the browser-vendor tests
# the <vendor>_test suite runs: docs/webkit_layouttests.md,
# docs/blink_web_tests.md, docs/gecko_tests.md) is populated, and publishes
# it as its OWN BART cache tarball -- same split as cache_wpt.sh and
# cache_webgl.sh, for the same reason: only that suite's job needs it, and for
# Blink the CDP job, whose Chromium tests are in the same checkout.
# Unlike those two it is not a `git submodule update` (.gitmodules sets
# update = none): each vendor tree is several GB, so tool/<vendor>/fetch.sh
# checks out only the test directories the suite serves, and the key covers
# that script and the sparse checkout it calls, so a changed directory list
# gets a fresh tarball.
#
# Run with cwd = a checkout of this repo. Prints the cache id to stdout.
set -euo pipefail
LIB_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=lib_submodule_cache.sh
source "$LIB_DIR/lib_submodule_cache.sh"

vendor=$1
path=third_party/$vendor
key=$({ submodule_cache_key "$path"
        cat "tool/$vendor/fetch.sh" tool/common/sparse_submodule.sh | sha256sum; } |
      sha256sum | cut -d' ' -f1)
id=$(submodule_cache_id "$vendor" "$key")

if submodule_cache_exists "$id" cache.tar.gz; then
  tarball=$(mktemp)
  submodule_cache_fetch "$id" cache.tar.gz "$tarball"
  tar xzf "$tarball"
  rm -f "$tarball"
  echo "$id"
  exit 0
fi

# stdout carries only the id (see submodule_cache_push).
"./tool/$vendor/fetch.sh" >&2
tarball=$(mktemp)
tar czf "$tarball" "$path"
submodule_cache_push "$id" "$tarball" cache.tar.gz
rm -f "$tarball"
echo "$id"
