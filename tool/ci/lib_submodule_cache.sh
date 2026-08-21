# Shared helpers for submodule caching against BART (this org's internal
# JFrog Artifactory instance, https://bart.sec.samsung.net/artifactory).
# Sourced by cache_starfish_thirdparty.sh, cache_wpt.sh, and
# cache_android_lwe_submodules.sh -- not meant to be executed directly.
#
# Third attempt at this (see git history for the first two): GitHub Actions
# cache/artifacts don't work at all on this GHES instance (no storage
# backend configured, silently no-ops on save/restore; cross-run artifact
# reuse via the REST API was slow/unreliable too), so the second attempt
# stored cache tarballs as parentless commits on `cache/sub-module/*`
# branches of this same GHES repo instead. That worked but fought git the
# whole way: a 100MB-per-blob hard limit meant every tarball had to be
# split into chunks and reassembled with a `git show` per chunk, and
# fetching an orphan ref made the GHES server rebuild its pack from scratch
# every time (no bitmap reuse against unrelated history) -- confirmed on a
# real run taking 8+ minutes to fetch a ~150MB tarball this way.
#
# BART is a plain artifact store already reachable directly from every
# runner pool this repo uses (same host these workflows already pull
# Docker images from) -- no blob-size chunking, no git pack machinery, just
# HTTP PUT/GET/HEAD of one file. Stored under this repo's own dedicated
# generic local repo (starfish-git-cache-service-generic-local) at
# starfish-ci-cache/<name>-<key>/cache.tar.gz -- overwriting the same path
# with the same content is a plain no-op file replace, so concurrent runs
# racing on the same key or a weekly rebuild re-publishing an unchanged pin
# set behave the same as the old content-addressed git commits did.

# Requires BART_API_KEY in the calling step's env (the caller must set
# `env: BART_API_KEY: ${{ secrets.BART_API_KEY }}` -- it's never set by
# default). Legacy Artifactory API key, sent via the X-JFrog-Art-Api header
# (confirmed working against this instance; newer Artifactory versions
# prefer bearer access tokens instead, but this org's key is the older
# format).
: "${BART_API_KEY:?lib_submodule_cache.sh needs BART_API_KEY in env -- add \`env: BART_API_KEY: \${{ secrets.BART_API_KEY }}\` to the calling step}"

# Self-hosted runners reuse the same on-disk workspace across unrelated
# jobs/PRs (the recurring theme of every leftover-state comment in this
# file), and each submodule is its OWN separate git repository from git's
# ownership-check point of view -- confirmed on a real run: plain `git
# submodule update --init` itself refusing to touch third_party/escargot
# with "fatal: detected dubious ownership in repository at
# '.../third_party/escargot'", i.e. this bites during the very FIRST
# populate, before any build/cmake step exists to work around it in.
# Every calling workflow's own "Add safe directory" YAML step only
# registers the ONE exact top-level checkout path, which doesn't cover
# this. Registering a wildcard here instead (in this shared, sourced-by-
# every-cache-script file, not per-workflow YAML) covers every submodule
# path any of these scripts touches, uniformly, with no YAML changes
# needed. Safe: every path under this workspace is this repo's own
# trusted content, never attacker-controlled.
git config --global --add safe.directory '*'

BART_BASE_URL="https://bart.sec.samsung.net/artifactory/starfish-git-cache-service-generic-local/starfish-ci-cache"
# --retry: BART is an internal host reachable directly from every runner
# pool here (no corporate egress proxy in the way, unlike the actual
# third-party submodule remotes), so failures are expected to be rare
# transient blips, not the frequent mid-fetch drops cache_wpt.sh works
# around for public github.com -- a handful of quick retries is enough.
CACHE_CURL=(curl -sS --fail --retry 3 --retry-delay 2 -H "X-JFrog-Art-Api: ${BART_API_KEY}")

# retry_submodule_update <n> -- <git submodule update args...> -- re-runs
# the given `git submodule update` invocation up to <n> times. Git's own
# submodule--helper already retries transient CLONE failures internally
# ("Failed to clone 'X'. Retry scheduled") but does NOT retry a partial/
# interrupted fetch that clones "successfully" (exit 0) yet leaves a pinned
# commit unreachable -- confirmed on a real self-hosted-runner run:
# `fatal: Unable to find current revision in submodule path
# 'third_party/windows/windows_pthread'`, even though that exact commit
# fetches fine from a normal, unaffected network path (a non-default branch
# commit, not missing/rewritten upstream). Re-running the whole command is
# idempotent -- already-populated submodules are left alone, so a retry only
# costs time on the ones that actually failed.
#
# git's stdout is redirected to stderr here (not just left alone): on a
# cache miss, `git submodule update --init` prints one "Submodule path 'X':
# checked out 'Y'" line per newly-cloned submodule to STDOUT by default --
# and every caller of this function is itself the last thing running inside
# a `id=$(cache_starfish_thirdparty.sh)`-style command substitution in the
# workflow YAML, which captures the *entire* script's stdout as the value.
# Those progress lines landed ahead of the script's final `echo "$id"` and
# got written verbatim into $GITHUB_OUTPUT together with it, breaking its
# `key=value` line format -- confirmed on a real run: "Error: Invalid
# format 'Submodule path '\''third_party/MP4Parse'\'': checked out ...'".
# Callers only ever check this function's exit status, never its stdout, so
# discarding git's own chatter here is safe.
retry_submodule_update() {
  local n="$1"; shift
  shift # drop the "--" separator
  local i
  for ((i = 1; i <= n; i++)); do
    if git submodule update "$@" >&2; then
      return 0
    fi
    echo "git submodule update failed (attempt $i/$n), retrying..." >&2
  done
  return 1
}

# submodule_make_standalone <path...> -- makes each given LEAF submodule
# directory (one with its own .git gitlink, not a directory that merely
# *contains* further submodules) fully self-contained: a real .git
# DIRECTORY living inside <path> itself, instead of a gitlink FILE pointing
# at this checkout's own .git/modules/<path>. Needed because the cache
# tarball gets extracted into a completely different checkout in every
# downstream job -- a gitlink pointing at *this* checkout's .git/modules is
# dangling the instant it's extracted anywhere else (confirmed on a real
# run: "fatal: not a git repository: binding_generator/../.git/modules/
# binding_generator" during CODE-Actions/checkout's own post-job cleanup,
# which unconditionally runs `git submodule foreach --recursive` regardless
# of the checkout's `submodules:` input).
#
# NOTE: this used to just MOVE each submodule's real gitdir into place
# (`mv .git/modules/<path> <path>/.git`) -- self-contained, but that gitdir
# carries the submodule's ENTIRE upstream history. Nothing downstream ever
# reads a submodule's git log, only its checked-out files -- confirmed on a
# real cache tarball: .git/objects across this job's submodules (webrtc
# alone: ~2.2GB; escargot: ~430MB) accounted for ~3GB of a 3.35GB tarball,
# making every download -- even a cache HIT -- take 5+ minutes.
#
# Fix: `git clone --depth 1` the submodule's own real gitdir into a scratch
# dir (a plain local path, no network -- this only ever READS from
# .git/modules/<path>, never mutates or moves it, unlike the old approach),
# then swap in *that* clone's tiny .git in place of the gitlink. A local
# clone of a repo whose HEAD is detached at a specific commit (exactly
# what a submodule always is) clones that exact commit, not some default
# branch -- so this preserves the precise pinned SHA (anything downstream
# that reads a submodule's own `git rev-parse HEAD` still sees the same
# commit it always did), it just drops everything only reachable through
# ancestry. Deliberately NOT `--no-checkout`: that leaves the clone's own
# index EMPTY (not populated to match HEAD) -- confirmed with a throwaway
# repo, `git status` afterwards showed the real, already-correct working
# tree files as merely "untracked". A normal (checked-out) clone builds a
# correct index against <path>'s existing, identical-content files; we
# only take its .git dir and throw the rest of the scratch clone away.
#
# Unlike the old move-based approach, this never touches this checkout's
# own .git/modules/<path> at all (only clones FROM it) -- so processing
# order between nested submodules (e.g. escargot's own third_party vs. its
# parent) no longer matters.
#
# BUG FIXED HERE: this used to skip any <path> whose .git was already a
# real DIRECTORY (only gitlink FILEs were considered convertible), on the
# assumption that a real directory meant "already standalone-ified by an
# earlier run of this same function". Self-hosted runners reuse the same
# on-disk workspace across unrelated jobs (confirmed elsewhere in this
# file, for wpt/escargot-test leftovers) -- so a directory left behind by
# a run from BEFORE this shallow-clone fix existed (back when this
# function just `mv`'d the entire non-shallow gitdir into place) is also a
# real directory, and looked identical to this function. Every later run
# on that same reused workspace silently kept re-publishing that stale,
# full-history .git forever, since nothing ever re-checked it. Confirmed
# on real cache tarballs still live in BART: webrtc/.git alone was still
# 2.19GB, long after this fix supposedly shipped. Fix: check whether an
# existing directory is actually shallow instead of assuming so.
submodule_make_standalone() {
  local path realgitdir tmp
  for path in "$@"; do
    if [ -f "$path/.git" ]; then
      : # gitlink file -- needs conversion below
    elif [ -d "$path/.git" ]; then
      [ "$(git -C "$path" rev-parse --is-shallow-repository 2>/dev/null)" = "true" ] && continue
    else
      continue # not a submodule at all
    fi
    realgitdir=$(git -C "$path" rev-parse --absolute-git-dir)
    tmp=$(mktemp -d)
    git clone --quiet --no-tags --depth 1 "file://$realgitdir" "$tmp"
    rm -rf "$path/.git"
    mv "$tmp/.git" "$path/.git"
    rm -rf "$tmp"
  done
}

# submodule_sync_standalone <path...> -- MUST be called after
# submodule_make_standalone / submodule_make_all_standalone, with the SAME
# top-level pathspec (not the recursively-expanded per-submodule list --
# `git submodule sync`'s own pathspec matching only resolves DIRECT
# children of the level it's run from; confirmed on a real checkout:
# `git submodule sync -- third_party/escargot/third_party/walrus` from the
# repo root errors "pathspec ... did not match any file(s) known to git",
# while `git submodule sync --recursive -- third_party` succeeds and walks
# the whole nested tree itself).
#
# The local clone in submodule_make_standalone leaves each submodule's own
# "origin" pointed at file://<this checkout's .git/modules/path> -- a
# local path with no meaning once this workspace is reused by a later job.
# Self-hosted runners reuse the same on-disk workspace/.git across
# unrelated jobs (same theme as every other leftover-state bug documented
# in this file), so that bogus origin silently survives into the NEXT
# job's plain `git submodule update`. That's harmless until the
# superproject bumps this submodule's pin to a commit the (depth-1,
# now-detached) local history doesn't have: update falls back to fetching
# from the submodule's configured origin -- confirmed on a real run:
# `fatal: transport 'file' not allowed` (git blocks the file transport for
# submodule fetches by default, CVE-2022-39253), even though the pinned
# commit fetches fine from the real upstream and the job's own checkout is
# otherwise unrelated to any of this.
#
# `git submodule sync` restores each path's origin from .gitmodules (the
# working-tree file, never touched by the clone above), not from any
# possibly-already-contaminated .git state -- safe to call unconditionally.
#
# `sync` alone is NOT enough for anything below the first swapped level,
# though: it only touches submodules already *registered* (a
# submodule.<name>.url entry in the parent's own .git/config, normally
# written by `submodule init`/`update --init`) -- and the swap's plain
# `git clone` produces a fresh .git with no such entries at all (clone
# doesn't carry them; only .gitmodules + an explicit init does). So for a
# submodule-of-a-submodule (e.g. mid/leaf), the very swap that just ran on
# "mid" wiped out mid's own registration of "leaf" in the same step --
# confirmed on a real test: `git submodule sync --recursive` on a freshly
# swapped tree silently synced only the top level and printed nothing for
# the nested one, no error either. `update --init --recursive` first
# re-registers every level from each level's own (untouched) .gitmodules;
# since the working tree is already checked out at the exact pinned commit
# (the swap only replaced .git metadata, not files), this is a pure
# no-op checkout with no network access -- confirmed no fetch/clone output.
submodule_sync_standalone() {
  git submodule update --init --recursive -- "$@" >&2
  git submodule sync --recursive -- "$@" >&2
}

# submodule_make_all_standalone <path...> -- submodule_make_standalone for
# every submodule registered under the given path(s), recursively -- so a
# directory-prefix pathspec like "third_party" (which isn't itself a
# submodule, just a plain directory containing many) expands to every
# individual submodule under it, including nested submodules-of-submodules,
# in one call.
submodule_make_all_standalone() {
  submodule_make_standalone $(git submodule status --recursive -- "$@" | awk '{print $2}')
  submodule_sync_standalone "$@"
}

# submodule_cache_key <path...> -- sha256 of the recorded gitlink pins under
# the given paths. Reads straight from the tree (`ls-tree -r` + filter down
# to gitlink/commit entries, i.e. actual submodules -- excludes plain files
# that happen to live alongside them), NOT
# `git submodule status`: that command reports whatever commit is actually
# checked out on disk for an already-initialized submodule (a "+<sha>" line
# with the on-disk HEAD, not the superproject's pinned one), plus a
# describe-style suffix that depends on which remote branches/tags this
# local repo happens to know about. Self-hosted runners reuse their on-disk
# workspace across unrelated jobs/PRs -- confirmed on a real checkout: the
# exact same pinned commit produced two different `submodule status` outputs
# (and thus two different cache keys) depending on leftover submodule state
# from whatever ran there before. `ls-tree` only ever reads the commit's own
# tree object, so it's fully deterministic regardless of what's sitting on
# disk or which remotes are configured.
submodule_cache_key() {
  git ls-tree -r HEAD -- "$@" | grep '^160000 commit ' | sha256sum | cut -d' ' -f1
}

# submodule_cache_id <name> <key> -- identifier string used as this cache's
# path under BART_BASE_URL. Callers pass this around (job outputs, etc.)
# the same opaque way they used to pass a git branch name.
submodule_cache_id() {
  echo "${1}-${2}"
}

# submodule_cache_exists <id> <filename> -- 0 (true) if <filename> (e.g.
# cache.tar.gz, cache.zip) already exists at this id's path on BART.
submodule_cache_exists() {
  local code
  code=$("${CACHE_CURL[@]}" -o /dev/null -w '%{http_code}' -I "${BART_BASE_URL}/$1/$2" 2>/dev/null) || true
  [ "$code" = "200" ]
}

# submodule_cache_fetch <id> <filename> <out-file> -- downloads this id's
# <filename> to <out-file>. <filename> is an explicit argument (not always
# "cache.tar.gz") so a caller can publish/consume a different archive format
# for the same mechanism.
submodule_cache_fetch() {
  "${CACHE_CURL[@]}" -o "$3" "${BART_BASE_URL}/$1/$2"
}

# submodule_cache_push <id> <local-file-path> <filename> -- uploads
# <local-file-path> to this id's <filename> path, overwriting whatever (if
# anything) was there before.
#
# -o /dev/null: curl prints the PUT response body (Artifactory's upload
# confirmation JSON) to stdout by default -- every caller of this function
# is itself the last thing running inside an `id=$(cache_starfish_thirdparty.sh)`
# -style command substitution in the workflow YAML, which captures the
# *entire* script's stdout as the value. That JSON landed ahead of the
# script's final `echo "$id"` and got written verbatim into $GITHUB_OUTPUT
# together with it, breaking its `key=value` line format -- confirmed on a
# real run: "Invalid format '  \"repo\" : \"escargot-generic-local\",'"
# (the exact same class of bug retry_submodule_update's own stdout
# redirect works around above). Callers only ever check this function's
# exit status, never its stdout, so discarding curl's response body here
# is safe.
submodule_cache_push() {
  "${CACHE_CURL[@]}" -o /dev/null -T "$2" "${BART_BASE_URL}/$1/$3"
}
