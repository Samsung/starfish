# Functional Requirements: shell

> **Relevant source files**
> - [`Shell.h`](src/shell/Shell.h#L31)
> - [`MiniBrowser.cpp`](src/shell/MiniBrowser.cpp#L34)
> - [`AppLoop.h`](src/shell/AppLoop.h#L1)

## FR-001: Shell Entry Point
**Description**: Shell.run() dispatches to MiniBrowser, UnitTest, CreateDestroyTest, or Replay mode based on arguments.
**Source**: [`Shell`](src/shell/Shell.h#L31)

## FR-002: MiniBrowser Mode
**Description**: MiniBrowser creates a window and LWEWebView to browse web content with configurable startup flags.
**Source**: [`MiniBrowser`](src/shell/MiniBrowser.cpp#L34)

## FR-003: Unit Test Runner
**Description**: UnitTestRunner runs unit tests via the Starfish unit-test embedding API.
**Source**: [`runUnitTest`](src/shell/Shell.h#L40)

## FR-004: Debug Dump Flags
**Description**: MiniBrowser supports --dump-computed-style, --dump-frame-tree, --dump-stacking-context, --pixel-test and other debug flags.
**Source**: [`StarfishStartUpFlag`](src/shell/MiniBrowser.cpp#L34)

## FR-005: Multi-Platform Window
**Description**: Window provides platform-specific implementations: EFL, headless, X11, Windows WGL.
**Source**: [`Window.h`](src/shell/Window.h#L1)

## FR-006: Multi-Platform App Loop
**Description**: AppLoop provides platform-specific event loops: Ecore, EFL, Glib, libUV, TcoreWl.
**Source**: [`AppLoop.h`](src/shell/AppLoop.h#L1)

## FR-007: API Replay
**Description**: APIReplayer replays recorded LWE API call sequences for regression testing (test mode only).
**Source**: [`APIReplayer.h`](src/shell/APIReplayer.h#L1)

## FR-008: Signal Handling
**Description**: AppLoopLibuv registers sigaction handler for process signal handling.
**Source**: [`AppLoopLibuv.cpp:101`](src/shell/libuv/AppLoopLibuv.cpp#L101)
