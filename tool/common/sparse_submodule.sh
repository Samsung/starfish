#!/usr/bin/env bash
# usage: sparse_submodule.sh <submodule path> <directory>...
#
# Checks out the pinned commit of a browser-vendor test submodule
# (third_party/webkit, third_party/blink, third_party/gecko) with only the
# given directories. Those trees are several GB, so .gitmodules sets
# update = none and `git submodule update` never checks them out; each
# suite's tool/<vendor>/fetch.sh calls this instead. Safe to rerun: it only
# fetches when the pin or the directory list changed.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

path=$1
shift

pin=$(git ls-files -s -- "$path" | awk '{print $2}')
url=$(git config -f .gitmodules "submodule.$path.url")
[ -n "$pin" ] || { echo "no gitlink recorded for $path" >&2; exit 1; }

if [ ! -e "$path/.git" ]; then
  git init -q "$path"
  git -C "$path" remote add origin "$url"
fi
git -C "$path" sparse-checkout set --cone "$@"
if [ "$(git -C "$path" rev-parse -q --verify HEAD || true)" != "$pin" ]; then
  # blob:none: only the blobs of the sparse directories are downloaded,
  # lazily by the checkout below.
  git -C "$path" fetch -q --depth 1 --filter=blob:none origin "$pin"
  git -C "$path" checkout -q --detach "$pin"
fi
