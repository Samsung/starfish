#!/usr/bin/env bash
# Checks out the pinned third_party/webkit commit with only the LayoutTests
# directories the webkit_test suite serves (see docs/webkit_layouttests.md):
# the ones the legacy webkit_* lists drew their copies from.
# tool/webkit/update_lists.py lists the tests of every directory checked out
# here, so this is the one place the served directories are named.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

t=LayoutTests
fast=(
  animation backgrounds block body-propagation borders box-decoration-break
  box-sizing css css3-text dom dynamic encoding events harness hidpi html
  images inline inline-block invalid layers loader parser repaint replaced
  selectors sub-pixel text tokenizer xmlhttprequest
)
exec tool/common/sparse_submodule.sh third_party/webkit \
  "$t/resources" \
  "$t/dom/html" \
  "${fast[@]/#/$t/fast/}"
