The Web Platfrom Tests Project
==============================

These are imported from [The Web Platform Tests Project](https://github.com/w3c/web-platform-tests). The Web Platform Tests Project is a W3C-coordinated attempt to build a cross-browser testsuite for the Web-platform stack. However, StarFish is not a browser, so we don't need to import all test suites.

The test suites that we use are like below:

* dom
* DOMEvents
* html
* page-visibility
* XMLHttpRequest

Running the Tests
=================

The tests of Web Platform Tests Project are designed to be run from your local computer using HTTP servers. But we don't use these tools for the test because StarFish doesn't support enough spec to run these tools. There are some changes.

* resources/testharness.js
* Absolute path is changed to relative path in the tests
