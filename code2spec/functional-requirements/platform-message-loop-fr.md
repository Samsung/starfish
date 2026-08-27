# Functional Requirements: platform-message-loop

> **Relevant source files**
> - [`MessageLoopGLib.h`](src/platform/message_loop/MessageLoopGLib.h#L30)
> - [`RunLoopGLib.h`](src/platform/message_loop/RunLoopGLib.h#L1)

## FR-001: Event Loop
**Description**: MessageLoop provides init(), run(), stop() for the main event loop.
**Source**: [`MessageLoopGLib`](src/platform/message_loop/MessageLoopGLib.h#L30)

## FR-002: Main Thread Sync
**Description**: runOnMainThreadSync executes a functor synchronously on the main thread.
**Source**: [`runOnMainThreadSync`](src/platform/message_loop/MessageLoopGLib.h#L37)

## FR-003: Main Thread Async
**Description**: runOnMainThreadAsync executes a functor asynchronously on the main thread.
**Source**: [`runOnMainThreadAsync`](src/platform/message_loop/MessageLoopGLib.h#L58)

## FR-004: Idler Management
**Description**: addIdler/removeIdler manage idle callbacks with GC rooting support.
**Source**: [`addIdler`](src/platform/message_loop/MessageLoopGLib.h#L42)

## FR-005: Timer Support
**Description**: TimerGLib/TimerLibUV provide timer functionality with the selected backend.
**Source**: [`TimerGLib`](src/platform/message_loop/TimerGLib.h#L1)

## FR-006: Backend Selection
**Description**: Two backends available: GLib (default) and libUV, selected at build time.
**Source**: [`PORT_EVENTLOOP_BACKEND_GLIB`](src/platform/message_loop/MessageLoopGLib.h#L20)
