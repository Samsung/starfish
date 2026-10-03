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

cache_public_submodule wpt third_party/wpt
