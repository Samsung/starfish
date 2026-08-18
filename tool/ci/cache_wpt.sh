#!/usr/bin/env bash
# Producer/self-heal: ensures third_party/wpt is populated, and publishes it
# as its OWN BART cache tarball -- kept separate from
# cache_starfish_thirdparty.sh's cache (~145MB) because wpt alone is
# ~940MB (528MB shallow .git + ~410MB working tree -- see .gitmodules'
# `shallow = true` for third_party/wpt) and most consumers (windows, minor,
# android, gl_backend, dynamic_loader, ...) never touch it at all; only the
# handful of jobs that actually run WPT tests do (worker.yml,
# x64_test.yml's wpt_serve_* jobs, wpt_status_nightly.yml). Folding it into
# the shared thirdparty tarball would make every other job pay for it too.
#
# wpt's remote is plain public github.com (see .gitmodules -- no internal
# proxy needed, unlike everything in cache_starfish_thirdparty.sh's PATHS),
# but this environment's network drops mid-fetch often enough on its own
# that a live ~940MB clone on every one of those call sites, on every run,
# is worth caching purely for reliability -- same rationale as everything
# else in lib_submodule_cache.sh, not a reachability problem this time.
#
# Run with cwd = a checkout of this repo. Self-contained: safe to call from
# any of the WPT-running jobs directly (no separate producer job/`needs:`
# wiring required, unlike starfish's own thirdparty cache) -- whichever job
# hits this first for a given wpt pin builds and publishes it, everyone
# else after that just fetches it. On success, prints the cache id to
# stdout.
set -euo pipefail
LIB_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=lib_submodule_cache.sh
source "$LIB_DIR/lib_submodule_cache.sh"

PATHS=(third_party/wpt)
key=$(submodule_cache_key "${PATHS[@]}")
id=$(submodule_cache_id wpt "$key")

if submodule_cache_exists "$id" cache.tar.gz; then
  tarball=$(mktemp)
  submodule_cache_fetch "$id" cache.tar.gz "$tarball"
  tar xzf "$tarball"
  rm -f "$tarball"
  echo "$id"
  exit 0
fi

# Miss: plain public github.com, no proxy needed -- just resilience against
# this network's frequent mid-fetch drops.
#
# Defensive unset first: cache_starfish_thirdparty.sh sets this locally
# (wpt isn't part of its own cache) and normally unsets it before finishing,
# but self-hosted runners reuse their on-disk workspace/.git across
# unrelated jobs of this repo -- if that ever leaks into whatever workspace
# this script runs in anyway, git would honor update=none and silently
# no-op the fetch below (exit 0, nothing fetched) instead of erroring.
# Confirmed on a real run before this script existed. Clearing it here
# means this script doesn't depend on some other job's cleanup succeeding.
git config --unset submodule.third_party/wpt.update 2>/dev/null || true
retry_submodule_update 3 -- --init "${PATHS[@]}"

# Same dangling-gitlink problem starfish's own cache script fixes -- make it
# self-contained before archiving so it survives being extracted into a
# completely different checkout by whichever job hits the cache next.
submodule_make_standalone "${PATHS[@]}"

tarball=$(mktemp)
tar czf "$tarball" "${PATHS[@]}"
submodule_cache_push "$id" "$tarball" cache.tar.gz
rm -f "$tarball"
echo "$id"
