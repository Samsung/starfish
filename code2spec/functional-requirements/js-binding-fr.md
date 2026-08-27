# Functional Requirements: js-binding

> **Relevant source files**
> - [`ScriptEngineInstance.h`](src/binding/ScriptEngineInstance.h#L33)
> - [`ScriptBindingInstance.h`](src/binding/ScriptBindingInstance.h#L68)
> - [`ScriptBindingSecurity.cpp`](src/binding/ScriptBindingSecurity.cpp#L1)

## FR-001: JS Engine Initialization
**Description**: ScriptEngineInstance initializes the Escargot VMInstanceRef with locale and timezone configuration.
**Source**: [`ScriptEngineInstance`](src/binding/ScriptEngineInstance.h#L33)

## FR-002: DOM Binding Registration
**Description**: ScriptBindingInstance.initBinding() registers DOM interface constructors and methods with the Escargot JS engine via STARFISH_ENUM_BINDING_NAMES.
**Source**: [`initBinding`](src/binding/ScriptBindingInstance.h#L77)

## FR-003: Window Global Bindings
**Description**: ScriptBindingWindowInstance registers window-scoped global bindings (STARFISH_ENUM_GLOBAL_BINDING_WINDOW_NAMES).
**Source**: [`ScriptBindingWindowInstance.cpp`](src/binding/ScriptBindingWindowInstance.cpp#L1)

## FR-004: Worker Global Bindings
**Description**: ScriptBindingWorkerInstance registers worker-scoped bindings for DedicatedWorker, SharedWorker, and ServiceWorker contexts.
**Source**: [`ScriptBindingWorkerInstance.cpp`](src/binding/ScriptBindingWorkerInstance.cpp#L1)

## FR-005: Microtask Management
**Description**: MicroTaskExecutionManager manages macro task counter and forces microtask queue draining.
**Source**: [`MicroTaskExecutionManager`](src/binding/ScriptEngineInstance.h#L62)

## FR-006: Security Enforcement
**Description**: ScriptBindingSecurity enforces same-origin policy and cross-origin access checks for JS bindings.
**Source**: [`ScriptBindingSecurity.cpp`](src/binding/ScriptBindingSecurity.cpp#L1)

## FR-007: GC Root Management
**Description**: Holdable classes (DocumentHoldable, WindowHoldable, WebViewHoldable) manage GC root references for JavaScript-accessible C++ objects.
**Source**: [`DocumentHoldable.h`](src/binding/DocumentHoldable.h#L1)
