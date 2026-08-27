# Functional Requirements: platform-loader

> **Relevant source files**
> - [`Resource.h`](src/platform/loader/Resource.h#L36)
> - [`ResourceLoader.h`](src/platform/loader/ResourceLoader.h#L40)
> - [`ResourceURL.h`](src/platform/loader/ResourceURL.h#L35)

## FR-001: Resource Fetching
**Description**: ResourceLoader provides fetch(), fetchText(), fetchImage(), fetchFont(), fetchHeader() for typed resource loading.
**Source**: [`ResourceLoader`](src/platform/loader/ResourceLoader.h#L40)

## FR-002: URL Protocol Support
**Description**: ResourceURL supports protocols: FILE, BLOB, DATA, ABOUT, HTTP, HTTPS, JAVASCRIPT, WS, WSS.
**Source**: [`Protocol enum`](src/platform/loader/ResourceURL.h#L35)

## FR-003: Resource State Machine
**Description**: Resource transitions through states: BeforeSend → Receiving → Finished/Failed/Canceled.
**Source**: [`State enum`](src/platform/loader/Resource.h#L42)

## FR-004: Resource Caching
**Description**: Font and image resources are cached with LRU eviction. Image cache can be cleared independently.
**Source**: [`m_imageResourceCache`](src/platform/loader/ResourceLoader.h#L121)

## FR-005: Load Progress Tracking
**Description**: ResourceLoader tracks load progress states (Normal, ParsingEnd, DomContentLoaded) for document load events.
**Source**: [`LoadProgressState`](src/platform/loader/ResourceLoader.h#L48)

## FR-006: Percent Encoding
**Description**: ResourceURL provides createPercentEncodingString and createPercentDecodingString for URL encoding.
**Source**: [`createPercentEncodingString`](src/platform/loader/ResourceURL.h#L50)
