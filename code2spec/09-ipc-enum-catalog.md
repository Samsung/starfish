# 09 - IPC, ENUM, and Constant Catalog

> **Relevant source files**
> - `src/platform/multimedia/StreamInfo.h`
> - `src/platform/loader/Resource.h`
> - `src/platform/network/http/HTTPStatus.h`
> - `inc/PlatformIntegrationData.h`

## ENUM Definitions

Extracted from AST and LLM analysis (63 total entries, 26 with identified values).

| Enum Name | Values | Source |
|-----------|--------|--------|
| KeyValue | UnidentifiedKey, AltLeftKey, ... (229 values) | [`PlatformIntegrationData.h:7`](inc/PlatformIntegrationData.h#L7) |
| MouseButtonValue | NoButton=0, LeftButton, MiddleButton=1, RightButton=2 | [`PlatformIntegrationData.h:239`](inc/PlatformIntegrationData.h#L239) |
| MouseButtonsValue | NoButtonDown=0, LeftButtonDown=1, RightButtonDown, MiddleButtonDown | [`PlatformIntegrationData.h:246`](inc/PlatformIntegrationData.h#L246) |
| TTSMode | Default=0, Forced=1 | [`PlatformIntegrationData.h:253`](inc/PlatformIntegrationData.h#L253) |
| NullOptionType | NullOption | [`StarfishBase.h:591`](src/StarfishBase.h#L591) |
| HistoryManagerOwner | OwnerIsWebView, OwnerIsHTMLIFrame | [`HistoryManager.h:119`](src/browser/history/HistoryManager.h#L119) |
| Command | MoveTo, LineTo, ArcNegative | [`CompositorGL.cpp:477`](src/platform/canvas/CompositorGL.cpp#L477) |
| FileMode | Read=1, Write, ReadWrite | [`PlatformFile.h:35`](src/platform/file/PlatformFile.h#L35) |
| Whence | SEEK_SET, SEEK_CUR, SEEK_END | [`PlatformFile.h:41`](src/platform/file/PlatformFile.h#L41) |
| Resource.State | BeforeSend, Receiving, Finished, Failed, Canceled | [`Resource.h:42`](src/platform/loader/Resource.h#L42) |
| Resource.Type | ResourceType, ImageResourceType, TextResourceType, FontResourceType | [`Resource.h:50`](src/platform/loader/Resource.h#L50) |
| ResourceURL.Protocol | FILE, BLOB, DATA, ABOUT, HTTP, HTTPS, JAVASCRIPT, WS, WSS, UNKNOWN | [`ResourceURL.h:35`](src/platform/loader/ResourceURL.h#L35) |
| RendezvousOwner | None, MainBlockedOnLWE, LWEPausingMain | [`MessageLoopGLib.cpp:99`](src/platform/message_loop/MessageLoopGLib.cpp#L99) |
| SeekWhence | SeekWhenceSet, SeekWhenceCurrent, SeekWhenceEnd, SeekWhenceLookSize | [`DemuxerSource.h:26`](src/platform/multimedia/DemuxerSource.h#L26) |
| PlaybackState | PLAYBACK_STATE_NONE, PLAYING, PAUSED, END | [`MediaPlayer.h:67`](src/platform/multimedia/MediaPlayer.h#L67) |
| SeekState | SEEKSTATE_NO_SEEK, SEEKING, WAITING | [`MediaPlayer.h:73`](src/platform/multimedia/MediaPlayer.h#L73) |
| StreamType | StreamTypeUnknown=1, Audio, Video, Subtitle | [`StreamInfo.h:52`](src/platform/multimedia/StreamInfo.h#L52) |
| MediaCodec | Unknown, AAC, MP3, Vorbis, Opus, H264, HEVC, VP9, AV1 | [`StreamInfo.h:59`](src/platform/multimedia/StreamInfo.h#L59) |
| AudioSampleFormat | None=-1, U8, S16, S32, FLT, DBL, U8P, S16P, S32P, FLTP, DBLP | [`StreamInfo.h:73`](src/platform/multimedia/StreamInfo.h#L73) |
| ScreenOrientationType | Undefined=0, PortraitPrimary, PortraitSecondary, LandscapePrimary, LandscapeSecondary | [`ScreenOrientationType.h:24`](src/platform/public/ScreenOrientationType.h#L24) |
| ImeComposingStatus | NORMAL, COMPOSING_START, COMPOSING_END | [`LweWebViewImpl.java:93`](src/public/bridge/android/java/com/samsung/android/lightweightwebengine/internal/LweWebViewImpl.java#L93) |
| Owner (EcoreWl2) | FREE, ENGINE, READY, PRESENTING | [`LWEWebViewEcoreWl2.cpp:190`](src/public/bridge/ecore_wl2/LWEWebViewEcoreWl2.cpp#L190) |
| Owner (EFL) | FREE, ENGINE, READY, DISPLAYING | [`LWEWebViewEFL.cpp:153`](src/public/bridge/efl/LWEWebViewEFL.cpp#L153) |
| PORT_WINDOW_BACKEND | GB, GL, HEADLESS | [`LWEWebViewFlutter.cpp:82`](src/public/bridge/flutter/LWEWebViewFlutter.cpp#L82) |
| PORT_COMPOSITOR_BACKEND | CAIRO, GL, MOCK | [`LWEWebViewFlutter.cpp:83`](src/public/bridge/flutter/LWEWebViewFlutter.cpp#L83) |
| StarfishStartUpFlag | enableComputedStyleDump, enableFrameTreeDump, ... (7 values) | [`MiniBrowser.cpp:34`](src/shell/MiniBrowser.cpp#L34) |

`LLM extraction llm-extraction-results.json`

## IPC Mechanism Summary

73 IPC patterns identified across 7 mechanism types:

| Mechanism | Count | Description |
|-----------|-------|-------------|
| socket | 55 | OpenGL API calls, X11 connections, HTTP server/client |
| signal | 8 | POSIX signal handling (sigaction) in shell event loops |
| http_client | 3 | curl-based HTTP transactions |
| other | 3 | GL/Evas rendering, JS web API |
| jni | 2 | Android JNI bridge |
| broadcast_receiver | 1 | JavaScript event listener |
| pipe | 1 | Subprocess communication |

### Key IPC Entries

| Mechanism | Component | Source |
|-----------|-----------|--------|
| http_client | HTTPTransaction | [`HTTPTransaction.cpp:49`](src/platform/network/http/HTTPTransaction.cpp#L49) |
| http_client | transformetoNetscapeCookieFormat | [`NetworkSharedResourceManager.cpp:208`](src/platform/network/curl/NetworkSharedResourceManager.cpp#L208) |
| socket | registerX11Fd | [`WindowX11Webcontainer.cpp`](src/shell/x11_webcontainer/WindowX11Webcontainer.cpp) |
| jni | LweWebViewImpl | [`LweWebViewImpl.java:93`](src/public/bridge/android/java/com/samsung/android/lightweightwebengine/internal/LweWebViewImpl.java#L93) |
| signal | sigaction | [`AppLoopLibuv.cpp:101`](src/shell/libuv/AppLoopLibuv.cpp#L101) |

`LLM extraction llm-extraction-results.json`

## Constant Definitions (Key Constants)

446 total constants identified. Key entries:

| Name | Value | Source |
|------|-------|--------|
| LWE_DEFAULT_FONT_SIZE | 16 | [`LWEWebView.h:190`](inc/LWEWebView.h#L190) |
| LWE_MIN_FONT_SIZE | 1 | [`LWEWebView.h:191`](inc/LWEWebView.h#L191) |
| LWE_MAX_FONT_SIZE | 72 | [`LWEWebView.h:192`](inc/LWEWebView.h#L192) |
| STARFISH_RESOURCE_CACHE_SIZE | 4MB | [`ResourceLoader.cpp:45`](src/platform/loader/ResourceLoader.cpp#L45) |
| MAX_PORT_NUMBER | 65535 | [`ResourceURL.cpp:26`](src/platform/loader/ResourceURL.cpp#L26) |
| STARFISH_VIDEO_MAX_WIDTH | 1920 | [`MediaPlayerLinux.cpp:134`](src/platform/multimedia/MediaPlayerLinux.cpp#L134) |
| STARFISH_VIDEO_MAX_HEIGHT | 1080 | [`MediaPlayerLinux.cpp:135`](src/platform/multimedia/MediaPlayerLinux.cpp#L135) |
| STARFISH_ESPP_TOTAL_BUFFER_SIZE | 64MB | [`MediaPlayerESPlusPlayer.cpp:65`](src/platform/multimedia/MediaPlayerESPlusPlayer.cpp#L65) |
| BDWGC_FREE_SPACE_DIVISOR | 12 | [`Starfish.h:40`](src/Starfish.h#L40) |
| CURLHANDLE_CACHE_IDLE_TIME_LIMIT_S | 60 | [`NetworkSharedResourceManager.cpp:50`](src/platform/network/curl/NetworkSharedResourceManager.cpp#L50) |
| HTTP_PORT | 8000 | [`wpt_server.py:43`](tool/wpt/scripts/wpt_server.py#L43) |

`LLM extraction llm-extraction-results.json`

## Error Code Catalog

| Name | Value | Source |
|------|-------|--------|
| TEST_PASSED | 0 | [`constants.py:13`](tool/drivers/basics/constants.py#L13) |
| TEST_FAILED | 1 | [`constants.py:14`](tool/drivers/basics/constants.py#L14) |
| TEST_STOPPED | 2 | [`constants.py:15`](tool/drivers/basics/constants.py#L15) |

`LLM extraction llm-extraction-results.json`

## Signal/Event Mapping

| Event | Emitter | Listener | Source |
|-------|---------|----------|--------|
| sigaction | AppLoopLibuv | OS signal handler | [`AppLoopLibuv.cpp:101`](src/shell/libuv/AppLoopLibuv.cpp#L101) |
| addEventListener | webapi_main.js | DOM event system | [`webapi_main.js:208`](docs/webpages/webapi/webapi_main.js#L208) |

`LLM extraction llm-extraction-results.json`