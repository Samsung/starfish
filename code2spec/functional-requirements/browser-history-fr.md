# Functional Requirements: browser-history

> **Relevant source files**
> - [`HistoryManager.h`](src/browser/history/HistoryManager.h#L119)

## FR-001: History Entry Management
**Description**: HistoryManager.addHistoryEntry adds a new entry to the session history stack.
**Source**: [`addHistoryEntry`](src/browser/history/HistoryManager.h#L1)

## FR-002: History Entry Check
**Description**: HistoryManager.checkHistoryEntry verifies if a history entry exists.
**Source**: [`checkHistoryEntry`](src/browser/history/HistoryManager.h#L1)

## FR-003: State Replacement
**Description**: pushReplaceStateInternal replaces the current history entry state.
**Source**: [`pushReplaceStateInternal`](src/browser/history/HistoryManager.h#L1)

## FR-004: Owner Type
**Description**: HistoryManagerOwner distinguishes between WebView-owned and IFrame-owned history.
**Source**: [`HistoryManagerOwner`](src/browser/history/HistoryManager.h#L119)
