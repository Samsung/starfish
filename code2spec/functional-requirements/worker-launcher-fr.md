# Functional Requirements: worker-launcher

> **Relevant source files**
> - [`ServiceWorkerEntry.cpp`](src/launcher/ServiceWorkerEntry.cpp#L1)
> - [`SharedWorkerEntry.cpp`](src/launcher/SharedWorkerEntry.cpp#L1)

## FR-001: ServiceWorker Entry
**Description**: ServiceWorkerEntry provides the process entry point for ServiceWorker execution.
**Source**: [`ServiceWorkerEntry`](src/launcher/ServiceWorkerEntry.cpp#L1)

## FR-002: SharedWorker Entry
**Description**: SharedWorkerEntry provides the process entry point for SharedWorker execution.
**Source**: [`SharedWorkerEntry`](src/launcher/SharedWorkerEntry.cpp#L1)

## FR-003: Worker Process Isolation
**Description**: Workers run in separate processes, gated by WORKER/SHARED_WORKER/SERVICE_WORKER build flags.
**Source**: [`docs/Spec.md:48`](docs/Spec.md#L48)
