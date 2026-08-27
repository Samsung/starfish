# Module Design Card: docs-webapi

> **Relevant source files**
> - [`run.py`](docs/generator/run.py#L1)
> - [`webapi_main.js`](docs/webpages/webapi/webapi_main.js#L1)

## Module Boundary
Documentation generator and web API documentation page.

**Confidence**: 0.90

## Source Files
3 files in `docs/generator/` and `docs/webpages/webapi/`

## Public Interface
- `run.py` — Documentation generator script. [`run.py`](docs/generator/run.py#L1)
- `webapi_main.js` — Web API documentation viewer JavaScript. [`webapi_main.js`](docs/webpages/webapi/webapi_main.js#L1)

## Architectural Rules
- Documentation generator runs as a Python script.
- Web API docs page uses JavaScript for interactive browsing.

## Dependencies
- External: Python 3 (generator), JavaScript (viewer)

## IPC / Message / Interface Contracts
- webapi_main.js uses addEventListener for DOM event handling (not cross-module IPC). [`webapi_main.js:208`](docs/webpages/webapi/webapi_main.js#L208)

## Quick Navigation
- [FR Document](../functional-requirements/docs-webapi-fr.md)
- [Architecture](../02-architecture.md)
