# 03 - Design Patterns

> **Relevant source files**
> - `src/StarfishBase.h`
> - `src/binding/`
> - `src/public/contract/`
> - `src/platform/canvas/`

## Identified Design Patterns

### 1. Delegate / Contract Pattern

The embedding API uses a delegate pattern where pure-virtual interfaces (`src/public/contract/`) define the contract between the API layer and the implementation, shared across the `.so` boundary.

- **Contract interfaces:** `LWEDelegate.h`, `LWEWebViewDelegate.h`, `LWEWebContainerDelegate.h`, `LWEWorkerDelegate.h`, `CookieManagerDelegate.h`, `SettingsDelegate.h`, `ResourceErrorDelegate.h`
- **Implementations:** `src/public/delegate/`

`AGENTS.md`, `src/public/contract/`

### 2. Bridge Pattern (Per-Platform Bridges)

Each platform has its own bridge implementation inheriting from the WebView delegate:
- Android: `src/public/bridge/android/AndroidBridge.cpp` (JNI)
- EFL: `src/public/bridge/efl/LWEWebViewEFL.cpp`
- Ecore WL2: `src/public/bridge/ecore_wl2/LWEWebViewEcoreWl2.cpp`
- Flutter: `src/public/bridge/flutter/LWEWebViewFlutter.cpp`
- X11: `src/public/bridge/x11/LWEWebViewX11.cpp`

`src/public/bridge/ directory structure`

### 3. Strategy Pattern (Backend Selection)

The canvas/compositing layer uses strategy pattern with multiple backends:
- **Cairo backend:** `CompositorCairo.cpp`, `CanvasCairo.cpp`, `PathCairo.cpp`
- **GL backend:** `CompositorGL.cpp`, `EvasGL.cpp`, `GenericGL.cpp`
- **Mock backend:** `CompositorMock.cpp`, `CanvasMock.cpp`, `PathMock.cpp`

`src/platform/canvas/ directory structure`

### 4. Holdable / GC Wrapper Pattern

The JS binding layer uses "Holdable" classes to manage GC root references for JavaScript-accessible objects:
- `DocumentHoldable`, `WebViewHoldable`, `WindowHoldable`, `StarfishHoldable`

`src/binding/*Holdable*`

### 5. Optional<T> Pattern

Values that can be absent use `Optional<T>` (`src/StarfishBase.h`), not raw pointers overloaded with `nullptr`. Checked with implicit truthiness (`if (node)` / `if (!node)`).

`AGENTS.md`, `src/StarfishBase.h`

### 6. Custom Binding Pattern

DOM interfaces get custom JS bindings via `*CustomBinding.cpp` files that register constructors and methods with the Escargot JS engine:
- `EventTargetCustomBinding.cpp`, `DocumentCustomBinding.cpp`, `HTMLElementCustomBinding.cpp`, `WindowCustomBinding.cpp`, `XMLHttpRequestCustomBinding.cpp`

`src/binding/*CustomBinding.cpp`

### 7. Resource Type Hierarchy

Resource loading uses a type hierarchy with a base `Resource` class and specialized subclasses:
- `Resource` (base) → `FontResource`, `ImageResource`, `TextResource`, `HeaderResource`

`src/platform/loader/Resource.h`, `src/platform/loader/`

### 8. Event Loop Abstraction

The message loop provides a unified API with two backend implementations:
- **GLib backend:** `MessageLoopGLib`, `RunLoopGLib`, `TimerGLib`
- **libUV backend:** `MessageLoopLibUV`, `RunLoopLibUV`, `TimerLibUV`

`src/platform/message_loop/`

### 9. Build-Conditional Feature Gating

Heavy or optional web capabilities are compile-time gated via `STARFISH_ENABLE_*` macros and CMake flags, defaulting off:
- WebGL (`WEBGL=1`), WebRTC (`WEBRTC=1`), Workers (`WORKER=1`), IndexedDB (`IDB=1`), WASM (`ENABLE_WASM=1`)

`docs/Spec.md:33-64`, `AGENTS.md`