# 08 - Security and Quality

> **Relevant source files**
> - `AGENTS.md`
> - `src/binding/ScriptBindingSecurity.cpp`
> - `docs/Spec.md`
> - `src/platform/network/http/HTTPStatus.h`

## Security Boundaries

### Same-Origin Policy
The engine enforces same-origin checks as part of web security boundaries. `AGENTS.md security section`

### CORS (Cross-Origin Resource Sharing)
CORS is listed as a supported HTTP feature in the spec. `docs/Spec.md:18-19`

### Content Security Policy (CSP)
CSP is listed as a supported HTTP feature. `docs/Spec.md:20`

### Script Binding Security
`ScriptBindingSecurity` (`src/binding/ScriptBindingSecurity.cpp`) handles security checks for JavaScript bindings. `src/binding/ScriptBindingSecurity.cpp`

### Cookie and Storage Scoping
Cookie management is handled through the delegate pattern with `CookieManagerDelegate`. `src/public/contract/CookieManagerDelegate.h`

### TLS Certificate Verification
Network communication uses curl with OpenSSL for TLS. `README.md:28 (libcurl4-openssl-dev`, libssl-dev))

## Security Review Principles

From `AGENTS.md`:
- Treat web content, network data, files, and inputs crossing script/native or process boundaries as attacker-controlled
- Flag reachable paths to memory corruption, code/command injection, path traversal, or unauthorized data access
- Preserve browser security boundaries: same-origin checks, CORS, CSP, cookie/storage scoping, permissions, TLS verification
- Explicit developer/embedder opt-outs are acceptable only when opt-in and not enableable by web content

`AGENTS.md security checker`

## Quality Assurance

### Code Style
- C++ style guide enforced: `docs/Coding_Style_Guide.md`
- `check_tidy.py` runs as PR CI gate
- `check_contract_abi.py` gates UWE delegate contract ABI breaks

`AGENTS.md`

### Testing Coverage
- WPT (Web Platform Tests) for spec compliance
- Internal tests with `console.assert` + `testEnd()`
- Unit tests for public embedding API (`Starfish unit-test`)
- Pixel/reftest for rendering correctness

`AGENTS.md testing section`

### Memory Efficiency
Low runtime memory usage is the engine's core constraint. Key practices:
- `Optional<T>` instead of pointer overloading (smaller footprint)
- `GCVector`/`GCTightVector` for GC-managed containers
- `BDWGC_FREE_SPACE_DIVISOR = 12` for GC tuning
- Lazy allocation and lean data structures preferred
- Memory savings must not degrade rendering performance on hot paths

`AGENTS.md memory efficiency rules`, `src/Starfish.h:40`

### Web Standards Compliance
- Relevant WHATWG/W3C/ECMA-262 spec is the source of truth for behavior
- Interface shape comes from `.idl` files mirroring specs
- WPT are the proof of spec compliance
- Deliberately scoped partial implementations are acceptable if the implemented part behaves per spec

`AGENTS.md web standards section`

## HTTP Status Codes

HTTP status codes are defined in `src/platform/network/http/HTTPStatus.h`. `src/platform/network/http/HTTPStatus.h:94`

## Error Handling

The engine uses `STARFISH_ASSERT` and `STARFISH_RELEASE_ASSERT` macros for invariant assertions. The coding rules discourage blanket defensive null checks in favor of fixing root causes. `AGENTS.md`, `src/StarfishBase.h`