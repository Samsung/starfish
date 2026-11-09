#!/usr/bin/env bash
# Checks out the pinned third_party/blink (Chromium) commit with only the web
# test directories the blink_test suite serves (see docs/blink_web_tests.md):
# the ones the legacy blink_* lists drew their copies from.
# tool/blink/update_lists.py lists the tests of every directory checked out
# here, so this is the one place the served directories are named. The
# exception is http/tests/inspector-protocol, which the CDP suite (tool/cdp_test)
# reads from this checkout and blink_test does not run.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

w=third_party/blink/web_tests
fast=(
  animation backgrounds block body-propagation borders box-shadow canvas css
  doctypes dom dynamic encoding eventsource harness hidpi html inline
  inline-block invalid js layers loader media overflow parser replaced
  scrolling sub-pixel table tokenizer xmlhttprequest
)
exec tool/common/sparse_submodule.sh third_party/blink \
  "$w/resources" \
  "$w/css3/flexbox" \
  "$w/svg" \
  "$w/http/tests/inspector-protocol" \
  "${fast[@]/#/$w/fast/}"
