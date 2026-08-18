#!/usr/bin/env bash
# Producer/self-heal: ensures lwe_android_rel's third-party submodules
# (library/src/main/cpp/*) are populated, and publishes them as a BART
# cache tarball. Same mechanism as cache_starfish_thirdparty.sh, applied to
# lwe_android_rel's own submodule set -- kept as a separate file (rather
# than parameterizing one script) since the two repos' submodule lists and
# fetch conditions (proxy vs. not) are unrelated and change independently.
#
# Run with cwd = a checkout of lws/lwe_android_rel (submodules: false),
# invoked via its path inside a starfish checkout (e.g.
# ../starfish/tool/ci/cache_android_lwe_submodules.sh) so it can find its
# sibling lib script regardless of cwd. On success, prints the cache id
# (android-lwe-rel-<key>) to stdout -- callers capture it as a job output.
set -euo pipefail
LIB_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=lib_submodule_cache.sh
source "$LIB_DIR/lib_submodule_cache.sh"

# Same 14-path list android.yml has always used. NOTE: this used to be a
# bash brace expansion (.../{curl,icu,...}) -- broke outright on a real run
# ("error: pathspec '.../{curl,icu,...}' did not match any file(s)"; not
# every /bin/sh expands braces). Spelled out explicitly instead.
PATHS=(
  library/src/main/cpp/curl
  library/src/main/cpp/icu
  library/src/main/cpp/libpng
  library/src/main/cpp/openssl
  library/src/main/cpp/pixman
  library/src/main/cpp/libjpeg-turbo
  library/src/main/cpp/fontconfig
  library/src/main/cpp/expat
  library/src/main/cpp/cairo
  library/src/main/cpp/libtuv
  library/src/main/cpp/harfbuzz_ng
  library/src/main/cpp/nghttp2
  library/src/main/cpp/freetype
  library/src/main/cpp/giflib
)

key=$(submodule_cache_key "${PATHS[@]}")
id=$(submodule_cache_id android-lwe-rel "$key")

if submodule_cache_exists "$id" cache.tar.gz; then
  tarball=$(mktemp)
  submodule_cache_fetch "$id" cache.tar.gz "$tarball"
  tar xzf "$tarball"
  rm -f "$tarball"
  echo "$id"
  exit 0
fi

# Miss: this runner pool has direct internet access for these public
# vendored-library remotes -- do NOT add the starfish-thirdparty proxy here,
# it's unreachable from this pool and makes every fetch time out.
#
# Self-healing: only touch a module whose directory is actually empty, so a
# partially-populated checkout (e.g. a submodule added since this exact key
# was last cached) costs no network calls for what's already there.
for m in "${PATHS[@]}"; do
  if [ -z "$(ls -A "$m" 2>/dev/null)" ]; then
    retry_submodule_update 3 -- --depth 1 --init "$m"
  fi
done

# Same dangling-gitlink problem starfish's own cache script fixes -- make
# each submodule self-contained before archiving. Every entry in PATHS is
# already an exact leaf submodule path (no directory-prefix expansion
# needed here, unlike starfish's own third_party), so the plain form is
# enough.
submodule_make_standalone "${PATHS[@]}"

tarball=$(mktemp)
tar czf "$tarball" "${PATHS[@]}"
submodule_cache_push "$id" "$tarball" cache.tar.gz
rm -f "$tarball"
echo "$id"
