#!/usr/bin/env bash
# Checks out the pinned third_party/gecko (Firefox) commit with only the test
# directories the gecko_test suite serves (see docs/gecko_tests.md): the ones
# the legacy gecko_* lists drew their copies from, the mochitest harness and
# the fonts the reftests load. tool/gecko/update_lists.py lists the tests of
# every directory checked out here that has a reftest.list or a
# mochitest.toml, so this is the one place the served directories are named.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

exec tool/common/sparse_submodule.sh third_party/gecko \
  testing/mochitest/tests/SimpleTest \
  dom/tests/mochitest/dom-level1-core \
  dom/tests/mochitest/dom-level2-core \
  dom/tests/mochitest/dom-level2-html \
  dom/tests/mochitest/general \
  layout/reftests/fonts \
  layout/reftests/box-sizing \
  layout/reftests/floats \
  layout/reftests/text-indent \
  layout/reftests/text-transform
