# Functional Requirements: platform-network

> **Relevant source files**
> - [`HTTPTransaction.h`](src/platform/network/http/HTTPTransaction.h#L46)
> - [`HTTPRequest.h`](src/platform/network/http/HTTPRequest.h#L27)
> - [`HTTPCache.h`](src/platform/network/http/HTTPCache.h#L31)
> - [`NetworkSharedResourceManager.cpp`](src/platform/network/curl/NetworkSharedResourceManager.cpp#L1)

## FR-001: HTTP Transaction
**Description**: HTTPTransaction manages curl-based HTTP requests with callback-based header/body delivery.
**Source**: [`HTTPTransaction`](src/platform/network/http/HTTPTransaction.h#L46)

## FR-002: HTTP/2 Support
**Description**: HTTPTransaction supports HTTP/2 multiplexing via CURLPIPE_MULTIPLEX.
**Source**: [`setUseHttp2`](src/platform/network/http/HTTPTransaction.h#L124)

## FR-003: HTTP Caching
**Description**: HTTPCache provides on-disk response caching with modes: LOAD_DEFAULT, LOAD_NORMAL, LOAD_CACHE_ELSE_NETWORK, LOAD_NO_CACHE, LOAD_CACHE_ONLY.
**Source**: [`HTTPCache`](src/platform/network/http/HTTPCache.h#L31)

## FR-004: curl Handle Pool
**Description**: NetworkSharedResourceManager maintains a curl handle pool with pruning (minimum 12 handles, 60s idle limit).
**Source**: [`NetworkSharedResourceManager`](src/platform/network/curl/NetworkSharedResourceManager.cpp#L1)

## FR-005: TLS Verification Control
**Description**: setGlobalIgnoreSSLVerify allows process-wide TLS certificate verification override (used by CDP Security.setIgnoreCertificateErrors).
**Source**: [`setGlobalIgnoreSSLVerify`](src/platform/network/http/HTTPTransaction.h#L43)

## FR-006: Cookie Format Conversion
**Description**: transformetoNetscapeCookieFormat converts cookie format for curl compatibility.
**Source**: [`transformetoNetscapeCookieFormat`](src/platform/network/curl/NetworkSharedResourceManager.cpp#L208)

## FR-007: Proxy Support
**Description**: HTTPTransaction supports proxy URL configuration.
**Source**: [`setProxyURL`](src/platform/network/http/HTTPTransaction.h#L90)

## FR-008: CORS Preflight
**Description**: HTTPTransaction supports preflight requests for CORS with startPreFlightRequest().
**Source**: [`startPreFlightRequest`](src/platform/network/http/HTTPTransaction.h#L59)
