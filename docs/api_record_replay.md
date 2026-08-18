# API Recording & Replay

A debugging utility for cases where a customer reports an issue with only a
screenshot and no reproduction steps. While the engine runs, it records every
public API call — with timing — to a JSONL file, and the same sequence can be
replayed later at the original cadence.

> **Build requirement:** Only active when built with `ENABLE_TEST=1`.
> In production builds all code is replaced with no-op macros, so there is
> zero overhead.

---

## Recording

### Activate via environment variable (Linux / general environments)

```bash
STARFISH_API_RECORD=/tmp/repro.jsonl ./lightweight-web-engine <url>
```

At startup the engine checks the `STARFISH_API_RECORD` path, opens the file,
and writes every subsequent API call to it. Each event is flushed with
`fflush`, so events up to the moment of a crash are preserved.

If the `fflush` overhead is a concern:

```bash
STARFISH_API_RECORD=/tmp/repro.jsonl STARFISH_API_RECORD_NO_FLUSH=1 ./lightweight-web-engine <url>
```

### Activate via trigger file (Tizen apps and other environments where setting env vars is hard)

Create the trigger file first with `sdb shell`. Its contents become the output
path.

```bash
# On the PC — before launching the app
sdb shell "echo '/tmp/repro.jsonl' > /tmp/starfish_api_record"

# Launch the app (no env var needed)

# Pull the recording back
sdb pull /tmp/repro.jsonl ./repro.jsonl

# Clean up (so recording does not repeat on the next run)
sdb shell "rm /tmp/starfish_api_record"
```

If the trigger file (`/tmp/starfish_api_record`) is missing or not readable, it
is silently ignored.

---

## Replay

```bash
# Replay at the original timing
./lightweight-web-engine replay /tmp/repro.jsonl

# Replay at 2x speed
./lightweight-web-engine replay /tmp/repro.jsonl --speed=2.0

# Set a timeout (useful in CI)
./lightweight-web-engine replay /tmp/repro.jsonl --timeout=10
```

Replay runs on the existing event loop (an `AddTimeout` chain), so it never
blocks.

---

## Recording file format (JSONL)

One event per line; the first line is always the header.

```jsonl
{"type":"header","ts_us":0,"args":{"version":1,"w":1920,"h":1080,"dpr":1.000,"font":"serif","locale":"ko-KR","tz":"Asia/Seoul"}}
{"type":"LoadURL","ts_us":1234,"args":{"url":"https://example.com"}}
{"type":"DispatchMouseMoveEvent","ts_us":500000,"args":{"button":0,"buttons":0,"x":400.0,"y":300.0}}
{"type":"DispatchMouseDownEvent","ts_us":1000000,"args":{"button":0,"buttons":1,"x":400.0,"y":300.0}}
{"type":"DispatchMouseUpEvent","ts_us":1100000,"args":{"button":0,"buttons":0,"x":400.0,"y":300.0}}
{"type":"DispatchKeyDownEvent","ts_us":2000000,"args":{"key":65}}
{"type":"ScrollTo","ts_us":3000000,"args":{"x":0,"y":200}}
{"type":"Reload","ts_us":5000000,"args":{}}
```

`ts_us` is the **relative microseconds** since the start of the recording
session.

### Recorded events

| Event | args fields |
|-------|-------------|
| `LoadURL` | `url` |
| `LoadData` | `data` (truncated above 64 KB) |
| `EvaluateJavaScript` | `script` |
| `ResizeTo` | `w`, `h` |
| `DispatchMouseMoveEvent` | `button`, `buttons`, `x`, `y` |
| `DispatchMouseDownEvent` | same |
| `DispatchMouseUpEvent` | same |
| `DispatchMouseWheelEvent` | `x`, `y`, `delta` |
| `DispatchKeyDownEvent` | `key` |
| `DispatchKeyPressEvent` | `key` |
| `DispatchKeyUpEvent` | `key` |
| `DispatchCompositionStartEvent` | `text` |
| `DispatchCompositionUpdateEvent` | `text` |
| `DispatchCompositionEndEvent` | `text` |
| `ScrollTo` / `ScrollBy` | `x`, `y` |
| `SetDevicePixelRatio` | `dpr` |
| `SetUserAgentString` | `ua` |
| `SetCacheMode` | `mode` |
| `SetDefaultFontSize` | `size` |
| `SetSettings` | all settings key/value pairs |
| `AddJavaScriptInterface` | `object`, `function` (see note) |
| `RemoveJavascriptInterface` | `object`, `function` |
| `Reload`, `StopLoading`, `GoBack`, `GoForward` | (none) |
| `ClearHistory`, `ClearCache`, `Pause`, `Resume`, `Focus`, `Blur` | (none) |

> **`AddJavaScriptInterface` note:** the native callback (`std::function`) cannot
> be serialized, so only the object/function names are recorded. On replay an
> echo stub is registered — the JS-side object and function will exist, but the
> return value will not match the original native handler. APIs whose arguments
> are raw pointers (`CallHandler`, `SetUserData`) are intentionally **not**
> recorded for the same reason.

---

## Related source files

| File | Role |
|------|------|
| `src/public/APIRecorder.h` | Recorder class and macro declarations |
| `src/public/APIRecorder.cpp` | Recorder implementation (`gettimeofday`-based) |
| `src/shell/APIReplayer.h` | Replayer declarations, `ReplayEvent` struct |
| `src/shell/APIReplayer.cpp` | JSONL parser + `AddTimeout`-chained replay |
| `src/public/LWEWebView.cpp` | Recording hooks inserted into each API function |
| `src/shell/Shell.cpp` | `replay` subcommand (`runReplay`) |
| `src/shell/MiniBrowser.cpp` | Schedules replay after the WebContainer is created |

---

## Tests

### Google Test (unit-test)

```bash
cmake CMakeLists.txt -DCMAKE_BUILD_TYPE=Debug -DBACKEND=glib_headless -DSHELL=glib_headless -DENABLE_TEST=1 -G Ninja
ninja starfish.executable
./Starfish unit-test
```

`APIRecorderRecordingTest` — verifies env-var recording behavior
`APIRecorderFileTriggerTest` — verifies trigger-file path and no-permission handling
`APIReplayerParserTest` — verifies JSONL parser accuracy (per-event-type field parsing)

### CI

The `test_api_record_replay` job in `minor.yml` automatically verifies:

1. `./Starfish unit-test` — full Google Test suite passes
2. Env-var recording → file exists and contains header/LoadURL events
3. Trigger-file recording → output file exists
4. Missing / unreadable trigger file → no crash
5. `./Starfish replay` → replay completes
