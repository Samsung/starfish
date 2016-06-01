The Web Platfrom Tests Project
==============================

These are imported from [The Web Platform Tests Project](https://github.com/w3c/web-platform-tests). The Web Platform Tests Project is a W3C-coordinated attempt to build a cross-browser testsuite for the Web-platform stack. However, StarFish is not a browser, so we don't need to import all test suites.

The test suites that we use are like below:

* dom
* DOMEvents
* html
* page-visibility
* progress-events
* XMLHttpRequest

Running the Tests
=================

The tests of Web Platform Tests Project are designed to be run from your local computer using HTTP servers. But we don't use these tools for the test because StarFish doesn't support enough spec to run these tools. There are some changes.

* resources/testharness.js
* Absolute path is changed to relative path in the tests

If you want to run the XMLHttpRequest test suites, your host PC and the Tizen device should be connected in the same network.

* Set up the test domains in your hosts file (/etc/hosts). The following entries are required:

```
192.168.0.10   web-platform.test
192.168.0.10   www.web-platform.test
192.168.0.10   www1.web-platform.test
192.168.0.10   www2.web-platform.test
192.168.0.10   xn--n8j6ds53lwwkrqhv28a.web-platform.test
192.168.0.10   xn--lve-6lad.web-platform.test
0.0.0.0     nonexistent-origin.web-platform.test
```

* Update the test input file ($LWE/test/tool/web-platform-tests/test_wpt_xhr) like below:

```
[host domain]/XMLHttpRequest/abort-after-receive.htm

e.g., http://192.168.0.10:8000/XMLHttpRequest/abort-after-receive.htm
      http://192.168.0.10:8000/XMLHttpRequest/abort-during-unsent.htm
```

* The test environment can then be started using

```
./serve
```

* You can run the XMLHttpRequest tests like below:

```
$ ./test/tool/runner.sh wpt_xhr
```
