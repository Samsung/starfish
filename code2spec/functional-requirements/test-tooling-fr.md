# Functional Requirements: test-tooling

> **Relevant source files**
> - [`test_runner.py`](tool/runner/test_runner.py#L1)
> - [`constants.py`](tool/drivers/basics/constants.py#L13)
> - [`check_tidy.py`](tool/lint/check_tidy.py#L1)
> - [`wpt_runner.py`](tool/wpt/scripts/wpt_runner.py#L1)

## FR-001: Test Runner
**Description**: test_runner.py orchestrates all test suites: WPT, DOM conformance, vendor, bidi, internal, pixel tests.
**Source**: [`test_runner.py`](tool/runner/test_runner.py#L1)

## FR-002: C++ Style Checking
**Description**: check_tidy.py validates C++ code style as a PR CI gate.
**Source**: [`check_tidy.py`](tool/lint/check_tidy.py#L1)

## FR-003: Contract ABI Checking
**Description**: check_contract_abi.py validates UWE delegate contract ABI compatibility.
**Source**: [`check_contract_abi.py`](tool/lint/check_contract_abi.py#L1)

## FR-004: WPT Execution
**Description**: wpt_runner.py runs Web Platform Tests against Starfish instances with .res list filtering.
**Source**: [`wpt_runner.py`](tool/wpt/scripts/wpt_runner.py#L1)

## FR-005: Pixel Testing
**Description**: starfish_pixel_test.py runs pixel comparison tests using imgdiff for rendering verification.
**Source**: [`starfish_pixel_test.py`](tool/drivers/basics/starfish_pixel_test.py#L1)

## FR-006: WPT Annotation
**Description**: wpt_annotate.py manages .res list annotations for WPT known-failures (write-once markers).
**Source**: [`wpt_annotate.py`](tool/wpt/scripts/wpt_annotate.py#L1)

## FR-007: HTTP Test Server
**Description**: http_server.py serves test pages to Starfish instances during test execution.
**Source**: [`http_server.py`](tool/runner/http_server.py#L1)

## FR-008: Test Result Constants
**Description**: TEST_PASSED=0, TEST_FAILED=1, TEST_STOPPED=2 define test outcome codes.
**Source**: [`constants.py:13`](tool/drivers/basics/constants.py#L13)
