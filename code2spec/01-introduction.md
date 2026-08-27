# 01 - Introduction

> **Relevant source files**
> - `README.md`
> - `AGENTS.md`
> - `CMakeLists.txt`
> - `docs/Spec.md`

## Project Identity

**Project name:** Starfish (also referred to as Lightweight Web Engine / LWE).

**Definition:** Starfish is a lightweight Web browser engine for TV, mobile, headless and wearable devices; low memory usage is the core constraint. `AGENTS.md`

**JS Engine:** Escargot (`third_party/escargot`). `AGENTS.md`

## Purpose

Starfish provides a web rendering engine optimized for resource-constrained devices. The engine supports HTML5, CSS, DOM, JavaScript (via Escargot), and various web APIs including Canvas, Multimedia, WebSocket, WebRTC, and more. `docs/Spec.md`

## Supported Platforms

- Ubuntu 24.04 / 22.04 (x64 native, aarch64 / armhf / x86 cross builds)
- Tizen
- Windows (x86 / x64)
- Android

`README.md:5-11`

## Build System

- **Build tool:** CMake + Ninja
- **Build targets:** `starfish.executable`, `starfish.shared_library`, `starfish.static_library`
- **Output directory:** `out/release/` (or `out/debug/`)
- **JS bindings:** Generated from `src/**/*.idl` at cmake configure time `README.md:55-58`

## Key Constraints

1. **Low memory usage** is the core constraint of the engine `AGENTS.md`
2. Heavy or optional web capabilities are compile-time gated and default off `AGENTS.md`
3. The JS engine is Escargot, not V8 or SpiderMonkey `AGENTS.md`

## Analysis Scope

- **Total files analyzed:** 330
- **AST nodes:** 7004 (File: 330, Constant: 604, Enum: 48, Function: 5630, Class: 373, Test: 19)
- **AST edges:** 31096 (CONTAINS: 6674, IMPORTS_FROM: 2083, CALLS: 21875, IPC: 297, INHERITS: 117, TESTED_BY: 50)
- **Logical modules:** 15 (all classified as Core)
- **Languages detected:** C (118 files), C++ (150 files), Java (11 files), JavaScript (8 files), Python (43 files)

`AST export graph-summary.json`

## Version

- Code2Spec tool version: v0.5.3 `.code2spec-tools/build-info.json`
- Analysis date: 2026-08-27