# 05 - External Interfaces

> **Relevant source files**
> - `inc/LWEWebView.h`
> - `inc/LWEWorker.h`
> - `inc/PlatformIntegrationData.h`
> - `src/public/contract/`
> - `src/public/bridge/`

## Public Embedding API

The primary external interface is the LWE (Lightweight Web Engine) embedding API, exposed through C headers:

### LWEWebView API (`inc/LWEWebView.h`)
The main WebView control interface for embedders. Key configuration constants:
- `LWE_DEFAULT_FONT_SIZE = 16`
- `LWE_MIN_FONT_SIZE = 1`
- `LWE_MAX_FONT_SIZE = 72`

`inc/LWEWebView.h:190-192`

### LWEWorker API (`inc/LWEWorker.h`)
Worker process API for ServiceWorker and SharedWorker.

### PlatformIntegrationData (`inc/PlatformIntegrationData.h`)
Data structures for platform integration:
- **KeyValue enum:** Keyboard key identifiers (UnidentifiedKey, AltLeftKey, AltRightKey, ControlLeftKey, etc.)
- **MouseButtonValue enum:** Mouse button mapping (NoButton, LeftButton, MiddleButton, RightButton)
- **TTSMode enum:** TTS mode (Default, Forced)

[Source: `inc/PlatformIntegrationData.h:7, 239, 253`]

## Delegate Contracts

Pure-virtual interfaces shared across the `.so` boundary:

| Contract | File | Purpose |
|----------|------|---------|
| LWEDelegate | `src/public/contract/LWEDelegate.h` | Core engine delegate |
| LWEWebViewDelegate | `src/public/contract/LWEWebViewDelegate.h` | WebView delegate |
| LWEWebContainerDelegate | `src/public/contract/LWEWebContainerDelegate.h` | Web container delegate |
| LWEWorkerDelegate | `src/public/contract/LWEWorkerDelegate.h` | Worker delegate |
| CookieManagerDelegate | `src/public/contract/CookieManagerDelegate.h` | Cookie management |
| SettingsDelegate | `src/public/contract/SettingsDelegate.h` | Settings management |
| ResourceErrorDelegate | `src/public/contract/ResourceErrorDelegate.h` | Resource error handling |

`src/public/contract/ directory`

## Platform Bridges

| Platform | Bridge File | Mechanism |
|----------|-------------|-----------|
| Android | `src/public/bridge/android/AndroidBridge.cpp` | JNI |
| EFL | `src/public/bridge/efl/LWEWebViewEFL.cpp` | EFL Evas |
| Ecore WL2 | `src/public/bridge/ecore_wl2/LWEWebViewEcoreWl2.cpp` | Wayland |
| Ecore X | `src/public/bridge/ecore_x/LWEWebViewEcoreX.cpp` | X11 |
| Flutter | `src/public/bridge/flutter/LWEWebViewFlutter.cpp` | Flutter platform channel |
| Tizen Core WL | `src/public/bridge/tcore_wl/LWEWebViewTcoreWl.cpp` | Tizen Wayland |
| X11 | `src/public/bridge/x11/LWEWebViewX11.cpp` | X11 direct |

`src/public/bridge/ directory`

## Android Java API

The Android bridge exposes a Java API at `src/public/bridge/android/java/com/samsung/android/lightweightwebengine/`:
- `WebView.java` - Main WebView class
- `WebSettings.java` - Settings configuration
- `WebViewClient.java` - WebView client callbacks
- `WebChromeClient.java` - Chrome client callbacks
- `LweWebViewImpl.java` - Internal implementation (contains `ImeComposingStatus` enum)

`src/public/bridge/android/java/`

## Network Interface

HTTP communication is handled through curl:
- `HTTPTransaction` (`src/platform/network/http/HTTPTransaction.cpp`) - HTTP transaction management
- `HTTPRequest` / `HTTPResponse` - Request/response objects
- `CURLPIPE_MULTIPLEX` - HTTP/2 multiplexing support

`src/platform/network/http/HTTPTransaction.cpp:49`

## Tizen Compatibility

`compat/tizen_5.0/inc/LWEWebView.h` provides backward compatibility for Tizen 5.0 API.

`compat/tizen_5.0/inc/LWEWebView.h`