# Functional Requirements: embedding-api

> **Relevant source files**
> - [`LWEWebView.h`](inc/LWEWebView.h#L20)
> - [`LWEWorker.h`](inc/LWEWorker.h#L1)
> - [`PlatformIntegrationData.h`](inc/PlatformIntegrationData.h#L7)
> - [`LWEDelegateLoader.cpp`](src/public/LWEDelegateLoader.cpp#L1)

## FR-001: Engine Initialization
**Description**: LWE::Initialize accepts InitializeOption flags (PreferSeparateThread, PreferIncrementalGC) to configure engine runtime behavior.
**Source**: [`InitializeOption`](inc/LWEWebView.h#L58)

## FR-002: WebView Control
**Description**: LWEWebView provides load, reload, stop, and navigation control for web content rendering.
**Source**: [`LWEWebView`](inc/LWEWebView.h#L20)

## FR-003: Worker Process Management
**Description**: LWEWorker API manages ServiceWorker and SharedWorker processes.
**Source**: [`LWEWorker`](inc/LWEWorker.h#L1)

## FR-004: Delegate Dynamic Loading
**Description**: LWEDelegateLoader dynamically loads delegate shared libraries to connect API layer with platform implementations.
**Source**: [`LWEDelegateLoader`](src/public/LWEDelegateLoader.cpp#L1)

## FR-005: Platform Integration Data
**Description**: PlatformIntegrationData defines KeyValue (229 key codes), MouseButtonValue, and TTSMode enums for platform input integration.
**Source**: [`PlatformIntegrationData.h:7`](inc/PlatformIntegrationData.h#L7)

## FR-006: Per-Platform Bridge
**Description**: Platform bridges (Android JNI, EFL, Ecore, Flutter, X11) adapt LWEWebView to native window systems.
**Source**: [`AndroidBridge.cpp`](src/public/bridge/android/AndroidBridge.cpp#L1)

## FR-007: API Recording and Replay
**Description**: APIRecorder records LWE API calls for testing and can replay them.
**Source**: [`APIRecorder.cpp`](src/public/APIRecorder.cpp#L1)

## FR-008: Cookie Management Delegate
**Description**: CookieManagerDelegate provides contract for cookie get/set/remove operations.
**Source**: [`CookieManagerDelegate.h`](src/public/contract/CookieManagerDelegate.h#L1)

## FR-009: Font Size Configuration
**Description**: WebView supports configurable font size with LWE_DEFAULT_FONT_SIZE=16, LWE_MIN_FONT_SIZE=1, LWE_MAX_FONT_SIZE=72.
**Source**: [`LWEWebView.h:190`](inc/LWEWebView.h#L190)
