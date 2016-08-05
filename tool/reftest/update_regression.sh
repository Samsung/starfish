#!/bin/bash

## W3C DOM Conformance Test Suites
cp tool/reftest/dom_conformance_test.res test/regression/tool/dom-conformance-test/test_dom_conformance
rm -rf test/regression/reftest/dom-conformance-test/html
cp -rf test/reftest/dom-conformance-test/html test/regression/reftest/dom-conformance-test/

## Web Platform Tests
# DOM
rm -rf test/regression/reftest/web-platform-tests/resources
cp -rf test/reftest/web-platform-tests/resources test/regression/reftest/web-platform-tests/
cp tool/reftest/wpt_dom.res test/regression/tool/web-platform-tests/test_wpt_dom
rm -rf test/regression/reftest/web-platform-tests/dom
cp -rf test/reftest/web-platform-tests/dom test/regression/reftest/web-platform-tests/

# DOM Events
cp tool/reftest/wpt_dom_events.res test/regression/tool/web-platform-tests/test_wpt_dom_events
rm -rf test/regression/reftest/web-platform-tests/DOMEvents
cp -rf test/reftest/web-platform-tests/DOMEvents test/regression/reftest/web-platform-tests/

# HTML
cp tool/reftest/wpt_html.res test/regression/tool/web-platform-tests/test_wpt_html
rm -rf test/regression/reftest/web-platform-tests/html
cp -rf test/reftest/web-platform-tests/html test/regression/reftest/web-platform-tests/

# Page Visibility
cp tool/reftest/wpt_page_visibility.res test/regression/tool/web-platform-tests/test_wpt_page_visibility
rm -rf test/regression/reftest/web-platform-tests/page-visibility
cp -rf test/reftest/web-platform-tests/page-visibility test/regression/reftest/web-platform-tests/

# Progress Events
cp tool/reftest/wpt_progress_events.res test/regression/tool/web-platform-tests/test_wpt_progress_events
rm -rf test/regression/reftest/web-platform-tests/progress-events
cp -rf test/reftest/web-platform-tests/progress-events test/regression/reftest/web-platform-tests/

# XMLHttpRequest
cp tool/reftest/wpt_xhr.res test/regression/tool/web-platform-tests/test_wpt_xhr
rm -rf test/regression/reftest/web-platform-tests/XMLHttpRequest
cp -rf test/reftest/web-platform-tests/XMLHttpRequest test/regression/reftest/web-platform-tests/

## Blink - W3C DOM Conformance Test Suites, fast/css, fast/dom, fast/html, fast/etc
cp tool/reftest/blink_dom_conformance_test.res test/regression/tool/vendor/blink/test_blink_dom_conformance
cp tool/reftest/blink_fast_css.res test/regression/tool/vendor/blink/test_blink_fast_css
cp tool/reftest/blink_fast_css_manual.res test/regression/tool/vendor/blink/test_blink_fast_css_manual
cp tool/reftest/blink_fast_dom.res test/regression/tool/vendor/blink/test_blink_fast_dom
cp tool/reftest/blink_fast_html.res test/regression/tool/vendor/blink/test_blink_fast_html
cp tool/reftest/blink_fast_etc.res test/regression/tool/vendor/blink/test_blink_fast_etc
cp tool/reftest/blink_fast_etc_manual.res test/regression/tool/vendor/blink/test_blink_fast_etc_manual
rm -rf test/regression/reftest/vendor/blink
cp -rf test/reftest/vendor/blink test/regression/reftest/vendor/

## Gecko - Mochitest: W3C DOM Conformance Test Suites
cp tool/reftest/gecko_dom_conformance_test.res test/regression/tool/vendor/gecko/test_gecko_dom_conformance
rm -rf test/regression/reftest/vendor/gecko/dom_tests_mochitest
cp -rf test/reftest/vendor/gecko/dom_tests_mochitest test/regression/reftest/vendor/gecko/

## WebKit - W3C DOM Conformance Test Suites, fast/css, fast/dom, fast/html, fast/etc
cp tool/reftest/webkit_dom_conformance_test.res test/regression/tool/vendor/webkit/test_webkit_dom_conformance
cp tool/reftest/webkit_fast_css.res test/regression/tool/vendor/webkit/test_webkit_fast_css
cp tool/reftest/webkit_fast_css_manual.res test/regression/tool/vendor/webkit/test_webkit_fast_css_manual
cp tool/reftest/webkit_fast_dom.res test/regression/tool/vendor/webkit/test_webkit_fast_dom
cp tool/reftest/webkit_fast_html.res test/regression/tool/vendor/webkit/test_webkit_fast_html
cp tool/reftest/webkit_fast_etc.res test/regression/tool/vendor/webkit/test_webkit_fast_etc
cp tool/reftest/webkit_fast_etc_manual.res test/regression/tool/vendor/webkit/test_webkit_fast_etc_manual
rm -rf test/regression/reftest/vendor/webkit
cp -rf test/reftest/vendor/webkit test/regression/reftest/vendor/

## TODO: W3C CSS Tests
# css1
rm -rf test/regression/reftest/csswg-test/css1
cp -rf test/reftest/csswg-test/css1_converted test/regression/reftest/csswg-test/css1
rm -rf test/regression/reftest/csswg-test/css1_result/font_independent
cp -rf test/reftest/csswg-test/css1_result/font_independent test/regression/reftest/csswg-test/css1_result

# css21
rm -rf test/regression/reftest/csswg-test/css21
cp -rf test/reftest/csswg-test/css21_converted test/regression/reftest/csswg-test/css21
rm -rf test/regression/reftest/csswg-test/css21_result/font_independent
cp -rf test/reftest/csswg-test/css21_result/font_independent test/regression/reftest/csswg-test/css21_result

# css-backgrounds-3
rm -rf test/regression/reftest/csswg-test/css-backgrounds-3
cp -rf test/reftest/csswg-test/css-backgrounds-3_converted test/regression/reftest/csswg-test/css-backgrounds-3
rm -rf test/regression/reftest/csswg-test/css-backgrounds-3_result/font_independent
cp -rf test/reftest/csswg-test/css-backgrounds-3_result/font_independent test/regression/reftest/csswg-test/css-backgrounds-3_result

# css-color-3
rm -rf test/regression/reftest/csswg-test/css-color-3
cp -rf test/reftest/csswg-test/css-color-3_converted test/regression/reftest/csswg-test/css-color-3
rm -rf test/regression/reftest/csswg-test/css-color-3_result/font_independent
cp -rf test/reftest/csswg-test/css-color-3_result/font_independent test/regression/reftest/csswg-test/css-color-3_result

# css-transforms-1
rm -rf test/regression/reftest/csswg-test/css-transforms-1
cp -rf test/reftest/csswg-test/css-transforms-1_converted test/regression/reftest/csswg-test/css-transforms-1
rm -rf test/regression/reftest/csswg-test/css-transforms-1_result/font_independent
cp -rf test/reftest/csswg-test/css-transforms-1_result/font_independent test/regression/reftest/csswg-test/css-transforms-1_result

