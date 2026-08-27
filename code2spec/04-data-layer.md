# 04 - Data Layer

> **Relevant source files**
> - `src/platform/network/http/`
> - `src/platform/loader/`
> - `src/browser/history/HistoryManager.h`
> - `src/StoragePathProvider.h`

## Data Storage Overview

Starfish does not use a traditional database. Data persistence is handled through file-based storage and HTTP caching.

## HTTP Cache

The HTTP cache subsystem provides on-disk caching of HTTP responses:
- **HTTPCache** (`src/platform/network/http/HTTPCache.cpp`): Main cache manager
- **HTTPCacheEntry** (`src/platform/network/http/HTTPCacheEntry.cpp`): Individual cache entries with freshness info
- **Cache properties:** CacheControl, HTTPContentInfo, HTTPFreshnessInfo

`src/platform/network/http/HTTPCache.cpp:441`

## Cookie Management

Cookie management is handled through the delegate pattern:
- **Contract:** `src/public/contract/CookieManagerDelegate.h`
- **Implementation:** `src/public/delegate/CookieManagerDelegate.cpp`
- Cookie format transformation: `transformetoNetscapeCookieFormat` in `NetworkSharedResourceManager.cpp`

`src/public/contract/CookieManagerDelegate.h`, `src/platform/network/curl/NetworkSharedResourceManager.cpp:208`

## Browser History

Session history is managed by `HistoryManager` (`src/browser/history/HistoryManager.h`):
- Track type: `HistoryManagerOwner` enum (`OwnerIsWebView`, `OwnerIsHTMLIFrame`)
- Operations: `addHistoryEntry`, `checkHistoryEntry`, `pushReplaceStateInternal`

`src/browser/history/HistoryManager.h:119`

## Resource Loading Pipeline

Resources are loaded through a typed pipeline:
- **URL resolution:** `ResourceURL` (`src/platform/loader/ResourceURL.h`) with protocol enum (FILE, BLOB, DATA, ABOUT, HTTP, HTTPS, JAVASCRIPT, WS, WSS)
- **Resource types:** `Resource` (base), `FontResource`, `ImageResource`, `TextResource`, `HeaderResource`
- **Resource states:** `BeforeSend` → `Receiving` → `Finished` / `Failed` / `Canceled`
- **Cache size:** `STARFISH_RESOURCE_CACHE_SIZE = 1024 * 1024 * 4` (4MB)

`src/platform/loader/Resource.h:42-50`, `src/platform/loader/ResourceURL.h:35`, `src/platform/loader/ResourceLoader.cpp:45`

## Storage Path Provider

`StoragePathProvider` (`src/StoragePathProvider.h`) manages file system paths for:
- HTTP cache data directory (`httpCacheDataDirectoryPath`)
- Storage directory (`m_storageDirectoryPath`)

`src/StoragePathProvider.h`

## CURL Handle Management

Network connections use a shared CURL handle pool:
- `CURLHANDLE_CACHE_PRUNE_MINIMUM_SIZE = 12`
- `CURLHANDLE_CACHE_IDLE_TIME_LIMIT_S = 60`
- `CURLHANDLE_CACHE_PRUNE_MINIMUM_INTERVAL_S = 0.5`

`src/platform/network/curl/NetworkSharedResourceManager.cpp:42-50`

## URL Configuration Constants

- `MAX_PORT_DIGITS = 5`
- `MAX_PORT_NUMBER = 65535`

`src/platform/loader/ResourceURL.cpp:25-26`