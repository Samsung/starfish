# Quick Reference (code2spec)

> 코드 수정 전 반드시 확인. 아키텍처 일관성 유지 필수.
> This file is the vibe-coding companion for AI assistants (Cline/Cursor).

## System Overview

- **Project:** Starfish — lightweight Web browser engine for TV, mobile, headless, wearable devices
- **Core constraint:** Low memory usage
- **JS Engine:** Escargot (`third_party/escargot`)
- **Build:** CMake + Ninja, output to `out/release/` or `out/debug/`
- **Architecture:** Layered with platform abstraction (Shell -> Embedding API -> Engine Core -> Platform)
- **Tech stack:** C++, Escargot JS, Boehm GC, Cairo/OpenGL, libcurl, GLib/libUV, HarfBuzz/Freetype/ICU

### Key Design Decisions

1. **GC-based object graph:** Boehm GC with `GCVector`/`GCTightVector` for GC-managed pointer containers. [`AGENTS.md`](AGENTS.md)
2. **Optional features default off:** WebGL, WebRTC, Workers, IndexedDB, WASM are compile-time gated. [`docs/Spec.md`](docs/Spec.md)
3. **Delegate contract pattern:** Pure-virtual interfaces in `src/public/contract/` across `.so` boundary. [`AGENTS.md`](AGENTS.md)
4. **Per-platform bridges:** Android, EFL, Ecore, Flutter, X11 each have own bridge. [`src/public/bridge/`](src/public/bridge/)
5. **Optional<T>** for nullable values, not raw pointer + nullptr. [`src/StarfishBase.h`](src/StarfishBase.h#L591)

## Core Module Locations

| Module | Key Files | Role |
|--------|-----------|------|
| engine-core | `src/Starfish.h`, `src/StarfishBase.h` | Engine foundation: GC config, platform macros, static strings |
| embedding-api | `inc/LWEWebView.h`, `src/public/` | Public embedding API, delegates, per-platform bridges |
| js-binding | `src/binding/ScriptBindingInstance.h` | Escargot JS bindings, DOM interface registration |
| platform-canvas | `src/platform/canvas/CompositorCairo.cpp` | Cairo/GL/Mock compositing, font rendering |
| platform-loader | `src/platform/loader/ResourceLoader.h` | Resource fetching (text, image, font, header) |
| platform-network | `src/platform/network/http/HTTPTransaction.h` | curl-based HTTP, caching, TLS |
| platform-multimedia | `src/platform/multimedia/MediaPlayer.h` | Media playback (Linux/Tizen/TV/WebRTC) |
| platform-message-loop | `src/platform/message_loop/MessageLoopGLib.h` | Event loop (GLib/libUV backends) |
| platform-system | `src/platform/file/PlatformFile.h` | File I/O, device info, TTS, screen orientation |
| shell | `src/shell/Shell.h` | MiniBrowser, Console, window management |
| browser-history | `src/browser/history/HistoryManager.h` | Session history management |
| worker-launcher | `src/launcher/ServiceWorkerEntry.cpp` | ServiceWorker/SharedWorker process entry |
| test-tooling | `tool/runner/test_runner.py` | Test runners, WPT, lint, pixel tests |
| third-party-libs | `third_party/robin_map/` | tsl::robin_map hash map (header-only) |
| docs-webapi | `docs/generator/run.py` | Documentation generator and viewer |

## Architecture Rules

1. **GC containers:** Use `GCVector`/`GCTightVector` for GC-managed pointer containers, never `std::vector`. [`AGENTS.md`](AGENTS.md)
2. **Nullable values:** Use `Optional<T>` with implicit truthiness. [`AGENTS.md`](AGENTS.md)
3. **Plain pointers are valid:** GC-based object graph — don't add defensive null checks. [`AGENTS.md`](AGENTS.md)
4. **Feature gating:** Heavy capabilities default off via `STARFISH_ENABLE_*` macros. [`AGENTS.md`](AGENTS.md)
5. **IDL changes:** Editing any `.idl` requires re-running cmake. [`README.md`](README.md)
6. **Fix root causes:** Don't paper over symptoms with defensive null checks. [`AGENTS.md`](AGENTS.md)

## Modification Reference

| Feature | File | Line |
|---------|------|------|
| Engine init/config | `src/Starfish.h` | L58 |
| GC frequency | `src/Starfish.h` | L109 |
| Renderer type | `src/Starfish.h` | L43 |
| WebView API | `inc/LWEWebView.h` | L20 |
| Font size constants | `inc/LWEWebView.h` | L190 |
| JS binding registration | `src/binding/ScriptBindingInstance.h` | L77 |
| Resource fetch | `src/platform/loader/ResourceLoader.h` | L40 |
| Resource URL protocols | `src/platform/loader/ResourceURL.h` | L35 |
| HTTP transaction | `src/platform/network/http/HTTPTransaction.h` | L46 |
| TLS verify override | `src/platform/network/http/HTTPTransaction.h` | L43 |
| HTTP cache modes | `src/platform/network/http/HTTPCache.h` | L34 |
| Media player | `src/platform/multimedia/MediaPlayer.h` | L62 |
| Media codecs | `src/platform/multimedia/StreamInfo.h` | L59 |
| Event loop (GLib) | `src/platform/message_loop/MessageLoopGLib.h` | L30 |
| File I/O | `src/platform/file/PlatformFile.h` | L33 |
| Compositor (Cairo) | `src/platform/canvas/CompositorCairo.cpp` | L42 |
| Compositor (GL) | `src/platform/canvas/CompositorGL.cpp` | L1 |
| Shell entry | `src/shell/Shell.h` | L31 |
| Test runner | `tool/runner/test_runner.py` | L1 |

## Code Modification Checklist

1. Find the relevant module in this Quick Reference
2. Read the Module Design Card (`modules/<name>.md`)
3. Check [02-architecture.md](02-architecture.md) for layer rule compliance
4. Check [03-design-patterns.md](03-design-patterns.md) for design pattern violations
5. Check [04-data-layer.md](04-data-layer.md) for data model changes
6. Check [05-external-interfaces.md](05-external-interfaces.md) for external API changes
7. Check [08-security-quality.md](08-security-quality.md) for security implications
8. If `.idl` changed: re-run cmake (not just ninja)
9. If web-facing behavior changed: add/activate tests, update `docs/Spec.md`

## Interface / Message Contract Review

- System-wide message contracts: [`.analysis/message-contract-candidates.md`](.analysis/message-contract-candidates.md)
- Module-local contract details: `modules/<name>.md` -> `IPC / Message / Interface Contracts`
