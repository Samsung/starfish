# Updatable Web Engine (UWE)

UWE pairs an installed public API library with a separately delivered impl
library. The delegate contract headers under `src/public/contract/` therefore
form a binary compatibility boundary.

`LWEDelegate::kDelegateAbiEpoch` identifies an incompatible generation of that
boundary. It is independent of the engine's release version. An impl exports
`LWEDelegate_GetAbiEpoch()`, and a loader rejects an impl whose epoch differs
before it resolves the engine ProcTables or calls through a delegate vtable.
When an updated impl is rejected or lacks a required symbol, the loader tries
the installed default impl instead.

The main engine, SharedWorker, and ServiceWorker delegate contracts share this
single epoch. Each corresponding impl library (`libStarfish-impl.so`,
`libStarfish-sharedworker-impl.so`, and `libStarfish-serviceworker-impl.so`)
exports `LWEDelegate_GetAbiEpoch()`, and each loader validates it before loading
its ProcTable. A breaking change in the worker contract therefore increments
the same `kDelegateAbiEpoch`; there is no worker-specific epoch.

Keep delegate contract changes append-only: add virtual methods, ProcTable
members, enum entries, and by-value struct fields at the end. Never reorder,
remove, or change an existing declaration. Increment `kDelegateAbiEpoch` for a
change that makes an old API library unsafe with a new impl library. The
contract ABI CI check enforces this policy and requires an explicit waiver for
an intentional incompatible change. It also requires a breaking change to
increment the epoch exactly once and rejects epoch changes for compatible ABI
updates.

## Rollout compatibility

Introducing the epoch handshake is backward-compatible in the UWE direction:
an API library installed before the handshake ignores the additional
`LWEDelegate_GetAbiEpoch()` symbol and can load a newer epoch-1 impl as long as
the rest of the contract remains compatible. Compatibility is not symmetric:

| Installed API | Impl selected by the loader | Result |
|---|---|---|
| Pre-handshake API | Newer epoch-1 impl | Compatible; the API ignores the additional epoch symbol |
| Epoch-1 API | Epoch-1 impl | Compatible; the handshake and required symbols are validated |
| Epoch-1 API | Pre-handshake impl | Rejected because the epoch symbol is missing; try the default impl |
| Pre-handshake API | Future epoch-2 impl | Unsafe; the API cannot detect the incompatible epoch |
| Epoch-1 API | Future epoch-2 impl | Rejected because the epoch differs; try the default impl |

The platform package must update every handshake-aware API library and its
matching main, SharedWorker, and ServiceWorker default impl together, all at the
same epoch. Otherwise a new API could reject its pre-handshake default impl and
have no valid fallback.

More importantly, a pre-handshake API cannot reject a future impl from a
different ABI epoch. Roll out the handshake as follows:

1. Install the handshake-aware API and a matching epoch-1 default impl through
   a platform update.
2. Keep every UWE impl compatible with epoch 1 while pre-handshake platforms
   remain eligible for UWE updates.
3. Gate an intentionally breaking UWE impl by a minimum platform/API version or
   an equivalent deployment allowlist, so it cannot reach a pre-handshake API.
4. Introduce the next epoch only after that deployment gate is in place. Land
   the epoch increment, breaking-change waiver, API/default-impl update, and
   fallback coverage together.

The engine release version still decides whether an updated impl is newer than
the installed default. It does not indicate delegate ABI compatibility and
must not replace the ABI epoch or the rollout gate.
