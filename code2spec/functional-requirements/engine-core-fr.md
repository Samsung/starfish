# Functional Requirements: engine-core

> **Relevant source files**
> - [`Starfish.h`](src/Starfish.h#L58)
> - [`StarfishBase.h`](src/StarfishBase.h#L44)
> - [`StoragePathProvider.h`](src/StoragePathProvider.h#L1)

## FR-001: Engine Initialization
**Description**: Starfish constructor accepts StarfishConfiguration (storage path, GC frequency, thread mode, backend, renderer type).
**Source**: [`Starfish`](src/Starfish.h#L58)

## FR-002: GC Configuration
**Description**: GC frequency is configurable via gcFrequency/setGCFrequency, defaulting to BDWGC_FREE_SPACE_DIVISOR=12.
**Source**: [`setGCFrequency`](src/Starfish.h#L109)

## FR-003: Renderer Type Selection
**Description**: StarfishRendererType supports kOpenGL, kSoftware, kHeadless rendering modes.
**Source**: [`StarfishRendererType`](src/Starfish.h#L43)

## FR-004: Static String Management
**Description**: StaticStrings provides pre-allocated string constants for engine-wide use.
**Source**: [`StaticStrings`](src/StaticStrings.h#L1)

## FR-005: Storage Path Provider
**Description**: StoragePathProvider manages file system paths for HTTP cache and local storage.
**Source**: [`StoragePathProvider`](src/StoragePathProvider.h#L1)

## FR-006: Optional Value Type
**Description**: Optional<T> provides nullable value semantics with implicit truthiness checking.
**Source**: [`Optional`](src/StarfishBase.h#L591)

## FR-007: Thread Mode
**Description**: StarfishConfiguration.isThreadMode enables multi-threaded engine operation.
**Source**: [`isThreadMode`](src/Starfish.h#L52)

## FR-008: Version Query
**Description**: Starfish.version() returns major, minor, patch version numbers.
**Source**: [`version`](src/Starfish.h#L115)
