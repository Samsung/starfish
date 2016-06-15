#!/bin/bash

## W3C DOM Conformance Test Suites
cp tool/reftest/dom_conformance_test.res test/regression/tool/dom-conformance-test/test_dom_conformance
rm test/regression/reftest/dom-conformance-test/html -rf
cp test/reftest/dom-conformance-test/html test/regression/reftest/dom-conformance-test/ -rf

## Web Platform Tests
# DOM
rm test/regression/reftest/web-platform-tests/resources -rf
cp test/reftest/web-platform-tests/resources test/regression/reftest/web-platform-tests/ -rf
cp tool/reftest/wpt_dom.res test/regression/tool/web-platform-tests/test_wpt_dom
rm test/regression/reftest/web-platform-tests/dom -rf
cp test/reftest/web-platform-tests/dom test/regression/reftest/web-platform-tests/ -rf

# DOM Events
cp tool/reftest/wpt_dom_events.res test/regression/tool/web-platform-tests/test_wpt_dom_events
rm test/regression/reftest/web-platform-tests/DOMEvents -rf
cp test/reftest/web-platform-tests/DOMEvents test/regression/reftest/web-platform-tests/ -rf

# HTML
cp tool/reftest/wpt_html.res test/regression/tool/web-platform-tests/test_wpt_html
rm test/regression/reftest/web-platform-tests/html -rf
cp test/reftest/web-platform-tests/html test/regression/reftest/web-platform-tests/ -rf

# Page Visibility
cp tool/reftest/wpt_page_visibility.res test/regression/tool/web-platform-tests/test_wpt_page_visibility
rm test/regression/reftest/web-platform-tests/page-visibility -rf
cp test/reftest/web-platform-tests/page-visibility test/regression/reftest/web-platform-tests/ -rf

# Progress Events
cp tool/reftest/wpt_progress_events.res test/regression/tool/web-platform-tests/test_wpt_progress_events
rm test/regression/reftest/web-platform-tests/progress-events -rf
cp test/reftest/web-platform-tests/progress-events test/regression/reftest/web-platform-tests/ -rf

# XMLHttpRequest
cp tool/reftest/wpt_xhr.res test/regression/tool/web-platform-tests/test_wpt_xhr
rm test/regression/reftest/web-platform-tests/XMLHttpRequest -rf
cp test/reftest/web-platform-tests/XMLHttpRequest test/regression/reftest/web-platform-tests/ -rf

## Blink
# W3C DOM Conformance Test Suites
cp tool/reftest/blink_dom_conformance_test.res test/regression/tool/vendor/blink/test_blink_dom_conformance
rm test/regression/reftest/vendor/blink/dom/html -rf
cp test/reftest/vendor/blink/dom/html test/regression/reftest/vendor/blink/dom/ -rf

# fast/dom
rm test/regression/reftest/vendor/blink/resources -rf
cp test/reftest/vendor/blink/resources test/regression/reftest/vendor/blink -rf
cp tool/reftest/blink_fast_dom.res test/regression/tool/vendor/blink/test_blink_fast_dom
rm test/regression/reftest/vendor/blink/fast/dom -rf
mkdir -p test/regression/reftest/vendor/blink/fast/dom
cp test/reftest/vendor/blink/fast/dom test/regression/reftest/vendor/blink/fast -rf

# fast/html
cp tool/reftest/blink_fast_html.res test/regression/tool/vendor/blink/test_blink_fast_html
rm test/regression/reftest/vendor/blink/fast/html -rf
mkdir -p test/regression/reftest/vendor/blink/fast/html
cp test/reftest/vendor/blink/fast/html test/regression/reftest/vendor/blink/fast -rf

## Gecko
# Mochitest - W3C DOM Conformance Test Suites
cp tool/reftest/gecko_dom_conformance_test.res test/regression/tool/vendor/gecko/test_gecko_dom_conformance
rm test/regression/reftest/vendor/gecko/dom_tests_mochitest -rf
cp test/reftest/vendor/gecko/dom_tests_mochitest test/regression/reftest/vendor/gecko/ -rf

## WebKit
# W3C DOM Conformance Test Suites
cp tool/reftest/webkit_dom_conformance_test.res test/regression/tool/vendor/webkit/test_webkit_dom_conformance
rm test/regression/reftest/vendor/webkit/dom/html -rf
cp test/reftest/vendor/webkit/dom/html test/regression/reftest/vendor/webkit/dom/ -rf

# fast/dom
rm test/regression/reftest/vendor/webkit/resources -rf
cp test/reftest/vendor/webkit/resources test/regression/reftest/vendor/webkit -rf
cp tool/reftest/webkit_fast_dom.res test/regression/tool/vendor/webkit/test_webkit_fast_dom
rm test/regression/reftest/vendor/webkit/fast/dom -rf
mkdir -p test/regression/reftest/vendor/webkit/fast/dom
cp test/reftest/vendor/webkit/fast/dom test/regression/reftest/vendor/webkit/fast -rf

# fast/html
cp tool/reftest/webkit_fast_html.res test/regression/tool/vendor/webkit/test_webkit_fast_html
rm test/regression/reftest/vendor/webkit/fast/html -rf
mkdir -p test/regression/reftest/vendor/webkit/fast/html
cp test/reftest/vendor/webkit/fast/html test/regression/reftest/vendor/webkit/fast -rf

## TODO: W3C Internationalization Tests

## TODO: W3C CSS Tests

