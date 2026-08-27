# 06 - Configuration and Deployment

> **Relevant source files**
> - `CMakeLists.txt`
> - `README.md`
> - `docs/Spec.md`
> - `src/StarfishConfig.h`

## Build System

### Build Tool
- **Generator:** CMake + Ninja
- **Configuration:** `cmake -Bout/release -DCMAKE_BUILD_TYPE=Release -DBACKEND=glib_cairo_gl -DSHELL=x11 -DTARGETNAME=Starfish -G Ninja`

`README.md:51`

### Build Targets
| Target | Output | Description |
|--------|--------|-------------|
| `starfish.executable` | `lightweight-web-engine` | Standalone executable |
| `starfish.shared_library` | `liblightweight-web-engine.so` | Shared library |
| `starfish.static_library` | `liblightweight-web-engine.a` | Static library |

`README.md:60-76`

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_SYSTEM_NAME` | native | Target: Linux, Tizen, Windows |
| `CMAKE_BUILD_TYPE` | Release | Debug or Release |
| `BACKEND` | glib_cairo_gl | Graphics backend: glib_cairo_gl, uv_cairo_gl |
| `CMAKE_SYSTEM_PROCESSOR` | x86_64 | Architecture: x86_64, aarch64, arm, x86 |
| `SHELL` | x11 | Shell type: x11, glib_headless |
| `LTO` | 0 | Link-time optimization |
| `ENABLE_DEBUGGER` | 0 | Enable debugger |
| `COVERAGE` | 0 | gcov coverage |
| `TARGETNAME` | lightweight-web-engine | Output name |

`README.md:83-102`

### Build-Conditional Feature Flags

| Feature | CMake Flag | Default | Effect when off |
|---------|-----------|---------|-----------------|
| Canvas | `STARFISH_ENABLE_CANVAS` | on | `getContext('2d')` returns null |
| Multimedia | `STARFISH_ENABLE_MULTIMEDIA` | on | `<video>`/`<audio>` fall back to unknown |
| WebGL | `WEBGL=1` | off | `getContext('webgl')` returns null |
| Workers | `WORKER=1` | off | Worker globals undefined |
| IndexedDB | `IDB=1` | off | `indexedDB` undefined |
| WebRTC | `WEBRTC=1` | off | RTC/MediaStream undefined |
| WebAudio | `STARFISH_ENABLE_WEBAUDIO` | x86_64 only | AudioContext undefined |
| WebSocket | `STARFISH_ENABLE_WEBSOCKET` | on | WebSocket undefined |
| TTS | `STARFISH_ENABLE_TTS` | x86_64 only | SpeechSynthesis undefined |
| WASM | `ENABLE_WASM=1` | off | WebAssembly undefined |
| ESPlusPlayer | `ENABLE_ESPLUSPLAYER=1` | off (Tizen 10+ auto) | MSE uses platform default |

`docs/Spec.md:43-64`

## Platform-Specific Builds

### Ubuntu (x64 native)
```sh
cmake -Bout/release -DCMAKE_BUILD_TYPE=Release -DBACKEND=glib_cairo_gl -DSHELL=x11 -DTARGETNAME=Starfish -G Ninja
ninja -C out/release starfish.executable
```
`README.md:51-53`

### Cross-Compile (aarch64/armhf/x86)
Uses Docker image with pre-built sysroots or manual cross-toolchain setup. `README.md:120-182`

### Tizen
Uses GBS build system. `README.md:184-204`

### Windows
Uses vcpkg for dependencies, MSVC compiler, Ninja generator. `README.md:206-280`

### Android
Uses Gradle build system with Android NDK r16b. `README.md:282-293`

## Output Structure
```
out/release/
  bin/lightweight-web-engine    // Starfish binary
  lib/                           // shared libraries
```
`README.md:108-113`

## Dependencies

**System libraries:** glib2, cairo, freetype, fontconfig, harfbuzz, X11, EGL, GLES, libpng, libjpeg, libgif, libwebp, curl, openssl, icu, libcap, alsa, zlib

`README.md:20-28`

**Third-party (submodule):** Escargot (JS engine), robin_map (hash map), libtuv (event loop), Boehm GC

`AGENTS.md`, `third_party/`