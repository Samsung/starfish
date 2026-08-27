# Module Design Card: third-party-libs

> **Relevant source files**
> - [`robin_map.h`](third_party/robin_map/include/tsl/robin_map.h#L1)
> - [`robin_hash.h`](third_party/robin_map/include/tsl/robin_hash.h#L1)
> - [`robin_growth_policy.h`](third_party/robin_map/include/tsl/robin_growth_policy.h#L1)

## Module Boundary
Third-party header-only libraries: tsl::robin_map hash map implementation.

**Confidence**: 0.95

## Source Files
5 files in `third_party/robin_map/include/tsl/`

## Public Interface
- `tsl::robin_map` — Hash map with open-addressing (Robin Hood hashing).
- `tsl::robin_set` — Hash set counterpart.
- `tsl::robin_vector` — Vector used internally.
- `tsl::detail_robin_hash::robin_hash` — Core hash table implementation.
- Growth policies in `robin_growth_policy.h`.

## Architectural Rules
- Header-only library, no compilation needed.
- Used throughout engine-core via `#include <tsl/robin_map.h>`. [`StarfishBase.h:82`](src/StarfishBase.h#L82)
- Provides GCUnorderedMap compatibility (used with GCVector/GCUnorderedMap).

## Dependencies
- Used by: engine-core (StarfishBase.h includes tsl headers)

## IPC / Message / Interface Contracts
- This module does not have IPC. It is a pure data structure library.

## Quick Navigation
- [FR Document](../functional-requirements/third-party-libs-fr.md)
- [Architecture](../02-architecture.md)
