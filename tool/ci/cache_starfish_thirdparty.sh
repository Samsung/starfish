#!/usr/bin/env bash
# Producer/self-heal: ensures this checkout's binding_generator + third_party
# submodules are populated, and publishes them as a BART cache tarball for
# every downstream CI job to pull directly -- without every job needing its
# own proxy path to the actual submodule remotes.
#
# Run with cwd = a checkout of this repo (submodules: false). On success,
# prints the cache id (starfish-thirdparty-<key>) to stdout -- callers
# capture it as a job output.
#
# See lib_submodule_cache.sh for the storage design.
set -euo pipefail
LIB_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=lib_submodule_cache.sh
source "$LIB_DIR/lib_submodule_cache.sh"

PATHS=(binding_generator third_party)
key=$(submodule_cache_key "${PATHS[@]}")
id=$(submodule_cache_id starfish-thirdparty "$key")

if submodule_cache_exists "$id" cache.tar.gz; then
  tarball=$(mktemp)
  submodule_cache_fetch "$id" cache.tar.gz "$tarball"
  tar xzf "$tarball"
  rm -f "$tarball"
  echo "$id"
  exit 0
fi

# Miss: this is (so far) the only job with a proxy path to the actual
# third-party submodule remotes -- every other runner pool either can't
# reach them at all, or would need its own proxy+deploy-key setup
# duplicated everywhere.
export https_proxy=http://10.112.1.184:8080
export http_proxy=http://10.112.1.184:8080
git config --global http.sslVerify false
git config submodule.third_party/wpt.update none
retry_submodule_update 3 -- --init "${PATHS[@]}"
(cd third_party/escargot && retry_submodule_update 3 -- --init third_party)
(cd third_party/escargot/third_party/walrus && retry_submodule_update 3 -- --init third_party/sljit)

# wpt is deliberately NOT part of this job's cache (941MB vs. ~145MB for
# everything else here -- see worker.yml/x64_test.yml/wpt_status_nightly.yml,
# which each fetch it directly, no proxy needed since it's plain public
# github.com unlike everything else in PATHS). But the `update none` above
# is a LOCAL .git/config setting on *this checkout* -- and self-hosted
# runners reuse their on-disk workspace (including .git) across unrelated
# jobs of this same repo. Leaving it set here silently turned every later
# job's `git submodule update third_party/wpt` on that same reused
# workspace into a no-op (git honors update=none and skips, exit 0, no
# fetch) -- confirmed on a real run: worker.yml's test job reported "wpt
# serve failed: no wpt checkout ... (missing ./wpt)" despite its own
# `git submodule update third_party/wpt` step reporting success. Restore it
# before this job ends, same as the old prepare_source_without_thirdparty.sh
# used to.
git config --unset submodule.third_party/wpt.update

# The proxy above is only for the actual third-party submodule remotes --
# unset it before touching BART (reachable directly from this runner pool;
# routing it through this proxy too is unnecessary at best and can hang/
# fail at worst).
unset https_proxy http_proxy

# Each submodule's .git is a thin gitlink FILE pointing at this checkout's
# own .git/modules/<path> -- meaningless once tarred and extracted into a
# *different* job's checkout (which has no such entry). submodule_make_
# all_standalone embeds each submodule's real git directory into its own
# working tree in place of the pointer, making it self-contained and safe
# to move anywhere. "third_party" isn't itself a submodule (just a plain
# directory containing many, e.g. third_party/escargot, third_party/
# deviceapi, ...) -- the "_all" / --recursive form expands that to every
# individual submodule under it, including nested ones (escargot's own
# third_party, walrus's sljit), in one call.
submodule_make_all_standalone "${PATHS[@]}"

# nanomsg's own CMakeLists.txt runs `git describe`/`git diff` against its
# own submodule dir at configure time purely to compose a version string
# -- confirmed on a real consumer-job run: `tar xzf` restores this dir
# owned by whatever UID the tarball recorded, which doesn't match the
# consuming container's user, and git refuses to touch a repo it doesn't
# recognize as owned by the current user ("fatal: detected dubious
# ownership"). Rather than teach every consumer job about safe.directory
# for this one nested path, just drop the standalone .git entirely here:
# nanomsg's own CMakeLists already falls back cleanly to its checked-in
# third_party/nanomsg/.version file when `.git` doesn't exist (confirmed
# that file is present), and an unpopulated submodule (no .git at all, not
# even a dangling gitlink) is exactly what CODE-Actions/checkout's
# unconditional post-job `git submodule foreach --recursive` cleanup
# already silently skips -- confirmed on a throwaway repo: exit 0, no
# attempt to even enter the path.
rm -rf third_party/nanomsg/.git

tarball=$(mktemp)
# --exclude=third_party/wpt: wpt is deliberately excluded from git management
# here (update=none above), but that only stops *git* from touching it --
# it says nothing about whatever's already sitting on disk at that path.
# Self-hosted/pooled runners reuse their on-disk workspace across unrelated
# jobs of this same repo (see the wpt comment above), and a prior job that
# actually ran WPT tests there leaves behind a real venv, including a
# symlink pointing at an absolute host path (third_party/wpt/_venv3/bin/
# python3.9 -> /usr/bin/python3.9) -- confirmed on a real run: extracting
# that tarball on windows.yml's runner failed outright
# ("tarfile.LinkOutsideDestinationError: ... would link to
# 'D:\usr\bin\python3.9', which is outside the destination", Python 3.12+'s
# default extraction filter refusing a symlink target that escapes the
# destination). A plain `tar czf third_party` archives whatever's on disk
# under that path regardless of git's view of it, so exclude it explicitly
# rather than relying on it merely being unpopulated.
#
# --exclude='third_party/escargot/test/*': same class of contamination --
# this job never inits escargot's own "test" submodules (kangax, octane,
# test262, vendortest, web-tooling-benchmark; only escargot's own
# "third_party" and walrus's "third_party/sljit" are explicitly inited
# above), but *other* jobs sharing this same reused workspace do populate
# them to actually run escargot's test suite. Confirmed on a real cache
# tarball: these five directories alone accounted for ~614MB of a 3.35GB
# tarball -- pure leftover, never read by anything this cache serves.
tar czf "$tarball" \
  --exclude='third_party/wpt' \
  --exclude='third_party/escargot/test/*' \
  "${PATHS[@]}"
submodule_cache_push "$id" "$tarball" cache.tar.gz
rm -f "$tarball"
echo "$id"
