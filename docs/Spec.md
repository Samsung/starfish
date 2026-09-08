# Lightweight Web Engine Specification

This document describes the complete list of features supported by the
lightweight Web engine (LWE).

## Table of Contents

- [Build-Conditional Surface](#build-conditional-surface)
- [HTML](#html)
- [DOM](#dom)
- [Events](#events)
- [Obsolete](#obsolete)
- [CSS](#css)
- [Obsolete CSS](#obsolete-css)
- [Selectors](#selectors)
- [SVG](#svg)
- [Cross-origin script API accessSection](#cross-origin-script-api-accesssection)
- [HTTP](#http)
    - [Cross-Origin Resource Sharing](#cross-origin-resource-sharing)
    - [Content Security Policy](#content-security-policy)
- [Additional Supported APIs](#additional-supported-apis)
    - [XMLHttpRequest](#xmlhttprequest)
    - [EventSource](#eventsource)
    - [Blob](#blob)
    - [Geolocation](#geolocation)
    - [Web Device API](#web-device-api)
    - [Accessible Rich Internet Applications (WAI-ARIA)](#accessible-rich-internet-applications-wai-aria)
    - [Web Speech APIs](#web-speech-apis)
    - [WebRTC](#webrtc)
    - [WebAudio](#webaudio)
    - [WebSocket](#websocket)

## Build-Conditional Surface

This spec is a **manually curated** description of the engine's web surface, and drifts from the implementation over time. The spec must stay consistent with three machine-readable sources: `src/**/*.idl` (interface/method/attribute exposure, plus `[Unimplemented]`, `[NoInterfaceObject]`, `[STARFISH_ENABLE_*]` extended attributes), `src/core/style/Style.h`'s `FOR_EACH_STYLE_ATTRIBUTE_*` macros (the exhaustive CSS property list), and the tag-mapping `if`/`else if` chain in `src/core/dom/HTMLDocument.cpp::createHTMLElement` (every HTML tag mapped to a dedicated `HTMLxxxElement` subclass vs. generic `HTMLElement`/`HTMLUnknownElement`, via `DEFINE_KNOWN_ELEMENT`). When this spec disagrees with those, **the IDL/source wins** — update the spec, not the code.

### Build-conditional flags table

The compile-time flags that gate large chunks of this spec. "Default" is for the `CMAKE_SYSTEM_NAME=Linux CMAKE_SYSTEM_PROCESSOR=x86_64 CMAKE_BUILD_TYPE=Release` build that `README.md` builds under **Compile Starfish**. Two kinds of flag appear here: `SET(... CACHE STRING ...)` options in `CMakeLists.txt`, which you pass as `-DNAME=1`, and `STARFISH_ENABLE_*` macros that `build/config.cmake` derives from `CMAKE_SYSTEM_PROCESSOR`/`CMAKE_SYSTEM_NAME`/`CUSTOM` and that have no `-D` option of their own.

> Note: `CMakeLists.txt` still defaults `BACKEND` to `efl_cairo_gl`, but `README.md` builds with `-DBACKEND=glib_cairo_gl -DSHELL=x11`. Backend choice does not change any row below.

| Spec section | CMake flag (or `STARFISH_ENABLE_*` macro) | Default | Effect when off |
|--------------|-------------------------------------------|---------|-----------------|
| HTML (`<canvas>`, `CanvasRenderingContext2D`) | `STARFISH_ENABLE_CANVAS` | on (every `CMAKE_SYSTEM_PROCESSOR`) | `<canvas>` parses but `getContext('2d')` returns null. |
| HTML (`<video>`, `<audio>`, `<source>`, `<track>`) | `STARFISH_ENABLE_MULTIMEDIA` (`STARFISH_WINDOWS_ENABLE_MULTIMEDIA=ON` on Windows) | on for the documented Linux and Windows builds | Tags fall back to `HTMLUnknownElement`. |
| WebGL (`WebGL*` interfaces) | `WEBGL=1` | **off** | `getContext('webgl')` returns null. |
| Workers | `WORKER=1`, `SHARED_WORKER=1`, `SERVICE_WORKER=1` | off, off, off | Worker globals undefined. Setting `SHARED_WORKER` or `SERVICE_WORKER` forces `WORKER=1`. |
| IndexedDB | `IDB=1` | off | `indexedDB` undefined. |
| WebRTC, MediaStream | `WEBRTC=1` (→ `STARFISH_ENABLE_WEBRTC`/`MULTIMEDIA`/`WEBSOCKET`/`WEBAUDIO`) | off | All `RTC*`/`MediaStream*` interfaces undefined. |
| WebAudio | `STARFISH_ENABLE_WEBAUDIO` | on for `CMAKE_SYSTEM_PROCESSOR=x86_64` only; also implied by `WEBRTC=1` | `AudioContext` etc. undefined. |
| WebSocket | `STARFISH_ENABLE_WEBSOCKET` | on (every `CMAKE_SYSTEM_PROCESSOR`) | `WebSocket` undefined. |
| Web Speech (TTS) | `STARFISH_ENABLE_TTS` | on for `CMAKE_SYSTEM_PROCESSOR=x86_64` only | `SpeechSynthesis*` undefined. |
| [WAI-ARIA](#accessible-rich-internet-applications-wai-aria) touch exploration | `STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION` (from `ENABLE_A11Y_TOUCH=1`) | on for `CMAKE_SYSTEM_PROCESSOR=x86_64`; on `CMAKE_SYSTEM_NAME=Tizen` it needs `-DENABLE_A11Y_TOUCH=1` and is force-disabled on TV profiles | Tap-to-speak / double-tap-activate / swipe navigation absent; ARIA attributes still reflect. |
| CSS transitions & animations | `STARFISH_ENABLE_ANIMATION` | on (every `CMAKE_SYSTEM_PROCESSOR`) | `@keyframes`, `transition`, and the Web Animations entry points do not run. |
| CSS legacy `-webkit-*` aliases | `STARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX`, `…_BOX_PREFIX`, `…_LINE_PREFIX`, `…_TRANSFORM_PREFIX`, `…_TRANSITION_PREFIX` | on (every `CMAKE_SYSTEM_PROCESSOR`, all five) | Aliases not parsed; use unprefixed forms. |
| [Obsolete](#obsolete) / [Obsolete CSS](#obsolete-css) | `STARFISH_ENABLE_OBSOLETE_SPEC` | on (every `CMAKE_SYSTEM_PROCESSOR`) | `Document.width`/`height`, `Window.event`, `Navigator.battery` and the obsolete CSS properties are absent. |
| WebAssembly (`WebAssembly` global) | `ENABLE_WASM=1` | off | `WebAssembly` undefined. |
| `Intl`, locale-sensitive formatting | `RUNTIME_ICU=1` (→ `STARFISH_ENABLE_RUNTIME_ICU_BINDER`) | on | ICU is linked directly (`icu-uc`/`icu-i18n` become build dependencies) instead of being bound at runtime. The JS-visible `Intl` surface is the same either way. |
| Battery Status | `STARFISH_ENABLE_BATTERY_STATUS` | off (`CMAKE_SYSTEM_NAME=Tizen` + `CUSTOM=unified_wearable` only) | `BatteryManager`, `navigator.getBattery` undefined. |
| Web Device API (`window.tizen`) | `CMAKE_SYSTEM_NAME=Tizen` + `TIZEN_DEVICE_API` | off (linux/windows/android) | `window.tizen` undefined. |
| MSE playback backend | `ENABLE_ESPLUSPLAYER=1` | off; auto-enabled on `CMAKE_SYSTEM_NAME=Tizen` with `TIZEN_MAJOR_VERSION >= 10`. Requires `CMAKE_SYSTEM_NAME=Tizen` | Media Source playback uses the platform-default media path. |
| ffmpeg media player | `USE_FFMPEG_MEDIA_PLAYER=1` | off | `<video>`/`<audio>` use the platform-default media path. |
| Chrome DevTools Protocol server | `STARFISH_ENABLE_CDP=1` | off | No CDP endpoint. This surface is never visible to page script either way — see `docs/CDP.md`. |

When you read a row in the tables below, assume the corresponding flag in this table is on unless the row's "Note" column says otherwise.

## Encoding Scheme
All files (i.e., .html, .css, and .js) are to be encoded in UTF-8. This is
needed for supporting multilanguages. Widgets may not be displayed correctly
especially when widgets contain non-ASCII characters when encodings other than
UTF-8 are used.

## HTML
This section describes the complete list of supported HTML tags and attributes
by LWE. Please note that only the tags and attributes mentioned explicitly in
this section are supported. In addition, LWE supports only HTML5 documents, and
it assumes all input documents are HTML5 documents even if `!DOCTYPE` is not
explicitly specified.


| HTML Tag | Attribute | Allowed Value | Note |
|----------|-----------|---------------|------|
| [Global Attribute](https://www.w3.org/TR/html5/dom.html#global-attributes) | class | &lt;string&gt; |  |
|  | dir | ltr | rtl is an experimental feature. |
|  | id | &lt;string&gt; |  |
|  | style | &lt;css_styles&gt; | &lt;css_styles&gt; must conform to the CSS section of this specification document. |
|  | lang | Refer to [ISO639](https://en.wikipedia.org/wiki/ISO_639) |  |
|  [html](https://www.w3.org/TR/html5/semantics.html#the-root-element)  |  |  |  |
|  [head](https://www.w3.org/TR/html5/document-metadata.html#the-head-element)  |  |  |  |
|  [link](https://www.w3.org/TR/html5/document-metadata.html#the-link-element)  | rel | stylesheet |  |
|  | href | &lt;URL&gt; |  |
|  | media | media query |  |
|  | type | text/css |  |
|  [meta](https://www.w3.org/TR/html5/document-metadata.html#the-meta-element)  | charset | UTF-8 | Only UTF-8 is supported  |
|  | name | tizen-transparent-background | name and content are used to set the background transparent only. To do so, both name and content must be set in the same meta tag, e.g., &lt;meta name="tizen-transparent-background" content="yes"&gt; |
|  | name | tizen-widget-transparent-background | name and content are used to set the widget background transparent only. To do so, both name and content must be set in the same meta tag, e.g., &lt;meta name="tizen-widget-transparent-background" content="yes"&gt; |
|  | content | yes &#124; no | e.g., &lt;meta name="tizen-widget-transparent-background" content="yes"&gt; |
|  [style](https://www.w3.org/TR/html5/document-metadata.html#the-style-element)  | media | media query |  |
|  | type | text/css | Only "text/css" type is supported. |
|  [body](https://www.w3.org/TR/html5/sections.html#the-body-element)  |  |  |  |
|  [base](https://www.w3.org/TR/html5/document-metadata.html#the-base-element)  |  |  |  |
|  [h1, h2, h3, h4, h5, and h6](https://www.w3.org/TR/html5/sections.html#the-h1,-h2,-h3,-h4,-h5,-and-h6-elements)  | align | left &#124; center &#124; right | The align attribute's value of &lt;h1&gt; to &lt;h6&gt; can be only "left", "center" and "right". The attribute is not supported in HTML5. Use CSS instead. |
|  [p](https://www.w3.org/TR/html5/grouping-content.html#the-p-element)  |  |  |  |
|  [div](https://www.w3.org/TR/html5/grouping-content.html#the-div-element)  |  |  |  |
|  [span](https://www.w3.org/TR/html5/text-level-semantics.html#the-span-element)  |  |  |  |
|  [br](https://www.w3.org/TR/html5/text-level-semantics.html#the-br-element)  |  |  |  |
|  [img](https://www.w3.org/TR/html5/embedded-content-0.html#the-img-element)  | src | &lt;URL&gt; | Supported images are of type .png, .jpg, and .bmp. The legacy `<image>` tag is auto-rewritten to `<img>` by the HTML5 parser. |
|  | height | pixels |  |
|  | width | pixels |  |
|  | alt | &lt;string&gt; | Alternative text for the image. |
|  [script](https://www.w3.org/TR/html5/scripting-1.html#the-script-element)  | src | &lt;URL&gt; |  |
|  | type | text/javascript |  |
|  | charset | UTF-8 | Only UTF-8 is supported |
|  [table](https://www.w3.org/TR/html5/tabular-data.html#the-table-element)  | width | pixels &#124; &lt;percentage&gt; |  |
|  | bgcolor | &lt;color&gt; |  |
|  | cellspacing | pixels |  |
|  [canvas](https://www.w3.org/TR/html5/semantics-scripting.html#elementdef-canvas) | width | pixels |  |
|  | height | pixels |  |
|  [caption](https://www.w3.org/TR/html5/tabular-data.html#the-caption-element) |  |  |  |
|  [colgroup](https://www.w3.org/TR/html5/tabular-data.html#the-colgroup-element)  |  |  |  |
|  [tbody](https://www.w3.org/TR/html5/tabular-data.html#the-tbody-element)  |  |  |  |
|  [thead](https://www.w3.org/TR/html5/tabular-data.html#the-thead-element)  |  |  |  |
|  [tfoot](https://www.w3.org/TR/html5/tabular-data.html#the-tfoot-element)  |  |  |  |
|  [tr](https://www.w3.org/TR/html5/tabular-data.html#the-tr-element)  |  |  |  |
|  [td](https://www.w3.org/TR/html5/tabular-data.html#the-td-element), [th](https://www.w3.org/TR/html5/tabular-data.html#the-th-element) | width | pixels &#124; &lt;percentage&gt; |  |
|  | colspan | number |  |
|  | rowspan | number |  |
|  | bgcolor | &lt;color&gt; |  |
|  [video](https://www.w3.org/TR/html5/embedded-content-0.html#the-video-element)  | src | &lt;URL&gt; | [local&#124;network][absolute&#124;relative] URL |
|  | autoplay | autoplay |  |
|  | loop | loop |  |
|  | muted | muted |  |
|  | width | pixels |  |
|  | height | pixels |  |
|  [a](https://www.w3.org/TR/html5/text-level-semantics.html#the-a-element) | href | &lt;URL&gt; |  |
|  [pre](https://www.w3.org/TR/html5/grouping-content.html#the-pre-element) |  |  |  |
|  [ul](https://www.w3.org/TR/html5/grouping-content.html#the-ul-element)  |  |  |  |
|  [li](https://www.w3.org/TR/html5/grouping-content.html#the-li-element)  |  |  |  |
|  [dd](https://www.w3.org/TR/html5/grouping-content.html#the-dd-element)  |  |  |  |
|  [dl](https://www.w3.org/TR/html5/grouping-content.html#the-dl-element)  |  |  |  |
|  [dt](https://www.w3.org/TR/html5/grouping-content.html#the-dt-element)  |  |  |  |
|  [audio](https://www.w3.org/TR/html5/embedded-content-0.html#the-audio-element) |  |  |  |
|  [source](https://www.w3.org/TR/html5/embedded-content-0.html#the-source-element) | src | &lt;URL&gt; |  |
|  | type | MIME-type | Only video/mp4 and audio/mp4 are supported. |
|  [object](https://www.w3.org/TR/html5/embedded-content-0.html#the-object-element) |  |  |  |
|  [strong](https://www.w3.org/TR/html5/text-level-semantics.html#the-strong-element) |  |  |  |
|  [s](https://www.w3.org/TR/html5/text-level-semantics.html#the-s-element) |  |  |  |
|  [dfn](https://www.w3.org/TR/html5/text-level-semantics.html#the-dfn-element) |  |  |  |
|  [i](https://www.w3.org/TR/html5/text-level-semantics.html#the-i-element) |  |  |  |
|  [b](https://www.w3.org/TR/html5/text-level-semantics.html#the-b-element) |  |  |  |
|  [u](https://www.w3.org/TR/html5/text-level-semantics.html#the-u-element) |  |  |  |
|  [mark](https://www.w3.org/TR/html5/text-level-semantics.html#the-mark-element) |  |  |  |
|  [font](https://www.w3.org/TR/html401/present/graphics.html#edef-FONT) | color |  | Obsolete features. |
|  | size | Possible values:<br>- An integer between 1 and 7. This sets the font to some fixed size, whose rendering depends on the user agent. Not all user agents may render all seven sizes.<br>-A relative increase in font size. The value "+1" means one size larger. The value "-3" means three sizes smaller. All sizes belong to the scale of 1 to 7. | Obsolete features. |
|  [fieldset](https://www.w3.org/TR/html5/forms.html#the-fieldset-element) |  |  |  |
|  [legend](https://www.w3.org/TR/html5/forms.html#the-legend-elementT) |  |  |  |
|  [DOCTYPE](https://www.w3.org/TR/html5/syntax.html#the-doctype)  |  | html | The DOCTYPE declaration must be the first tag in your HTML document. The lightweight web engine supports HTML5 only. Other versions of HTMLs and HTML modes (such as quirks mode) are not supported.|

### Additional supported tags

The HTML parser and DOM expose the following tags as well; they were missing from the table above. Verified by runtime probe (each tag returns its dedicated `HTMLxxxElement` constructor at runtime).

| HTML Tag | Attribute | Allowed Value | Note |
|----------|-----------|---------------|------|
|  [ol](https://www.w3.org/TR/html5/grouping-content.html#the-ol-element) | start, reversed, type |  |  |
|  [hr](https://www.w3.org/TR/html5/grouping-content.html#the-hr-element) |  |  |  |
|  [title](https://www.w3.org/TR/html5/document-metadata.html#the-title-element) |  |  |  |
|  [iframe](https://www.w3.org/TR/html5/embedded-content-0.html#the-iframe-element) | src | &lt;URL&gt; | Cross-origin access follows the rules in [Cross-origin script API access](#cross-origin-script-api-accesssection). |
|  | width, height | pixels |  |
|  [dialog](https://html.spec.whatwg.org/multipage/interactive-elements.html#the-dialog-element) | open | open | The HTMLDialogElement is exposed and the `open` attribute reflects, but `showModal()` modal stacking is partial. |
|  [col](https://www.w3.org/TR/html5/tabular-data.html#the-col-element) | span | number |  |
|  [param](https://www.w3.org/TR/html5/embedded-content-0.html#the-param-element) | name, value | &lt;string&gt; | Companion to `<object>`. |
|  [form](https://www.w3.org/TR/html5/forms.html#the-form-element) | action | &lt;URL&gt; |  |
|  | method | get &#124; post |  |
|  [input](https://www.w3.org/TR/html5/forms.html#the-input-element) | type | text &#124; password &#124; checkbox &#124; radio &#124; submit &#124; reset &#124; button &#124; hidden &#124; file &#124; number &#124; range &#124; email &#124; url &#124; date &#124; time | The exact set of types depends on platform input widget support; layout falls back to text for unsupported types. |
|  | name, value, placeholder | &lt;string&gt; |  |
|  | disabled, readonly, checked, required | boolean attribute |  |
|  [button](https://www.w3.org/TR/html5/forms.html#the-button-element) | type | submit &#124; reset &#124; button |  |
|  | disabled | disabled |  |
|  [select](https://www.w3.org/TR/html5/forms.html#the-select-element) | multiple, disabled |  |  |
|  | size | number |  |
|  [option](https://www.w3.org/TR/html5/forms.html#the-option-element) | value | &lt;string&gt; |  |
|  | selected | selected |  |
|  [optgroup](https://www.w3.org/TR/html5/forms.html#the-optgroup-element) | label | &lt;string&gt; |  |
|  [textarea](https://www.w3.org/TR/html5/forms.html#the-textarea-element) | rows, cols | number |  |
|  | placeholder | &lt;string&gt; |  |
|  | disabled, readonly |  |  |
|  [label](https://www.w3.org/TR/html5/forms.html#the-label-element) | for | id reference |  |
|  [output](https://www.w3.org/TR/html5/forms.html#the-output-element) | for | id reference |  |
|  [data](https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-data-element) | value | &lt;string&gt; | HTMLDataElement is exposed. |
|  [q](https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-q-element) | cite | &lt;URL&gt; | Maps to HTMLQuoteElement (shared with `<blockquote>`). |
|  [blockquote](https://html.spec.whatwg.org/multipage/grouping-content.html#the-blockquote-element) | cite | &lt;URL&gt; | Maps to HTMLQuoteElement. |
|  [ins](https://html.spec.whatwg.org/multipage/edits.html#the-ins-element) | cite, datetime | &lt;URL&gt;, &lt;string&gt; | Maps to HTMLModElement (shared with `<del>`). |
|  [del](https://html.spec.whatwg.org/multipage/edits.html#the-del-element) | cite, datetime | &lt;URL&gt;, &lt;string&gt; | Maps to HTMLModElement. |
|  [map](https://html.spec.whatwg.org/multipage/image-maps.html#the-map-element) | name | &lt;string&gt; | Maps to HTMLMapElement; image-map hit-testing is layout-only. |
|  [area](https://html.spec.whatwg.org/multipage/image-maps.html#the-area-element) | href, alt, coords, shape | | Maps to HTMLAreaElement; companion to `<map>`. |
|  [template](https://html.spec.whatwg.org/multipage/scripting.html#the-template-element) |  |  | Maps to HTMLTemplateElement. The `content` DocumentFragment is exposed; element does not render its children. |
|  [slot](https://html.spec.whatwg.org/multipage/scripting.html#the-slot-element) | name | &lt;string&gt; | Maps to HTMLSlotElement. Slotting works inside a shadow tree: `assignedNodes()`/`assignedElements()` (with `{flatten}`), fallback content, slot reassignment, and the `slotchange` event are implemented. |
|  [track](https://html.spec.whatwg.org/multipage/media.html#the-track-element) | kind, src, srclang, label, default |  | Maps to HTMLTrackElement. **Build flag:** `STARFISH_ENABLE_MULTIMEDIA`. |

> **Tags accepted but exposed as generic `HTMLElement` (no element-specific DOM API):** `center`, `i`, `s`, `dfn`, `b`, `u`, `mark`, `strong`, `cite`, `em`, `var`, `address`, `article`, `aside`, `details`, `footer`, `header`, `hgroup`, `main`, `nav`, `section`, `summary`, `code`, `dt`, `dd`. (Exact list: search `DEFINE_KNOWN_ELEMENT` in `src/core/dom/HTMLDocument.cpp`.) Layout follows HTML5 defaults; element-specific behaviors (e.g. the `<details>` toggle, `<summary>` activation) are NOT implemented — falling back to a closed `<details>` content region rendering all children. Any other custom or HTML5 tag the parser doesn't recognize (`figure`, `time`, `picture`, `kbd`, `small`, `wbr`, `ruby`, …) becomes `HTMLUnknownElement` — they parse and lay out as inline boxes but expose no element-specific DOM members.

> **Build flags:** `<canvas>` is conditional on `STARFISH_ENABLE_CANVAS`. `<video>`, `<audio>`, `<source>`, `<track>` are conditional on `STARFISH_ENABLE_MULTIMEDIA`. With those flags off, the elements fall through to `HTMLUnknownElement`.

> **SVG:** All `<svg>` and SVG child elements (`<circle>`, `<rect>`, `<path>`, `<g>`, …) are accepted by the parser, IDL interfaces (`SVGSVGElement`, `SVGRectElement`, …) are exposed, and **rendering is implemented** — shapes, gradients, filters, masks, clipping, markers, text, and SMIL animation are all painted. See the [SVG](#svg) section for the full supported surface.

## DOM

This section describes the complete list of supported DOM interfaces by LWE.
Please note that only the attributes and methods mentioned explicitly in this
section are supported.

> **See also:** core event interfaces (`Event`, `MouseEvent`, `KeyboardEvent`, `CustomEvent`, …) are listed in the [Events](#events) section. `XMLHttpRequest`, `Blob`/`File`/`FileReader`/`FormData`, `EventSource`, `Headers`/`Request`/`Response`/`URL`/`URLSearchParams`/`TextEncoder`/`TextDecoder` live under [Additional Supported APIs](#additional-supported-apis). Don't conclude an interface is missing just because it is absent from this DOM table.

> **Build-conditional interfaces.** The following classes of IDL interfaces are only exposed when their build flag is on; when off, the constructor is `undefined` at runtime:
> - `RTC*`, `MediaStream`, `MediaStreamTrack`, `MediaDevices` — `WEBRTC=1` (`STARFISH_ENABLE_WEBRTC`)
> - `AudioContext`, `BaseAudioContext`, `AudioBuffer*`, `AudioNode*` — `STARFISH_ENABLE_WEBAUDIO` (default on for `CMAKE_SYSTEM_PROCESSOR=x86_64`)
> - `WebSocket` — `STARFISH_ENABLE_WEBSOCKET` (default on for `CMAKE_SYSTEM_PROCESSOR=x86_64`)
> - `WebGL*`, `EXT_*`, `OES_*`, `WEBGL_*` — `WEBGL=1`
> - `Worker`, `WorkerGlobalScope`, `DedicatedWorkerGlobalScope` — `WORKER=1`
> - `SharedWorker`, `SharedWorkerGlobalScope` — `SHARED_WORKER=1`
> - `ServiceWorker`, `ServiceWorkerRegistration`, `Notification`, `PushManager`, `Cache`, `caches`, `FetchEvent`, `ExtendableEvent` — `SERVICE_WORKER=1`
> - `IDBFactory`, `IDBDatabase`, `IDBObjectStore`, … — `IDB=1`
> - `SpeechSynthesis`, `SpeechSynthesisUtterance`, `SpeechSynthesisVoice`, `SpeechSynthesisEvent` — `STARFISH_ENABLE_TTS` (default on for `CMAKE_SYSTEM_PROCESSOR=x86_64`)
> - `BatteryManager`, `navigator.getBattery()` — `STARFISH_ENABLE_BATTERY_STATUS` (Tizen wearable only)
>
> The `CMAKE_SYSTEM_NAME=Linux CMAKE_SYSTEM_PROCESSOR=x86_64` release build that `README.md` builds ships with `WEBGL=0`, `WEBRTC=0`, `WORKER=0`, `SHARED_WORKER=0`, `SERVICE_WORKER=0`, `IDB=0`, plus TTS/WebAudio/WebSocket on. Other hosts and arches differ — see [Build-Conditional Surface](#build-conditional-surface) for the full table.

> **Observers deliver callbacks.** `MutationObserver`/`MutationRecord`, `IntersectionObserver`/`IntersectionObserverEntry`, and `ResizeObserver`/`ResizeObserverEntry`/`ResizeObserverSize` are implemented, not stubs — the observation logic is wired up and invokes the JS callback (`MutationObserver::notify`, `ResizeObserver::notify`, `IntersectionObserverCallback`).
>
> **Shadow DOM and Custom Elements are implemented.** `Element.attachShadow()`, `ShadowRoot`, `Slottable.assignedSlot`, and `HTMLSlotElement.assignedNodes()`/`assignedElements()` work, including slot assignment, the `slotchange` event, and spec event retargeting across shadow boundaries (`EventTarget.cpp` `retarget()`). `CustomElementRegistry` implements `define()`, `get()`, `getName()`, `whenDefined()`, and `upgrade()`. A shadow tree can be built imperatively (`createElement` + `appendChild`), from markup (`ShadowRoot.innerHTML`), or styled through `adoptedStyleSheets`. See the rows below, and the [Selectors](#selectors) section for `:host`/`::slotted`.
>
> **`PerformanceObserver` is not exposed at all** — it has no `.idl` file, so the constructor is `undefined`. There is no entry-buffer observation API; poll `performance.getEntries()` instead.
>
> **`SVG*` interfaces are exposed and rendered** — see the [SVG](#svg) section for the full supported surface.

> **Frequently used core interfaces also exposed but not row-by-row documented below** (treat as confirmed at the interface level; rely on the WHATWG/W3C spec for member details): `DocumentFragment`, `DOMImplementation`, `DOMTokenList` (`Element.classList`/`relList`), `DOMStringMap` (`HTMLElement.dataset`), `HTMLCollection`, `Range`, `NodeFilter`, `NodeIterator`, `TreeWalker`, `MessageEvent`, `HashChangeEvent`, `PopStateEvent`, `Performance`, `PerformanceEntry`, `Storage`, `URL`, `URLSearchParams`, `TextEncoder`, `TextDecoder`, `Crypto`, `HTMLDialogElement`, `HTMLObjectElement`, `HTMLOutputElement`, `HTMLTitleElement`, `HTMLUnknownElement`. Methods on these mostly follow the standard; if you depend on a non-standard behavior, run a runtime probe.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Attr](https://dom.spec.whatwg.org/#interface-attr) | interface | Attr | Attr nodes are simply known as attributes. They are sometimes referred to as content attributes to avoid confusion with IDL attributes. |
|  | attribute | localName | Return the local name. |
|  | attribute | name | Return the qualified name. |
|  | attribute | value | Return the value. |
|  | attribute | ownerElement | Return context object’s element. |
|  | attribute | specified | Return true. |
| [CanvasRenderingContext2D](https://html.spec.whatwg.org/multipage/canvas.html#canvasrenderingcontext2dsettings) | interface | CanvasRenderingContext2D |  |
|  | attribute | canvas | Return associated \<canvas\> element |
| [CanvasState](https://html.spec.whatwg.org/multipage/canvas.html#canvasstate) | interface mixin | CanvasState |  |
|  | method | void save() | Push state on state stack |
|  | method | void restore() | Pop state stack and restore state |
| [CanvasDrawPath](https://html.spec.whatwg.org/multipage/canvas.html#canvasdrawpath) | interface mixin | CanvasDrawPath |  |
|  | method | void beginPath() | Starts a new path by emptying the list of sub-paths |
|  | method | void fill(optional CanvasFillRule fillRule = "nonzero") | Fills the current path with the current fillStyle. |
|  | method | void fill(Path2D path, optional CanvasFillRule fillRule = "nonzero") | Fills the given path with the current fillStyle. |
|  | method | void stroke() | Strokes (outlines) the current path with the current stroke style. |
|  | method | void stroke(Path2D path) | Strokes (outlines) the given path with the current stroke style. |
|  | method | void clip(optional CanvasFillRule fillRule = "nonzero") |  Turns the current path into the current clipping region |
|  | method | void clip(Path2D path, optional CanvasFillRule fillRule = "nonzero") | Turns the given path into the current clipping region |
|  | method | boolean isPointInPath(unrestricted double x, unrestricted double y, optional CanvasFillRule fillRule = "nonzero") | Reports whether or not the specified point is contained in the current path. |
|  | method | boolean isPointInPath(Path2D path, unrestricted double x, unrestricted double y, optional CanvasFillRule fillRule = "nonzero") | Reports whether or not the specified point is contained in the current path. |
|  | method | boolean isPointInStroke(unrestricted double x, unrestricted double y) | Reports whether or not the specified point is inside the area contained by the stroking of a path. |
|  | method | boolean isPointInStroke(Path2D path, unrestricted double x, unrestricted double y) | Reports whether or not the specified point is inside the area contained by the stroking of a path. |
| [CanvasText](https://html.spec.whatwg.org/multipage/canvas.html#canvastext) | interface mixin | CanvasText |  |
|  | method | void fillText(DOMString text, unrestricted double x, unrestricted double y, optional unrestricted double maxWidth) | Render the given text at the given (x, y) coordinates ensuring that the text isn't wider than maxWidth if specified. |
|  | method | void strokeText(DOMString text, unrestricted double x, unrestricted double y, optional unrestricted double maxWidth) | Render the given text at the given (x, y) coordinates ensuring that the text isn't wider than maxWidth if specified. |
|  | method | TextMetrics measureText(DOMString text) | Return a new TextMetrics object. |
| [CanvasDrawImage](https://html.spec.whatwg.org/multipage/canvas.html#canvasdrawimage) | interface mixin | CanvasDrawImage |  |
|  | method | void drawImage(CanvasImageSource image, unrestricted double dx, unrestricted double dy) | Provides different ways to draw an image onto the canvas. |
|  | method | void drawImage(CanvasImageSource image, unrestricted double dx, unrestricted double dy, unrestricted double dw, unrestricted double dh) | Provides different ways to draw an image onto the canvas. |
|  | method | void drawImage(CanvasImageSource image, unrestricted double sx, unrestricted double sy, unrestricted double sw, unrestricted double sh, unrestricted double dx, unrestricted double dy, unrestricted double dw, unrestricted double dh) | Provides different ways to draw an image onto the canvas. |
| [CanvasImageData](https://html.spec.whatwg.org/multipage/canvas.html#canvasimagedata) | interface mixin | CanvasImageData |  |
|  | method | ImageData createImageData(long sw, long sh) | Creates a new, blank ImageData object with the specified dimensions |
|  | method | ImageData createImageData(ImageData imagedata) | Creates a new, blank ImageData object with the specified dimensions |
|  | method | ImageData getImageData(long sx, long sy, long sw, long sh) | Returns an ImageData object representing the underlying pixel data for a specified portion of the canvas. |
|  | method | void putImageData(ImageData imagedata, long dx, long dy) | Paints the data from the given ImageData object onto the bitmap. If a dirty rectangle is provided, only the pixels from that rectangle are painted. |
|  | method | void putImageData(ImageData imagedata, long dx, long dy, long dirtyX, long dirtyY, long dirtyWidth, long dirtyHeight) | Paints the data from the given ImageData object onto the bitmap. If a dirty rectangle is provided, only the pixels from that rectangle are painted. |
| [CanvasLineCap](https://html.spec.whatwg.org/multipage/canvas.html#canvaslinecap) | enum | CanvasLineCap | "butt", "round", "square" |
| [CanvasLineJoin](https://html.spec.whatwg.org/multipage/canvas.html#canvaslinejoin) | enum | CanvasLineJoin | "round", "bevel", "miter" |
| [CanvasTextAlign](https://html.spec.whatwg.org/multipage/canvas.html#canvastextalign) | enum | CanvasTextAlign | "start", "end", "left", "right", "center" |
| [CanvasTextBaseline](https://html.spec.whatwg.org/multipage/canvas.html#canvastextbaseline) | enum | CanvasTextBaseline | "top", "hanging", "middle", "alphabetic", "bottom" |
| [CanvasDirection](https://html.spec.whatwg.org/multipage/canvas.html#canvasdirection) | enum | CanvasDirection | "ltr", "rtl", "inherit" |
| [CanvasPath ](https://html.spec.whatwg.org/multipage/canvas.html#canvaspath) | interface mixin | CanvasPath |  |
|  | method | void closePath() | Attempts to add a straight line from the current point to the start of the current sub-path. If the shape has already been closed or has only one point, this function does nothing. |
|  | method | void moveTo(unrestricted double x, unrestricted double y) | Begins a new sub-path at the point specified by the given (x, y) coordinates. |
|  | method | void lineTo(unrestricted double x, unrestricted double y) | Adds a straight line to the current sub-path by connecting the sub-path's last point to the specified (x, y) coordinates. |
|  | method | void quadraticCurveTo(unrestricted double cpx, unrestricted double cpy, unrestricted double x, unrestricted double y) | Adds a quadratic Bézier curve to the current sub-path |
|  | method | void bezierCurveTo(unrestricted double cp1x, unrestricted double cp1y, unrestricted double cp2x, unrestricted double cp2y, unrestricted double x, unrestricted double y) | Adds a cubic Bézier curve to the current sub-path |
|  | method | void arcTo(unrestricted double x1, unrestricted double y1, unrestricted double x2, unrestricted double y2, unrestricted double radius) | Adds a circular arc to the current sub-path, using the given control points and radius |
|  | method | void rect(unrestricted double x, unrestricted double y, unrestricted double w, unrestricted double h) | Adds a rectangle to the current path. |
|  | method | void arc(unrestricted double x, unrestricted double y, unrestricted double radius, unrestricted double startAngle, unrestricted double endAngle, optional boolean anticlockwise = false) | Adds a circular arc to the current sub-path. |
|  | method | void ellipse(unrestricted double x, unrestricted double y, unrestricted double radiusX, unrestricted double radiusY, unrestricted double rotation, unrestricted double startAngle, unrestricted double endAngle, optional boolean anticlockwise = false) | Adds an elliptical arc to the current sub-path. |
| [CanvasGradient ](https://html.spec.whatwg.org/multipage/canvas.html#canvasgradient) | interface mixin | CanvasGradient | |
|  | method | void addColorStop(double offset, DOMString color) | Adds a new color stop, defined by an offset and a color, to a given canvas gradient. |
| [CanvasPattern](https://html.spec.whatwg.org/multipage/canvas.html#canvaspattern) | interface mixin | CanvasPattern | |
|  | method | void setTransform(optional DOMMatrix2DInit transform) | Sets the transformation matrix that will be used when rendering the pattern during a fill or stroke painting operation. |
| [TextMetrics](https://html.spec.whatwg.org/multipage/canvas.html#textmetrics) | interface mixin | TextMetrics | |
|  | attribute | width | The text's advance width. |
| [CanvasPathDrawingStyles](https://html.spec.whatwg.org/multipage/canvas.html#canvaspathdrawingstyles) | interface mixin | CanvasPathDrawingStyles |  |
|  | attribute | lineWidth | Sets/Gets the thickness of lines. |
|  | attribute | lineCap | Determines the shape used to draw the end points of lines. |
|  | attribute | lineJoin | Determines the shape used to join two line segments where they meet. |
|  | attribute | miterLimit | Sets/Gets the miter limit ratio. |
|  | method | void setLineDash(sequence\<unrestricted double\> segments) | Sets the line dash pattern used when stroking lines. |
|  | method | sequence\<unrestricted double\> getLineDash() | Returns the current line dash pattern. |
|  | attribute | lineDashOffset | Sets/Gets the line dash offset, or "phase." |
| [CanvasTextDrawingStyles](https://html.spec.whatwg.org/multipage/canvas.html#canvastextdrawingstyles) | interface mixin | CanvasTextDrawingStyles |  |
|  | attribute | font | Sets/Gets the font value of text. |
|  | attribute | textAlign | Determines the Align value of text. |
|  | attribute | textBaseline | Allowed keywords correspond to alignment points in the font. |
|  | attribute | direction | Sets/Gets the directionality of the canvas element. |
| [CanvasTransform](https://html.spec.whatwg.org/multipage/canvas.html#canvastransform) | interface mixin | CanvasTransform |  |
|  | method | void scale(unrestricted double x, unrestricted double y) | Add the scaling transformation to the current transformation matrix. |
|  | method | void rotate(unrestricted double angle) | Add the rotation transformation to the current transformation matrix. |
|  | method | void translate(unrestricted double x, unrestricted double y) | Add the translation transformation to the current transformation matrix. |
|  | method | void transform(unrestricted double a, unrestricted double b, unrestricted double c, unrestricted double d, unrestricted double e, unrestricted double f) | Replace the current transformation matrix with the result of multiplying the current transformation matrix and paramter. |
|  | method | void setTransform(unrestricted double a, unrestricted double b, unrestricted double c, unrestricted double d, unrestricted double e, unrestricted double f) | Replace the current transformation matrix with parameter. |
|  | method | void resetTransform() | Reset the current transformation matrix to the identity matrix. |
| [CanvasImageSmoothing ](https://html.spec.whatwg.org/multipage/canvas.html#canvasimagesmoothing) | interface mixin | CanvasImageSmoothing |  |
|  | attribute | imageSmoothingEnabled | Determines whether scaled images are smoothed (true, default) or not (false) |
|  | attribute | imageSmoothingQuality | Set/Get the quality of image smoothing. |
| [CanvasFillStrokeStyles ](https://html.spec.whatwg.org/multipage/canvas.html#canvasfillstrokestyles) | interface mixin | CanvasFillStrokeStyles |  |
|  | attribute | strokeStyle | Specifies the color, gradient, or pattern to use for the strokes (outlines) around shapes. |
|  | attribute | fillStyle | Specifies the color, gradient, or pattern to use inside shapes. |
|  | method | CanvasGradient createLinearGradient(double x0, double y0, double x1, double y1) | Creates a gradient along the line connecting two given coordinates. |
|  | method | CanvasGradient createRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1) | Creates a radial gradient using the size and coordinates of two circles. |
|  | method | CanvasPattern? createPattern(CanvasImageSource image, [TreatNullAs=EmptyString] DOMString repetition) | creates a pattern using the specified image and repetition. This method returns a CanvasPattern. |
| [CanvasShadowStyles](https://html.spec.whatwg.org/multipage/canvas.html#canvasshadowstyles) | interface mixin | CanvasShadowStyles | |
|  | attribute | shadowOffsetX | Specify the distance that the shadow will be offset in the positive horizontal distance respectively. |
|  | attribute | shadowOffsetY | Specify the distance that the shadow will be offset in the positive vertical distance respectively. |
|  | attribute | shadowBlur | Specifies the level of the blurring effect. |
|  | attribute | shadowColor | sets the color of the shadow. |
| [CanvasRect](https://html.spec.whatwg.org/multipage/canvas.html#canvasrect) | interface mixin | CanvasRect |  |
|  | method | void clearRect(unrestricted double x, unrestricted double y, unrestricted double w, unrestricted double h) | Erases the pixels in a rectangular area by setting them to transparent black. |
|  | method | void fillRect(unrestricted double x, unrestricted double y, unrestricted double w, unrestricted double h) | Draws a rectangle that is filled according to the current fillStyle. |
|  | method | void strokeRect(unrestricted double x, unrestricted double y, unrestricted double w, unrestricted double h) | Draws a rectangle that is stroked (outlined) according to the current strokeStyle and other context settings |
| [CDATASection](https://dom.spec.whatwg.org/#interface-cdatasection) | interface | CDATASection |  |
| [CharacterData](https://dom.spec.whatwg.org/#interface-characterdata) | interface | CharacterData | CharacterData is an abstract interface and does not exist as node. It is used by Text, ProcessingInstruction, and Comment nodes. |
|  | attribute | data | Getter must return context object’s data. Its setter must replace data with node context object, offset 0, count context object’s length, and data new value. |
|  | attribute | length | Return context object’s length. |
|  | method | void appendData(DOMString data) | Append data |
|  | method | void insertData(unsigned long offset, DOMString data) | Insert data |
|  | method | void deleteData(unsigned long offset, unsigned long count) | Replace data to empty string |
|  | method | void replaceData(unsigned long offset, unsigned long count, DOMString data) | Replace data |
| [ChildNode](https://dom.spec.whatwg.org/#childnode) | interface | ChildNode | Mixin implemented by `Element`, `CharacterData`, and `DocumentType`. |
|  | method | void remove() | Removes the node from its parent's children list. |
|  | method | void before((Node or DOMString)... nodes) | Inserts *nodes* in the parent just before this node, replacing strings with Text nodes. |
|  | method | void after((Node or DOMString)... nodes) | Inserts *nodes* in the parent just after this node. |
|  | method | void replaceWith((Node or DOMString)... nodes) | Replaces this node in its parent with *nodes*. |
| [Comment](https://dom.spec.whatwg.org/#interface-comment) | interface | Comment | The Comment interface represents textual notations within markup; although it is generally not visually shown, such comments are available to be read in the source view |
| | constructor | Comment(optional DOMString data = "") | Returns a Comment object with the parameter as its textual content. |
| [CSSRule](https://drafts.csswg.org/cssom/#the-cssrule-interface) | interface | CSSRule | The CSSRule interface represents an abstract, base CSS style rule. Each distinct CSS style rule type is represented by a distinct interface that inherits from this interface. |
|  | constant | STYLE_RULE = 1 |  |
|  | constant | CHARSET_RULE = 2 |  |
|  | constant | IMPORT_RULE = 3 |  |
|  | constant | MEDIA_RULE = 4 |  |
|  | constant | FONT_FACE_RULE = 5 |  |
|  | constant | PAGE_RULE = 6 |  |
|  | constant | KEYFRAMES_RULE = 7 |  |
|  | constant | KEYFRAME_RULE = 8 |  |
|  | constant | MARGIN_RULE = 9 |  |
|  | constant | NAMESPACE_RULE = 10 |  |
|  | constant | COUNTER_STYLE_RULE = 11 |  |
|  | constant | SUPPORTS_RULE = 12 |  |
|  | constant | DOCUMENT_RULE = 13 |  |
|  | constant | FONT_FEATURE_VALUES_RULE = 14 |  |
|  | constant | VIEWPORT_RULE = 15 |  |
|  | constant | REGION_STYLE_RULE = 16 |  |
|  | attribute | type | One of the Type constants indicating the type of CSS rule. |
|  | attribute | cssText | Returns a serialization of the CSS rule. |
|  | attribute | parentRule | Returns the parent CSS rule. |
|  | attribute | parentStyleSheet | Returns the parent CSS style sheet. |
| [CSSStyleDeclaration](https://dev.w3.org/csswg/cssom/#the-cssstyledeclaration-interface) | interface | CSSStyleDeclaration | The CSSStyleDeclaration interface represents a CSS declaration block, including its underlying state, where this underlying state depends upon the source of the CSSStyleDeclaration instance. |
|  | attribute | cssText | Returns the result of serializing the declarations, or sets cssText attribute if after parsing the given value, the return value is not null. |
|  | attribute | length | Returns the number of CSS declarations in the declarations. |
|  | method | getter DOMString item(unsigned long index) | Returns the property name of the CSS declaration at position index. |
|  | method | DOMString getPropertyValue(DOMString property) | Returns the property value |
|  | method | void setProperty(DOMString property, [TreatNullAs=EmptyString] DOMString value, [TreatNullAs=EmptyString] optional DOMString priority = "") | Sets the property |
|  | attribute | parentRule | Returns the parent CSS rule. |
|  | attribute | cssFloat | Returns the result of invoking getPropertyValue() with float as argument. |
| [CSSStyleRule](https://dev.w3.org/csswg/cssom/#the-cssstylerule-interface) | interface | CSSStyleRule | Represents a style rule. |
|  | attribute | selectorText | Returns the result of serializing the associated group of selectors. (Note: We will support result separated by ',' for a while.)|
|  | attribute | style | Returns a CSSStyleDeclaration object for the style rule. |
| [CSSImportRule](https://drafts.csswg.org/cssom/#the-cssimportrule-interface) | interface | CSSImportRule | Represents an @import at-rule. |
|  | attribute | href | Returns the URL specified by the @import at-rule. |
|  | attribute | media | Returns the value of the media attribute of the associated CSS style sheet. |
|  | attribute | styleSheet | Returns a CSS style sheet downloaded by @import at-rule. |
| [CSSGroupingRule](https://drafts.csswg.org/cssom/#the-cssgroupingrule-interface) | interface | CSSGroupingRule | Represents an at-rule that contains other rules nested inside itself. |
|  | attribute | cssRules | Returns a CSSRuleList object for the child CSS rules. |
|  | method | unsigned long insertRule(CSSOMString rule, optional unsigned long index = 0) | Returns the result of invoking insert a CSS rule rule into the child CSS rules at index. |
|  | method | void deleteRule(unsigned long index) | Removes a CSS rule from the child CSS rules at index. |
| [CSSConditionRule](https://drafts.csswg.org/css-conditional-3/#cssconditionrule) | interface | CSSConditionRule | Represents all the “conditional” at-rules, which consist of a condition and a statement block. |
|  | attribute | conditionText | Experimental. Partial support (only getter works). Returns the result of serializing the associated condition. |
| [CSSMediaRule](https://drafts.csswg.org/css-conditional-3/#cssmediarule) | interface | CSSMediaRule | Represents a @media at-rule. |
|  | attribute | media | Returns a MediaList object for the list of media queries specified with the @media at-rule.|
| [CSSSupportsRule](https://drafts.csswg.org/css-conditional-3/#csssupportsrule) | interface | CSSSupportsRule | Represents a @supports at-rule. |
|  | attribute | conditionText | Returns the value of conditionText on the rule. (CSSSupportsRule-specific definition for attribute on CSSConditionRule) On setting, if the given conditionText evaluates to true, the original conditionText is replaced by the given conditionText. |
| [CSSKeyframesRule](https://drafts.csswg.org/css-animations/#interface-csskeyframesrule) | interface | CSSKeyframesRule | Represents a complete set of keyframes for a single animation. |
|  | attribute | name | Returns the name of the keyframes, used by the animation-name property. |
|  | attribute | cssRules | Gives access to the keyframes in the list. |
|  | method | void appendRule(CSSOMString rule) | Appends the passed CSSKeyframeRule at the end of the keyframes rule. |
|  | method | void deleteRule(CSSOMString select) | Deletes the last declared CSSKeyframeRule matching the specified keyframe selector. If no matching rule exists, the method does nothing. |
|  | method | CSSKeyframeRule? findRule(CSSOMString select) | Returns the last declared CSSKeyframeRule matching the specified keyframe selector. If no matching rule exists, the method does nothing. |
| [CSSKeyframeRule](https://drafts.csswg.org/css-animations/#interface-csskeyframerule) | interface | CSSKeyframeRule | Represents the style rule for a single key. |
|  | attribute | keyText | Represents the keyframe selector as a comma-separated list of percentage values. |
|  | attribute | style | Return a CSSStyleDeclaration object for the keyframe rule |
| [CSSStyleSheet](https://drafts.csswg.org/cssom/#the-cssstylesheet-interface) | interface | CSSStyleSheet | Represents a CSS style sheet. |
| | attribute | ownerRule | If this style sheet is imported into the document using an @import rule, the ownerRule property will return that CSSImportRule, otherwise it returns null. |
| | attribute | cssRules | Returns a live CSSRuleList, listing the CSSRule objects in the style sheet. |
| | attribute | rules | Non-standard. Synonym for cssRules. |
| | method | unsigned long insertRule(CSSOMString rule, optional unsigned long index = 0) | Inserts a new rule at the specified position in the style sheet, given the textual representation of the rule. |
| | method | void deleteRule(unsigned long index) | Deletes a rule at the specified position from the style sheet. |
| [CSSRuleList](https://drafts.csswg.org/cssom/#the-cssrulelist-interface) | interface | CSSRuleList | Represents an ordered collection of CSS style rules. |
| | method | getter CSSRule? item(unsigned long index) | Returns the indexth CSSRule object in the collection. |
| | attribute | length | Returns the number of CSSRule objects represented by the collection. |
| [Document](https://www.w3.org/TR/dom/#interface-document) | interface | Document | Also refer to Document [1](https://drafts.csswg.org/cssom/#extensions-to-the-document-interface), [2](https://www.w3.org/TR/dom/#interface-nonelementparentnode) and [3](https://www.w3.org/TR/dom/#parentnode)   |
|  | attribute | documentURI | Returns document's URL. |
|  | attribute | URL | Returns document's URL (legacy alias of `documentURI`). |
|  | attribute | domain | Gets/sets the document's effective domain. (Sets only — same-origin checks are not enforced in LWE.) |
|  | attribute | referrer | Returns the URL of the Document from which the user navigated to this one, unless it was blocked or there was no such document, in which case it returns the empty string. |
|  | attribute | origin | Returns document's origin. |
|  | attribute | compatMode | Returns the string "CSS1Compat". |
|  | attribute | charset | Returns document's encoding type ""UTF8"". |
|  | attribute | inputEncoding | Returns document's encoding (alias of `characterSet`; legacy). |
|  | attribute | characterSet | Returns document's encoding type ""UTF8"". |
|  | attribute | contentType | Returns document's content type. |
|  | attribute | doctype | Returns the doctype or null if there is none. |
|  | attribute | implementation | Returns the DOMImplementation object associated with the document. |
|  | attribute | documentElement | Returns the document element. |
|  | attribute | title | Returns the title of document. |
|  | attribute | dir | Returns the dir attribute of html element. |
|  | attribute | currentScript | Returns HTMLScriptElement, or SVGScriptElement, that is currently executing |
|  | attribute | readyState | Returns loading state of the document |
|  | attribute | onreadystatechange | Event handler related with document's readyState |
|  | method | HTMLCollection getElementsByTagName(DOMString qualifiedName) | If localName is "\*" returns an HTMLCollection of all descendant elements.Otherwise, returns an HTMLCollection of all descendant elements whose local name is localName. |
|  | method | HTMLCollection getElementsByTagNameNS(DOMString namespace, DOMString localName) | Returns a HTMLCollection of all descendant elements whose namespace is namespace and local name is localName. |
|  | method | NodeList getElementsByName(DOMString name) | returns an NodeList of all descendant elements whose name is name. |
|  | method | HTMLCollection getElementsByClassName(DOMString classNames) | Returns an HTMLCollection of the elements in the object on which the method was invoked (a document or an element) that have all the classes given by classes. |
|  | method | Element createElement(DOMString localName) | Returns an element in the HTML namespace with localName as local name. |
|  | method | Element createElementNS(DOMString namespace, DOMString localName) | Returns an element with namespace namespace. Its namespace prefix will be everything before ":" (U+003E) in qualifiedName or null. Its local name will be everything after ":" (U+003E) in qualifiedName or qualifiedName. |
|  | method | DocumentFragment createDocumentFragment() | Returns a new DocumentFragment node with its node document set to the context object. |
|  | method | Text createTextNode(DOMString data) | Returns a Text node whose data is data. |
|  | method | Comment createComment(DOMString data) | Returns a Comment node whose data is data. |
| [Document](https://dom.spec.whatwg.org/#interface-document) | method | CDATASection createCDATASection(DOMString data) | Returns a CDATASection node whose data is data. |
|  | method | ProcessingInstruction createProcessingInstruction(DOMString target, DOMString data) | Return a new ProcessingInstruction node, with target set to target, data set to data |
|  | method | Attr createAttribute(DOMString localName) | Return a new attribute whose local name is localName and node document is context object. |
|  | method | Attr createAttributeNS(DOMString? namespace, DOMString qualifiedName) | Creates an attribute of the given qualified name and namespace URI. |
|  | method | Document open( [ type [, replace ] ] ) | Causes the Document to be replaced in-place |
|  | method | void close() | Closes the input stream that was opened by the document.open() method |
|  | method | void write(text...) | In general, adds the given string(s) to the Document's input stream. |
|  | method | void writeln(text...) | Adds the given string(s) to the Document's input stream, followed by a newline character |
|  | method | Node importNode(Node node, optional boolean deep = false) | Creates a new copy of the specified Node or DocumentFragment from another document. |
|  | method | Node adoptNode(Node node) | Moves node from another document and returns it. |
|  | method | Event createEvent(DOMString type) | Returns a new Event object whose type is `type` (e.g. `"Event"`, `"CustomEvent"`). |
|  | method | Range createRange() | Returns a new live `Range` whose start and end are `(this, 0)`. |
|  | method | NodeIterator createNodeIterator(Node root, optional unsigned long whatToShow = 0xFFFFFFFF, optional any filter = null) | Returns a new `NodeIterator` rooted at `root`. **Iteration is not actually wired up — `nextNode()` returns null immediately.** Use `createTreeWalker` instead. |
|  | method | TreeWalker createTreeWalker(Node root, optional unsigned long whatToShow = 0xFFFFFFFF, optional any filter = null) | Returns a new `TreeWalker` rooted at `root`. Fully functional. |
|  | method | boolean hasFocus() | Returns whether the document has focus. |
| [Document](https://html.spec.whatwg.org/multipage/dom.html#the-document-object) | attribute | location | Return this Document object's relevant global object's Location object |
|  | attribute | body | Returns body element or null if not exists |
|  | attribute | head | Returns head element or null if not exists |
|  | attribute | images | Returns an HTMLCollection rooted at the Document node, whose filter matches only img elements |
|  | attribute | forms | Returns an HTMLCollection rooted at the Document node, whose filter matches only form elements |
|  | attribute | scripts | Returns an HTMLCollection rooted at the Document node, whose filter matches only script elements |
|  | attribute | links | Returns an HTMLCollection of all `a` and `area` elements with an `href` attribute. |
|  | attribute | anchors | Returns an HTMLCollection of all `a` elements with a `name` attribute. |
|  | attribute | defaultView | Returns this Document's browsing context's WindowProxy object, if this Document has an associated browsing context, or null otherwise |
|  | attribute | activeElement | Returns the currently focused element. |
|  | attribute | designMode | Returns "on" if the document is editable, and "off" if it isn't. Can be set, to change the document's current state. This focuses the document and resets the selection in that document. |
|  | attribute | cookie | Represents the cookies of the resource identified by the document's URL. |
|  | misc | **Unsupported in LWE** (`[Unimplemented]` in IDL — these return `undefined` at runtime) | `lastModified`, `embeds`, `plugins`, `applets`, `all`, `elementsFromPoint`, `caretPositionFromPoint`, `execCommand`, `queryCommandEnabled`, `queryCommandIndeterm`, `queryCommandState`, `queryCommandSupported`, `queryCommandValue`. Use `elementFromPoint` for the topmost element only. |
| [Document](https://drafts.csswg.org/cssom-view/#extensions-to-the-document-interface) | method | Element? elementFromPoint(double x, double y); | If there is a layout box in the viewport that would be a target for hit testing at coordinates x,y, return the associated element. If the document has a root element, returns the root element. Otherwise returns null |
| | attribute | scrollingElement | Returns a reference to the Element that scrolls the document. |
| [Document](https://drafts.csswg.org/cssom/#extensions-to-the-document-interface) | attribute | styleSheets | Returns a StyleSheetList collection representing the document CSS style sheets. |
| [Document](https://www.w3.org/TR/page-visibility/#sec-document-interface) | attribute | hidden | Returns true if the Document contained by the top level browsing context (root window in the browser's viewport) is not visible at all. |
| | attribute | visibilityState | Returns one of the following strings: "hidden", or "visible" |
| [Document](https://fullscreen.spec.whatwg.org/#api) | attribute | fullscreenElement | Returns the element currently displayed fullscreen, or null. `webkitFullscreenElement` is an alias. |
| | attribute | fullscreenEnabled | Returns whether fullscreen is available. `webkitFullscreenEnabled` is an alias. |
| | method | void exitFullscreen() | Exits fullscreen. `webkitExitFullscreen()` is an alias. |
| Document (non-standard) | method | (HTMLCollection or Node or null) document._nodeName_ | Returns elements of type a, applet, area, embed, form, frameset, img, or object with name="_nodeName_". Returns an element if there is only one such element. |
| [VisibilityChange Event](https://www.w3.org/TR/page-visibility/#sec-visibilitychange-event) | Event Handler | visibilitychange | Fire when the content of a tab has become visible or has been hidden. |
| [DocumentFragment](https://dom.spec.whatwg.org/#interface-documentfragment) | interface | DocumentFragment | A minimal node container; siblings inserted into the live tree via `appendChild` are moved out of the fragment. The `DocumentFragment()` constructor is **not** implemented — calling `new DocumentFragment()` throws `TypeError: Illegal constructor`; use `document.createDocumentFragment()` instead. Implements `NonElementParentNode` + `ParentNode` (so `getElementById`, `children`, `firstElementChild`, `lastElementChild`, `childElementCount`, `prepend`, `append`, `querySelector`, `querySelectorAll`). |
| [DocumentType](https://dom.spec.whatwg.org/#documenttype) | interface | DocumentType | Document type |
|  | attribute | name | Return the context object’s name. |
|  | attribute | publicId | Return the context object’s public ID. |
|  | attribute | systemId | Return the context object’s system ID. |
| [DOMStringList](https://html.spec.whatwg.org/#domstringlist) | interface | DOMStringList | |
|  | attribute | length | Returns the number of strings in strings. |
|  | method | DOMString? item(unsigned long index) | Returns the string with index index from strings. |
|  | method | contain | Returns true if strings contains string, and false otherwise. |
| [DOMException](https://heycam.github.io/webidl/#idl-exceptions) | interface | DOMException |  |
|  | attribute | code | Exception code |
|  | attribute | name | optional exception name |
|  | attribute | message | optional exception message |
|  | constant | INDEX_SIZE_ERR = 1 | Deprecated. Use RangeError instead. |
|  | constant | HIERARCHY_REQUEST_ERR = 3 | The operation would yield an incorrect node tree. |
|  | constant | WRONG_DOCUMENT_ERR = 4 | The object is in the wrong document. |
|  | constant | INVALID_CHARACTER_ERR = 5 | The string contains invalid characters. |
|  | constant | NO_MODIFICATION_ALLOWED_ERR = 7 | The object can not be modified. |
|  | constant | NOT_FOUND_ERR = 8 | The object can not be found here. |
|  | constant | NOT_SUPPORTED_ERR = 9 | The operation is not supported. |
|  | constant | INUSE_ATTRIBUTE_ERR = 10 | The attribute is in use. |
|  | constant | INVALID_STATE_ERR = 11 | The object is in an invalid state. |
|  | constant | SYNTAX_ERR = 12 | The string did not match the expected pattern. |
|  | constant | INVALID_MODIFICATION_ERR = 13 | The object can not be modified in this way. |
|  | constant | NAMESPACE_ERR = 14 | The operation is not allowed by Namespaces in XML.  |
|  | constant | INVALID_ACCESS_ERR = 15 | Deprecated. Use TypeError for invalid arguments, "NotSupportedError" DOMException for unsupported operations, and "NotAllowedError" DOMException for denied requests instead. |
|  | constant | SECURITY_ERR = 18 | The operation is insecure. |
|  | constant | NETWORK_ERR = 19 | A network error occurred. |
|  | constant | ABORT_ERR = 20 | The operation was aborted. |
|  | constant | URL_MISMATCH_ERR = 21 | The quota has been exceeded.The given URL does not match another URL. |
|  | constant | QUOTA_EXCEEDED_ERR = 22 | The quota has been exceeded. |
|  | constant | TIMEOUT_ERR = 23 | The operation timed out. |
|  | constant | INVALID_NODE_TYPE_ERR = 24 | The supplied node is incorrect or has an incorrect ancestor for this operation. |
|  | constant | DATA_CLONE_ERR = 25 | The object can not be cloned. |
| [DOMParser](https://w3c.github.io/DOM-Parsing/#the-domparser-interface) | interface | DOMParser | DOMParser can parse XML or HTML source stored in a string into a DOM Document.  |
|  | constructor | DOMParser() | Create a new DOMParser |
|  | enum | SupportedType | "text/html", "text/xml", "application/xml", "application/xhtml+xml", "image/svg+xml" |
|  | method | Document parseFromString(DOMString str, SupportedType type) | Parse str using a parser that matches type's supported MIME types. |
| [DOMPoint](https://drafts.fxtf.org/geometry/#DOMPoint) | interface | DOMPoint |  |
|  | constructor | DOMPoint(optional unrestricted double x = 0, optional unrestricted double y = 0, optional unrestricted double z = 0, optional unrestricted double w = 1) | Creates a new DOMPoint object. |
|  | attribute | x | Return the x coordinate value of the object it was invoked on. |
|  | attribute | y | Return the y coordinate value of the object it was invoked on. |
|  | attribute | z | Return the z coordinate value of the object it was invoked on. |
|  | attribute | w | Return the w perspective value of the object it was invoked on. |
| [DOMPointInit](https://drafts.fxtf.org/geometry/#dictdef-dompointinit) | dictionary | DOMPointInit | Members `x`, `y`, `z`, `w` (all `unrestricted double`) used to initialize a `DOMPoint`/`DOMPointReadOnly`. |
|  [DOMPointReadOnly](https://drafts.fxtf.org/geometry/#dompointreadonly)  |  attribute  |  x  |  Return  x coordinate value of the object  |
|    |  attribute  |  y  |  Return y coordinate value of the object  |
|    |  attribute  |  z  |  Return z coordinate value of the object  |
|    |  attribute  |  w  |  Return w perspective value of the object  |
| [DOMQuad](https://drafts.fxtf.org/geometry/#DOMQuad) | interface | DOMQuad | Objects implementing the DOMQuad interface represents a quadrilateral. |
| | constructor | DOMQuad(optional DOMPointInit p1, optional DOMPointInit p2, optional DOMPointInit p3, optional DOMPointInit p4) | |
|  | attribute | p1 | Return a DOMPoint that represents p1 of the quadrilateral |
|  | attribute | p2 | Return a DOMPoint that represents p2 of the quadrilateral |
|  | attribute | p3 | Return a DOMPoint that represents p3 of the quadrilateral |
|  | attribute | p4 | Return a DOMPoint that represents p4 of the quadrilateral |
|  | method | DOMRect getBounds() | Return bounds |
|  [DOMRect](https://drafts.fxtf.org/geometry/#domrect) | interface | DOMRect | Represents a rectangle. |
|    |  attribute  |  x  |  Return x coordinate value of the object   |
|    |  attribute  |  y  |  Return y coordinate value of the object   |
|    |  attribute  |  width  |  Return width dimension value of the object  |
|    |  attribute  |  height  |  Return height dimension value of the object  |
| [DOMRectList](https://dxr.mozilla.org/mozilla-central/source/dom/webidl/DOMRectList.webidl) | interface | DOMRectList | The DOMRectList objects are collections of DOMRects. DOMRectList must be supported for legacy reasons. New interfaces must not use DOMRectList and may use Sequences instead. |
|  | attribute | length | Returns the total number of DOMRect objects associated with the object. |
|  | method | DOMRect? item(unsigned long index) | Returns the DOMRect with the index number. |
|  [DOMRectReadOnly](https://drafts.fxtf.org/geometry/#domrectreadonly)  | interface | DOMRectReadOnly | Specifies the standard properties used by DOMRect to define a rectangle. |
|    |  attribute  |  x  |  Return x coordinate value of the object   |
|    |  attribute  |  y  |  Return y coordinate value of the object   |
|    |  attribute  |  width  |  Return width dimension value of the object  |
|    |  attribute  |  height  |  Return height dimension value of the object  |
|    |  attribute  |  top  |  Return min(y coordinate, y coordinate + height dimension) of the object  |
|    |  attribute  |  right  |  Return max(x coordinate, x coordinate + width dimension) of the object  |
|    |  attribute  |  bottom  |  Return max(y coordinate, y coordinate + height dimension) of the object  |
|    |  attribute  |  left  |  Return min(x coordinate, x coordinate + width dimension) of the object  |
|  [DOMTokenList](https://dom.spec.whatwg.org/#interface-domtokenlist)  |  attribute  |  length  |  Returns the number of tokens. |
|    | method | DOMString? item(unsigned long index) (or tokenlist[index])  |  Returns the token with the index index number. |
|    | method | boolean contains(DOMString token)  |  Returns true if token is present, and false otherwise. |
|    | method | void add(DOMString... tokens)  |  Adds all arguments passed, except those already present. |
|    | method | void remove(DOMString... tokens)  |  Removes arguments passed, if they are present. |
|    | method | boolean toggle(DOMString token, optional boolean force = false)  |  If force is not specified, "toggles" token, removing it if it is present and adding it if it is not. If force is true, adds token (same as add()). If force is false, removes token (same as remove()). |
|    | method | boolean replace(DOMString token, DOMString newToken)  |  Replaces an existing token with a new token. |
|    | method | boolean supports(DOMString token)  |  Returns true if a given token is in the associated attribute's supported tokens. |
|    | attibute | value | Represents The value of the list as a DOMString. |
|  [DOMImplementation](https://dom.spec.whatwg.org/#domimplementation)  |  method  |  DocumentType createDocumentType(DOMString qualifiedName, DOMString publicId, DOMString systemId)  |  Returns a doctype, with the given qualifiedName, publicId, and systemId. If qualifiedName does not match the Name production, an InvalidCharacterError is thrown, and if it does not match the QName production, a NamespaceError is thrown. |
|    |  method  |    XMLDocument createDocument(DOMString? namespace, [TreatNullAs=EmptyString] DOMString qualifiedName, optional DocumentType? doctype = null)  |  Returns an XMLDocument, with a document element whose local name is qualifiedName and whose namespace is namespace (unless qualifiedName is the empty string), and with doctype, if it is given, as its doctype. |
|    |  method  |  Document createHTMLDocument(optional DOMString title);  |  Returns a document, with a basic tree already constructed including a title element, unless the title argument is omitted. |
|    |  method  |  boolean hasFeature(); |  useless; always returns true |
| [Element](https://dom.spec.whatwg.org/#interface-element) | interface | Element | Element nodes are simply known as elements. |
|  | attribute | prefix | Return the context object’s namespace prefix. |
|  | attribute | namespaceURI | Return the context object’s namespace. |
|  | attribute | localName | Return the value of the attribute in element's attribute list whose namespace is namespace and local name is localName, if it has one, and null otherwise. |
|  | attribute | tagName | If namespace prefix is not null, returns the concatenation of namespace prefix, ":", and local name. Otherwise it returns the local name. |
|  | attribute | id | Reflects the "id" content attribute. |
|  | attribute | className | Reflects the "class" content attribute. |
|  | attribute | classList | Returns the associated DOMTokenList object representing the context object's classes. |
|  | attribute | attributes | Returns a NamedNodeMap. |
|  | method | DOMString? getAttribute(DOMString qualifiedName) | Returns the value of the first attribute in the context object's attribute list whose name is *qualifiedName*, or null otherwise. |
|  | method | DOMString? getAttributeNS(DOMString? namespace, DOMString localName) | Returns the value of the first attribute in the context object's attribute list whose name is *localName* and namespace is *namespace*, or null otherwise. |
|  | method | void setAttribute(DOMString qualifiedName, DOMString value) | Changes the attribute from context object whose name is *qualifiedName* to *value*. |
|  | method | void setAttributeNS(DOMString? namespace, DOMString localName, DOMString value) | Changes the attribute from context object whose name is *localName* and namespace is *namespace*  to *value*. |
|  | method | void removeAttribute(DOMString qualifiedName) | Removes the first attribute from the context object whose name is *qualifiedName*, if exists. |
|  | method | void removeAttributeNS(DOMString? namespace, DOMString localName) | Removes the first attribute from the context object whose name is *localName* and namespace is *namespace*, if exists. |
|  | method | boolean hasAttribute(DOMString qualifiedName) | Returns true if the context object has an attribute whose name is *qualifiedName*, or false otherwise. |
|  | method | boolean hasAttributeNS(DOMString? namespace, DOMString localName) | Returns true if the context object has an attribute whose name is *localName* and namespace is *namespace*, or false otherwise. |
|  | method | Attr? getAttributeNode(DOMString qualifiedName) | Returns the specified attribute of the specified element, as an Attr node. |
|  | method | Attr? getAttributeNodeNS(DOMString? namespace, DOMString localName) | Retrieves an Attr node by local name and namespace URI. |
|  | method | Attr? setAttributeNode(Attr attr) | Adds a new Attr node to the specified element. If the attribute named already exists on the element, that attribute is replaced with the new one and the replaced one is returned. |
|  | method | Attr? setAttributeNodeNS(Attr attr) | Adds a new Attr node. If an attribute with that local name and that namespace URI is already present in the element, it is replaced by the new one. |
|  | method | Attr removeAttributeNode(Attr attr) | Removes the specified attribute from the current element. |
|  | method | boolean hasAttributes() | Returns true if element has any attribute. |
|  | method | sequence&lt;DOMString&gt; getAttributeNames() | Returns sequence of Attributes's QualifiedName. |
|  | method | Element? closest(DOMString selectors) | Returns the closest ancestor of the current element (or the current element itself) which matches the selectors given in parameter. If there isn't such an ancestor, it returns null. |
|  | method | boolean matches(DOMString selectors) | Returns true if matching selectors against element’s root yields element, and false otherwise. |
|  | method | HTMLCollection getElementsByTagName(DOMString qualifiedName) | Returns the list of elements with local name localName for the context object. |
|  | method | HTMLCollection getElementsByTagNameNS(DOMString namespace, DOMString localName) | Returns a HTMLCollection of all descendant elements whose namespace is namespace and local name is localName. |
|  | method | HTMLCollection getElementsByClassName(DOMString classNames) | Returns the list of elements with class names classNames for the context object. |
|  | method | Node? insertAdjacentElement(DOMString where, Element element) | Inserts *element* into the tree at the position given by *where* (`beforebegin`/`afterbegin`/`beforeend`/`afterend`). |
|  | method | void insertAdjacentText(DOMString where, DOMString data) | Inserts a Text node at the position given by *where*. |
|  | attribute | slot | Reflects the `slot` content attribute. |
|  | method | boolean toggleAttribute(DOMString qualifiedName, optional boolean force) | Toggles the named attribute; with `force` set, conditionally adds or removes it. Returns the new presence state. |
|  | method | ShadowRoot attachShadow(ShadowRootInit init) | Creates a shadow root for the element. `ShadowRootInit` accepts `mode` (required), `delegatesFocus`, `slotAssignment`, `clonable`, and `serializable`. |
|  | attribute | shadowRoot | Returns the open shadow root attached via `attachShadow({mode:"open"})`, or `null`. |
|  | method | void setPointerCapture(long pointerId) | Stub: bound but currently a no-op (logs `UNIMPLEMENTED`). |
|  | method | void releasePointerCapture(long pointerId) | Stub: bound but currently a no-op. |
|  | method | boolean hasPointerCapture(long pointerId) | Stub: always returns `false`. |
| [Element](https://w3c.github.io/DOM-Parsing/#extensions-to-the-element-interface) | attribute | innerHTML | Return a fragment of HTML or XML that represents the element's contents.|
|| attribute | outerHTML | Return a fragment of HTML or XML that represents the element|
|| method | insertAdjacentHTML | Parses the given string text as HTML or XML and inserts the resulting nodes into the tree in the position given by the position argument |
| [ShadowRoot](https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#dom-innerhtml) | attribute | innerHTML | Same `InnerHTML` mixin member as on `Element`, exposed on `ShadowRoot` too: builds the shadow tree from markup (parsed with the shadow host as the context element) and serializes it back. `outerHTML`/`insertAdjacentHTML` are Element-only per spec. |
| [Element](https://drafts.csswg.org/cssom-view/#extension-to-the-element-interface) | method | getClientRects | Return a collection of rectangles that indicate the bounding rectangles for each box in a client. (Note: This API is supported only in case of that display property is `BLOCK`.)|
|  | method | getBoundingClientRect | Return the size of an element and its position relative to the viewport. (Note: This API is supported only in case of that display property is `BLOCK`.)|
|  | attribute | clientTop | Return the width of the top border of an element in pixels. |
|  | attribute | clientLeft | Return the width of the left border of an element in pixels. |
|  | attribute | clientWidth | Return zero for elements with no CSS or inline layout boxes, otherwise the inner width of an element in pixels. |
|  | attribute | clientHeight | Return zero for elements with no CSS or inline layout boxes, otherwise the inner height of an element in pixels. |
|  | attribute | scrollLeft | gets or sets the number of pixels that an element's content is scrolled to the left |
|  | attribute | scrollTop | gets or sets the number of pixels that an element's content is scrolled to the top |
|  | attribute | scrollWidth | returns either the width in pixels of the content of an element or the width of the element itself, whichever is greater |
|  | attribute | scrollHeight | returns either the height in pixels of the content of an element or the height of the element itself, whichever is greater |
|  | method | scrollIntoView | scrolls the element on which it's called into the visible area of the browser window. |
|  | method | scrollIntoView(bool alignToTop) | scrolls the element on which it's called into the visible area of the browser window. |
|  | method | scrollIntoView(ScrollIntoViewOptions options) | Accepts `{block, inline}` (`start`/`center`/`end`/`nearest`). |
|  | method | scroll(x, y) / scroll(ScrollToOptions) | Scrolls the element's scrolling box. |
|  | method | scrollTo(x, y) / scrollTo(ScrollToOptions) | Same as `scroll`. |
|  | method | scrollBy(x, y) / scrollBy(ScrollToOptions) | Scrolls the element's scrolling box by the given delta. |
| [EventTarget](https://dom.spec.whatwg.org/#interface-eventtarget) | interface | EventTarget | Represents the target to which an event is dispatched when something has occurred. |
| | method | void addEventListener(DOMString type, EventListener? callback, optional boolean capture=false) | Adds the specified EventListener-compatible object to the list of event listeners for the specified event type on the EventTarget on which it's called. (NOTE: The lightweight web engine only supports boolean type for third argument) |
| | method | void removeEventListener(DOMString type, EventListener? callback, optional boolean captures=false) | Removes from the EventTarget an event listener previously registered with EventTarget.addEventListener(). (NOTE: The lightweight web engine only supports boolean type for third argument) |
| | method | boolean dispatchEvent(Event event) | Dispatches an Event at the specified EventTarget, invoking the affected EventListeners in the appropriate order. |
| [ElementCSSInlineStyle](https://drafts.csswg.org/cssom/#elementcssinlinestyle) | interface | ElementCSSInlineStyle | The ElementCSSInlineStyle interface provides access to inline style properties of an element. |
|  | attribute | style | Return a live CSS declaration block. |
| [HTMLAnchorElement](https://html.spec.whatwg.org/multipage/semantics.html#the-a-element) | interface | HTMLAnchorElement | The HTMLAnchorElement interface represents hyperlink elements and provides special properties and methods (beyond those of the regular HTMLElement object interface that they inherit from) for manipulating the layout and presentation of such elements. |
|  | attribute | target | Reflect the respective content attribute of the same name |
|  | attribute | rel | Is a DOMString that reflects the rel HTML attribute, specifying the relationship of the target object to the linked object. |
|  | attribute | relList | Returns a DOMTokenList that reflects the rel HTML attribute, as a list of tokens. |
|  | attribute | hreflang | Is a DOMString that reflects the hreflang HTML attribute, indicating the language of the linked resource. |
|  | attribute | type | Is a DOMString that reflects the type HTML attribute, indicating the MIME type of the linked resource. |
|  | attribute | text| Is a DOMString being a synonym for the Node.textContent property. |
|  | attribute | coord | Is a DOMString representing a comma-separated list of coordinates. |
|  | attribute | charset | Is a DOMString representing the character encoding of the linked resource. |
|  | attribute | name | Is a DOMString representing the anchor name. |
|  | attribute | rev | Is a DOMString representing that the rev HTML attribute, specifying the relationship of the link object to the target object. |
|  | attribute | shape | Is a DOMString representing the shape of the active area. |
| [HTMLAreaElement](https://html.spec.whatwg.org/multipage/image-maps.html#the-area-element) | interface | HTMLAreaElement | The area element represents either a hyperlink with some text and a corresponding area on an image map, or a dead area on an image map. |
|  | attribute | target | Reflect the target HTML attribute, indicating the browsing context in which to open the linked resource. |
|  | attribute | noHref | Indicate if the area is inactive (true) or active (false). |
|  | attribute | rel | Is a DOMString that reflects the rel HTML attribute, indicating relationships of the current document to the linked resource. |
|  | attribute | relList | Returns a DOMTokenList that reflects the rel HTML attribute, indicating relationships of the current document to the linked resource, as a list of tokens. |
|  | attribute | referrerPolicy | Reflects the referrerpolicy HTML attribute indicating which referrer to use when fetching the linked resource. |
| [HTMLHyperlinkElementUtils](https://html.spec.whatwg.org/multipage/links.html#api-for-a-and-area-elements) | interface | HTMLHyperlinkElementUtils | The HTMLHyperlinkElementUtils mixin defines utility methods and properties to work with HTMLAnchorElement and HTMLAreaElement. These utilities allow to deal with common features like URLs. |
|  | attribute | href | Return the whole URL. |
|  | attribute | origin | Return the origin. |
|  | attribute | protocol | Return the whole protocol. |
|  | attribute | host | Return the whole host. |
|  | attribute | hostname | Return the hostname. |
|  | attribute | port | Return the port. |
|  | attribute | username | Return the username. |
|  | attribute | password | Return the password. |
|  | attribute | search | Return the search. |
|  | attribute | hash | Return the hash. |
| [HTMLAudioElement](https://www.w3.org/TR/html5/embedded-content-0.html#the-audio-element) | interface | HTMLAudioElement | The audio element represents a sound or audio stream. (Note: Currently, elements related to multimedia are checked on Tizen 2.4 TV Product.)|
|  | constructor | Audio(optional DOMString src="") | |
| [HTMLBodyElement](https://html.spec.whatwg.org/multipage/semantics.html#the-body-element) | interface | HTMLBodyElement | The body element represents the main content of the document. |
|  | attribute | onblur | Is an EventHandler for Window representing the code to be called when the blur event is raised. |
|  | attribute | onerror | Is an OnErrorEventHandler for Window representing the code to be called when the error event is raised. |
|  | attribute | onfocus | Is an EventHandler for Window representing the code to be called when the focus event is raised. |
|  | attribute | onload | Fired at the Window when the document has finished loading; fired at an element containing a resource (e.g. img, embed) when its resource has finished loading |
|  | attribute | onresize | Is an EventHandler for Window representing the code to be called when the resize event is raised. |
|  | attribute | bgColor | Is a DOMString that represents the background color for the document. |
|  | attribute | background | Is a DOMString that represents the description of the location of the background image resource. |
|  | attribute | text | Is a DOMString that represents the foreground color of text. |
| [HTMLBaseElement](https://html.spec.whatwg.org/multipage/semantics.html#the-base-element) | interface | HTMLBaseElement | The base element specifies the base URL to use for all relative URLs contained within a document. |
|  | attribute | href | The base URL to be used throughout the document for relative URL addresses. |
|  | attribute | target | A name or keyword indicating the default location to display the result when hyperlinks or forms cause navigation |
| [HTMLButtonElement](https://html.spec.whatwg.org/#the-button-element) | interface | HTMLButtonElement | The button element represents a button labeled by its contents. |
|  | attribute | disabled | Returns whether the button is disabled. |
|  | attribute | form | Returns the element's form element, if any, or null otherwise. |
|  | attribute | formAction | Specifies the URL of the file that will process the input control when the form is submitted. |
|  | attribute | formEnctype | Specifies how the form-data should be encoded when submitting it to the server. |
|  | attribute | formMethod | Defines the HTTP method for sending data to the action URL. |
|  | attribute | name | Returns the name of the input element. |
|  | attribute | type | Returns the type of the input element. |
|  | attribute | value | Returns the value of the input element. |
|  | attribute | labels | Is a NodeList that represents a list of label elements that are labels for this button. |
| [HTMLBRElement](https://html.spec.whatwg.org/multipage/semantics.html#the-br-element) | interface | HTMLBRElement | The br element represents a line break. |
| [HTMLCanvasElement](https://html.spec.whatwg.org/#the-canvas-element) | interface | HTMLCanvasElement | The canvas element provides scripts with a resolution-dependent bitmap canvas, which can be used for rendering graphs, game graphics, art, or other visual images on the fly. |
|  | attribute | width | Reflects the width HTML attribute. |
|  | attribute | height | Reflects the height HTML attribute. |
|  | method | getContext | Returns a drawing context on the canvas, or null if the context identifier is not supported. |
| [HTMLCollection](https://dom.spec.whatwg.org/#htmlcollection) | interface | HTMLCollection | A live, ordered collection of `Element` objects. Returned by `getElementsByTagName(NS)`, `getElementsByClassName`, and HTML form/`tbody`/`select` accessors. |
|  | attribute | length | Returns the number of elements in the collection. |
|  | method | Element? namedItem(DOMString name) | Returns the first element whose `id` or (for HTML form-associated elements) `name` matches. Named property access (`coll['someId']`) is equivalent. |
|  | iterable | iterable&lt;Node&gt; | Supports `for..of`, `forEach`. |
|  [HTMLCollection — legacy entry](https://dom.spec.whatwg.org/#htmlcollection)  |  attribute  |  length  |  (duplicate row preserved for legacy spec compatibility) |
|    |  method  |  Element? item(unsigned long index) (or collection[index])  |  Returns the element with index index number from the collection. The elements are sorted in tree order.  |
| [HTMLDivElement](https://www.w3.org/TR/html5/grouping-content.html#the-div-element) | interface | HTMLDivElement | Offers a generic mechanism for adding structure to documents |
| [HTMLDocument](https://www.w3.org/TR/DOM-Level-2-HTML/html.html#ID-26809268) | interface | HTMLDocument | An HTMLDocument is the root of the HTML hierarchy and holds the entire content. |
| [HTMLElement](https://html.spec.whatwg.org/multipage/dom.html#htmlelement) | interface | HTMLElement |  |
|  | attribute | dir | Returns the dir attribute specifies the element's text directionality |
|  | attribute | title | Reflects the "title" content attribute of HTMLElement. |
|  | attribute | lang |  Reflects the "lang" content attribute of HTMLElement. |
|  | attribute | hidden | Reflects the `hidden` boolean content attribute. |
|  | attribute | innerText | Like `textContent`, but observes CSS `display:none`/`white-space` rules. Writable. |
|  | attribute | dataset | Returns a `DOMStringMap` for `data-*` attributes (kebab-case → camelCase). |
|  | mixin | ElementContentEditable | Provides `contentEditable`/`isContentEditable`. |
|  | mixin | ElementCSSInlineStyle | Provides `style` (`CSSStyleDeclaration`). |
|  | mixin | ElementAnimation | Provides `animate()`; `getAnimations()` is unimplemented. |
|  | misc | **Unsupported in LWE** (IDL `[Unimplemented]` — return `undefined`) | `translate`, `accessKey`, `accessKeyLabel`, `draggable`, `contextMenu`, `spellcheck`, `forceSpellCheck`, `nonce`, `autofocus`. |
|  | misc | **Not exposed at all** | `popover`/`togglePopover`/`showPopover`/`hidePopover`, `outerText`, `inert`, `enterKeyHint`, `inputMode`. |
|  | method | Promise&lt;void&gt; requestFullscreen() | Requests that the element be displayed fullscreen. `webkitRequestFullscreen()`/`webkitRequestFullScreen()` are aliases of the same operation. |
|  | method | void click() | Acts as if the element was clicked. |
|  | attribute | tabIndex | Reflects the value of the "tabindex" content attribute of HTMLElement. Its default value is 0 for elements that are focusable and −1 for elements that are not focusable. |
|  | method | void focus(optional FocusOptions options) | This method sets focus on the specified element, if it can be focused. `FocusOptions` accepts `preventScroll` (default `false`) and `focusVisible`. |
|  | method | void blur() | This method removes focus from the current element. |
| [HTMLElement 2](https://drafts.csswg.org/cssom-view/#extensions-to-the-htmlelement-interface) | attribute | offsetParent | Returns a reference to the object which is the closest (nearest in the containment hierarchy) positioned containing element. |
| | attribute | offsetTop | Returns the distance of the current element relative to the top of the offsetParent node. |
| | attribute | offsetLeft | Returns the number of pixels that the upper left corner of the current element is offset to the left within the HTMLElement.offsetParent node. |
| | attribute | offsetWidth | Returns the border edge width of the first CSS layout box associated with the element. |
| | attribute | offsetHeight | Returns the border edge height of the first CSS layout box associated with the element. |
| [HTMLFieldSetElement](https://html.spec.whatwg.org/multipage/form-elements.html#the-fieldset-element) | interface | HTMLFieldSetElement |  represents a set of form controls optionally grouped under a common name. |
|  | attribute | form | Returns the element's form element, if any, or null otherwise. |
|  | attribute | name | Returns the element's name. |
|  | attribute | type | Returns the string "fieldset". |
|  | attribute | disabled | Returns whether the form control is disabled. |
| [HTMLFontElement](https://html.spec.whatwg.org/#htmlfontelement) | interface | HTMLFontElement | The font element defines the font size, font face, and color of text. |
|  | attribute | color | This attribute sets the text color. |
|  | attribute | size | This attribute sets the size of the font. |
| [HTMLFormElement](https://html.spec.whatwg.org/#forms) | interface | HTMLFormElement | The HTMLFormElement interface provides methods to create and modify form elements. |
|  | attribute | action | Returns action attribute that specifies where to send the form-data when a form is submitted. |
|  | attribute | enctype | Returns enctype attribute that specifies how the form-data should be encoded when submitting it to the server. |
|  | attribute | encoding | Reflect the enctype content attribute. |
|  | attribute | method | Returns method attribute that specifies the HTTP method to use when sending form-data |
|  | attribute | name | Returns the name of the form. |
|  | attribute | target | Reflecting the value of the form's target HTML attribute, indicating where to display the results received from submitting the form. |
|  | attribute | elements | A HTMLFormControlsCollection holding all form controls belonging to this form element. |
|  | attribute | length | A long reflecting  the number of controls in the form. |
| [HTMLHeadElement](https://html.spec.whatwg.org/multipage/semantics.html#the-head-element) | interface | HTMLHeadElement | The head element represents a collection of metadata for the Document. |
| [HTMLHRElement](https://html.spec.whatwg.org/multipage/grouping-content.html#the-hr-element) | interface | HTMLHRElement | The hr element represents a thematic break between paragraph-level elements. |
| [HTMLHeadingElement](https://html.spec.whatwg.org/#htmlheadingelement) | interface | HTMLHeadingElement | The h1, h2, h3, h4, h5 and h6 elements represent headings for their sections. |
|  | attribute | align | Returns the current value of the align content attribute. |
| [HTMLHtmlElement](https://html.spec.whatwg.org/multipage/semantics.html#the-html-element) | interface | HTMLHtmlElement | The html element represents the root of an HTML document. |
| [HTMLImageElement](https://html.spec.whatwg.org/multipage/embedded-content.html#the-img-element) | interface | HTMLImageElement | Represents an image. |
|  | constructor | Image(optional unsigned long width = 0, optional unsigned long height = 0) |  |
|  | attribute | src | Reflects the src HTML attribute, containing the full URL of the image including base URI. |
|  | attribute | crossOrigin | A DOMString representing the CORS setting for this image element. |
|  | attribute | width | Reflects the width HTML attribute, indicating the rendered width of the image in CSS pixels. |
|  | attribute | height | Reflects the height HTML attribute, indicating the rendered height of the image in CSS pixels. |
|  | attribute | referrerPolicy | Reflects the referrerpolicy HTML attribute indicating which referrer to use when fetching the linked resource. |
|  | attribute | name | Represents the name of the element. |
| [HTMLInputElement](https://html.spec.whatwg.org/multipage/input.html#the-input-element) | interface | HTMLInputElement | The input element represents a typed data field, usually with a form control to allow the user to edit the data. |
|  | attribute | autofocus | Returns / Sets the element's autofocus attribute, which specifies that a form control should have input focus when the page loads. |
|  | attribute | defaultChecked | Returns / Sets the default state of a radio button or checkbox as originally specified in HTML that created this object. |
|  | attribute | checked | Returns / Sets the current state of the element when type is checkbox or radio. |
|  | attribute | disabled | Returns / Sets the element's disabled attribute, indicating that the control is not available for interaction. |
|  | attribute | form | Returns the element's form owner, or null if there is not one. |
|  | attribute | formAction | Specifies the URL of the file that will process the input control when the form is submitted. |
|  | attribute | formEnctype | Specifies how the form-data should be encoded when submitting it to the server. |
|  | attribute | formMethod | Defines the HTTP method for sending data to the action URL. |
|  | attribute | formTarget | Returns / Sets the element's formtarget attribute, containing a name or keyword indicating where to display the response that is received after submitting the form. |
|  | attribute | max | Returns / Sets the element's max attribute, containing the maximum (numeric or date-time) value for this item, which must not be less than its minimum (min attribute) value. |
|  | attribute | maxLength | Returns / Sets the element's maxlength attribute, containing the maximum length of characters (in Unicode code points) that the value can have. |
|  | attribute | min | Returns / Sets the element's min attribute, containing the minimum (numeric or date-time) value for this item, which must not be greater than its maximum (max attribute) value. |
|  | attribute | minLength | Returns / Sets the element's minlength attribute, containing the minimum length of characters (in Unicode code points) that the value can have. |
|  | attribute | multiple | Returns / Sets the element's multiple attribute, indicating whether more than one value is possible (e.g., multiple files). |
|  | attribute | name | Returns the name of the input element. |
|  | attribute | placeholder | Returns / Sets the element's placeholder attribute, containing a hint to the user of what can be entered in the control. |
|  | attribute | required | Returns / Sets the element's required attribute, indicating that the user must fill in a value before submitting a form. |
|  | attribute | size | Returns / Sets the element's size attribute, containing size of the control. |
|  | attribute | step | Granularity to be matched by the form control's value |
|  | attribute | type | Returns the type of the input element. |
|  | attribute | defaultValue | Returns / Sets the default value as originally specified in the HTML that created this object. |
|  | attribute | labels | Is a NodeList that represents a list of label elements that are labels for this button. |
| [HTMLIFrameElement](https://html.spec.whatwg.org/multipage/iframe-embed-object.html#the-iframe-element) | interface | HTMLIFrameElement |  |
|  | attribute | src | Reflects the src HTML attribute, containing the full URL of the frame including base URI. |
|  | attribute | name | Reflects the name HTML attribute, containing a name by which to refer to the frame. |
|  | attribute | width | Reflects the width HTML attribute, indicating the rendered width of the frame in CSS pixels. |
|  | attribute | height | Reflects the height HTML attribute, indicating the rendered height of the frame in CSS pixels. |
|  | attribute | contentDocument | Returns the iframe element's content document. |
|  | attribute | contentWindow | Returns the WindowProxy object of the iframe element's nested browsing context, if its nested browsing context is non-null, or null otherwise. |
|  | attribute | referrerPolicy | Reflects the referrerpolicy HTML attribute indicating which referrer to use when fetching the linked resource. |
|  | attribute | scrolling | Specifies whether or not to display scrollbars in an iframe |
| [HTMLLabelElement](https://html.spec.whatwg.org/#the-label-element) | interface | HTMLLabelElement | Represents a caption in a user interface. |
|  | attribute  | form | Is a HTMLFormElement object representing the form with which the labeled control is associated |
|  | attribute  | htmlFor | Is a string containing the ID of the labeled control. This reflects the for attribute. |
|  | attribute  | control | Is a HTMLElement representing the control with which the label is associated. |
| [HTMLLegendElement](https://html.spec.whatwg.org/multipage/form-elements.html#the-legend-element) | interface | HTMLLegendElement | Represents a caption for the rest of the contents of the legend element's parent fieldset element, if any. |
|  | attribute  | form | Returns the element's form owner, or null if there is not one. |
| [HTMLLIElement](https://html.spec.whatwg.org/#htmllielement) | interface | HTMLLIElement | Represents a list item. |
|  | attribute  | value | Returns the ordinal value of the list item, or 1 if there is no one. |
|  | attribute  | type | Returns the style of the bullet point of a list item in a list. |
| [HTMLLinkElement](https://html.spec.whatwg.org/multipage/semantics.html#the-link-element) | interface | HTMLLinkElement | The HTMLLinkElement interface represents reference information for external resources and the relationship of those resources to a document and vice-versa |
|  | attribute | href | Is a DOMString representing the URI for the target resource. |
|  | attribute | crossOrigin | A DOMString that corresponds to the CORS setting for this link element. |
|  | attribute | rel | Is a DOMString representing the forward relationship of the linked resource from the document to the resource. |
|  | attribute | relList | Is a DOMTokenList that reflects the rel HTML attribute, as a list of tokens. |
|  | attribute | media | Is a DOMString representing a list of one or more media formats to which the resource applies. |
|  | attribute | hreflang | Is a DOMString representing the language code for the linked resource. |
|  | attribute | type | Is a DOMString representing the MIME type of the linked resource. |
|  | attribute | referrerPolicy | Is a DOMString the referrerpolicy HTML attribute indicating which referrer to use when fetching the linked resource. |
|  | attribute | charset | Is a DOMString representing the character encoding for the target resource. |
|  | attribute | rev | Is a DOMString representing the reverse relationship of the linked resource from the resource to the document. |
|  | attribute | target | Is a DOMString representing the name of the target frame to which the resource applies. |
| [HTMLOListElement](https://html.spec.whatwg.org/#htmlolistelement)  | interface | HTMLOListElement |  |
|  | attribute | reversed | Specifies that the list order should be descending. |
|  | attribute | start | Specifies the start value of an ordered list. |
|  | attribute | type | Specifies the kind of marker to use in the list. |
|  | attribute | compact | This variable just relect attribute 'compact' |
| [HTMLUListElement](https://html.spec.whatwg.org/#htmlulistelement)  | interface | HTMLUListElement |  |
|  | attribute | type | This variable just relect attribute 'type' |
|  | attribute | compact | This variable just relect attribute 'compact' |
| [HTMLDListElement](https://html.spec.whatwg.org/#htmldlistelement)  | interface | HTMLDListElement |  |
|  | attribute | compact | This variable just relect attribute 'compact' |
| [HTMLMapElement](https://html.spec.whatwg.org/multipage/image-maps.html#the-map-element) | interface | HTMLMapElement | The map element, in conjunction with an img element and any area element descendants, defines an image map. |
|  | attribute | name | Represents the map element for referencing it other context. |
|  | attribute | areas | Represents the area elements associated to this map. |
| [HTMLMediaElement](https://html.spec.whatwg.org/multipage/embedded-content.html#htmlmediaelement) | interface | HTMLMediaElement | The HTMLMediaElement interface adds to HTMLElement the properties and methods needed to support basic media-related capabilities that are common to audio and video. The HTMLVideoElement and HTMLAudioElement elements both inherit this interface. (Note: Currently, elements related to multimedia are checked on Tizen 2.4 TV Product.)|
|  | enum | CanPlayTypeResult | "", "maybe", "probably" |
|  | attribute | src | Is a DOMString that reflects the src HTML attribute, which contains the URL of a media resource to use. |
|  | attribute | currentSrc | Returns a DOMString with the absolute URL of the chosen media resource. |
|  | attribute | crossOrigin | A DOMString indicating the CORS setting for this media element. |
|  | constant | NETWORK_EMPTY = 0 |  |
|  | constant | NETWORK_IDLE = 1 |  |
|  | constant | NETWORK_LOADING = 2 |  |
|  | constant | NETWORK_NO_SOURCE = 3 |  |
|  | attribute | networkState | Returns a unsigned short (enumeration) indicating the current state of fetching the media over the network. |
|  | attribute | preload | Is a DOMString that reflects the preload HTML attribute, indicating what data should be preloaded, if any. Possible values are: none, metadata, auto. |
|  | attribute | buffered | Returns a TimeRanges object that indicates the ranges of the media source that the browser has buffered (if any) at the moment the buffered property is accessed. |
|  | method |  void load() | Resets the media element and restarts the media resource. Any pending events are discarded. How much media data is fetched is still affected by the preload attribute. This method can be useful for releasing resources after any src attribute and source element descendants have been removed. Otherwise, it is usually unnecessary to use this method, unless required to rescan source element children after dynamic changes. |
|  | method | CanPlayTypeResult canPlayType(DOMString type) | Determines whether the specified media type can be played back. |
|  | constant | HAVE_NOTHING = 0 |  |
|  | constant | HAVE_METADATA = 1 |  |
|  | constant | HAVE_CURRENT_DATA = 2 |  |
|  | constant | HAVE_FUTURE_DATA = 3 |  |
|  | constant | HAVE_ENOUGH_DATA = 4 |  |
|  | attribute | readyState | Returns a unsigned short (enumeration) indicating the readiness state of the media. |
|  | attribute | seeking | Returns true if the media element is currently seeking. |
|  | attribute | currentTime | Is a double indicating the current playback time in seconds. Setting this value seeks the media to the new time. |
|  | attribute | duration | Returns a double indicating the length of the media in seconds, or 0 if no media data is available. |
|  | attribute | paused | Returns a Boolean that indicates whether the media element is paused. |
|  | attribute | played | Returns a TimeRanges object that contains the ranges of the media source that the browser has played, if any. |
|  | attribute | seekable | Returns a TimeRanges object that contains the time ranges that the user is able to seek to, if any. |
|  | attribute | ended | Returns a Boolean that indicates whether the media element has finished playing. |
|  | attribute | autoplay | A Boolean that reflects the autoplay HTML attribute, indicating whether playback should automatically begin as soon as enough media is available to do so without interruption. |
|  | attribute | loop | Is a Boolean that reflects the loop HTML attribute, which indicates whether the media element should start over when it reaches the end. |
|  | method | Promise\<void\> play() | Begins playback of the media. |
|  | method | void pause() | Pauses the media playback. |
|  | attribute | controls | Is a Boolean that reflects the controls HTML attribute, indicating whether user interface items for controlling the resource should be displayed. |
|  | attribute | controlsList | Returns a DOMTokenList that helps the user agent select what controls to show on the media element whenever the user agent shows its own set of controls. |
|  | attribute | volume | Is a double indicating the audio volume, from 0.0 (silent) to 1.0 (loudest). |
|  | attribute | muted | Is a Boolean that determines whether audio is muted. true if the audio is muted and false otherwise. |
|  | attribute | textTracks | Returns the list of TextTrack objects contained in the element. |
|  | method | TextTrack addTextTrack(TextTrackKind kind, optional DOMString label = "", optional DOMString language = "") |  |
| [HTMLModElement](https://html.spec.whatwg.org/#htmlmodelement) | interface | HTMLModElement | The mod element represents edits to the document. |
|  | attribute | cite | Containing a URI of a resource explaining the change. |
|  | attribute | datetime | Containing a date-and-time string representing a timestamp for the change. |
| [HTMLParagraphElement](https://html.spec.whatwg.org/multipage/semantics.html#the-p-element)  | interface | HTMLParagraphElement |  |
| [HTMLParamElement](https://html.spec.whatwg.org/multipage/iframe-embed-object.html#the-param-element) | interface | HTMLParamElement | The param element defines parameters for plugins invoked by object elements. It does not represent anything on its own. |
|  | attribute | name | Represents the name of the parameter. |
|  | attribute | value | Represents the value associated to the parameter. |
|  | attribute | type | Contains the type of the parameter. |
|  | attribute | valueType | Contains the type of the value. |
| [HTMLPreElement](https://html.spec.whatwg.org/multipage/semantics.html#the-pre-element) | interface | HTMLPreElement | The HTMLPreElement interface expose specific properties and methods for manipulating block of preformatted text. |
| [HTMLQuoteElement](https://html.spec.whatwg.org/multipage/grouping-content.html#the-blockquote-element) | interface | HTMLQuoteElement | The blockquote element represents a section that is quoted from another source. |
|  | attribute | cite | Containing a URL for the source of the quotation. |
| [HTMLScriptElement](https://html.spec.whatwg.org/multipage/scripting.html#the-script-element) | interface | HTMLScriptElement | The script element allows authors to include dynamic script and data blocks in their documents. |
|  | attribute | src | Address of the resource.<br>&lt;URL&gt; must be a local path. |
|  | attribute | type | Type of embedded resource.<br>Allowed value: text/javascript |
|  | attribute | noModule | Stops the script's execution in browsers. |
|  | attribute | charset | Character encoding of the external script resource.<br>Allowed value: UTF-8 |
|  | attribute | text | Return the child text content of the script element |
|  | attribute | crossOrigin | A DOMString reflecting the CORS setting for the script element. |
|  | attribute | event | An old, quirky way of registering event handlers on elements in an HTML document. |
|  | attribute | htmlFor | Use DOM events mechanisms to register event listeners. |
| [HTMLSelectElement](https://html.spec.whatwg.org/#the-select-element) | interface | HTMLSelectElement |  The select element represents a control for selecting amongst a set of options. |
|  | attribute | disabled | Returns whether the select element is disabled. |
|  | attribute | form | Returns the element's form owner, or null if there is not one. |
|  | attribute | name | Returns the name of the select element. |
|  | attribute | selectedOptions | Returns an HTMLCollection that contains options that have their selectedness set to true. |
|  | attribute | options | Returns an HTMLOptionsCollection containing all options rooted at this select node |
|  | attribute | selectedIndex | Returns the index of the first selected item, if any, or −1 if there is no selected item. Can be set, to change the selection. |
|  | attribute | value | Returns the value of the first selected item, if any, or the empty string if there is no selected item. Can be set, to change the selection. |
|  | attribute | type | Returns "select-multiple" if the element has a multiple attribute, and "select-one" otherwise. |
|  | attribute | multiple | Whether to allow multiple values |
|  | attribute | required | Whether the control is required for form submission |
|  | attribute | size | Size of the control |
|  | attribute | length | The number of \<option\> elements in this select element. |
|  | method | getter Element? item(unsigned long index) | Gets an item from the options collection for this \<select\> element. |
|  | method | HTMLOptionElement? namedItem(DOMString name) | Gets the item in the options collection with the specified name. |
|  | method | void add((HTMLOptionElement or HTMLOptGroupElement) element, optional (HTMLElement or long)? before = null) | Adds an element to the collection of option elements for this select element. |
|  | method | void remove() | Removes the element from the options collection for this select element. |
|  | method | void remove(long index) | Removes the element at the specified index from the options collection for this select element. |
|  | method | setter void (unsigned long index, HTMLOptionElement? option) | Access an item by specifying the index in array-style brackets or parentheses, without calling this method explicitly. |
|  | attribute | labels | Is a NodeList that represents a list of label elements that are labels for this button. |
| [HTMLOptionElement](https://html.spec.whatwg.org/#htmloptionelement) | interface | HTMLOptionElement | The option element represents an option in a select element or as part of a list of suggestions in a datalist element. |
|  | attribute | defaultSelected | Contains the initial value of the selected HTML attribute, indicating whether the option is selected by default or not. |
|  | attribute | disabled | Returns whether the option element is disabled. |
|  | attribute | form | Returns the element's form owner, or null if there is not one. |
|  | attribute | selected | Returns true if the element's selectedness is true, or false otherwise. |
|  | attribute | value | Returns the value of the option element. |
|  | attribute | text | Same as textContent, except that spaces are collapsed and script elements are skipped. |
|  | attribute | index | The position of the option within the list of options it belongs to, in tree-order. |
| [HTMLOptGroupElement](https://html.spec.whatwg.org/#htmloptgroupelement) | interface | HTMLOptGroupElement | The optgroup element represents a group of option elements with a common label. |
|  | attribute | disabled | Whether the form control is disabled. |
| [HTMLOptionsCollection](https://html.spec.whatwg.org/multipage/common-dom-interfaces.html#dom-htmloptionscollection) | interface | HTMLOptionsCollection | The HTMLOptionsCollection interface is used for collections of option elements. It is always rooted on a select element and has attributes and methods that manipulate that element's descendants. |
|  | attribute | length | Returns the number of elements in the collection. |
|  | attribute | selectedIndex | Returns the index of the first selected item, if any, or −1 if there is no selected item. Can be set, to change the selection. |
|  | method | add | Inserts element before the node given as argument. |
|  | method | remove | Removes the item with index index from the collection. |
| [HTMLSourceElement](https://developer.mozilla.org/en-US/docs/Web/API/HTMLSourceElement) | interface | HTMLSourceElement | The HTMLSourceElement interface provides special properties for manipulating <source> elements. |
|  | attribute | src | DOMString reflecting the src HTML attribute, containing the URL for the media resource. (Note: Current version of HTMLSourceElement considers only media element related case, not picture case.) |
|  | attribute | type | DOMString reflecting the type HTML attribute, containing the type of the media resource. |
| [HTMLSpanElement](https://html.spec.whatwg.org/multipage/semantics.html#the-span-element) | interface | HTMLSpanElement | The span element is a generic inline container for phrasing content. |
| [HTMLStyleElement](https://html.spec.whatwg.org/multipage/semantics.html#the-style-element) | interface | HTMLStyleElement | The style element allows authors to embed style information in their documents. |
|  | attribute | disabled | Represent whether or not the stylesheet is disabled (true) or not (false). |
|  | attribute | media | Applicable media. |
|  | attribute | type | Type of embedded resource.<br>&lt;URL&gt; must be a local path.<br>Allowed value: text/css |
| [HTMLTableElement](https://html.spec.whatwg.org/#the-table-element) | interface | HTMLTableElement | The HTMLTableElement interface provides special properties and methods for manipulating the layout and presentation of tables in an HTML document. |
|    | attribute |  caption  |  Represents the first &lt;caption&gt;.  |
|    | attribute |  tHead  |  Represents the first &lt;thead&gt; that is a child of the element, or null if none is found.  |
|    | attribute |  tFoot  |  Represents the first &lt;tfoot&gt; that is a child of the element, or null if none is found.  |
|    | attribute |  tBodies  |  Contains all the &lt;tbody&gt; of the element.  |
|    | attribute |  rows  |  Returns a live HTMLCollection containing all the rows of the element, that is all &lt;tr&gt; that are a child of the element, or a child or one of its &lt;thead&gt;, &lt;tbody&gt; and &lt;tfoot&gt; children. |
|    | attribute |  align  |  Returns an enumerated value reflecting the align attribute.  |
|    | attribute |  border  |  Contains the width in pixels of the border of the table.  |
|    | attribute |  frame  |  Contains the type of the external borders of the table.  |
|    | attribute |  rules  |  Contains the type of the internal borders of the table.  |
|    | attribute |  summary  |  Contains a description of the purpose or the structure of the table.  |
|    | attribute |  width  |  Contains the length in pixels or in percentage of the desired width fo the entire table.  |
|    | attribute |  bgColor  | Contains the background color of the cells.  |
|    | attribute |  cellPadding  |  Contains the width in pixels of the horizontal and vertical sapce between cell content and cell borders.  |
|    | attribute |  cellSpacing  |  Contains the width in pixels of the horizontal and vertical separation between cells.  |
|    | method |  createCaption  |  Returns an HTMLElement representing the first &lt;caption&gt; that is a child of the element. If none is found, a new one is created and inserted in the tree as the first child of the &lt;table&gt; element.  |
|    | method |  deleteCaption  |  Removes the first &lt;caption&gt; that is a child of the element.  |
|    | method |  createTHead  |  Returns an HTMLElement representing the first &lt;thead&gt; that is a child of the element.  |
|    | method |  deleteTHead  |  Removes the first &lt;thead&gt; that is a child of the element.  |
|    | method |  createTFoot  |  Returns an HTMLElement representing the first &lt;tfoot&gt; that is a child of the element.  |
|    | method |  deleteTFoot  |  Removes the first &lt;tfoot&gt; that is a child of the element.  |
|    | method |  insertRow  |  Returns an HTMLTableRowElement representing a new row of the table.  |
|    | method |  deleteRow  |  Removes the row corresponding to the index given in parameter.  |
| [HTMLTableRowElement](https://html.spec.whatwg.org/#htmltablerowelement) | interface | HTMLTableRowElement | The HTMLTableRowElement interface provides special properties and methods for manipulating the layout and presentation of rows in an HTML table. |
|    | attribute |  rowIndex  |  Returns a long value which gives the logical position of the row within the entire table. If the row is not part of a table, returns -1.  |
|    | attribute |  sectionRowIndex  |  Returns a long value which gives the logical position of the row within the table section it belongs to.  |
|    | attribute |  cells  |  Returns the cells in the row.  |
|    | attribute |  insertCell  |  Inserts a new cell just before the given position in the row.  |
|    | attribute |  deleteCell  |  Removes the cell at the given position in the row.  |
|    | attribute |  bgColor  |  Returns the background color of the cells. |
|    | attribute |  align  |  Returns an enumerated value reflecting the align attribute.  |
| [HTMLTableCaptionElement](https://html.spec.whatwg.org/multipage/tables.html#htmltablecaptionelement) | interface |  HTMLTableCaptionElement | Represents the title of the table that is its parent, if it has a parent and that is a table element. |
|  | attribute |  align | Represents an enumerated attribute indicating alignment of the caption with respect to the table. |
|  [HTMLTableCellElement](https://html.spec.whatwg.org/#htmltablecellelement)  |  attribute  |  colSpan  |  colspan content attribute  |
|    | attribute |  rowSpan  |  rowspan content attribute  |
|    | attribute |  headers  |  Describe a list of id of \<th\> elements that represents headers associated with the cell.  |
|    | attribute |  cellIndex  | Returns the cell's position in the cells collection of the \<tr\> the cell is contained within.  |
|    | attribute |  scope  |  Indicates the scope of a \<th\> cell.  |
|    | attribute |  abbr  |  Speicify an alternative label for the header cell.  |
|    | attribute |  align  |  Returns an enumerated value reflecting the align attribute.  |
|    | attribute |  axis  |  Contains a name grouping cells in virtual. It reflects the obsolete axis attribute.  |
|    | attribute |  height  |  Contains a length of pixel of the hinted height of the cell.  |
|    | attribute |  width  |  Specify the number of pixels wide the cell should be drawn, if possible.  |
|    | attribute |  noWrap |  Reflects the nowrap attribute and indicating if cell content can be broken in several lines.  |
|    | attribute |  vAlign  |  Returns an enumerated value indicating how the content of the cell must be vertically aligned.  |
|    | attribute |  bgColor  |  bgcolor content attributes  |
| [HTMLTableColElement](https://html.spec.whatwg.org/#htmltablecolelement) | interface | HTMLTableColElement |  |
|  | attribute | span | Number of columns spanned by the element. |
|  | attribute | align |  Returns an enumerated value reflecting the align attribute.  |
|  | attribute | vAlign |  Returns an enumerated value indicating how the content of the cell must be vertically aligned.  |
|  | attribute | width |  Returns default column width. |
| [HTMLTableSectionElement](https://html.spec.whatwg.org/#htmltablesectionelement) | interface | HTMLTableSectionElement |  |
|    | attribute |  align  |  Returns an enumerated value reflecting the align attribute.  |
|    | attribute |  rows   |  Returns containing the rows in the section.  |
|    | attribute |  vAlign  |  Returns an enumerated value indicating how the content of the cell must be vertically aligned.  |
|    | method |  insertRow  |  Inserts a new row just before the given position in the section.  |
|    | method |  deleteRow  |  Removes the cell at the given position in the section.  |
| [HTMLTextAreaElement](https://html.spec.whatwg.org/#the-textarea-element) | interface | HTMLTextAreaElement | Provides special properties and methods for manipulating the layout and presentation of textarea elements. |
|  | attribute | autofocus | Returns / Sets the element's autofocus attribute, indicating that the control should have input focus when the page loadsApplicable media. |
|  | attribute | cols | Returns / Sets the element's cols attribute, indicating the visible width of the text area. |
|  | attribute | dirName |  |
|  | attribute | disabled | Returns / Sets the element's disabled attribute, indicating that the control is not available for interaction. |
|  | attribute | form | Returns a reference to the parent form element. |
|  | attribute | maxLength | Returns / Sets the element's maxlength attribute, indicating the maximum number of characters the user can enter. |
|  | attribute | minLength | Returns / Sets the element's minlength attribute, indicating the minimum number of characters the user can enter.  |
|  | attribute | name | Returns / Sets the element's name attribute, containing the name of the control. |
|  | attribute | placeholder | Returns / Sets the element's placeholder attribute, containing a hint to the user about what to enter in the control. |
|  | attribute | readOnly | Returns / Sets the element's readonly attribute, indicating that the user cannot modify the value of the control. |
|  | attribute | required | Returns / Sets the element's required attribute, indicating that the user must specify a value before submitting the form. |
|  | attribute | rows | Returns / Sets the element's rows attribute, indicating the number of visible text lines for the control. |
|  | attribute | type | Returns the string textarea. |
|  | attribute | defaultValue | Returns / Sets the control's default value, which behaves like the Node.textContent property. |
|  | attribute | value | Returns / Sets the raw value contained in the control. |
|  | attribute | textLength | Returns the codepoint length of the control's value. Same as calling value.length |
|  | attribute | labels | Returns a list of label elements associated with this select element. |
| [HTMLTrackElement](https://html.spec.whatwg.org/multipage/embedded-content.html#the-track-element) | interface | HTMLTrackElement | The track element allows authors to specify explicit external timed text tracks for media elements. It does not represent anything on its own. |
|  | attribute | kind | Return value of keywords such as subtitles, captions, descriptions, chapters and metadata. |
|  | attribute | src | Gives the URL of the text track data. |
|  | attribute | srclang | Gives the language of the text track data. |
|  | attribute | label | Gives a user-readable title for the track. |
|  | attribute | default | Indicates that the track is to be enabled if the user's preferences do not indicate that another track would be more appropriate. |
|  | constant | NONE | Indicates that the text track's cues have not been obtained. |
|  | constant | LOADING | Indicates that the text track is loading and there have been no fatal errors encountered so far. Further cues might still be added to the track by the parser. |
|  | constant | LOADED | Indicates that the text track has been loaded with no fatal errors. |
|  | constant | ERROR | Indicates that the text track was enabled, but when the user agent attempted to obtain it, this failed in some way. Some or all of the cues are likely missing and will not be obtained. |
|  | attribute | readyState | Returns the numeric value corresponding to the text track readiness state  |
|  | attribute | track | Returns the TextTrack object corresponding to the text track of the track element. |
| [HTMLVideoElement](https://html.spec.whatwg.org/#htmlvideoelement) | interface | HTMLVideoElement | A video element is used for playing videos or movies, and audio files with captions. (Note: Currently, elements related to multimedia are checked on Tizen 2.4 TV Product.)|
|  | attribute | width | Returns the dimensions of the visual content of the video. |
|  | attribute | height | Returns the dimensions of the visual content of the video. |
|  | attribute | videoWidth | Returns the intrinsic dimensions of the video, or zero if the dimensions are not known. |
|  | attribute | videoHeight | Returns the intrinsic dimensions of the video, or zero if the dimensions are not known. |
| [ImageData](https://html.spec.whatwg.org/multipage/canvas.html#imagedata) | interface | ImageData | The ImageData interface represents the underlying pixel data of an area of a <canvas> element. It is created using the ImageData() constructor or creator methods on the CanvasRenderingContext2D object associated with a canvas: createImageData() and getImageData(). It can also be used to set a part of the canvas by using putImageData(). |
|  | attribute | width | Returns the number of pixels per row in the ImageData object. |
|  | attribute | height | Returns the number of rows in the ImageData object. |
|  | attribute | data | Returns a Uint8ClampedArray that contains the ImageData object's pixel data. Data is stored as a one-dimensional array in the RGBA order, with integer values between 0 and 255 (inclusive). |
| [ImageBitmap](https://html.spec.whatwg.org/multipage/imagebitmap-and-animations.html#imagebitmap) | interface | ImageBitmap | Represents a bitmap image which can be drawn to a <canvas> without undue latency. It can be created from a variety of source objects using the createImageBitmap() factory method |
|  | attribute | width | Returns the ImageBitmap object's width in CSS pixels. |
|  | attribute | height | Returns the ImageBitmap object's height in CSS pixels. |
| [LinkStyle](https://drafts.csswg.org/cssom/#the-linkstyle-interface) | interface | LinkStyle | The associated CSS style sheet of a node is the CSS style sheet in the list of document CSS style sheets of which the owner node implements the LinkStyle interface. |
|  | attribute | sheet | Returns the associated CSS style sheet for the node or null if there is no associated CSS style sheet. |
| [MediaList](https://drafts.csswg.org/cssom/#the-medialist-interface) | interface | MediaList | MediaList interface has an associated collection of media queries. |
|  | attribute | mediaText | Returns a serialization of the collection of media queries. |
|  | attribute | length | Returns the number of media queries in the collection of media queries. |
|  | method | getter CSSOMString? item(unsigned long index) | Returns a serialization of the media query in the collection of media queries given by index, or null, if index is greater than or equal to the number of media queries in the collection of media queries. |
|  | method | void appendMedium(CSSOMString medium) | Adds a media type to the mediaList collection. |
|  | method | void deleteMedium(CSSOMString medium) | Removes a media type from the mediaList collection. |
| [MediaQueryList](https://drafts.csswg.org/cssom-view/#mediaquerylist) | interface | MediaQueryList | A MediaQueryList object stores information on a media query applied to a document, and handles sending notifications to listeners when the media query state change (i.e. when the media query test starts or stops evaluating to true). |
|  | attribute | media | Returns the associated media. |
|  | attribute | matches | Returns the associated matches state. |
|  | methods | addListener | Adds a listener to associated list of event listeners that will run a custom callback function in response to the media query status changing. |
|  | methods | removeListener | Removes a listener from the associated list of event listeners. |
|  | attribute | onchange | An event handler property representing a function that is invoked when the change event fires. |
| [MessageChannel](https://www.w3.org/TR/webmessaging/#messagechannel) | interface | MessageChannel | To enable independent pieces of code (e.g. running in different browsing contexts) to communicate directly, authors can use channel messaging. |
|  | attribute | port1 | Returns the first MessagePort object. |
|  | attribute | port2 | Returns the second MessagePort object. |
| [MessagePort](https://html.spec.whatwg.org/multipage/web-messaging.html#messageport) | interface | MessagePort | Each channel has two message ports. Data sent through one port is received by the other port, and vice versa. |
|  | method | postMessage | Posts a message through the channel. |
|  | method | start | Begins dispatching messages received on the port. |
|  | method | close | Disconnects the port, so that it is no longer active. |
|  | attribute | onmessage | Fired at an object when it receives a message. |
|  | attribute | onmessageerror | Fired at an object when it receives a message that cannot be deserialized. |
| [NamedNodeMap](https://dom.spec.whatwg.org/#interface-namednodemap) | interface | NamedNodeMap |  |
|  | attribute | length | Return the attribute list’s size. |
|  | method | Attr? item(unsigned long index) | Return the attribute at the given index, or null if the index is higher or equal to the number of nodes. |
|  | method | Attr? getNamedItem(DOMString qualifiedName) | Return the result of getting an attribute given qualifiedName and element. |
|  | method | Attr? getNamedItemNS(DOMString? namespace, DOMString localName) | Retrieves a node specified by local name and namespace URI. |
|  | method | Attr? setNamedItem(Attr attr) | Return the result of setting an attribute given attr and element. |
|  | method | Attr? setNamedItemNS(Attr attr) | Return the result of setting an attribute given attr and element. |
|  | method | Attr removeNamedItem(DOMString qualifiedName) | Remove the attribute identified by the given map. |
|  | method | Attr removeNamedItemNS(DOMString? namespace, DOMString localName) | Removes a node specified by local name and namespace URI. |
| [Node](https://dom.spec.whatwg.org/#interface-node) | interface | Node | Node is an abstract interface and does not exist as node. It is used by all nodes (Document, DocumentType, DocumentFragment, Element, Text, ProcessingInstruction, and Comment). |
|  | constant | ELEMENT_NODE | Node is an element. |
|  | constant | ATTRIBUTE_NODE | Node is an attribute |
|  | constant | TEXT_NODE | Node is a Text node. |
|  | constant | CDATA_SECTION_NODE | Node is a CDATASection node. |
|  | constant | ENTITY_REFERENCE_NODE | Node is an entry preference node |
|  | constant | ENTITY_NODE | Node is an entry node |
|  | constant | PROCESSING_INSTRUCTION_NODE | Node is a ProcessingInstruction node. |
|  | constant | COMMENT_NODE | Node is a Comment node. |
|  | constant | DOCUMENT_NODE | Node is a document. |
|  | constant | DOCUMENT_TYPE_NODE | Node is a doctype. |
|  | constant | DOCUMENT_FRAGMENT_NODE | Node is a DocumentFragment node. |
|  | constant | NOTATION_NODE | Node is a notation node |
|  | attribute | nodeType | Returns the node type. |
|  | attribute | nodeName | Returns the node name. |
|  | attribute | baseURI | Returns the document base URL (resolved against `<base href>` if present). |
|  | attribute | isConnected | Returns true if the node is in the document tree. |
|  | attribute | ownerDocument | Returns the node document. Returns null for documents. |
|  | method | Node getRootNode(optional GetRootNodeOptions options) | Returns the context object's root. With `{composed: true}` the shadow-including root is returned; otherwise the walk stops at the containing `ShadowRoot`. |
|  | attribute | parentNode | Returns the parent. |
|  | attribute | parentElement | Returns the parent element, or null if the parent is not an Element. |
|  | method | boolean hasChildNodes() | Returns whether node has children. |
|  | attribute | childNodes | Returns a live `NodeList` of the children. |
|  | attribute | firstChild | Returns the first child. |
|  | attribute | lastChild | Returns the last child. |
|  | attribute | previousSibling | Returns the previous sibling. |
|  | attribute | nextSibling | Returns the next sibling. |
|  | attribute | nodeValue | Gets/sets the value for `Attr`/`Text`/`ProcessingInstruction`/`Comment`; null for other node types. |
|  | attribute | textContent | Gets/sets the textual content of `DocumentFragment`/`Element`/`Attr`/`Text`/`ProcessingInstruction`/`Comment`; null for `Document`/`DocumentType`. |
|  | method | void normalize() | Removes empty exclusive `Text` nodes and concatenates contiguous text. |
|  | method | Node cloneNode(optional boolean deep = false) | Returns a copy of node. If `deep` is true, the copy also includes the node's descendants. |
|  | method | boolean isEqualNode(Node? otherNode) | Returns whether node and otherNode have equal properties. |
|  | method | boolean isSameNode(Node? otherNode) | Historical alias for `===` reference equality. |
|  | constant | DOCUMENT_POSITION_DISCONNECTED = 0x01; | Set when node and other are not in the same tree. |
|  | constant | DOCUMENT_POSITION_PRECEDING = 0x02; | Set when other is preceding node. |
|  | constant | DOCUMENT_POSITION_FOLLOWING = 0x04; | Set when other is following node. |
|  | constant | DOCUMENT_POSITION_CONTAINS = 0x08; | Set when other is an ancestor of node. |
|  | constant | DOCUMENT_POSITION_CONTAINED_BY = 0x10; | Set when other is a descendant of node. |
|  | constant | DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC = 0x20; |  |
|  | method | unsigned short compareDocumentPosition(Node other) | Returns a bitmask indicating the position of other relative to node.  |
|  | method | boolean contains(Node? other) | Returns true if other is an inclusive descendant of context object, and false otherwise |
|  | method | Node insertBefore(Node node, Node? child) | Returns the result of pre-inserting node into context object before child. |
|  | method | Node appendChild(Node node) | Returns the result of appending node to context object. |
|  | method | Node replaceChild(Node node, Node child) | Returns the result of replacing child with node within context object. |
|  | method | Node removeChild(Node child) | Returns the result of pre-removing child from context object. |
|  | method | DOMString? lookupPrefix(DOMString? namespace) | Returns the prefix associated with the given namespace, or null. |
|  | method | DOMString? lookupNamespaceURI(DOMString? prefix) | Returns the namespace URI associated with the given prefix (HTML elements default to `http://www.w3.org/1999/xhtml`). |
|  | method | boolean isDefaultNamespace(DOMString? namespace) | Returns whether the given namespace is the default namespace at the context node. |
| [GetRootNodeOptions](https://dom.spec.whatwg.org/#dictdef-getrootnodeoptions) | dictionary | GetRootNodeOptions | `{ boolean composed = false }`. With `composed: true`, `getRootNode()` crosses shadow boundaries and returns the shadow-including root. |
| [NodeList](https://dom.spec.whatwg.org/#nodelist) | interface | NodeList | A `NodeList` object is a collection of nodes. Live for `Node.childNodes`; static for `querySelectorAll`. |
|  | attribute | length | Returns the number of nodes in the collection. |
|  | method | Node? item(unsigned long index) | Returns the node at the given tree-ordered index, or null. Indexed access (`list[i]`) is equivalent. |
|  | iterable | iterable&lt;Node&gt; | Supports `for..of`, `forEach`, `entries`, `keys`, `values`. |
| [NonDocumentTypeChildNode](https://dom.spec.whatwg.org/#nondocumenttypechildnode) | interface | NonDocumentTypeChildNode | The NonDocumentTypeChildNode interface contains methods that are particular to Node Object that can have a sibling. |
|  | attribute | previousElementSibling | Returns the Element immediately prior to this node in its parent's children list, or null if there is no Element in the list prior to this node. |
|  | attribute | nextElementSibling | Returns the Element immediately following this node in its parent's children list, or null if there is no Element in the list following this node. |
| [NonElementParentNode](https://www.w3.org/TR/dom/#interface-nonelementparentnode) | interface | NonElementParentNode |  |
|  | method | Element? getElementById(DOMString elementId) | Returns the first element within node's descendants whose ID is elementId. |
| ParentNode | interface | ParentNode | The ParentNode interface contains methods that are particular to Node objects that can have children. |
|  | attribute | firstElementChild | Returns the Element that is the first child of this ParentNode, or null if there is none. |
|  | attribute | lastElementChild | Returns the Element that is the last child of this ParentNode, or null if there is none. |
|  | attribute | childElementCount | Returns an unsigned long giving the amount of children that the object has. |
|  | method | void prepend((Node or DOMString)... nodes) | Inserts nodes before the first child of node, while replacing strings in nodes with equivalent Text nodes. |
|  | method | void append((Node or DOMString)... nodes) | Inserts nodes after the last child of node, while replacing strings in nodes with equivalent Text nodes. |
|  | method | Element? querySelector(DOMString selectors) | Returns the first Element with the current element as root that matches the specified group of selectors. |
|  | method | NodeList querySelectorAll(DOMString selectors) | Returns a NodeList representing a list of elements with the current element as root that matches the specified group of selectors. |
|  | misc | replaceChildren | **Not implemented** in LWE. Calling `el.replaceChildren(...)` raises `TypeError`. Use `el.innerHTML = ''` followed by `append(...)` instead. |
| [Slottable](https://dom.spec.whatwg.org/#slotable) | interface | Slottable | Mixin implemented by `Element` and `Text`. |
|  | attribute | assignedSlot | Returns the assigned `<slot>` element, or `null`. |
| [ElementAnimation](https://www.w3.org/TR/web-animations-1/#extensions-to-the-element-interface) | interface | ElementAnimation | Mixin on `Element`. |
|  | method | Animation animate(sequence&lt;any&gt;? keyframes, optional KeyframeAnimationOptions options) | Creates and starts an animation; returns an `Animation` object. |
|  | method | sequence&lt;Animation&gt; getAnimations() | **Not implemented** (returns `undefined`). |
| [Animation](https://www.w3.org/TR/web-animations-1/#the-animation-interface) | interface | Animation | Returned by `Element.animate()`. An `EventTarget`; not constructible. `KeyframeEffect`, `AnimationEffect`, `AnimationTimeline` and `document.timeline` are not exposed. |
|  | method | void cancel() | Drops the animation and any value it was filling, then fires `cancel`. |
|  | misc | `finish` / `cancel` / `remove` events | Dispatched on the `Animation`; reachable through `addEventListener()` only, as `onfinish`/`oncancel`/`onremove` are unimplemented. |
|  | misc | [Replacing animations](https://www.w3.org/TR/web-animations-1/#replacing-animations) | A finished script animation whose filled properties are all animated by a later `Element.animate()` on the same element is removed (fires `remove`). The check runs when `Element.animate()` starts a new animation. |
|  | misc | **Unsupported in LWE** (IDL `[Unimplemented]` — return `undefined`) | `id`, `effect`, `timeline`, `startTime`, `currentTime`, `playbackRate`, `playState`, `pending`, `ready`, `finished`, `onfinish`, `oncancel`, `finish()`, `play()`, `pause()`, `updatePlaybackRate()`, `reverse()`. |
| [Range](https://dom.spec.whatwg.org/#interface-range) | interface | Range | Represents a contiguous range of content. Constructor: `new Range()` (range starts collapsed at `(document, 0)`). Also returned by `document.createRange()`. |
|  | attribute | startContainer / startOffset / endContainer / endOffset | Boundary points of the range. |
|  | attribute | collapsed | True iff start === end. |
|  | attribute | commonAncestorContainer | Deepest node containing both endpoints. |
|  | constant | START_TO_START / START_TO_END / END_TO_END / END_TO_START | Selectors for `compareBoundaryPoints` (0/1/2/3). |
|  | method | setStart / setEnd / setStartBefore / setStartAfter / setEndBefore / setEndAfter / collapse / selectNode / selectNodeContents / compareBoundaryPoints / isPointInRange / comparePoint / intersectsNode / deleteContents / extractContents / insertNode / surroundContents / cloneRange / detach | Standard `Range` operations — all implemented. |
|  | method | DOMRectList getClientRects() / DOMRect getBoundingClientRect() | (CSSOM-View) Per-fragment client rects. |
|  | stringifier |  | Returns the textual content of the range. |
|  | misc | **Not implemented** | `cloneContents`, `createContextualFragment`, `expand` are `[Unimplemented]` and raise `TypeError` on call. |
| [NodeIterator](https://dom.spec.whatwg.org/#interface-nodeiterator) | interface | NodeIterator | **Use `TreeWalker` instead.** The interface and `createNodeIterator` factory are exposed and configuration attributes (`root`, `referenceNode`, `pointerBeforeReferenceNode`, `whatToShow`, `filter`) report correct values, but `nextNode()` returns null immediately on the first call. Iteration is not actually wired up. |
| [TreeWalker](https://dom.spec.whatwg.org/#interface-treewalker) | interface | TreeWalker | Created via `document.createTreeWalker(root, whatToShow=SHOW_ALL, filter=null)`. Fully functional — supports custom `acceptNode` filter callbacks (callable or `{acceptNode}` object). |
|  | attribute | root / whatToShow / filter / currentNode | Configuration; `currentNode` is writable. |
|  | method | parentNode / firstChild / lastChild / previousSibling / nextSibling / previousNode / nextNode | Standard traversal that respects `whatToShow` and `filter`. |
| [NodeFilter](https://dom.spec.whatwg.org/#interface-nodefilter) | callback interface | NodeFilter | Pass either a function `(node)=>FILTER_*` or an object `{acceptNode(node){…}}` to `TreeWalker`. |
|  | constant | FILTER_ACCEPT (1) / FILTER_REJECT (2) / FILTER_SKIP (3) |  |
|  | constant | SHOW_ALL (0xFFFFFFFF) / SHOW_ELEMENT (0x1) / SHOW_TEXT (0x4) / SHOW_COMMENT (0x80) / SHOW_PROCESSING_INSTRUCTION (0x40) / SHOW_DOCUMENT (0x100) / SHOW_DOCUMENT_TYPE (0x200) / SHOW_DOCUMENT_FRAGMENT (0x400) | |
|  | constant | SHOW_ATTRIBUTE / SHOW_CDATA_SECTION / SHOW_ENTITY_REFERENCE / SHOW_ENTITY / SHOW_NOTATION | Historical — present for spec parity, no nodes of these types are produced by HTML parsing. |
| [Text](https://dom.spec.whatwg.org/#text) | interface | Text | Text node whose data is data and node document is current global object's associated Document. |
|  | method | Text splitText(unsigned long offset) | Breaks the node into two nodes at a specified offset. |
|  | attribute | wholeText | Returns the combined data of all direct Text node siblings. |
| [TextTrack](https://html.spec.whatwg.org/#texttrack)  | interface | TextTrack |  |
|  | enum | TextTrackMode | "disabled",  "hidden",  "showing" |
|  | enum | TextTrackKind | "subtitles",  "captions",  "descriptions",  "chapters",  "metadata" |
|  | attribute  | kind | Returns the text track kind string. |
|  | attribute  | label | Returns the text track label, if there is one, or the empty string otherwise  |
|  | attribute  | language | Returns the text track language string. |
|  | attribute  | id | Returns the ID of the given track. |
|  | attribute  | mode | Gets and sets the text track mode |
|  | attribute  | cues | Returns the text track list of cues, as a TextTrackCueList object. |
|  | attribute  | activeCues | Returns a live TextTrackCueList object  |
|  | method | void addCue(TextTrackCue cue) | Adds cue to the method's TextTrack object's text track's text track list of cues. |
|  | method | void removeCue(TextTrackCue cue) | Removes cue from the method's TextTrack object's text track's text track list of cues. |
|  | attribute  | oncuechange | The event handler for the cue change event  |
| [TextTrackCue](https://html.spec.whatwg.org/#texttrackcue) | interface | TextTrackCue | A text track cue is the unit of time-sensitive data in a text track, corresponding for instance for subtitles and captions to the text that appears at a particular time and disappears at another time. |
|  | attribute | track | Returns the TextTrack object to which this text track cue belongs, if any, or null otherwise. |
|  | attribute | id | Gets and sets the text track cue identifier. |
|  | attribute | startTime | Gets and setsthe text track cue start time, in seconds. |
|  | attribute | endTime | Gets and sets the text track cue end time, in seconds. |
|  | attribute | onenter | The event handler for the enter event  |
|  | attribute | onexit | The event handler for the exit event  |
|  [TextTrackCueList](https://html.spec.whatwg.org/#texttrackcuelist)  |  attribute  |  length |  Return the number of cues in the list represented by the TextTrackCueList object  |
|    |  method  |  TextTrackCue[unsigned long index]  |  Return Text track cue object with index  |
| [TextTrackList](https://html.spec.whatwg.org/#texttracklist) | interface | TextTrackList | A TextTrackList object represents a dynamically updating list of text tracks in a given order. |
|  | attribute | length | Returns the number of text tracks associated with the media element. |
|  | method | TextTrack (unsigned long index) | Returns the TextTrack object representing the nth text track in the media element's list of text tracks. |
|  | method | TextTrack? getTrackById(DOMString id) | Returns the TextTrack object with the given identifier, or null if no track has that identifier. |
| [VTTCue](https://w3c.github.io/webvtt/#vttcue) | interface | VTTCue | VTTCues represent a cue in a text track. |
| | constructor | VTTCue(double startTime, double endTime, DOMString text) | Create a new VTTCue |
|  | enum | AutoKeyword | "auto" |
|  | typedef | (double or AutoKeyword) LineAndPositionSetting |  |
|  | enum | DirectionSetting | "", "rl", "lr" |
|  | enum | LineAlignSetting | "start", "center", "end" |
|  | enum | PositionAlignSetting | "line-left", "center", "line-right", "auto" |
|  | enum | AlignSetting | "start", "center", "end", "left", "right" |
|  | attribute | text | Return the raw text track cue text of the WebVTT cue that the VTTCue object represents. On setting, the text track cue text must be set to the new value. |
|  | method | DocumentFragment getCueAsHTML() | Convert the text track cue text to a DocumentFragment for the responsible document specified by the entry settings object by applying the WebVTT cue text DOM construction rules to the result of applying the WebVTT cue text parsing rules to the text track cue text. |
| [XMLDocument](https://www.w3.org/TR/dom/#interface-document) | interface | XMLDocument | The XMLDocument interface represent an XML document. |
| [History](https://html.spec.whatwg.org/multipage/browsers.html#the-history-interface) | interface | History | The History interface allows to manipulate the browser session history, that is the pages visited in the tab or frame that the current page is loaded in. |
|  | attribute | length | Returns an Integer representing the number of elements in the session history, including the currently loaded page. For example, for a page loaded in a new tab this property returns 1. |
|  | attribute | state | Returns an any value representing the state at the top of the history stack. This is a way to look at the state without having to wait for a popstate event. |
|  | method | void go(optional long delta = 0) | Loads a page from the session history, identified by its relative location to the current page, for example -1 for the previous page or 1  for the next page. |
|  | method | void back() | Goes to the previous page in session history, the same action as when the user clicks the browser's Back button. Equivalent to history.go(-1). |
|  | method | void forward() | Goes to the next page in session history, the same action as when the user clicks the browser's Forward button; this is equivalent to history.go(1). |
|  | method | void pushState(any data, DOMString title, optional DOMString? url = null) | Pushes the given data onto the session history stack with the specified title and, if provided, URL. |
|  | method | void replaceState(any data, DOMString title, optional DOMString? url = null) | Updates the most recent entry on the history stack to have the specified data, title, and, if provided, URL |
|  | misc | Not implemented | `scrollRestoration` is in `History.idl` with `[Unimplemented]` and returns `undefined`. |
|  [Location](https://html.spec.whatwg.org/multipage/browsers.html#location) | interface | Location | Represents the location (URL) of the object it is linked to. |
|    |  misc  |  Not implemented  |  `ancestorOrigins` is in `Location.idl` with `[Unimplemented]` and returns `undefined`. |
|    |  attribute  |  protocol  |  Return  Location object's url's scheme, followed by ":".  |
|    |  attribute  |  href  |  Return this Location object's url, serialized.  |
|    |  attribute  |  origin  | Return the serialization of this Location object's url's origin.  |
|    |  attribute  |  host  |  Return url's host, serialized, followed by ":" and url's port, serialized.  |
|    |  attribute  |  hostname  | Return this Location object's url's host, serialized.  |
|    |  attribute  |  port  | Return this Location object's url's port. Can be set, to navigate to the same URL with a changed port.  |
|    |  attribute  |  pathname  |  Return "/", followed by the strings in url's path (including empty strings), separated from each other by "/".  |
|    |  attribute  |  search  |  Return "?", followed by this Location object's url's query.  |
|    |  attribute  |  hash  |  Return "#", followed by this Location object's url's fragment.  |
|    |  method  |  assign(DOMString url)  |  Loads the resource at the URL provided in parameter.  |
|    |  method  |  replace(DOMString url)  |  Replaces the current resource with the URL provided in parameter.  |
|    |  method  |  reload()  |  Reloads the resource from the current URL.  |
| [MediaSource](https://w3c.github.io/media-source/#mediasource) | interface | MediaSource |The MediaSource object represents a source of media data for an HTMLMediaElement. |
|  | enum | ReadyState | "closed", "open", "ended" |
|  | enum | EndOfStreamError  | "network", "decode" |
|  | attribute | sourceBuffers | Contains the list of SourceBuffer objects associated with this MediaSource. |
|  | attribute | activeSourceBuffers | Contains the subset of sourceBuffers that are providing the selected video track, the enabled audio track(s), and the "showing" or "hidden" text track(s). |
|  | attribute | readyState | Indicates the current state of the MediaSource object. |
|  | attribute | duration | Allows the web application to set the presentation duration. |
|  | method | void endOfStream(optional EndOfStreamError error) | Signals the end of the stream. |
|  [SourceBuffer](https://w3c.github.io/media-source/#sourcebuffer)  | interface | SourceBuffer | Represents a chunk of media to be passed into an HTMLMediaElement and played, via a MediaSource object. |
|    |  enum  |  AppendMode  |  "segments", "sequence"  |
|    |  attribute  |  mode  |  Controls how a sequence of media segments are handled  |
|    |  attribute  |  updating  |  Return whether the asynchronous continuation of an appendBuffer() or remove() operation is still being processed  |
|    |  attribute  |  buffered  |  Return what TimeRanges are buffered in the SourceBuffer  |
|    |  attribute  |  textTracks  |  Return The list of TextTrack objects created by this object  |
|    |  attribute  |  appendWindowStart  |  The presentation timestamp for the start of the append window  |
|    |  attribute  |  appendWindowEnd  |  The presentation timestamp for the end of the append window  |
|    |  attribute  |  onupdatestart  |  The event handler for the updatestart event  |
|    |  attribute  |  onupdate  |  The event handler for the update event  |
|    |  attribute  |  onupdateend  |  The event handler for the updateend event  |
|    |  attribute  |  onerror  |  The event handler for the error event  |
|    |  attribute  |  onabort  |  The event handler for the abort event  |
|    |  method  |  void appendBuffer(BufferSource data)  |  Appends the segment data in an BufferSource to the source buffer |
|    |  method  |  void remove(double start, unrestricted double end)  |  Removes media for a specific time range  |
| [SourceBufferList](https://w3c.github.io/media-source/#sourcebufferlist) | interface | SourceBufferList | Represents a simple container list for multiple SourceBuffer objects. |
|    | attribute | length |  Return number of SourceBuffer objects in the list.  |
|    |  method  |  SourceBuffer[unsigned long index]  |  Return SourceBuffer object with index  |
| [StyleSheet](https://drafts.csswg.org/cssom/#the-stylesheet-interface) | interface | StyleSheet | The StyleSheet interface represents an abstract, base style sheet. |
| | attribute | type | Specifies the style sheet language for this style sheet. |
| | attribute | href | If the style sheet is a linked style sheet, the value of its attribute is its location. |
| | attribute | parentStyleSheet | Returns the parent CSS style sheet. |
| | attribute | title | Returns the advisory title of the current style sheet. |
| | attribute | media | Returns the MediaList object that is associated with the CSS style sheet. |
| | attribute | disabled | Indicates whether the style sheet is prevented from applying to the document. A style sheet may be disabled by manually setting this property to true. |
| | attribute | ownerNode | Returns a Node associating this style sheet with the current document. |
| [StyleSheetList](https://drafts.csswg.org/cssom/#the-stylesheetlist-interface) | interface | StyleSheetList | The StyleSheetList interface represents an ordered collection of CSS style sheets. |
| | method | getter StyleSheet? item(unsigned long index) | Return the indexth CSS style sheet in the collection. |
| | attribute | length | Return the number of CSS style sheets represented by the collection. |
| [TimeRanges](https://html.spec.whatwg.org/multipage/embedded-content.html#time-ranges) | interface | TimeRanges | The TimeRanges interface represent a list of ranges (periods) of time. |
|  | attribute | length | Returns the number of ranges in the object. |
|  | method | double start(unsigned long index) | Returns the time for the start of the range with the given index. |
|  | method | double end(unsigned long index) | Returns the time for the end of the range with the given index. |
| [Touch](https://w3c.github.io/touch-events/#idl-def-touch) | interface | Touch | Describes an individual touch point for a touch event. |
| | attribute | target | The EventTarget on which the touch point started when it was first placed on the surface. |
| | attribute | screenX | The horizontal coordinate of point relative to the screen in pixels. |
| | attribute | screenY | The vertical coordinate of point relative to the screen in pixels. |
| | attribute | clientX | The horizontal coordinate of point relative to the viewport in pixels, excluding any scroll offset. |
| | attribute | clientY | The vertical coordinate of point relative to the viewport in pixels, excluding any scroll offset. |
| [TouchInit](https://w3c.github.io/touch-events/#idl-def-touchinit) | dictionary | TouchInit | Dictionary that is used to create TouchInit. |
| | attribute | target | Initializes the target attribute of the Touch object |
| | attribute | screenX | Initializes the screenX attribute of the Touch object |
| | attribute | screenY | Initializes the screenY attribute of the Touch object |
| | attribute | clientX | Initializes the clientX attribute of the Touch object |
| | attribute | clientY | Initializes the clientY attribute of the Touch object |
| [TouchList](https://w3c.github.io/touch-events/#idl-def-touchlist) | interface | TouchList | Defines a list of individual points of contact for a touch event. |
| | attribute | length | Returns the number of Touch objects in the list |
| [Window](https://html.spec.whatwg.org/#the-window-object) | interface | Window | The Window has an associated Document, which is a Document object. |
|  | attribute | window | Returns window. |
|  | attribute | top | Returns window for the top-level browsing context. |
|  | attribute | parent | Returns parent window. |
|  | attribute | frameElement | Returns the Element for the browsing context container. Returns null if there isn’t one, and in cross-origin situations. |
|  | attribute | document | Returns the document associated with window. |
|  | attribute | name | Gets/sets the name of the window. |
|  | attribute | location | Return this Window object's Location object. |
|  | attribute | history | Return the object implementing the History interface for this Window object's associated Document. |
|  | attribute | navigator | Return an instance of the Navigator interface, which represents the identity and state of the user agent (the client), and allows Web pages to register themselves as potential protocol and content handlers |
|  | attribute | frames | Return Window object's browsing context's WindowProxy object. |
|  | attribute | length | Return the number of document-tree child browsing contexts of this Window object. |
|  | attribute | self | Returns window. (Per HTML spec equivalent to `window` and `frames`.) |
|  | attribute | customElements | Returns the [CustomElementRegistry](https://html.spec.whatwg.org/multipage/custom-elements.html#customelementregistry) for this Window. `define()`, `get()`, `getName()`, `whenDefined()`, and `upgrade()` are implemented, including `[CEReactions]` and the `extends` definition option. |
|  | method | void alert(optional DOMString message = "") | Displays a modal dialog with the given message. LWE prints the message via TTS instead of opening a real dialog. |
|  | method | void focus() / void blur() | Callable but a no-op (logs `Unsupported Window function: focus/blur`). |
|  | method | postMessage(message, targetOrigin, transfer) | Posts a message to the given window. |
|  | misc | **Unsupported in LWE** (`[Unimplemented]` in `Window.idl`) | `close`, `closed`, `stop`, `open(url, target, features)`, `opener`, `confirm`, `prompt`, `print`, `status`, `applicationCache`, `external`, `locationbar`/`menubar`/`personalbar`/`scrollbars`/`statusbar`/`toolbar` (BarProp), `captureEvents`/`releaseEvents`, `moveTo`/`moveBy`/`resizeTo`/`resizeBy`, `outerWidth`/`outerHeight`, `screenX`/`screenY`. `getSelection`, `requestIdleCallback`/`cancelIdleCallback`, `screenLeft`/`screenTop` are not in IDL at all. |
| [Window](https://www.w3.org/TR/cssom-view-1/#extensions-to-the-window-interface) | enum | ScrollBehavior | "auto", "instant", "smooth" |
|  | attribute | innerWidth | Return the viewport width including the size of a rendered scroll bar (if any), or zero if there is no viewport.  |
|  | attribute | innerHeight | Return the viewport height including the size of a rendered scroll bar (if any), or zero if there is no viewport. |
|  | attribute | scrollX | property of the Window interface returns the number of pixels that the document is currently scrolled horizontally |
|  | attribute | scrollY | property of the Window interface returns the number of pixels that the document is currently scrolled vertically |
|  | attribute | pageXOffset | property of the Window interface returns the number of pixels that the document is currently scrolled horizontally |
|  | attribute | pageYOffset | property of the Window interface returns the number of pixels that the document is currently scrolled vertically |
|  | method | scroll(optional ScrollToOptions) | Scrolls the window to a particular place in the document. |
|  | method | scroll(x, y) | Scrolls the window to a particular place in the document. |
|  | method | scrollTo(optional ScrollToOptions) | Scrolls the window to a particular place in the document. |
|  | method | scrollTo(x, y) | Scrolls the window to a particular place in the document. |
|  | method | scrollBy(optional ScrollToOptions) | Scrolls the window by the given delta. |
|  | method | scrollBy(x, y) | Scrolls the window by the given delta. |
| [Window](https://www.w3.org/TR/animation-timing/#Window-interface-extensions) | method | unsigned long requestAnimationFrame(FrameRequestCallback callback) | Used to signal to the user agent that a script-based animation needs to be resampled. |
| | method | void cancelAnimationFrame(unsigned long handle) | Used to cancel a previously made request to schedule an animation frame update. |
| | callback | FrameRequestCallback | `void (DOMHighResTimeStamp time)` — invoked once before the next repaint with the current high-resolution timestamp. |
| [Window](https://drafts.csswg.org/cssom/#extensions-to-the-window-interface) | method | CSSStyleDeclaration getComputedStyle(Element elt, optional CSSOMString? pseudoElt) | Gives the values of all the CSS properties of an element after applying the active stylesheets and resolving any basic computation those values may contain. |
| [Window](https://drafts.csswg.org/cssom-view/#extensions-to-the-window-interface) | method | MediaQueryList matchMedia(CSSOMString query) | Returns a new MediaQueryList object representing the parsed results of the specified media query string. |
| | attribute | screen | Returns a reference to the screen object associated with the window. |
| | attribute | devicePixelRatio | Returns the ratio between physical pixels and device independent pixels in the current display. |
| [WindowOrWorkerGlobalScope](https://html.spec.whatwg.org/multipage/webappapis.html#windoworworkerglobalscope) | interface mixin | WindowOrWorkerGlobalScope | Mixin shared between `Window` and worker globals. `[NoInterfaceObject]`. |
|  | method | DOMString btoa(DOMString data) | Returns the base64 encoding of `data`. |
|  | method | DOMString atob(DOMString data) | Decodes a base64 string. Throws `InvalidCharacterError` on invalid input. |
|  | method | undefined queueMicrotask(VoidFunction callback) | Queues `callback` to run as a microtask. |
|  | method | any structuredClone(any value, optional StructuredSerializeOptions options) | Deep-clones `value` using the structured-clone algorithm. |
|  | misc | `origin` is in IDL with `[Unimplemented]` and returns `undefined`. Timers (`setTimeout`/`setInterval`/`clearTimeout`/`clearInterval`) are listed under `WindowTimers` and reach through this mixin. |  |
|  | method | Promise<ImageBitmap> createImageBitmap(ImageBitmapSource image, optional ImageBitmapOptions options) | Creates a bitmap from a given source, optionally cropped to contain only a portion of that source |
|  | method | Promise<ImageBitmap> createImageBitmap(ImageBitmapSource image, long sx, long sy, long sw, long sh, optional ImageBitmapOptions options) | Creates a bitmap from a given source, optionally cropped to contain only a portion of that source |
| [Named Access on the Window Object](https://html.spec.whatwg.org/multipage/browsers.html#named-access-on-the-window-object) | misc | window[id] | Named access on the Window object returns the indicated element, where id is a non-empty ID of an HTML element in the current document. |
| [Screen](https://drafts.csswg.org/cssom-view/#the-screen-interface) | interface | Screen | Returned by `window.screen`. |
|  | attribute | width / height | Output device dimensions in CSS pixels. |
|  | attribute | availWidth / availHeight | Available output dimensions. On LWE these equal `width`/`height`. |
|  | attribute | colorDepth / pixelDepth | Color/pixel depth in bits. LWE returns 24. |
|  | misc | Not exposed: `orientation`, `availLeft`, `availTop`, `onchange`. The Screen Orientation API is unavailable. | |
|  [ScrollOptions](https://www.w3.org/TR/cssom-view-1/#dictdef-scrolloptions) | dictionary | ScrollOptions |  |
|    | attribute | behavior | Initializes the behavior attribute of the ScrollOptions object |
|  [ScrollToOptions](https://www.w3.org/TR/cssom-view-1/#dictdef-scrolltooptions) | dictionary | ScrollToOptions |  |
|    | attribute | left | Initializes the left attribute of the ScrollToOptions object |
|    | attribute | top | Initializes the top attribute of the ScrollToOptions object |
| [URL](https://url.spec.whatwg.org/#url) | interface | URL | The URLinterface represent an object providing static methods used for creating object URLs. |
| | constructor | URL(DOMString url, optional DOMString base) | Create a new URL |
|  | attribute | href | A DOMString containing the whole URL. |
|  | attribute | origin | A DOMString containing the origin of the URL, that is its scheme, its domain and its port. |
|  | attribute | protocol | A DOMString containing the protocol scheme of the URL, including the final ':'. |
|  | attribute | username | A DOMString containing the username specified before the domain name. |
|  | attribute | password | A DOMString containing the password specified before the domain name. |
|  | attribute | host | A DOMString containing the host, that is the hostname, a ':', and the port of the URL. |
|  | attribute | hostname | A DOMString containing the domain of the URL. |
|  | attribute | port | A DOMString containing the port number of the URL. |
|  | attribute | pathname | A DOMString containing an initial '/' followed by the path of the URL. |
|  | attribute | search | A DOMString containing a '?' followed by the parameters of the URL. |
|  | attribute | hash | A DOMString containing a '#' followed by the fragment identifier of the URL. |
|  | method | static DOMString createObjectURL(Blob blob) | Returns a DOMString containing a unique blob URL, that is a URL with blob: as its scheme, followed by an opaque string uniquely identifying the object in the browser. |
|  | method | static DOMString createObjectURL(MediaSource mediaSource) | Returns a DOMString containing a unique blob URL, that is a URL with media source: as its scheme, followed by an opaque string uniquely identifying the object in the browser. |
|  | method | static void revokeObjectURL(DOMString url) | Revokes an object URL previously created using URL.createObjectURL() |
| [WindowTimers](https://www.w3.org/TR/html5/webappapis.html#timers) | method | long setTimeout(TimerHandler handler, optional long timeout = 0, any... arguments) | Calls a function or evaluates an expression after a specified number of milliseconds. |
|  | method | void clearTimeout(optional long handle = 0) | Clears a timer set with setTimeout(). |
|  | method | long setInterval(TimerHandler handler, optional long timeout = 0, any... arguments) | Calls a function or evaluates an expression at specified intervals (in milliseconds). |
|  | method | void clearInterval(optional long handle = 0) | Clears a timer set with setInterval(). |
|  | typedef | (DOMString or Function) TimerHandler | |
| [ElementContentEditable](https://html.spec.whatwg.org/multipage/interaction.html#elementcontenteditable) | interface |  |  |
|  | attribute | contentEditable | contentEditable property is used to indicate whether or not the element is editable. |
|  | attribute | isContentEditable | returns a Boolean that is true if the contents of the element are editable; otherwise it returns false.|

## Events

This section describes the complete list of supported events by LWE. Please note
that only the attributes and methods mentioned explicitly in this section are
supported.

| Interface | Type | Name | Description |
|-----------|------|------|-------------|
| [Event](https://dom.spec.whatwg.org/#interface-event) | interface | Event | |
| | constructor | Event(DOMString type, optional EventInit eventInitDict) | Creates a new Event object. |
| | constant | NONE = 0 | Events not currently dispatched are in this phase. |
| | constant | CAPTURING_PHASE = 1 | When an event is dispatched to an object that participates in a tree it will be in this phase before it reaches its target attribute value. |
| | constant | AT_TARGET = 2 | When an event is dispatched it will be in this phase on its target attribute value. |
| | constant | BUBBLING_PHASE = 3 | When an event is dispatched to an object that participates in a tree it will be in this phase after it reaches its target attribute value. |
| | attribute | bubbles | Returns true or false depending on how event was initialized. True if event goes through its target attribute value’s ancestors in reverse tree order, and false otherwise. |
| | attribute | cancelable | Returns true or false depending on how event was initialized. Its return value does not always carry meaning, but true can indicate that part of the operation during which event was dispatched, can be canceled by invoking the preventDefault() method. |
| | attribute | currentTarget | Returns the object whose event listener’s callback is currently being invoked. |
| | attribute | defaultPrevented | Returns true if preventDefault() was invoked successfully to indicate cancellation, and false otherwise. |
| | attribute | eventPhase | Returns the event’s phase, which is one of NONE, CAPTURING_PHASE, AT_TARGET, and BUBBLING_PHASE. |
| | attribute | target | Returns the object to which event is dispatched. |
| | attribute | isTrusted | Returns true when the event was generated by a user action, and false when the event was created or modified by a script or dispatched via dispatchEvent. |
| | attribute | timeStamp | Returns the creation time of event as the number of milliseconds that passed since 00:00:00 UTC on 1 January 1970. |
| | attribute | type | Returns the type of event, e.g. "click, "hashchange", or "submit" |
| | method | void stopPropagation() | When dispatched in a tree, invoking this method prevents event from reaching any objects other than the current object. |
| | method | void stopImmediatePropagation() | Invoking this method prevents event from reaching any registered event listeners after the current one finishes running and, when dispatched in a tree, also prevents event from reaching any other objects. |
| | method | void preventDefault() | If invoked when the cancelable attribute value is true, and while executing a listener for the event with passive set to false, signals to the operation that caused event to be dispatched that it needs to be canceled. |
| | dictionary | EventInit::bubbles = false | Initializes an Event object with bubbles. |
| | dictionary | EventInit::cancelable = false | Initializes an Event object with cancelable. |
| [CustomEvent](https://www.w3.org/TR/dom/#interface-customevent) | interface | CustomEvent | Events using the CustomEvent interface can be used to carry custom data. |
| | constructor | CustomEvent(DOMString type, optional CustomEventInit eventInitDict) | Create a new CustomEvent. |
| | attribute | detail | Returns any custom data event was created with. Typically used for synthetic events. |
| [FocusEvent](https://w3c.github.io/uievents/#interface-focusevent) | interface | FocusEvent | The FocusEvent interface represents focus-related events like focus, blur, focusin, or focusout. |
| | constructor | FocusEvent(DOMString type, optional FocusEventInit eventInitDict) | Create a new FocusEvent |
| | attribute | relatedTarget | Used to identify a secondary EventTarget related to a Focus event, depending on the type of event. |
| [GlobalEventHandlers](https://html.spec.whatwg.org/multipage/webappapis.html#globaleventhandlers) | partial<br>interface | GlobalEventHandlers | The GlobalEventHandlers are the event handlers common to several interfaces like HTMLElement, Document, or Window. |
| | attribute | onabort | Fired at the Window when the download was aborted by the user |
| | attribute | onblur | Fired at nodes when they stop being focused |
| | attribute | oncanplay | Fired when the user agent can resume playback of the media data, but estimates that if playback were to be started now, the media resource could not be rendered at the current playback rate up to its end without having to stop for further buffering of content. |
| | attribute | oncanplaythrough | Fired when the user agent estimates that if playback were to be started now, the media resource could be rendered at the current playback rate all the way to its end without having to stop for further buffering. |
| | attribute | onchange | Fired at controls when the user commits a value change. |
| | attribute | onclick | Fired when the click event is raised. |
| | attribute | ondurationchange | Fired when the duration attribute has just been updated. |
| | attribute | onemptied | Fired when a media element whose networkState was previously not in the NETWORK_EMPTY state has just switched to that state. |
| | attribute | onended | Fired when playback has stopped because the end of the media resource was reached. |
| | attribute | onerror | Fired when the error event is raised. |
| | attribute | onfocus | Fired when the focus event is raised. |
| | attribute | oninput | Fired at controls when the user changes the value, before the change is committed. |
| | attribute | onkeydown | Fired when the keydown event is raised. |
| | attribute | onkeypress | Fired when the keypress event is raised. |
| | attribute | onkeyup | Fired when the keyup event is raised. |
| | attribute | onload | Fired when the load event is raised. |
| | attribute | onloadeddata | Fired when the user agent can render the media data at the current playback position for the first time. |
| | attribute | onloadedmetadata | Fired when the user agent has just determined the duration and dimensions of the media resource and the text tracks are ready. |
| | attribute | onloadstart | Fired when the user agent begins looking for media data, as part of the resource selection algorithm. |
| | attribute | onmousedown | Fired when a mouse button is pressed over the element. |
| | attribute | onmousemove | Fired when the pointer moves over the element. |
| | attribute | onmouseup | Fired when a mouse button is released over the element. |
| | attribute | onmouseenter | Fired when the pointer enters the element (does not bubble). |
| | attribute | onmouseleave | Fired when the pointer leaves the element (does not bubble). |
| | attribute | onmouseout | Fired when the pointer exits the element. |
| | attribute | onmouseover | Fired when the mouseover event is raised. |
| | attribute | onpointerdown | Fired when a pointer becomes active over the element. (Other `onpointer*` handlers are listed in the **Unsupported** row below.) |
| | attribute | onpointermove | Fired when a pointer changes coordinates. |
| | attribute | onpointerup | Fired when a pointer is no longer active. |
| | attribute | onscroll | Fired when the document view or an element has scrolled. |
| | attribute | onsubmit | Fired at a `<form>` when it is submitted. |
| | attribute | onpause | Fired when the element has been paused. |
| | attribute | onplay | Fired when the element is no longer paused. Fired after the play() method has returned, or when the autoplay attribute has caused playback to begin. |
| | attribute | onplaying | Fired when playback is ready to start after having been paused or delayed due to lack of media data. |
| | attribute | onprogress | Fired when the user agent is fetching media data. |
| | attribute | onratechange | Fired when either the defaultPlaybackRate or the playbackRate attribute has just been updated. |
| | attribute | onresize | Fired at the Window when the viewport is resized. |
| | misc | **Multimedia-gated handlers** (only fire when `STARFISH_ENABLE_MULTIMEDIA` is on) | `oncanplay`, `oncanplaythrough`, `ondurationchange`, `onemptied`, `onended`, `onloadeddata`, `onloadedmetadata`, `onpause`, `onplay`, `onplaying`, `onratechange`, `onseeked`, `onseeking`, `onstalled`, `onsuspend`, `ontimeupdate`, `onvolumechange`, `onwaiting`. Without the flag, the handler attribute still parses but the event never fires. |
| | misc | **Unsupported in LWE** (IDL `[Unimplemented]` — assigning to the handler succeeds, but the event never fires) | `onauxclick`, `oncancel`, `onclose`, `oncontextmenu`, `oncuechange`, `ondblclick`, `ondrag`/`ondragend`/`ondragenter`/`ondragexit`/`ondragleave`/`ondragover`/`ondragstart`/`ondrop`, `oninvalid`, `onloadend`, `onwheel`, `onreset`, `onselect`, `onshow`, `ontoggle`, `onpointerover`/`onpointerenter`/`onpointercancel`/`onpointerout`/`onpointerleave`/`ongotpointercapture`/`onlostpointercapture`/`onpointerrawupdate`. **Drag-and-drop and the wheel event are unsupported on LWE.** |
| [InputEvent](https://w3c.github.io/input-events/#interface-InputEvent) | interface | InputEvent | The InputEvent interface represents an event notifying of editable content change. |
| | attribute | data | Returns a DOMString with the inserted characters. |
| [MouseEvent](https://w3c.github.io/uievents/#idl-mouseevent) | interface | MouseEvent |  |
| | attribute | screenX | The horizontal coordinate at which the event occurred relative to the origin of the screen |
| | attribute | screenY | The vertical coordinate at which the event occurred relative to the origin of the screen |
| | attribute | clientX | The horizontal coordinate at which the event occurred relative to the viewport |
| | attribute | clientY | The vertical coordinate at which the event occurred relative to the viewport |
| | attribute | button | Indicates which button was pressed on the mouse to trigger the event |
| | attribute | buttons | Indicates which buttons are pressed on the mouse when the event is triggered. |
| | attribute | relatedTarget | Used to identify a secondary EventTarget related to a UI event, depending on the type of event |
| | method | initMouseEvent | Initializes attributes of a MouseEvent object. |
| [MouseEventInit](https://w3c.github.io/uievents/#idl-mouseeventinit) | dictionary | MouseEventInit |  |
| | attribute | screenX | Initializes the screenX attribute of the MouseEvent object |
| | attribute | screenY | Initializes the screenY attribute of the MouseEvent object |
| | attribute | clientX | Initializes the clientX attribute of the MouseEvent object |
| | attribute | clientY | Initializes the clientY attribute of the MouseEvent object |
| | attribute | button | Initializes the button attribute of the MouseEvent object |
| | attribute | buttons | Initializes the buttons attribute of the MouseEvent object |
| | attribute | relatedTarget | Initializes the relatedTarget attribute of the MouseEvent object |
| [KeyboardEvent](https://w3c.github.io/uievents/#interface-keyboardevent) | interface | KeyboardEvent | KeyboardEvent objects describe a user interaction with the keyboard. Each event describes a key; the event type (keydown, keypress, or keyup) identifies what kind of activity was performed. |
|  | constant | DOM_KEY_LOCATION_STANDARD = 0x00 |  |
|  | constant | DOM_KEY_LOCATION_LEFT = 0x01 |  |
|  | constant | DOM_KEY_LOCATION_RIGHT = 0x02 |  |
|  | constant | DOM_KEY_LOCATION_NUMPAD = 0x03 |  |
|  | attribute | key | Returns the key value of the key pressed. |
|  | attribute | code | Returns a string that identifies the physical key being pressed. |
|  | attribute | ctrlKey | Returns a Boolean that is true if the Ctrl key was active when the key event was generated. |
|  | attribute | shiftKey | Returns a Boolean that is true if the Shift key was active when the key event was generated. |
|  | attribute | altKey | Returns a Boolean that is true if the Alt key was active when the key event was generated. |
|  | attribute | metaKey | Returns a Boolean that is true if the Meta key was active when the key event was generated. |
|  | attribute | repeat | Returns a Boolean that is true if the key has been pressed in a sustained manner. |
|  | attribute | keyCode | Returns a Number representing a system and implementation dependent numerical code identifying the unmodified value of the pressed key. |
| [KeyboardEventInit](https://w3c.github.io/uievents/#idl-keyboardeventinit) | dictionary | KeyboardEventInit |  |
|  | attribute | key | Initializes the key attribute of the KeyboardEvent object to the unicode character string representing the meaning of a key after taking into account all keyboard modifiers (such as shift-state). This value is the final effective value of the key. |
|  | attribute | code | Initializes the code attribute of the KeyboardEvent object to the unicode character string representing the key that was pressed, ignoring any keyboard modifications such as keyboard layout. |
|  | attribute | repeat | Initializes the repeat attribute of the KeyboardEvent object. |
| [ProgressEvent](https://www.w3.org/TR/progress-events/#interface-progressevent) | interface | ProgressEvent | The ProgressEvent interface represents events measuring progress of an underlying process, like an HTTP request (for an XMLHttpRequest, or the loading of the underlying resource of an \<img\>, \<audio\>, \<video\>, \<style\> or \<link\>). |
| | constructor | ProgressEvent(DOMString type, optional FocusEventInit eventInitDict) | Create a new ProgressEvent |
| | attribute | lengthComputable | Is a Boolean flag indicating if the total work to be done, and the amount of work already done, by the underlying process is calculable. In other words, it tells if the progress is measurable or not. |
| | attribute | loaded | Is an unsigned long long representing the amount of work already performed by the underlying process. The ratio of work done can be calculated with the property and ProgressEvent.total. When downloading a resource using HTTP, this only represent the part of the content itself, not headers and other overhead. |
| | attribute | total | Is an unsigned long long representing the total amount of work that the underlying process is in the progress of performing. When downloading a resource using HTTP, this only represent the content itself, not headers and other overhead. |
| [UIEvent](https://w3c.github.io/uievents/#interface-UIEvent) | interface | UIEvent | The UIEvent interface provides specific contextual information associated with User Interface events. |
| | attribute | view | The view attribute identifies the Window from which the event was generated |
| | attribute | detail | Specifies some detail information about the Event, depending on the type of event. |
| [UIEventInit](https://w3c.github.io/uievents/#dictdef-uieventinit) | dictionary | UIEventInit | Dictionary that is used to create UIEvent. |
| | attribute | view | Should be initialized to the Window object of the global environment in which this event will be dispatched |
| | attribute | detail | This value is initialized to a number that is application-specific. |
| [CompositionEvent](https://w3c.github.io/uievents/#events-compositionevents) | interface | CompositionEvent | Composition Events provide a means for inputing text in a supplementary or alternate manner than by Keyboard Events, in order to allow the use of characters that might not be commonly available on keyboard. |
| | attribute | data | data holds the value of the characters generated by an input method.  |
| [CompositionEventInit](https://w3c.github.io/uievents/#idl-compositioneventinit) | dictionary | CompositionEventInit | Dictionary that is used to create CompositionEvent. |
| | attribute | data | Initializes the data attribute of the CompositionEvent object to the characters generated by the IME composition. |
| [EventModifierInit](https://w3c.github.io/uievents/#dictdef-eventmodifierinit) | dictionary | EventModifierInit | The MouseEvent and KeyboardEvent interfaces share a set of keyboard modifier attributes. EventModifierInit enables authors to initialize keyboard modifier attributes of the MouseEvent and KeyboardEvent interfaces. |
| | attribute | ctrlKey | true if the Control key modifier is to be considered active, false otherwise |
| | attribute | shiftKey | true if the Shift key modifier is to be considered active, false otherwise. |
| | attribute | altKey | true if the Alt (alternative) (or Option) key modifier is to be considered active, false otherwise. |
| | attribute | metaKey | true if the Meta key modifier is to be considered active, false otherwise. |
| [TouchEvent](https://w3c.github.io/touch-events/#touchevent-interface) | interface | TouchEvent | Defines the touchstart, touchend, touchmove, and touchcancel event types. |
| | attribute | touches | A list of Touch objects for every point of contact currently touching the surface. |
| [MessageEvent](https://html.spec.whatwg.org/multipage/comms.html#messageevent) | interface | MessageEvent |Messages in server-sent events, Web sockets, cross-document messaging, channel messaging, and broadcast channels use the MessageEvent interface for their message events. |
| | attribute | data | Returns the data of the message. |
| | attribute | origin | Returns the origin of the message, for server-sent events and cross-document messaging. |
| | attribute | lastEventId | Returns the last event ID string, for server-sent events. |
| | attribute | source | Returns the WindowProxy of the source window, for cross-document messaging, and the MessagePort being attached, in the connect event fired at SharedWorkerGlobalScope objects. |
| | attribute | ports | Returns the MessagePort array sent with the message, for cross-document messaging and channel messaging. |
| [MediaQueryListEvent](https://drafts.csswg.org/cssom-view/#mediaquerylistevent) | interface | MediaQueryListEvent | The MediaQueryListEvent object stores information on the changes that have happened to a MediaQueryList object. |
| | attribute | media | A DOMString representing a serialized media query. |
| | attribute | matches | A Boolean that returns true if the document currently matches the media query list, or false if not. |
| [MediaQueryListEventInit](https://drafts.csswg.org/cssom-view/#dictdef-mediaquerylisteventinit) | dictionary | MediaQueryListEventInit | Dictionary that is used to create MediaQueryListEvent. |
| | attribute | media | Returns the value it was initialized to. |
| | attribute | matches | Returns the value it was initialized to. |
| [WindowEventHandlers](https://html.spec.whatwg.org/multipage/webappapis.html#windoweventhandlers) | partial<br>interface | WindowEventHandlers | WindowEventHandlers are the event handlers common to several interfaces like Window, or HTMLBodyElement and  HTMLFrameSetElement. Each of these interfaces can implement additional specific event handlers. |
| | attribute | onmessage | Fired at an object when it receives a message. |
| | attribute | onmessageerror | Fired at an object when it receives a message that cannot be deserialized. |
| | attribute | onhashchange | Fired at the `Window` when the fragment identifier of the URL changes. (Gated by `STARFISH_WEBWORKER_NOT_HOST` — only on the host page, not in workers.) |
| | attribute | onunload | Fired at the Window object when the page is going away. |
| | misc | **Unsupported in LWE** (`[Unimplemented]` — never fire) | `onafterprint`, `onbeforeprint`, `onbeforeunload`, `onlanguagechange`, `onoffline`, `ononline`, `onpagehide`, `onpageshow`, `onpopstate`, `onrejectionhandled`, `onstorage`, `onunhandledrejection`. |
| [SecurityPolicyViolationEventInit](https://www.w3.org/TR/CSP2/#securitypolicyviolationeventinit-interface) | dictionary | SecurityPolicyViolationEventInit | Dictionary that is used to create SecurityPolicyViolationEvent. |
| | attribute | blockedURI | Returns the requested URL of the resource that was prevented from loading. |
| | attribute | violatedDirective | Returns the policy directive that was violated. |
| [AnimationEvent](https://drafts.csswg.org/css-animations/#events) | interface | AnimationEvent | Provides specific contextual information associated with Animation events. |
| | attribute | animationName | Returns the value of the animation-name property of the animation that fired the event. |
| | attribute | elapsedTime | Returns the amount of time the animation has been running, in seconds, when this event fired, excluding any time the animation was paused. |
| | attribute | onanimationstart | Occurs at the start of the animation. If there is an animation-delay then this event will fire once the delay period has expired. |
| | attribute | onanimationend | occurs when the animation finishes. In this case the value of the elapsedTime member of the event is equal to the active duration. |
| [AnimationEventInit](https://drafts.csswg.org/css-animations/#events) | dictionary  | AnimationEventInit | Dictionary that is used to create AnimationEvent. |
| | attribute | animationName | Returns the value it was initialized to. |
| | attribute | elapsedTime | Returns the value it was initialized to. |
| [TransitionEvent](https://drafts.csswg.org/css-transitions/#interface-transitionevent) | interface | TransitionEvent | Fired when a CSS transition completes (`transitionend`) or is cancelled (`transitioncancel`). Constructor `new TransitionEvent(type, init)` is supported. |
| | attribute | propertyName | The name of the CSS property the transition is associated with. |
| | attribute | elapsedTime | Time, in seconds, the transition had been running at the time the event fired (`transition-delay` is not counted). |
| | attribute | pseudoElement | The pseudo-element on which the transition ran (e.g. `"::before"`); empty string if not on a pseudo-element. **`[Unimplemented]` in `TransitionEventInit`** — read as the default empty string. |
| [ErrorEvent](https://html.spec.whatwg.org/multipage/webappapis.html#the-errorevent-interface) | interface | ErrorEvent | Fired at the global object when an uncaught script error or rejected promise occurs. Constructor `new ErrorEvent(type, init)` is supported. See also the audit row in the Events appendix below for `document.createEvent('ErrorEvent')` caveat. |
| | attribute | message, filename, lineno, colno, error | The error description, source URL, line/column, and the optional `Error` instance, all round-tripped through the constructor. |
| [CloseEvent](https://html.spec.whatwg.org/multipage/web-sockets.html#the-closeevent-interface) | interface | CloseEvent | Fired at a `WebSocket` when the connection closes. Constructor `new CloseEvent(type, init)` is supported. |
| | attribute | wasClean | Whether the connection was cleanly closed. |
| | attribute | code | The WebSocket connection close code. |
| | attribute | reason | The WebSocket connection close reason string. |

## Obsolete

This section describes the list of Obsolete interfaces.
To use these features, you need to define STARFISH_ENABLE_OBSOLETE_SPEC.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| Document  | attribute | width | Returns the width of the &lt;body&gt; element of the current document in pixels. |
|  | attribute | height | Returns the height of the &lt;body&gt; element of the current document in pixels.  |
| Window  | attribute | event | Returns current dispatching event. |
| Navigator  | attribute | battery | The battery read-only property returns a BatteryManager provides information about the system's battery charge level. |


## CSS

This section describes the complete list of supported CSS properties by LWE.
Please note that only the properties and values mentioned explicitly in this
section are supported.


| Type | Property | Allowed Value | Description | Note |
|------|----------|---------------|-------------|------|
| [Margin](https://www.w3.org/TR/CSS2/box.html#margin-properties) | margin | &lt;margin-width&gt;{1,4} | The margin shorthand property sets all the margin properties in one declaration | The margin properties specify the width of the margin area of a box. &lt;margin-width&gt; may take one of the following values: &lt;length&gt;, &lt;percentage&gt;, auto. (Also check [Length](https://www.w3.org/TR/CSS2/syndata.html#length-units)) |
| | margin-bottom | &lt;margin-width&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the bottom margin of an element | |
| | margin-left | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the left margin of an element | |
| | margin-right | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the right margin of an element | |
| | margin-top | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the top margin of an element | |
| [Padding](https://www.w3.org/TR/CSS2/box.html#padding-properties) | padding | &lt;padding-width&gt;{1,4} | The padding shorthand property sets all the padding properties in one declaration | &lt;padding-width&gt; may take one of the following values: &lt;length&gt;, &lt;percentage&gt; |
| | padding-bottom | &lt;length&gt; &#124; &lt;percentage&gt;	| Sets the bottom padding for an element | |
| | padding-left | &lt;length&gt; &#124; &lt;percentage&gt;	| Sets the left padding for an element | |
| | padding-right | &lt;length&gt; &#124; &lt;percentage&gt; | Sets the right padding for an element | |
| | padding-top | &lt;length&gt; &#124; &lt;percentage&gt; | Sets the top padding for an element | |
| [Border](https://www.w3.org/TR/css3-border/) | border | &lt;border-width&gt; &lt;border-style&gt; &lt;border-color&gt; | Sets all the border properties (shorthand). | The border can either be a predefined style (solid line) or it can be an image. In the former case, various properties define the style (&lt;border-style&gt;), color (&lt;border-color&gt;), and thickness (&lt;border-width&gt;) of the border. &lt;border-width&gt; may take one of the following values: thin, medium, thick, and &lt;length&gt;. &lt;border-color&gt; may take one of the following values: &lt;color&gt;, and transparent. &lt;border-style&gt; may take one of the following values: none, solid, dashed, inset, and outset. (Also check Border Properties) |
| | border-bottom | &lt;border-width&gt;   &lt;border-style&gt;   &lt;border-color&gt; | Sets all the bottom border properties (shorthand). | |
| | border-bottom-color | &lt;color&gt; &#124; transparent | Sets the color of the bottom border. | |
| | border-bottom-style | none &#124; hidden &#124; solid &#124; dashed &#124; dotted &#124; double &#124; inset &#124; outset &#124; groove &#124; ridge | Sets the style of the bottom border. | |
| | border-bottom-width | medium &#124; thin &#124; thick &#124; &lt;length&gt; | Sets the width of the bottom border. | |
| | border-color | &lt;border-color&gt;{1,4} | Sets the color of the four borders (shorthand). | |
| | border-left | &lt;border-width&gt;   &lt;border-style&gt;   &lt;border-color&gt; | Sets all the left border properties (shorthand). | |
| | border-left-color | &lt;color&gt; &#124; transparent | Sets the color of the left border. | |
| | border-left-style | none &#124; hidden &#124; solid &#124; dashed &#124; dotted &#124; double &#124; inset &#124; outset &#124; groove &#124; ridge | Sets the style of the left border. | |
| | border-left-width | medium &#124; thin &#124; thick &#124; &lt;length&gt; | Sets the width of the left border. | |
| | border-right | &lt;border-width&gt;   &lt;border-style&gt;   &lt;border-color&gt;	| Sets all the right border properties (shorthand). | |
| | border-right-color | &lt;color&gt; &#124; transparent | Sets the color of the right border. | |
| | border-right-style | none &#124; hidden &#124; solid &#124; dashed &#124; dotted &#124; double &#124; inset &#124; outset &#124; groove &#124; ridge | Sets the style of the right border. | |
| | border-right-width | medium &#124; thin &#124; thick &#124; &lt;length&gt;	| Sets the width of the right border. | |
| | border-style | &lt;border-style&gt;{1,4} | Sets the style of the four borders (shorthand). | |
| | border-top | &lt;border-width&gt; &lt;border-style&gt; &lt;border-color&gt;	| Sets all the top border properties (shorthand). | |
| | border-top-color | &lt;color&gt; &#124; transparent | Sets the color of the top border. | |
| | border-top-style | none &#124; hidden &#124; solid &#124; dashed &#124; dotted &#124; double &#124; inset &#124; outset &#124; groove &#124; ridge | Sets the style of the top border. | |
| | border-top-width | medium &#124; thin &#124; thick &#124; &lt;length&gt;	| Sets the width of the top border. | |
| | border-width | &lt;border-width&gt; | Sets the width of the four borders (shorthand). | |
| | border-image | &lt;border-image-source&gt; &#124;&#124; &lt;border-image-slice&gt; [/ &lt;border-image-width&gt; &#124; / &lt;border-image-width&gt;? / &lt;border-image-outset&gt;]? &#124;&#124; &lt;border-image-repeat&gt; | Lets you draw an image in place of an element's border-style. | |
| | border-image-source | &lt;image&gt; &#124; none | Specifies the source image used to create an element's border image. | |
| | border-image-slice | [&lt;number&gt; &#124; &lt;percentage&gt;]{1,4} && fill? | Divides the image specified by border-image-source into regions. These regions are used to form the components of an element's border image.| |
| | border-image-width | [&lt;length-percentage&gt; &#124; &lt;number&gt; &#124; auto]{1,4}	| Specifies the width of an element's border image. | |
| | border-image-outset | [&lt;length&gt; &#124; &lt;number&gt;]{1,4}	| Specifies the distance by which an element's border image is set out from its border box. | |
| | border-image-repeat | [ stretch &#124; repeat &#124; round &#124; space ]	| Defines how the edge regions of a source image are adjusted to fit the dimensions of an element's border image. | |
| | border-radius | &lt;length-percentage&gt;{1,4} [ / &lt;length-percentage&gt;{1,4} ]?	| define the radii of a quarter ellipse that defines the shape of the corner of the outer border edge | |
| | border-top-left-radius, border-top-right-radius, border-bottom-right-radius, border-bottom-left-radius | &lt;length-percentage&gt;{1,2} | define the radii of a quarter ellipse that defines the shape of the corner of the outer border edge | |
| [Logical Borders](https://drafts.csswg.org/css-logical-1/#border-properties) | border-block-start, border-block-end, border-inline-start, border-inline-end | &lt;border-width&gt; &lt;border-style&gt; &lt;border-color&gt; | Logical-direction shorthands; resolve against `writing-mode`. (Style.h FOR_EACH_STYLE_ATTRIBUTE_SHORTHAND.) | |
| | border-block-start-color, border-block-end-color, border-inline-start-color, border-inline-end-color | &lt;color&gt; &#124; transparent | Logical-direction border colors. | |
| | border-block-start-style, border-block-end-style, border-inline-start-style, border-inline-end-style | none &#124; hidden &#124; solid &#124; dashed &#124; dotted &#124; double &#124; inset &#124; outset &#124; groove &#124; ridge | Logical-direction border styles. | |
| | border-block-start-width, border-block-end-width, border-inline-start-width, border-inline-end-width | medium &#124; thin &#124; thick &#124; &lt;length&gt; | Logical-direction border widths. | |
| [Outline](https://www.w3.org/TR/css-ui-3/) | outline | &lt;border-width&gt; &lt;border-style&gt; &lt;border-color&gt; | Sets all the border properties (shorthand). | In the former case, various properties define the style (&lt;border-style&gt;), color (&lt;border-color&gt;), and thickness (&lt;border-width&gt;) of the border. &lt;border-width&gt; may take one of the following values: thin, medium, thick, and &lt;length&gt;. &lt;border-color&gt; may take one of the following values: &lt;color&gt;, and transparent. &lt;border-style&gt; may take one of the following values: none, solid, inset, and outset. (Also check Border Properties) |
| | outline-color | &lt;border-color&gt; | Sets the color of the outline |
| | outline-style | &lt;border-style&gt; &#124; auto | Sets the style of the outline. Shares `updateValueUnitBorderStyle` with `border-style`, so the same 10 keywords parse. **`auto` is accepted but mapped to `solid`** (the engine has no UA-specific focus-outline style). | |
| | outline-width | &lt;border-width&gt; | Sets the width of the outline | |
| | outline-offset | &lt;length&gt; | Sets the offset of the outline | |
| | resize | none | Specifies whether or not an element is resizable by the user, and if so, along which axis/axes. | Development status: experimental |
| [Display](https://www.w3.org/TR/CSS2/visuren.html#display-prop) | display | inline &#124; block &#124; inline-block &#124; table &#124; inline-table &#124; table-row-group &#124; table-header-group &#124; table-footer-group &#124; table-row &#124; table-column-group &#124; table-column &#124; table-cell &#124; table-caption &#124; flex &#124; inline-flex &#124; grid &#124; inline-grid &#124; list-item &#124; inline-list-item &#124; contents &#124; none | The display property specifies the type of box used for an HTML element (also check [Visibility](#visibility) and the runtime caveats appendix below). `-webkit-box`/`-webkit-inline-box` are also accepted under `STARFISH_ENABLE_CSS_WEBKIT_BOX_PREFIX`. **`flow-root`, `run-in`, `ruby*`, and multi-token L3 syntax (`block flex`) are silently dropped** — see the audit-additions table below. |  |
| [Position](https://www.w3.org/TR/CSS2/visuren.html#positioning-scheme) | position | static &#124; absolute &#124; relative &#124; fixed | The position property specifies the type of positioning method used for an element. | Each element in the document tree generates zero or more boxes according to the box model. The layout of these boxes is governed by box dimensions, type, positioning scheme, relationships between in the document tree and external information. \*CSS direction property only accepts "ltr" as a value. To support right-to-left text, the dir attribute in an HTML element should be used, e.g., &lt;html dir="rtl"&gt; (Also check Layers, Direction, Visual Formatting Model, and Visual Effects) |
| | top | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the top edge of an element to a unit above/below the top edge of its nearest positioned ancestor. | |
| | right | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the right edge of an element to a unit above/below the right edge of its nearest positioned ancestor. | |
| | bottom | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the bottom edge of an element to a unit above/below the bottom edge of its nearest positioned ancestor. | |
| | left | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto | Sets the left edge of an element to a unit above/below the left edge of its nearest positioned ancestor. | |
| [Floats](https://www.w3.org/TR/2011/REC-CSS2-20110607/visuren.html#floats) | float | left &#124; right &#124; none | Specifies whether a box should float to the left, right, or not at all. | |
| | clear | none &#124; left &#124; right &#124; both | Indicates which sides of an element's box(es) may not be adjacent to an earlier floating box. | |
| [Flex](https://www.w3.org/TR/css-flexbox-1/) | flex-direction | row &#124; row-reverse &#124; column &#124; column-reverse | Specifies how flex items are placed in the flex container, by setting the direction of the flex container’s main axis. | All four values implemented. |
| | flex-wrap | nowrap &#124; wrap &#124; wrap-reverse | Controls whether the flex container is single-line or multi-line, and the direction of the cross-axis. | All three values implemented. |
| | flex-flow | &lt;flex-direction&gt; &#124;&#124; &lt;flex-wrap&gt; | Shorthand for `flex-direction` and `flex-wrap`. | Single-component (e.g. `flex-flow: wrap`) and two-component forms accepted. |
| | order | &lt;integer&gt; | Controls visual order of flex items via ordinal groups. | Negative integers accepted. |
| | flex | none &#124; [ &lt;flex-grow&gt; &lt;flex-shrink&gt;? &#124;&#124; &lt;flex-basis&gt; ] | Shorthand for `flex-grow` / `flex-shrink` / `flex-basis`. | Keywords `auto`, `none`, `initial` accepted; numeric flex-basis without unit (e.g. `flex: 1`) becomes `0%` per spec. |
| | flex-grow | &lt;number&gt; | Flex grow factor. | Non-negative numbers honored; negative values clamped to `0`; non-numeric tokens (`abc`) parse as `0`. |
| | flex-shrink | &lt;number&gt; | Flex shrink factor. | Non-negative numbers honored; negative values rejected (computed value falls back to `1`). |
| | flex-basis | content &#124; &lt;'width'&gt; | Flex basis (`auto`, `content`, `<length>`, `<percentage>`). | All listed values accepted; negative lengths rejected. |
| | justify-content | normal &#124; flex-start &#124; flex-end &#124; start &#124; end &#124; center &#124; space-between &#124; space-around &#124; stretch | Aligns flex items along the main axis. | Unsupported values (`space-evenly`, `left`, `right`) cause the declaration to be **dropped** — the property falls back to its initial value (`normal`). `start`/`end`/`stretch` are experimentally supported (layout may not match spec). |
| | align-items | flex-start &#124; flex-end &#124; start &#124; end &#124; center &#124; baseline &#124; stretch | Aligns flex items along the cross axis. | Unsupported values (`first baseline`, `last baseline`, `self-start`, `self-end`, `normal`) cause the declaration to be **dropped** — the property falls back to its initial value (`stretch`). `start`/`end` are experimentally supported (layout may not match spec). |
| | align-self | auto &#124; flex-start &#124; flex-end &#124; start &#124; end &#124; center &#124; baseline &#124; stretch | Per-item override of `align-items`. | Same value subset as `align-items`; `auto` resolves to the parent’s `align-items` value. `start`/`end` are experimentally supported (layout may not match spec). |
| | align-content | flex-start &#124; flex-end &#124; center &#124; space-between &#124; space-around &#124; stretch | Aligns flex container’s lines along the cross axis. | Unsupported values (`space-evenly`, `start`, `end`, `normal`, `baseline`/`first baseline`/`last baseline`) cause the declaration to be **dropped** — the property falls back to its initial value (`stretch`). |
| | row-gap | normal &#124; &lt;length-percentage&gt; | Cross-axis (row-flex) / main-axis (column-flex) line gap. | Applied in both flex and grid containers. In flex, `row-gap`/`column-gap` are mapped to the main/cross gap according to `flex-direction`, and a percentage `row-gap` resolves against the block size. In grid, only fixed lengths are honored — a percentage gap computes to 0. |
| | column-gap | normal &#124; &lt;length-percentage&gt; | Inline-axis gap. | Applied in both flex and grid containers; in a column-direction flex container the inline axis is the cross axis, so it becomes the cross gap there. In flex, percentages resolve against the available inline size; in grid, only fixed lengths are honored. |
| | gap | &lt;'row-gap'&gt; &lt;'column-gap'&gt;? | Shorthand for `row-gap` and `column-gap`. | Expands to both longhands: one value sets `row-gap` and `column-gap` alike, two values set them in that order. `calc()` is not accepted. |
| | justify-items / justify-self | normal &#124; stretch &#124; start &#124; end &#124; center &#124; flex-start &#124; flex-end | CSS Box Alignment longhands for the inline axis. | Supported. |
| | place-items / place-content / place-self | — | CSS Box Alignment shorthands. | **NOT supported** — the shorthands are unknown to the property trie, so the declaration is dropped. Use the `align-*` / `justify-*` longhands. |
| [Grid](https://www.w3.org/TR/css-grid-1/) | grid-template-columns | &lt;length-percentage&gt; &#124; &lt;fr&gt; &#124; auto &#124; min-content &#124; max-content &#124; minmax() &#124; repeat() | This property defines the track sizing of the grid columns. | `minmax()` and `repeat()` (including `auto-fill`/`auto-fit`) are supported. `fit-content()`, line-name brackets `[name]`, and `subgrid` are not — the whole declaration is dropped. |
| | grid-template-rows | &lt;length-percentage&gt; &#124; &lt;fr&gt; &#124; auto &#124; min-content &#124; max-content &#124; minmax() &#124; repeat() | This property defines the track sizing of the grid rows. | Same support and same exclusions as `grid-template-columns`. |
| | grid-column-gap | &lt;length&gt; | Legacy alias for `column-gap`. | Mapped to `column-gap`; percentage values are not supported in grid (compute to 0). |
| | grid-row-gap | &lt;length&gt; | Legacy alias for `row-gap`. | Mapped to `row-gap`; percentage values are not supported in grid (compute to 0). |
| | grid-gap | &lt;length&gt; | Legacy shorthand for `row-gap` and `column-gap`. | Expands to both longhands; percentage values are not supported in grid (compute to 0). |
| | grid-column-start | auto &#124; &lt;integer&gt; &#124; &lt;custom-ident&gt; &#124; span &lt;integer&gt; &#124; span &lt;custom-ident&gt; | Specifies a grid item's start position within the grid column. | Negative integers parse but layout effect (counting from end) is not guaranteed. |
| | grid-column-end | auto &#124; &lt;integer&gt; &#124; &lt;custom-ident&gt; &#124; span &lt;integer&gt; &#124; span &lt;custom-ident&gt; | Specifies a grid item's end position within the grid column. | Same support and exclusions as `grid-column-start`. |
| | grid-row-start | auto &#124; &lt;integer&gt; &#124; &lt;custom-ident&gt; &#124; span &lt;integer&gt; &#124; span &lt;custom-ident&gt; | Specifies a grid item's start position within the grid row. | Same support and exclusions as `grid-column-start`. |
| | grid-row-end | auto &#124; &lt;integer&gt; &#124; &lt;custom-ident&gt; &#124; span &lt;integer&gt; &#124; span &lt;custom-ident&gt; | Specifies a grid item's end position within the grid row. | Same support and exclusions as `grid-column-start`. |
| | grid-row | &lt;grid-row-start&gt; / &lt;grid-row-end&gt; | Shorthand for `grid-row-start` and `grid-row-end`. | Same value support as the longhands. |
| | grid-column | &lt;grid-column-start&gt; / &lt;grid-column-end&gt; | Shorthand for `grid-column-start` and `grid-column-end`. | Same value support as the longhands. |
| | grid-template-areas | &lt;string&gt;+ | Specifies named grid areas. | |
| | grid-template | &lt;grid-template-rows&gt; / &lt;grid-template-columns&gt; &#124; [ &lt;string&gt; &lt;track-size&gt;? ]+ / &lt;grid-template-columns&gt; | Shorthand for `grid-template-rows`, `grid-template-columns`, and `grid-template-areas`. | Expands to all three sub-properties. |
| | grid-area | &lt;grid-row-start&gt; / &lt;grid-column-start&gt; / &lt;grid-row-end&gt; / &lt;grid-column-end&gt; &#124; &lt;custom-ident&gt; | Shorthand for `grid-row-start`/`grid-column-start`/`grid-row-end`/`grid-column-end`, or a named area. | |
| [Layered presentation](https://www.w3.org/TR/2011/REC-CSS2-20110607/visuren.html#layers) | z-index | auto &#124; &lt;integer&gt; | Specifies the stack order of an element. | |
| [Text direction](https://www.w3.org/TR/2011/REC-CSS2-20110607/visuren.html#direction) | direction | ltr &#124; rtl | Specifies the text direction. **Both `ltr` and `rtl` PARSE**, but layout/selectors honor only `ltr` (the `:dir(rtl)` selector parses but never matches). For RTL content prefer the HTML attribute `<html dir="rtl">`, which is honored by the line-break/bidi pipeline. | Development status: experimental |
| | unicode-bidi | normal &#124; embed &#124; isolate | Together with `direction`, controls handling of bidirectional text. **`bidi-override`, `isolate-override`, `plaintext` are NOT recognized.** | Development status: experimental |
| [Width, height](https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#q10.0) | width | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto &#124; available &#124; min-content &#124; max-content &#124; fit-content | Sets the width of an element. The intrinsic-sizing keywords (`available`/`min-content`/`max-content`/`fit-content`) parse and are honored by layout. **Standard CSS3 `fit-content(<length>)` function form is NOT recognized** — only the bare keyword. **Quirks-mode unitless lengths** (`width: 100`) are accepted only when the document has no DOCTYPE (`document.compatMode === "BackCompat"`). | |
| | min-width | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto &#124; available &#124; min-content &#124; max-content &#124; fit-content | Sets the minimum width of an element. | |
| | max-width | &lt;length&gt; &#124; &lt;percentage&gt; &#124; none &#124; available &#124; min-content &#124; max-content &#124; fit-content | Sets the maximum width of an element. | |
| | height | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto &#124; available &#124; min-content &#124; max-content &#124; fit-content | Sets the height of an element. | |
| | min-height | &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto &#124; available &#124; min-content &#124; max-content &#124; fit-content | Sets the minimum height of an element. | |
| | max-height | &lt;length&gt; &#124; &lt;percentage&gt; &#124; none &#124; available &#124; min-content &#124; max-content &#124; fit-content | Sets the maximum height of an element. | |
| [Box Model](https://www.w3.org/TR/css-ui-3/#box-model) | box-sizing | content-box &#124; border-box | Tells the padding and border is included to actual width&#124;height of an element's box. | |
| [Line height](https://www.w3.org/TR/2011/REC-CSS2-20110607/visudet.html#line-height) | line-height | normal &#124; &lt;number&gt; &#124; &lt;length&gt; &#124; &lt;percentage&gt; | Sets the line height. | |
| | vertical-align | baseline &#124; sub &#124; super &#124; top &#124; text-top &#124; middle &#124; bottom &#124; text-bottom &#124; &lt;length&gt; &#124; &lt;percentage&gt; | Sets the vertical alignment of an element. | |
| [Overflow](https://www.w3.org/TR/2011/REC-CSS2-20110607/visufx.html#overflow) | overflow | visible &#124; hidden &#124; auto &#124; scroll | Specifies what happens if content overflows an element's box. | |
| [Visibility](https://www.w3.org/TR/2011/REC-CSS2-20110607/visufx.html#visibility) | visibility | visible &#124; hidden &#124; collapse | Specifies whether or not an element should be visible | |
| [Generated content](https://www.w3.org/TR/2011/REC-CSS2-20110607/generate.html#content) | content | normal &#124; none &#124; [ &lt;string&gt; &#124; counter(&lt;name&gt; [, &lt;style&gt;]) &#124; counters(&lt;name&gt;, &lt;sep&gt; [, &lt;style&gt;]) &#124; attr(&lt;identifier&gt;) &#124; url(...) &#124; open-quote &#124; close-quote &#124; no-open-quote &#124; no-close-quote ]+ | Used with `::before`/`::after` to generate content. **Both `counter()` and `counters()` are parsed**; quote tokens (`open-quote`/`close-quote`/`no-open-quote`/`no-close-quote`) parse but are typically rendered as no-op since the engine has no built-in quote-pair table for `quotes` property (which itself is unsupported). The `<image>` form is `url()` only — `image-set()`, gradients, `linear-gradient()` as content are NOT parsed. | |
| [Color](https://www.w3.org/TR/css3-color/) | color | &lt;color&gt; | Sets the color of text. Supported color formats: hex (`#rgb`, `#rrggbb`, with optional alpha), `rgb()`/`rgba()`, `hsl()`/`hsla()`, named colors, `transparent`, `currentColor`. Modern color spaces (`lab()`, `lch()`, `hwb()`, `color()`) are NOT supported. | CSS uses color-related properties and values to color the text, backgrounds, borders, and other parts of elements in a document. |
| | opacity | alpha value (0.0 ~ 1.0) | Sets the opacity level for an element | |
| [Background](https://www.w3.org/TR/css3-background) | background | [&lt;bg-layer&gt;]* &lt;final-bg-layer&gt; | Shorthand for the longhands listed below. Multiple comma-separated layers ARE accepted (e.g. `background: red url(a.png) no-repeat, linear-gradient(red,blue)`). The `background-blend-mode` longhand is NOT part of this shorthand and is unsupported (see below). | |
| | background-color | &lt;color&gt; &#124; transparent | Specifies the background color. Same color formats as `color` (hex, named, `rgb()`/`rgba()`, `hsl()`/`hsla()`, `transparent`, `currentColor`). | |
| | background-image | [ &lt;url&gt; &#124; &lt;gradient&gt; &#124; none ]# | Comma-separated list of background-image layers IS supported (`backgroundLayerSize()` reflects layer count). Accepted gradient functions are `linear-gradient(...)` and `radial-gradient(...)` only. **`conic-gradient`, `repeating-linear-gradient`, `repeating-radial-gradient` are silently parsed as `none`** with no warning. | All other longhands (`background-position-{x,y}`, `background-size`, `background-repeat-{x,y}`, `background-attachment`, `background-clip`, `background-origin`) only return non-initial values from `getComputedStyle()` when at least one image layer is set. |
| | background-position | &lt;bg-position&gt; [, &lt;bg-position&gt;]* where &lt;bg-position&gt; = [ &lt;percentage&gt; &#124; &lt;length&gt; &#124; left &#124; center &#124; right ] [ &lt;percentage&gt; &#124; &lt;length&gt; &#124; top &#124; center &#124; bottom ]? | 1- and 2-value forms supported. **The CSS-3 4-value form `left 10px top 20px` is parsed without error but the side keywords are dropped — only the two length/percentage components survive (computed: `10px 20px`).** | |
| | background-position-x | [ center &#124; left &#124; right &#124; &lt;length-percentage&gt; ] | Single token only. | |
| | background-position-y | [ center &#124; top &#124; bottom &#124; &lt;length-percentage&gt; ] | Single token only. | |
| | background-repeat | `repeat` &#124; `no-repeat` &#124; `repeat-x` &#124; `repeat-y` | Shorthand expands to `background-repeat-x`/`-y`. The shorthand accepts the `repeat-x`/`repeat-y` keywords (mapped to asymmetric pairs); the longhands accept only `repeat`/`no-repeat`. Two-value form (`repeat no-repeat`) supported. **`space` and `round` are NOT recognized** — `updateValueUnitRepeatStyle` rejects them and the entire declaration is dropped. (The `RepeatStyleValue` enum has a `// TODO: space, round` comment.) | |
| | background-size | [ &lt;length-percentage&gt; &#124; auto ]{1,2} &#124; cover &#124; contain | 1- and 2-value forms supported; comma-separated layer list supported. | |
| | background-attachment | scroll &#124; fixed &#124; local [, ... ]* | All three keywords parse and round-trip through `getComputedStyle`. Layout/paint behavior of `fixed`/`local` (e.g. viewport-fixed painting) is **not** verified by this audit; treat as best-effort. | |
| | background-origin | &lt;box&gt; [, &lt;box&gt;]* where &lt;box&gt; = border-box &#124; padding-box &#124; content-box | All three values supported. | |
| | background-clip | &lt;box&gt; [, &lt;box&gt;]* where &lt;box&gt; = border-box &#124; padding-box &#124; content-box | `border-box`, `padding-box`, `content-box` supported. **`background-clip: text` is silently dropped to `border-box`** with no warning. | |
| | background-blend-mode | — | **Not supported.** Logged at parse time as `Unsupported css property: background-blend-mode`. Use `mix-blend-mode` on the element if a single blend is acceptable. | |
| [Font](https://www.w3.org/TR/CSS2/fonts.html) | font-style | normal &#124; italic &#124; oblique | Specifies the font style for text. | A font provides a resource containing the visual representation of characters. |
| | font-family | &lt;family-name&gt;# &#124; &lt;generic-family&gt; | Comma-separated prioritized list of font family names and/or generic family keywords (`serif`, `sans-serif`, `monospace`, `cursive`, `fantasy`). Quoted family names with spaces are accepted. | |
| | font-weight | normal &#124; bold &#124; bolder &#124; lighter &#124; 100 &#124; 200 &#124; 300 &#124; 400 &#124; 500 &#124; 600 &#124; 700 &#124; 800 &#124; 900 | Specifies the weight of a font. **Variable-font numeric values like `font-weight: 350` are not supported** — only the listed multiples of 100. | |
| | font-kerning | auto &#124; normal &#124; none | Specifies kerning mode of a font. | |
| | font-size | &lt;absolute-size&gt; &#124; &lt;relative-size&gt; &#124; &lt;length&gt; &#124; &lt;percentage&gt; | Specifies the font size of text. | Possible values of an &lt;absolute-size&gt; keyword: [ xx-small &#124; x-small &#124; small &#124; medium &#124; large &#124; x-large &#124; xx-large ] <br> Possible values of an &lt;relative-size&gt; keyword: [ larger &#124; smaller] |
| | font | [&lt;font-style&gt;? &lt;font-weight&gt;?] &lt;font-size&gt; [/ &lt;line-height&gt;]? &lt;font-family&gt; | Shorthand. **`font-stretch`, `font-variant` are NOT consumed by the shorthand parser.** System fonts (`caption`/`icon`/`menu`/...) are NOT recognized. | |
| [Text](https://www.w3.org/TR/CSS2/text.html) | text-indent | &lt;length&gt; &#124; &lt;percentage&gt; | Specifies the indentation of the first line of text in a block container.  |
| | text-align | left &#124; right &#124; center &#124; start &#124; end &#124; -webkit-center &#124; -moz-center | Specifies the horizontal alignment of text in an element. **`justify` is NOT supported.** This CSS3 module defines properties for text manipulation. |
| | text-decoration | none &#124; [ underline &#124;&#124; line-through ] | Specifies the decoration added to the text | |
| | text-decoration-line | none &#124; [ underline &#124;&#124; line-through ] | Specifies the decoration added to the text | |
| | text-decoration-style | solid | Sets the style of the lines specified by text-decoration-line. **Only `solid` actually applies at runtime** — `double`, `dotted`, `dashed`, `wavy` are parsed but `updateValueTextDecorationStyle` returns `false` for each (the declaration is rejected and the value is dropped). | |
| | text-decoration-color | &lt;color&gt; |  The text-decoration-color CSS property sets the color of the decorative additions to text that are specified by text-decoration-line.  | |
| | text-shadow | none &#124; [ &lt;length&gt;{2,3} && &lt;color&gt;? ]# | Adds shadows to text. It accepts a comma-separated list of shadows to be applied to the text and any of its decorations. Each shadow is described by some combination of X and Y offsets from the element, blur radius, and color. | |
| | text-transform | none &#124; capitalize &#124; uppercase &#124; lowercase | Appears in all-uppercase or all-lowercase, or with each word capitalized. (CSS-wide keywords like `initial`, `inherit`, `unset` work via the cascade — they are not specific to this property.) **`full-width`, `full-size-kana`, `math-auto` are NOT supported.** | |
| | white-space | normal &#124; pre &#124; nowrap &#124; pre-wrap &#124; pre-line | Describes how whitespace inside the element is handled. | |
| | word-spacing | normal &#124; length &#124; initial &#124; inherit | Specifies the spacing behavior between tags and words. | |
| | line-break | auto &#124; normal &#124; loose &#124; strict | Specifies how (or if) to break lines when working with punctuation and symbols. This only affects text in Chinese, Japanese, or Korean (CJK). | At present, loose and strict behaves the same as normal. |
| | hyphens | none &#124; auto | This property controls whether hyphenation is allowed to create more soft wrap opportunities within a line of text. | At present, only none is supported. auto behaves the same as none. |
| [Text](https://www.w3.org/TR/css-text-3/) | overflow-wrap &#124; word-wrap | normal &#124; break-word | Specifies whether the UA may break at otherwise disallowed points within a line to prevent overflow, when an otherwise-unbreakable string is too long to fit within the line box, or when sequences of preserved white space would hang. | It only has an effect when white-space allows wrapping. |
| | word-break | normal &#124; break-all &#124; keep-all | sets whether line breaks appear wherever the text would otherwise overflow its content box. |
| [Table](https://www.w3.org/TR/2011/REC-CSS2-20110607/tables.html#q17.0) | table-layout | fixed &#124; auto | Defines the algorithm to be used to lay out table cells, rows, and columns. | |
| | caption-side | 	top &#124; bottom | Positions the content of a table's &lt;caption&gt; on the specified side. | |
| | border-spacing | 	&lt;length&gt; &lt;length&gt;? | Specifies the distance between the borders of adjacent table cells (only for the separated borders model). | |
| | empty-cells | show &#124; hide | Hide border and background on empty cells in a table. | |
| [Transform](https://www.w3.org/TR/css-transforms-1/) | transform | none &#124; &lt;transform-function&gt;+ | Applies a transformation to an element. **Recognized 2D functions:** `matrix(a,b,c,d,e,f)`, `translate(tx [, ty])`, `translateX(tx)`, `translateY(ty)`, `scale(sx [, sy])`, `scaleX(sx)`, `scaleY(sy)`, `rotate(<angle>)`, `skew(<angle> [, <angle>])`, `skewX(<angle>)`, `skewY(<angle>)`. **Recognized 3D functions:** `matrix3d(<16 numbers>)`, `translate3d(tx, ty, tz)`, `translateZ(tz)`, `scale3d(sx, sy, sz)`, `scaleZ(sz)`, `rotate3d(x, y, z, <angle>)`, `perspective(<length>)`. **`rotateX()`, `rotateY()`, `rotateZ()` are NOT recognized** — use `rotate3d(1,0,0,θ)`, `rotate3d(0,1,0,θ)`, `rotate3d(0,0,1,θ)` instead. **Bug:** `getComputedStyle(el).transform` always returns `"none"` regardless of the actual transform — see "Angles" in the units audit. |
| | transform-origin | &lt;percentage&gt; &#124; &lt;length&gt; &#124; top &#124; right &#124; bottom &#124; left &#124; center | Changes the position of transformed elements | |
| [User Interface](https://www.w3.org/TR/css-ui-4/) | user-select | none &#124; auto  | The user-select property enables authors to specify which elements in the document can be selected by the user and how. |  |
| | caret-color | auto &#124; transparent &#124; currentColor &#124; &lt;color&gt; | The caret-color CSS property sets the color of the insertion caret. | |
| [Functional Notations](https://www.w3.org/TR/css3-values/#functional-notations) | calc / min / max / clamp | refer to spec | Mathematical expressions with `+`, `-`, `*`, `/`, plus `min()`, `max()`, `clamp()`. | Supported on length, time, and angle (and inside `var()` substitution). See "CSS units & functional notations" section below. |
| | var(--name, fallback) | `var(--x [, fallback])` | CSS Custom Properties + `var()` substitution. Nested fallbacks (`var(--a, var(--b, 19px))`) are honored. | Implemented in `src/core/style/CSSVariableSyntaxTreeBuilder.cpp`; combines with `calc()` (e.g. `calc(var(--w) * 2 + 3px)`). |
| | env(name [, fallback]) | — | **NOT supported.** The whole declaration containing `env(...)` is dropped at parse time — even when a literal fallback is supplied. There is no implementation of env() / safe-area-inset-* / titlebar-area-* in `src/core/style/`. | Use a static value or feed the inset via `--my-safe-area: 20px;` at runtime instead of `env(safe-area-inset-top)`. |
| [Media Queries - Media Types](https://www.w3.org/TR/css3-mediaqueries/) | all &#124; screen | all &#124; screen | Describes media types supported by lightweight web engine. | ‘all’ means suitable for all supported devices. |
| [Media Queries - Media Features](https://www.w3.org/TR/css3-mediaqueries/#media1) | width | &lt;length&gt; | Describes the width of the targeted display area of the output device. | |
| | height | &lt;length&gt; | Describes the height of the targeted display area of the output device. | |
| | device-width | &lt;length&gt; | Describes the width of the rendering surface of the output device. | |
| | device-heigth | &lt;length&gt; | Describes the height of the rendering surface of the output device. | |
| | orientation | portrait &#124; landscape| ‘portrait’ when the value of the ‘height’ media feature is greater than or equal to the value of the ‘width’ media feature. Otherwise ‘orientation’ is ‘landscape’. | |
| | aspect-ratio | &lt;ratio&gt; | The ratio of the value of the ‘width’ media feature to the value of the ‘height’ media feature. | |
| | device-aspect-ratio | &lt;ratio&gt; | The ratio of the value of the ‘device-width’ media feature to the value of the ‘device-height’ media feature. | |
| | color | &lt;integer&gt; | Describes the number of bits per color component of the output device. If the device is not a color device, the value is zero. | |
| | color-index | &lt;integer&gt; | Describes the number of entries in the color lookup table of the output device. If the device does not use a color lookup table, the value is zero. | |
| | monochrome | &lt;integer&gt; | Describes the number of bits per pixel in a monochrome frame buffer. If the device is not a monochrome device, the output device value will be 0. | |
| | resolution | &lt;resolution&gt; | Describes the resolution of the output device, i.e. the density of the pixels.  | |
| | scan | progressive &#124; interlace | Describes the scanning process of "tv" output devices. | The lightweight web engine doesn't support this feature. |
| | grid | &lt;integer&gt; | This is used to query whether the output device is grid or bitmap. If the output device is grid-based (e.g., a "tty" terminal, or a phone display with only one fixed font), the value will be 1. Otherwise, the value will be 0. | The lightweight web engine supports only bitmap device. |
| [Media Queries - Media Features](https://drafts.csswg.org/mediaqueries-4/) | hover | none &#124; hover | The 'hover' media feature is used to query the user’s ability to hover over elements on the page with the primary pointing device. | |
| | any-hover | none &#124; hover | The 'any-hover' media feature is identical to the 'hover' media feature, but this corresponds to the union of capabilities of all the pointing devices available to the user. | |
| | pointer | none &#124; fine | The 'pointer' media feature is used to query the presence and accuracy of a pointing device such as a mouse. | |
| | any-pointer | none &#124; fine | The 'any-pointer' media feature is identical to the 'pointer' media feature, but this corresponds to the union of capabilities of all the pointing devices available to the user. | |
| | update | none &#124; fast | The 'update' media feature is used to query the ability of the output device to modify the apearance of content once it has been rendered. | |
| | overflow-block | none &#124; scroll | The 'overflow-block' media feature describes the behavior of the device when content overflows the initial containing block in the block axis. | |
| | overflow-inline | none &#124; scroll | The 'overflow-inline' media feature describes the behavior of the device when content overflows the initial containing block in the inline axis. | |
| [Media Queries - Media Features](https://drafts.csswg.org/mediaqueries-5/) | scripting | none &#124; enabled | The 'scripting' media feature is used to query whether scripting languages, such as JavaScript, are supported on the current document. | |
| [Media Queries - Media Features](https://w3c.github.io/manifest/#the-display-mode-media-feature) | display-mode | browser | The 'display-mode' media feature represents the display mode of the web application. |  |
| [List](https://www.w3.org/TR/CSS2/generate.html#lists) | list-style | &lt;list-style-type&gt; &#124; &lt;list-style-position&gt; &#124; &lt;list-style-image&gt; | Shorthand | |
| | list-style-type | &lt;counter-style&gt; &#124; &lt;string&gt; &#124; none | Specifies the appearance of a list item element. **The CSS parser accepts any custom-identifier** (no validation against a known counter-style list); rendered marker support is limited to the keywords used by `<ul>`/`<ol type=...>`: `disc`, `decimal`, `lower-alpha`, `upper-alpha`, `lower-roman`, `upper-roman`. Other CSS counter styles (`circle`, `square`, `decimal-leading-zero`, `armenian`, `georgian`, `cjk-ideographic`, `katakana`, …) parse without error but the marker glyph fall-back is undefined. String values (`list-style-type: "→ "`) parse and store but the renderer ignores the string in favor of a default bullet. | |
| | list-style-position | inside  &#124; outside | Specifies the position of the marker box in the principal block box. | |
| | list-style-image | &lt;url&gt; &#124; none | Specifies an image to be used as the list item marker. | Development status: experimental |
| | counter-increment | [ &lt;custom-ident&gt; &lt;integer&gt;? ]+ &#124; none | Increases or decreases the value of a CSS counter by a given value. |
| | counter-reset | [ &lt;custom-ident&gt; &lt;integer&gt;? ]+ &#124; none | Resets a CSS counter to a given value. |
| [Box-shadow](https://www.w3.org/TR/css-backgrounds-3/#the-box-shadow) | box-shadow | none &#124; &lt;shadow&gt;# | Attaches one or more drop-shadows to the box. The property accepts either the none value, which indicates no shadows, or a comma-separated list of shadows, ordered front to back. | &lt;shadow&gt; = inset? && &lt;length&gt;{2,4} && &lt;color&gt;? |
| [Will Change](https://drafts.csswg.org/css-will-change/#will-change) | will-change | scroll-position &#124; contents &#124; &lt;custom-ident&gt; | Provide a way for authors to hint browsers about the kind of changes to be expected on an element, so that the browser can set up appropriate optimizations ahead of time before the element is actually changed. | |
| [Animation](https://drafts.csswg.org/css-animations/) | animation-name | none &#124; &lt;keyframes-name&gt;# | Defines a list of animations that apply. Each name is used to select the keyframe at-rule that provides the property values for the animation. | |
| | animation-duration | &lt;time&gt;# | Specifies the length of time that an animation takes to complete one cycle. | |
| | animation-timing-function | &lt;easing-function&gt;# | Describes how the animation will progress between each pair of keyframes. | |
| | animation-iteration-count | &lt;single-animation-iteration-count&gt;# | Specifies the number of times an animation cycle is played. | |
| | animation-direction | &lt;single-animation-direction&gt;# | Defines whether or not the animation should play in reverse on some or all cycles. | |
| | animation-play-state | &lt;single-animation-play-state&gt;# | Defines whether the animation is running or paused. | |
| | animation-delay | &lt;time&gt;# | Defines when the animation will start. | |
| | animation-fill-mode | none &#124; forwards &#124; backwards &#124; both | Defines what styles apply to the animation outside its execution time (before it starts and after it ends). | |
| | animation | &lt;single-animation&gt;# | This shorthand property defines a comma-separated list of animation definitions. | |
| [Transition](https://drafts.csswg.org/css-transitions/) | transition | &lt;single-transition&gt;# | Shorthand for `transition-property` / `transition-duration` / `transition-timing-function` / `transition-delay`. | |
| | transition-property | none &#124; &lt;single-transition-property&gt;# | The CSS properties to which a transition is applied. | |
| | transition-duration | &lt;time&gt;# | Length of time a transition takes to complete. | |
| | transition-timing-function | &lt;easing-function&gt;# | The easing function used during the transition. | |
| | transition-delay | &lt;time&gt;# | Delay before the transition starts. | |

The following properties are also implemented but were missing from earlier revisions of this spec; they are confirmed by the runtime CSS audit (no `Unsupported css property:` warning):

| Category | Property | Allowed Value | Description | Note |
|----------|----------|---------------|-------------|------|
| Box Model     | gap, row-gap, column-gap | &lt;length-percentage&gt; | Modern aliases for grid-gap / grid-row-gap / grid-column-gap; also active in flexbox. | |
| Box Model     | inset | [ &lt;length&gt; &#124; &lt;percentage&gt; &#124; auto ]{1,4} | Logical shorthand for top / right / bottom / left. | |
| Box Model     | margin-block, margin-block-start, margin-block-end, margin-inline, margin-inline-start, margin-inline-end | &lt;length-percentage&gt; &#124; auto | Logical-property forms of `margin-*` (parser-recognized; resolved against `direction`/`writing-mode`). | |
| Box Model     | padding-block, padding-inline, padding-block-start, padding-block-end, padding-inline-start, padding-inline-end | &lt;length-percentage&gt; | Logical-property forms of `padding-*`. | |
| Background    | background-repeat-y | repeat &#124; no-repeat | Y-axis longhand of `background-repeat`. (`background-repeat-x` is the X-axis longhand.) **`space`/`round` not recognized; `repeat-x`/`repeat-y` keywords are valid for the shorthand only.** | |
| Text          | text-underline-position | auto &#124; under | Sets the position of the underline created by `text-decoration-line: underline`. `from-font`, `left`, `right` not recognized. | |
| Layout        | grid-area | &lt;grid-line&gt;{1,4} | Shorthand for `grid-row-start` / `grid-column-start` / `grid-row-end` / `grid-column-end`. | |
| Layout        | grid-template | &lt;grid-template-rows&gt; / &lt;grid-template-columns&gt; | Shorthand combining `grid-template-rows`, `grid-template-columns`, and `grid-template-areas`. | |
| Overflow      | overflow-x, overflow-y | visible &#124; hidden &#124; auto &#124; scroll | Single-axis variants of `overflow`. | |
| Text          | letter-spacing | normal &#124; &lt;length&gt; | Specifies the spacing between adjacent text characters. | |
| Text          | text-overflow | clip &#124; ellipsis | Specifies how overflowed inline-axis text is signalled. Requires `overflow:hidden` and `white-space:nowrap`. | |
| UI            | pointer-events | auto &#124; none &#124; visible &#124; visiblepainted &#124; visiblefill &#124; visiblestroke &#124; painted &#124; fill &#124; stroke &#124; all | Whether the element can be the target of pointer events. **All 10 SVG-style values parse and round-trip via `getComputedStyle`, but hit-testing currently does NOT consult the computed value** — see the runtime caveat below. | |
| UI            | appearance | auto &#124; none | Reset native form-control rendering. | |
| UI            | image-rendering | auto &#124; crisp-edges &#124; pixelated | Hint for image scaling algorithm. | |
| Replaced Content | object-fit | fill &#124; contain &#124; cover &#124; none &#124; scale-down | How a replaced element's content is fitted to its box. | |
| Replaced Content | object-position | &lt;position&gt; | Position of replaced content within its box. | |
| Filter        | filter | none &#124; &lt;filter-function-list&gt; | Applies a graphical filter. **Recognized function names** (`CSSFilterFunction.cpp:31`): `blur()`, `drop-shadow()`, `hue-rotate()`, `brightness()`, `contrast()`, `grayscale()`, `invert()`, `opacity()`, `saturate()`, **`sephia()`** (note: typo — the standard name `sepia()` is NOT recognized; the engine looks for `sephia` only), and `url(#filter-id)` for SVG filter references. **Only `blur()` and the SVG `url(...)` reference actually render**; the others parse but have no visible effect. | |
| Compositing   | mix-blend-mode | &lt;blend-mode&gt; | Defines blending of the element with its backdrop. | |
| Clipping (legacy) | clip | auto &#124; rect(&lt;top&gt;, &lt;right&gt;, &lt;bottom&gt;, &lt;left&gt;) | The deprecated CSS 2.1 `clip` property is parsed (`updateValueClip` accepts `auto` or `rect(...)` with comma- or space-separated values). Modern code should use `clip-path` (limited to `url(#id)`, see below). | |
| Clipping      | clip-path | none &#124; url(#id) | Clips the element to a path. **Only `url(...)` form is accepted by `updateValueClipPath`** — `inset()`, `circle()`, `ellipse()`, `polygon()`, `path()`, `shape()` basic-shape functions are silently rejected. The audit row in the runtime caveats below is the source of truth (earlier docs claiming basic-shape support were incorrect). | |
| Mask          | mask, mask-image, mask-size, mask-position, mask-repeat, mask-type | see CSS Masking 1 | Apply a mask to the element. **`mask-image` accepts only `url(...)` and gradient values**; the `none` keyword is parsed but logged as `Unsupported css property: mask-image with 6` and has no effect. **Per-axis longhands `mask-position-x`/`-y` and `mask-repeat-x`/`-y` are NOT in the parser trie** even though `Style.h` declares them — only the shorthand forms parse. **`mask-mode`, `mask-composite`, `mask-clip`, `mask-origin`, `mask-border` and its longhands are NOT in the parser trie** — they cannot be tuned independently. | |
| Decoration    | box-decoration-break | slice &#124; clone | Slice/clone box decorations across line/page breaks. | |
| List / Text   | line-clamp / -webkit-line-clamp | none &#124; &lt;integer&gt; | Limits text content of a block container to the specified number of lines. Behaves like `-webkit-line-clamp`; requires `display:-webkit-box; -webkit-box-orient:vertical; overflow:hidden`. **Unprefixed `line-clamp` and unprefixed `box-orient` are NOT in the parser trie** — only `-webkit-line-clamp` and `-webkit-box-orient` (gated by `STARFISH_ENABLE_CSS_WEBKIT_LINE_PREFIX` / `…_BOX_PREFIX` respectively) work. | |

> **Note on `cursor`:** The `cursor` property is parsed into an inherited computed-style value (keyword form only — `url()` image cursors are not supported, the engine draws no cursor). The tracked keywords are `auto`, `default`, `pointer`, `none`, and all other CSS cursor keywords (collapsed to a single `CursorOtherValue` that serializes back to `auto` via `getComputedStyle`, since the concrete keyword is not retained). The `pointer` value drives the tap-sound (link effect) feedback on Tizen; an author setting any non-`pointer` keyword cleanly overrides an inherited `pointer`. No actual cursor is rendered.

### CSS Scrolling / Overflow

Verified against `src/core/style/CSSStyleLookupTrie.cpp`, `src/core/style/Style.cpp::updateValueOverflowX/Y`, `src/core/dom/Element.cpp` (programmatic scroll APIs), and runtime probes (`getComputedStyle` + `scrollTo`/`scrollIntoView` round-trip).

**Supported overflow keywords.** `overflow`, `overflow-x`, `overflow-y` accept `visible`, `hidden`, `auto`, `scroll`. Per-axis values round-trip through inline style and `getComputedStyle`. When one axis is set to `visible` while the other is non-`visible`, the non-`visible` axis correctly remaps the `visible` axis to `auto` at computed-style time (per CSS Overflow 3 §3).

**`overflow: clip` is unsupported.** A single-value `overflow: clip` declaration is silently rewritten to `hidden` at the parser; the inline style stores the empty string and `getComputedStyle` returns `"hidden"`. No warning is logged for this rewrite (unlike the other unsupported scroll properties below). `overflow-clip-margin` is fully unsupported — the declaration is dropped and `Unsupported css property: overflow-clip-margin` is logged.

**Two-value `overflow` shorthand is unsupported.** `overflow: hidden auto` (separate `overflow-x` / `overflow-y`) parses but only the first keyword is consumed; both axes end up with the first value. Author code that needs different per-axis behaviour must use the longhand `overflow-x` / `overflow-y` properties explicitly.

**Scroll module CSS properties are entirely unsupported.** Each of the following parses-fail at `CSSStyleDeclaration::operator()(129)` with `Unsupported css property: <name>`; the inline-style string is empty after assignment and `getComputedStyle` returns the empty string:

- `overflow-anchor` (`auto`/`none`)
- `overflow-clip-margin`
- `scroll-behavior` (`auto`/`smooth`)
- `scroll-snap-type`, `scroll-snap-align`, `scroll-snap-stop`
- `scroll-padding`, `scroll-padding-{top,right,bottom,left,block,inline}`
- `scroll-margin`, `scroll-margin-{top,right,bottom,left,block,inline}`
- `scrollbar-width` (`auto`/`thin`/`none`), `scrollbar-color`, `scrollbar-gutter`
- `overscroll-behavior`, `overscroll-behavior-x`, `overscroll-behavior-y`

**Programmatic scrolling — APIs work, `behavior:'smooth'` is instant.** `Element.scrollTop`, `scrollLeft`, `scrollWidth`, `scrollHeight` are reflected. `Element.scroll(...)`, `scrollTo(...)`, `scrollBy(...)`, `scrollIntoView(...)` accept both numeric and `ScrollOptions`/`ScrollIntoViewOptions` dictionary forms (`behavior`, `block`, `inline`, `top`, `left`). The `Window` equivalents (`scroll`, `scrollTo`, `scrollBy`) likewise work. **However** `behavior: 'smooth'` is treated as `'instant'` — the scroll position jumps to the target on the same tick (verified by reading `scrollTop` immediately after the call). The `ScrollBehavior::Smooth` enum exists in `src/core/page/ScrollOptions.h` but no animator is wired up in `Element.cpp` / `Window.cpp`. Web apps that require visible easing must implement their own `requestAnimationFrame` loop on `scrollTop`.

**`scroll` event fires.** Both `addEventListener('scroll', ...)` on the scrolling element and on `window` are dispatched after `scrollTo`/`scrollBy`/`scrollIntoView`/manual user scroll — one `scroll` event per programmatic scroll call.

### CSS units & functional notations

Verified against `src/core/style/CSSStyleLookupTrie.cpp::lookupUnitType`, `src/core/style/CSSParser.h::parseNonNamedColor`, and runtime probes (`getComputedStyle` + style-rule round-trip).

**Length / size units.** Supported: `px`, `em`, `rem`, `ex`, `ch`, `pt`, `pc`, `cm`, `mm`, `in`, `vw`, `vh`, `vmin`, `vmax`, `%`, `fr` (grid-tracks only). NOT supported: `q`/`Q` (quarter-millimeter) — the trie has no entry for `q`, so the entire declaration is silently dropped (the rule serializes back as empty). New viewport units `svw`/`lvw`/`dvw`/`svh`/… and container units (`cqw`, `cqi`, `cqb`, …) are likewise absent.

**Angles.** Parsing accepts `deg`, `rad`, `turn`, `grad` (`CSSStyleLookupTrie` returns the right `UnitType`). However, `getComputedStyle(el).transform` always returns `"none"` regardless of the rotate angle — i.e. the layout side does not synthesize the matrix at computed-style read time. The underlying rule is preserved (visible via `cssRules[i].cssText`), and rendering uses it; only the JS-visible computed value is missing.

**Times.** `s` and `ms` are supported on `transition-duration`/`animation-duration` and round-trip correctly; `calc(<time>)` works.

**Resolution.** `dpi`, `dpcm`, `dppx` are all recognized inside `@media (min-resolution: …)`. Confirmed: `96dpi`, `37dpcm`, `1dppx` each match an `@media` branch on a 1× display.

**`calc()` / `min()` / `max()` / `clamp()`.** All four function names are accepted (`Style.cpp::updateValueUnitCalc`). Mixed-unit `calc(50% - 10px)` resolves correctly; nested `calc(calc(10px+5px)*2)` resolves to 30px; `calc(var(--len) * 2 + 3px)` resolves correctly through `var()` substitution.

**`var()`.** Custom properties + fallback work, including nested fallback (`var(--a, var(--b, 19px))`). Combination with `calc()` works. The primary substitution path is reliable; see "CSS Custom Properties (`--*` / `var()`) — runtime caveats" below for the cascade edge cases that aren't (indirect-cycle hangs, dropped `!important`, empty `cssText`).

**`env()`.** **Completely unimplemented.** `env(safe-area-inset-top)` is stripped during parsing — the rule body is left empty (`#x { }`). The literal fallback (`env(safe-area-inset-top, 11px)`) is also discarded; the engine does *not* fall back to it. There is no `env()` token consumer anywhere in `src/core/style/`. Webapps must avoid `env()` entirely and inline the safe-area inset (e.g. via a CSS Custom Property the host app injects).

**Color formats.** `parseNonNamedColor` only recognizes `rgb()`, `rgba()`, `hsl()`, `hsla()` (modern slash/space syntax `rgb(R G B / A)` works) and hex literals `#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa`. Named colors (incl. `rebeccapurple`), `transparent`, and `currentColor` work. `hwb()`, `lab()`, `lch()`, `oklab()`, `oklch()`, `color()`, and `color-mix()` all parse-fail and the property falls back to its initial value (`rgb(0,0,0)`); the rule body retains the unparsable text but the value is unused.

## Obsolete CSS

This section describes the list of obsolete CSS properties.
To use these features, you need to define specific string when compile. the string is described below.

| String you need to define to use property | Property | Note |
|-------------------------------------------|----------|------|
| STARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX | display | -webkit-flex &#124; -webkit-inline-flex are supported |
| | -webkit-flex | alias of flex |
| | -webkit-flex-direction | alias of flex-direction |
| | -webkit-flex-wrap | alias of flex-wrap |
| | -webkit-flex-flow | alias of flex-flow |
| | -webkit-flex-grow | alias of flex-grow |
| | -webkit-flex-shrink | alias of flex-shrink |
| | -webkit-flex-basis | alias of flex-basis |
| | -webkit-align-items | alias of align-items |
| | -webkit-align-content | alias of align-content |
| | -webkit-align-self | alias of align-self |
| | -webkit-order | alias of order |
| | -webkit-justify-content | alias of justify-content |
| STARFISH_ENABLE_CSS_WEBKIT_TRANSFORM_PREFIX | -webkit-transform | alias of transform |
| | -webkit-transform-origin | alias of transform-origin |
| STARFISH_ENABLE_CSS_WEBKIT_TRANSITION_PREFIX | -webkit-transition | alias of transition |

## Selectors

This section describes the complete list of supported selectors by LWE.

| Selectors | Type | Pattern | Usage | Description |
|-----------|------|---------|-------|-------------|
| [Logical combinators](https://www.w3.org/TR/selectors/#logical-combination) | The negation pseudo-class | :not() | :not(p) | Selects elements that do not match a list of selectors |
| [Elemental selectors](https://www.w3.org/TR/selectors/#elemental-selectors) | Type (tag name) selector | element | p | Selects all \<p\> elements. The 'OR' condition is allowed (e.g., element, element) |
| | Universal selector | * | * | Selects all elements |
| [Attribute selectors](https://www.w3.org/TR/selectors/#attribute-selectors) | Attribute presence and value selectors | [att] | [target] | Selects all elements with a target attribute |
| | | [att=val] | [lang=en] | Selects all elements with lang="en" |
| | | [att~=val] | [title~=flower] | Selects all elements with a title attribute containing the word "flower" |
| | | [att&#124;=val] | [lang&#124;=en] | Selects all elements with a lang attribute value starting with "en" |
| | Case-insensitive attribute flag | [att=val i] | input[type="email" i] | The `i` flag forces case-insensitive matching (CSS Selectors Level 4). Supported by `getAttributeFlags`. **`s` flag (force case-sensitive in HTML)** is NOT supported. |
| | Substring matching attribute selectors | [att^=val] | a[href^="https"] | Selects every \<a\> element whose href attribute value begins with "https" |
| | | [att$=val] | a[href$=".pdf"] | Selects every \<a\> element whose href attribute value ends with ".pdf" |
| | | [att*=val] | a[href*="w3schools"] | Selects every \<a\> element whose href attribute value contains the substring "w3schools" |
| | Class selector | element.class | div.intro | Selects all \<div\> elements with class="intro". A subset matching of "class" values is not allowed (for example, div.class1.class2) |
| | ID selector | element#id | div#firstname | Selects an \<div\> element with id="firstname" |
| [Linguistic pseudo-classes](https://www.w3.org/TR/selectors/#linguistic-pseudos) | The directionality pseudo-class | :dir() | :dir(ltr) | Selects any element with left-to-right text <br> NOTE: 'rtl' value is not supported yet |
| | The language pseudo-class | :lang() | p:lang(it) | Selects every \<p\> element with a lang attribute equal to "it" (Italian) |
| [Location pseudo-classes](https://www.w3.org/TR/selectors/#location) | The link pseudo-class | :link | a:link | Selects an element that has not yet been visited |
| | The target pseudo-class | :target | #news:target | Selects the current active #news element (clicked on a URL containing that anchor name) |
| [User action pseudo-classes](https://www.w3.org/TR/selectors/#useraction-pseudos) | The pointer hover pseudo-class | :hover | a:hover | Selects links on mouse over |
| | The activation pseudo-class | :active | a:active | Selects the active link |
| | The input focus pseudo-class | :focus | input:focus | Selects the input element which has focus |
| [The input pseudo-classes](https://www.w3.org/TR/selectors/#input-pseudos) |  The ':enabled' pseudo-classes | :enabled | input:enabled | Selects every enabled \<input\> element |
| | The ':disabled' pseudo-classes | :disabled | input:disabled | Selects every disabled \<input\> element |
| | The placeholder-shown pseudo-class | :placeholder-shown | :placeholder-shown | Selects any \<input\> or \<textarea\> element that is currently displaying placeholder text. |
| | The selected-option pseudo-class | :checked | input:checked | Selects any radio(\<input type="radio"\>), checkbox (\<input type="checkbox"\>), or option(\<option\> in a \<select\>) element that is checked or toggled to an on state |
| [Tree-structural pseudo-classes](https://www.w3.org/TR/selectors/#structural-pseudos) | ':root' pseudo-class | :root | :root | Selects the document's root element |
| | ':empty' pseudo-class | :empty | p:empty | Selects any element that has no children |
| | 'nth-child()' pseudo-class  | :nth-child() | p:nth-child(2) | Selects every \<p\> element that is the second child of its parent |
| | ':nth-last-child()' pseudo-class | :nth-last-child() | p:nth-last-child(2) | Selects every \<p\> element that is the second child of its parent, counting from the last child |
| | ':first-child' pseudo-class | :first-child | p:first-child | Selects every \<p\> element that is the first child of its parent |
| | ':last-child' pseudo-class | :last-child | p:last-child | Selects every \<p\> element that is the last child of its parent |
| | ':only-child' pseudo-class | :only-child | p:only-child | Selects every \<p\> element that is the only child of its parent |
| | ':nth-of-type()' pseudo-class | :nth-of-type() | p:nth-of-type(2) | Selects every \<p\> element that is the second \<p\> element of its parent |
| | ':nth-last-of-type()' pseudo-class | :nth-last-of-type() | p:nth-last-of-type(2) | 	Selects every \<p\> element that is the second \<p\> element of its parent, counting from the last child |
| | ':first-of-type' pseudo-class | :first-of-type | p:first-of-type | Selects every \<p\> element that is the first \<p\> element of its parent |
| | ':last-of-type' pseudo-class | :last-of-type | p:last-of-type | Selects every \<p\> element that is the last \<p\> element of its parent |
| | ':only-of-type' pseudo-class | :only-of-type | p:only-of-type | Selects every \<p\> element that is the only \<p\> element of its parent |
| [Reference selectors](https://www.w3.org/TR/selectors-4/#scoping) | ':scope' pseudo-class | :scope | :scope > .child | Matches the scoping root (`Element.querySelector(...)` call site, otherwise `documentElement`). |
| [Custom-element pseudo-classes](https://drafts.csswg.org/selectors-4/#custom-pseudo) | ':defined' pseudo-class | :defined | a:defined | Matches any element the parser maps to a known `HTMLxxxElement` (everything except `HTMLUnknownElement`). LWE has no Custom Elements registry, so this is effectively "is the tag in the parser's known list?". |
| [Shadow DOM pseudo-classes](https://drafts.csswg.org/css-scoping/#host-selector) | ':host' / ':host()' pseudo-classes | :host, :host(...) | :host(.themed) | Matches the shadow host from within a shadow tree, including via `adoptedStyleSheets` on the `ShadowRoot`. |
| [Combinators](https://www.w3.org/TR/selectors/#combinators) | Descendant combinator ( ) | selector1 selector2 | div p | Selects all \<p\> elements inside \<div\> elements |
| | Child combinator (>) | selector1 > selector2 | div > p | Selects all \<p\> elements that are immediate children of a \<div\> element |
| | Next-sibling combinator (+) | selector1 + selector2 | div + p | Selects all \<p\> elements that are placed immediately after \<div\> elements |
| | Subsequent-sibling combinator (~) | selector1 ~ selector2 | div ~ p | Selects all \<p\> elements that are siblings of \<div\> elements |
| [Typographic Pseudo-elements](https://www.w3.org/TR/css-pseudo-4/#typographic-pseudos) | The ::first-line pseudo-element | ::first-line | p::first-line | Selects the first line of every \<p\> element|
| | The ::first-letter pseudo-element | ::first-letter | p::first-letter | Selects the first letter of every \<p\> element |
| [Tree-Abiding Pseudo-elements](https://www.w3.org/TR/css-pseudo-4/#treelike) | Generated Content Pseudo-elements: '::before' | ::before | p::before | Insert something before the content of each \<p\> element |
| | Generated Content Pseudo-elements: '::after' | ::after | p::after | Insert something after the content of each \<p\> element |

### Selector caveats

The following selectors are **parsed without error but do not actually match anything** at style time (the runtime emits `Style.cpp: checkPseudoClass: Unsupported css pseudo-element: <N>`). They should NOT be relied on:

`:any-link`, `:focus-visible`, `:focus-within`, `:in-range`, `:out-of-range`, `:indeterminate`, `:invalid`, `:valid`, `:optional`, `:required`, `:read-only`, `:read-write`, `:target-within`, `:visited`. Use `:focus` instead of `:focus-visible`/`:focus-within`; for form-validation states, query the underlying state in JS.

The `::marker` pseudo-element is likewise parsed without error but never styles the list marker; only the pseudo-elements listed in the table above (`::before`, `::after`, `::first-line`, `::first-letter`) actually generate or style boxes.

`:is(...)` and `:where(...)` are implemented, and `:not(...)` accepts a full selector list. `:has(...)` is **not** implemented and raises `SyntaxError` at parse time — rewrite it using a regular descendant or compound selector.

`:scope` is supported and matches `:root` in document context.
`:dir(ltr)` matches; `:dir(rtl)` parses but never matches because LWE has only LTR direction infrastructure.
Shadow-DOM-related selectors (`:host`, `:host(...)`, `::slotted(...)`, `:defined`) are implemented and match inside shadow trees.

### @-rules

The CSS section above does not enumerate at-rules. Implementation status:

| @-rule | Status |
|--------|--------|
| `@import` | Supported. Forms: `@import url("a.css");`, `@import url("a.css") <media-query>;`. **The CSS Cascade 5 `supports(<condition>)` clause** (`@import url(...) supports(<cond>) <media-query>;`) is **NOT specifically parsed** — the `supports(...)` function tokens are consumed as media query text and the import is loaded unconditionally. |
| `@media` | Supported. |
| `@font-face` | Supported (descriptors `unicode-range` and `font-display` are NOT). |
| `@keyframes` | Supported. **`@-webkit-keyframes` is NOT recognized** — only the unprefixed `@keyframes` token is in `CSSParser::parseAtRule`. (Earlier docs claiming the prefixed form worked were incorrect.) |
| `@supports` | Parsed via `parseSupportsRule`. Condition evaluation works for the basic `<feature> := (prop: value)` form (a declaration is "supported" iff `parseDeclaration` produces non-empty `cssText`), and the boolean operators `and` / `or` / `not` plus parenthesized groups are honored (`m_supportOperand`/`m_supportOperator` stacks). **However:** the JS-side `CSS.supports(prop, value)` returns `false` for many valid declarations (already documented under CSS Houdini), so do not assume @supports and `CSS.supports()` agree. The general-enclosed `<supports-feature>` fallback (function syntax: `selector(...)`, `font-tech(...)`, `font-format(...)`) is recognized as the catch-all but does not check actual support. |
| `@namespace` | Implemented. Prefixes declared here are resolved by the parser and drive namespace-aware type selector matching (`ns\|E`, `*\|E`, `\|E`, `ns\|*`), including the default-namespace rule. Placement validity (after `@charset`/`@import`, before style rules) is enforced, and the rule is exposed as a `CSSNamespaceRule`. |
| `@charset` | Parsed at the top of a stylesheet only; affects byte-level decoding (must be the very first rule, no whitespace before). |
| `@counter-style` | **Recognized as an at-rule by the dispatcher**, but `parseCounterStyleRule` is a `// TODO` stub returning `nullptr` — the rule is silently dropped. |
| `@page` | **Not parsed** — falls through `addUnknownAtRule()`. No rendering effect (LWE has no print pipeline). |
| `@layer`, `@container`, `@scope`, `@viewport`, `@document`, `@font-feature-values`, `@color-profile`, `@property`, `@view-transition`, `@position-try`, `@starting-style`, `@nest` | **Not supported.** Silently skipped by the parser (`addUnknownAtRule()`). |

### @media query features

`window.matchMedia(query)` and `<style>@media (...) { ... }</style>` use the same evaluator. Recognized features (parser table at `src/core/style/CSSParser.h:1598`):

`width`, `height`, `aspect-ratio`, `orientation`, `resolution`, `device-width`, `device-height`, `device-aspect-ratio`, `color`, `color-index`, `monochrome`, `grid`, `hover`, `any-hover`, `pointer`, `any-pointer`, `update`, `display-mode`, `overflow-block`, `overflow-inline`, `scan`, `scripting` (all support the `min-`/`max-` form where applicable).

| Feature | Status |
|---------|--------|
| `(min-width: ...)` / `(max-width: ...)`, similarly for height/resolution/aspect-ratio | **Implemented**, viewport-driven. |
| `(orientation: landscape\|portrait)` | Implemented. |
| `(resolution: ...dppx\|dpi\|dpcm)`, `(min-resolution: 96dpi)` | Implemented (uses `devicePixelRatio`). |
| `(scripting: enabled)` | Implemented (returns `enabled`/`none` based on script-engine presence; `initial-only` not modelled). |
| `(grid: 0)` | Implemented (always `0` — bitmap UA). |
| `(hover: hover)`, `(any-hover: hover)` | **Hard-coded** to `hover` regardless of device. `(hover: none)` always false. |
| `(pointer: fine)`, `(any-pointer: fine)` | **Hard-coded** to `fine`. `(pointer: coarse)`/`(pointer: none)` always false — touch-only TVs cannot be detected. |
| `(update: fast)` | **Hard-coded** to `fast`. |
| `(overflow-block: scroll)`, `(overflow-inline: scroll)` | **Hard-coded** to `scroll`. |
| `(display-mode: browser)` | **Hard-coded** to `browser`. PWA modes (`fullscreen`/`standalone`/`minimal-ui`) not detectable. |
| **NOT recognized** (parser drops the entire `@media` block; `matchMedia` returns `false` for every value): | `prefers-color-scheme`, `prefers-reduced-motion`, `prefers-reduced-data`, `prefers-reduced-transparency`, `prefers-contrast`, `forced-colors`, `device-pixel-ratio`, `-webkit-device-pixel-ratio`, `dynamic-range`, `video-dynamic-range`, `inverted-colors`, `nav-controls`, `color-gamut`, `environment-blending`. |

> **Footgun:** since the missing `prefers-*` features are silently dropped, `if (matchMedia('(prefers-reduced-motion: no-preference)').matches)` returns `false` on LWE — feature-detection patterns that gate behavior on the spec-default sentinel will silently disable themselves. Either (a) treat `matches === false` as "no preference / proceed" instead of "user opted out," or (b) feature-detect via something else.

### CSS units & functional notations (quick reference)

| Category | Supported | Not supported |
|----------|-----------|---------------|
| Length | `px`, `em`, `rem`, `ex`, `ch`, `pt`, `pc`, `cm`, `mm`, `in`, `vw`, `vh`, `vmin`, `vmax`, `%`, `fr` (grid only) | `q`/`Q`, container-relative units (`cqw`/`cqi`/`cqb`/...), small/large/dynamic viewport units (`svw`/`lvw`/`dvw`/`svh`/...) |
| Angle | `deg`, `rad`, `turn`, `grad` (parsed; rules retained) | — but `getComputedStyle.transform` always returns `"none"` (the computed-style serializer is missing for transforms; rendering still works) |
| Time | `s`, `ms` | — |
| Resolution | `dpi`, `dpcm`, `dppx` (work in `@media`) | — |
| Functional | `calc()`, `min()`, `max()`, `clamp()` (mixed units, nested, with `var()`); `var(--x, fallback)` including nested fallback | **`env(...)` is parser-rejected** — the entire declaration is dropped at parse time; the fallback value is **NOT** honored. |
| Color | hex 3/4/6/8, `rgb()`, `rgba()`, `rgb(R G B / A)` modern syntax, `hsl()`, `hsla()`, named colors (incl. `rebeccapurple`), `transparent`, `currentColor` | `hwb()`, `lab()`, `lch()`, `oklab()`, `oklch()`, `color()`, `color-mix()` — all silently fall back to `rgb(0,0,0)`. |

## Cross-origin script API accessSection

JavaScript APIs like iframe.contentWindow, window.parent, window.open, and window.opener allow documents to directly reference each other. When two documents do not have the same origin, these references provide very limited access to Window and Location objects, as described in the next two sections.
To communicate between documents from different origins, use window.postMessage.

The following cross-origin access to these properties is allowed:

| Interface | Type | Name | Description |
|-----------|------|------|-------------|
| Window | method | focus | |
| | method | blur | |
| | method | postMessage | |
| | attribute | frames | read only |
| | attribute | length | read only |
| | attribute | top | read only |
| | attribute | location | read/write |
| Location | method | replace ||
| | attribute | href | write only |

## HTTP

### Cross-Origin Resource Sharing

#### The HTTP response headers

This section describes the HTTP response headers that the server responds to for access control, as defined by the [Cross-Origin Resource Sharing](https://www.w3.org/TR/cors/) specification.

#### Supported HTTP response headers
| Name      | Description | Note |
|-----------|-------------|------|
| [Access-Control-Allow-Origin](https://fetch.spec.whatwg.org/#http-access-control-allow-origin) | Indicates whether the response can be shared, via returning the literal value of the `Origin` request header (which can be `null`) or `*` in a response. | |
| [Access-Control-Allow-Credentials](https://fetch.spec.whatwg.org/#http-access-control-allow-credentials) | Indicates whether the response can be shared when request’s credentials mode is "include". | |
| [Access-Control-Allow-Methods](https://fetch.spec.whatwg.org/#http-access-control-allow-methods) | Indicates which methods are supported by the response’s URL for the purposes of the CORS protocol. | |
| [Access-Control-Expose-Headers](https://fetch.spec.whatwg.org/#http-access-control-expose-headers) | Indicates which headers can be exposed as part of the response by listing their names. | |

#### The HTTP request headers
This section describes the HTTP request headers that the user-agent request to make use of the cross-origin sharing feature, as defined by the [Cross-Origin Resource Sharing](https://www.w3.org/TR/cors/) specification.

| Name      | Description | Note |
|-----------|-------------|------|
| [Access-Control-Request-Method](https://fetch.spec.whatwg.org/#http-access-control-request-method) | Indicates which method a future CORS request to the same resource might use. | |
| [Access-Control-Request-Headers`](https://fetch.spec.whatwg.org/#http-access-control-request-headers) | Indicates which method a future CORS request to the same resource might use. | |

#### Preflighted requests
[Preflighted requests](https://fetch.spec.whatwg.org/#cors-preflight-fetch) is a CORS request that checks to see if the CORS protocol is understood. It uses `OPTIONS` as method and includes above request headers

#### X-Frame-Options
The [X-Frame-Options](https://tools.ietf.org/html/rfc7034) HTTP response header can be used to indicate whether or not a browser should be allowed to render a page in a \<frame\>, \<iframe\>, \<embed\> or \<object\> .

| Directive      | Description | Note |
|----------------|-------------|------|
| [deny]() | The page cannot be displayed in a frame, regardless of the site attempting to do so. | |
| [sameorigin]() | The page cannot be displayed in a frame, regardless of the site attempting to do so. | |
| [allow-from uri]() | The page cannot be displayed in a frame, regardless of the site attempting to do so. | |

Note: CORS handling is wired through the network stack and runs for `XMLHttpRequest` and the (partial) `fetch()` implementation. The `fetch()` global is exposed but parts of the Fetch spec (streaming response bodies, `Request.body.getReader()`) remain incomplete; for production paths prefer `XMLHttpRequest`.

### Content Security Policy

This section describes the list of supported `Directives` and their corresponding `Sources` of Content Security Policies. To enable CSP, configuring a policy via [Content-Security-Policy HTTP header](https://www.w3.org/TR/CSP2/#content-security-policy-header-field) or [HTML meta Element](https://www.w3.org/TR/CSP2/#delivery-html-meta-element) is required.

#### Supported Directives

| Directive      | Description | Note |
|----------------|-------------|------|
| [base-uri](https://www.w3.org/TR/CSP2/#directive-base-uri) | The base-uri directive restricts the URLs that can be used to specify the document base URL. | |
| [child-src](https://www.w3.org/TR/CSP2/#directive-child-src) | The child-src directive governs the creation of nested browsing contexts (e.g. iframe and frame navigations). | |
| [connect-src](https://www.w3.org/TR/CSP2/#directive-connect-src) | The connect-src directive restricts which URLs the protected resource can load using script interfaces. | |
| [default-src](https://www.w3.org/TR/CSP2/#directive-default-src) | The default-src directive sets a default source list for a number of directives. | |
| [form-action](https://www.w3.org/TR/CSP2/#directive-form-action) | The form-action restricts which URLs can be used as the action of HTML form elements. | |
| [frame-src](https://www.w3.org/TR/CSP2/#directive-frame-src) | The frame-src directive restricts from where the protected resource can embed frames. | |
| [img-src](https://www.w3.org/TR/CSP2/#directive-img-src) | The img-src directive restricts from where the protected resource can load images. | |
| [media-src](https://www.w3.org/TR/CSP2/#directive-media-src) | The media-src directive restricts from where the protected resource can load video, audio, and associated text tracks. | |
| [script-src](https://www.w3.org/TR/CSP2/#directive-script-src) | The script-src directive restricts which scripts the protected resource can execute. | |
| [style-src](https://www.w3.org/TR/CSP2/#directive-style-src) | The style-src directive restricts which styles the user may applies to the protected resource. | |

#### Supported Sources

| Source      | Description | Note |
|----------------|-------------|------|
| [\<host-source\>](https://www.w3.org/TR/CSP2/#source-list-syntax) | Internet hosts by name or IP address, as well as an optional URL scheme and/or port number. The site's address may include an optional leading wildcard (the asterisk character, '\*'), and you may use a wildcard (again, '\*') as the port number, indicating that all legal ports are valid for the source. | |
| [\<scheme-source\>](https://www.w3.org/TR/CSP2/#source-list-syntax) | A schema such as 'http:' or 'https:'. *The colon is required, single quotes shouldn't be used.* You can also specify data schemas (not recommended). | |
| ['self'](https://www.w3.org/TR/CSP2/#source-list-syntax) | Refers to the origin from which the protected document is being served, including the same URL scheme and port number. You must include the single quotes. | |
| ['unsafe-inline'](https://www.w3.org/TR/CSP2/#source-list-syntax) | Allows the use of inline resources, such as inline \<script\> elements, javascript: URLs, inline event handlers, and inline \<style\> elements. You must include the single quotes. | |
| ['unsafe-eval'](https://www.w3.org/TR/CSP2/#source-list-syntax) | Allows the use of `eval()` and similar methods for creating code from strings. You must include the single quotes. | |
| ['none'](https://www.w3.org/TR/CSP2/#source-list-syntax) | Refers to the empty set; that is, no URLs match. The single quotes are required. | |
| ['nonce-\<base64-value\>'](https://www.w3.org/TR/CSP2/#source-list-syntax) | A whitelist for specific inline scripts using a cryptographic nonce. Specifying nonce will ignore 'unsafe-inline'. | |
| ['\<hash-algorithm\>-\<base64-value\>'](https://www.w3.org/TR/CSP2/#source-list-syntax) | A sha256, sha384 or sha512 hash of scripts or styles. The use of this source consists of two portions separated by a dash: the encryption algorithm used to create the hash and the base64-encoded hash of the script or style. | |

## SVG
This section describes SVG support in LWE. Inline `<svg>...</svg>` in
HTML, standalone `.svg` documents (`image/svg+xml`), and SVG-as-image
(`<img src="*.svg">`, CSS `background-image: url(*.svg)`) are all
rendered. 45 element subclasses are registered in
`SVGDocument::createSVGElement`, each with a dedicated `SVG*Element`
C++ class and a corresponding `FrameSVG*Box` layout box. Please see
[SVG 2 Spec](https://www.w3.org/TR/SVG2/) for more information.

### Supported elements

| Element | Note |
|---------|------|
| [svg](https://www.w3.org/TR/SVG2/struct.html#NewDocument) | Root element. Supports `width`, `height`, `viewBox`, `preserveAspectRatio`. Nested `<svg>` creates a new viewport context. |
| [g](https://www.w3.org/TR/SVG2/struct.html#Groups) | Grouping element. |
| [defs](https://www.w3.org/TR/SVG2/struct.html#Head) | Non-rendered container for reusable definitions. |
| [use](https://www.w3.org/TR/SVG2/struct.html#UseElement) | Clones a referenced element via `href`/`xlink:href`. Resolved through an internal shadow root. |
| [symbol](https://www.w3.org/TR/SVG2/struct.html#SymbolElement) | Defines a reusable graphic, instantiated by `<use>`. |
| [switch](https://www.w3.org/TR/SVG2/struct.html#SwitchElement) | Conditional rendering of child elements. |
| [rect](https://www.w3.org/TR/SVG2/shapes.html#RectElement) | Supports `x`, `y`, `width`, `height`, `rx`, `ry`. |
| [circle](https://www.w3.org/TR/SVG2/shapes.html#CircleElement) | Supports `cx`, `cy`, `r`. |
| [ellipse](https://www.w3.org/TR/SVG2/shapes.html#EllipseElement) | Supports `cx`, `cy`, `rx`, `ry`. |
| [line](https://www.w3.org/TR/SVG2/shapes.html#LineElement) | Supports `x1`, `y1`, `x2`, `y2`. |
| [polyline](https://www.w3.org/TR/SVG2/shapes.html#PolylineElement) | Supports `points`. |
| [polygon](https://www.w3.org/TR/SVG2/shapes.html#PolygonElement) | Supports `points`. |
| [path](https://www.w3.org/TR/SVG2/shapes.html#PathElement) | Full path data (`M`, `L`, `C`, `Q`, `A`, `Z`, etc.). |
| [image](https://www.w3.org/TR/SVG2/embedded.html#ImageElement) | Embeds raster image via `href`/`xlink:href`. |
| [text](https://www.w3.org/TR/SVG2/text.html#TextElement) | Supports `x`, `y`, `dx`, `dy`, `fill`, `font-*`. |
| [tspan](https://www.w3.org/TR/SVG2/text.html#TextElement) | Sub-text within `<text>`. |
| [linearGradient](https://www.w3.org/TR/SVG2/pservers.html#LinearGradients) | Supports `x1`, `y1`, `x2`, `y2`, `gradientUnits`, `gradientTransform`, `spreadMethod`. |
| [radialGradient](https://www.w3.org/TR/SVG2/pservers.html#RadialGradients) | Supports `cx`, `cy`, `r`, `fx`, `fy`, `fr`, `gradientUnits`. |
| [stop](https://www.w3.org/TR/SVG2/pservers.html#GradientStops) | Gradient stop with `offset`, `stop-color`, `stop-opacity`. |
| [clipPath](https://www.w3.org/TR/SVG2/paths.html#ClippingPaths) | Clipping path. Supports `clipPathUnits`. |
| [mask](https://www.w3.org/TR/SVG2/masking.html#MaskElement) | Supports `maskUnits`, `maskContentUnits`, `x`, `y`, `width`, `height`. |
| [marker](https://www.w3.org/TR/SVG2/painting.html#MarkerElement) | Supports `markerWidth`, `markerHeight`, `refX`, `refY`, `orient`, `markerUnits`. |
| [script](https://www.w3.org/TR/SVG2/interact.html#ScriptElement) | Executes script within SVG context. |
| [style](https://www.w3.org/TR/SVG2/styling.html#StyleElement) | Embedded CSS within SVG. |

### Filter primitives

SVG filter elements are registered with dedicated IDL interfaces and
rendered. The `<filter>` container supports `filterUnits`,
`primitiveUnits`, `x`, `y`, `width`, `height`.

| Element | Note |
|---------|------|
| [feGaussianBlur](https://drafts.csswg.org/filter-effects-1/#feGaussianBlurElement) | `stdDeviation`, `edgeMode` (`duplicate`/`wrap`/`none`). |
| [feColorMatrix](https://drafts.csswg.org/filter-effects-1/#feColorMatrixElement) | `type` (`matrix`/`saturate`/`hueRotate`/`luminanceToAlpha`), `values`. |
| [feComponentTransfer](https://drafts.csswg.org/filter-effects-1/#feComponentTransferElement) | With `<feFuncR>`, `<feFuncG>`, `<feFuncB>`, `<feFuncA>` children. |
| [feComposite](https://drafts.csswg.org/filter-effects-1/#feCompositeElement) | `operator` (`over`/`in`/`out`/`atop`/`xor`/`arithmetic`), `k1`–`k4`. |
| [feFlood](https://drafts.csswg.org/filter-effects-1/#feFloodElement) | `flood-color`, `flood-opacity`. |
| [feMerge](https://drafts.csswg.org/filter-effects-1/#feMergeElement) | With `<feMergeNode>` children referencing filter results. |
| [feMorphology](https://drafts.csswg.org/filter-effects-1/#feMorphologyElement) | `operator` (`erode`/`dilate`), `radius`. |
| [feOffset](https://drafts.csswg.org/filter-effects-1/#feOffsetElement) | `dx`, `dy`. |
| [feTurbulence](https://drafts.csswg.org/filter-effects-1/#feTurbulenceElement) | `baseFrequency`, `numOctaves`, `seed`, `stitchTiles`, `type` (`fractalNoise`/`turbulence`). |
| [feDisplacementMap](https://drafts.csswg.org/filter-effects-1/#feDisplacementMapElement) | `in`, `in2`, `scale`, `xChannelSelector`, `yChannelSelector`. |

### SMIL animation

SVG SMIL animation is implemented via `SVGAnimationApplier`
(`src/core/animation/SVGAnimationApplier.cpp`), which bridges SMIL
timing onto the engine's CSS animation pipeline.

| Element | Supported attributes | Note |
|---------|---------------------|------|
| [animate](https://www.w3.org/TR/SVG2/animate.html#AnimateElement) | `attributeName`, `begin`, `dur`, `end`, `repeatCount`, `fill`, `calcMode`, `values`, `keyTimes`, `keySplines`, `from`, `to`, `by` | Animates SVG presentation attributes and geometry properties. |
| [animateTransform](https://www.w3.org/TR/SVG2/animate.html#AnimateTransformElement) | Same timing attrs + `type` (`translate`/`scale`/`rotate`/`skewX`/`skewY`) | Supports `transform-origin`. |
| [animateMotion](https://www.w3.org/TR/SVG2/animate.html#AnimateMotionElement) | Same timing attrs + `path`, `keyPoints`, `rotate`, `mpath` | Moves an element along a path. |
| [mpath](https://www.w3.org/TR/SVG2/animate.html#MPathElement) | `href`/`xlink:href` | References a `<path>` for `<animateMotion>`. |

Animation events `beginEvent`, `endEvent`, and `repeatEvent` are
dispatched. Programmatic control via `beginElement()` is supported
(`endElement()` is `[Unimplemented]`). `pauseAnimations()` and
`unpauseAnimations()` on the `<svg>` root element are supported;
`setCurrentTime()` and `getCurrentTime()` are `[Unimplemented]`.

### CSS presentation properties

SVG presentation attributes (`fill`, `stroke`, `opacity`, etc.) are
mapped to CSS and participate in the cascade. Both attribute form
(`<rect fill="red">`) and CSS form (`rect { fill: red; }`) are
supported. See the [SVG presentation properties](#svg-presentation-properties)
subsection under CSS for the property list.

### Known limitations

| Surface | Status |
|---------|--------|
| `getBBox()`, `getCTM()`, `getScreenCTM()`, `getTotalLength()`, `getPointAtLength()`, `pathLength` | Not on the prototype — calling throws `TypeError`. |
| `SVGPoint`, `SVGRect`, `SVGMatrix` | Not exposed as constructable globals. |
| `SVGGraphicsElement`, `SVGGeometryElement` | Not exposed — elements inherit directly from `SVGElement` without the SVG2 graphics-element layer. |
| `SVGSVGElement.createSVGRect/Point/Matrix()` | Throw `TypeError` (`[Unimplemented]`). `createSVGLength/Number/Angle/Transform()` work. |
| `<foreignObject>` | No dedicated `SVGForeignObjectElement` class — falls through to generic `SVGElement`. |
| `<pattern>` | IDL registered but no dedicated layout box — not rendered. |
| SVG fonts | Not supported. |

## Additional Supported APIs

### XMLHttpRequest
XMLHttpRequest is a constructor object. It is created by a `new` command, e.g., `var xhr = new XMLHttpRequest();` In addition, two XHR objects are created and executed concurrently by the threadpool by default.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [XMLHttpRequestResponseType](https://xhr.spec.whatwg.org/#enumdef-xmlhttprequestresponsetype) | enum | XMLHttpRequestResponseType | "", "arraybuffer", "blob", "document", "json", "text" |
| [XMLHttpRequestEventTarget](https://xhr.spec.whatwg.org/#xmlhttprequesteventtarget) | interface mixin | XMLHttpRequestEventTarget | The event-target mixin shared by `XMLHttpRequest` and `XMLHttpRequestUpload`. |
| | attribute	| onloadstart	| Function called when the request starts. Usage: onloadstart: function() {} |
| | attribute	| onprogress	| Function called when transmitting data. Usage: onprogress: function() {} |
| | attribute	| onabort	| Function called when the request has been aborted. For instance, by invoking the abort() method. Usage: onabort: function() {} |
| | attribute	| onerror	| Function called when the request has failed. Usage: onerror: function() {} |
| | attribute	| onload	| Function called when the request has successfully completed. Usage: onload: function() {} |
| | attribute	| ontimeout	| Function called when the author specified timeout has passed before the request completed. Usage: ontimeout: function() {} |
| | attribute	| onloadend	| Function called when the request has completed (either in success or failure). Usage: onloadend: function() {} |
| [XMLHttpRequest](https://xhr.spec.whatwg.org/#xmlhttprequest) | constructor | XMLHttpRequest() |  |
| | attribute    | onreadystatechange | The readyState attribute changes value, except when it changes to UNSENT. Usage: onreadystatechange: function() {} |
| | attribute	| timeout	| Can be set to a time in milliseconds.Terminates fetching after the given time (in milliseconds) has passed. If the fetching has not completed after the time passed and the synchronous flag is unset, a timeout event will be dispatched. |
| | attribute	| status	| Returns 0 if the state is UNSENT or OPENED, or error flag is set. Otherwise returns the HTTP status code.|
| | attribute	| statusText	| Returns empty string if the state is UNSENT or OPENED, or error flag is set. Otherwise returns the HTTP status text.|
| | attribute	| responseType	| Sets or returns the response type, which is either "", "blob", "json", or "text".|
| | attribute	| response	| Returns the response entity body, which is either string, Blob object, object, or string when responseType is "", "blob", "json", or "text", respectively.|
| | attribute	| responseText	| Returns an empty string if the state is not LOADING or DONE, or error flag is set. Returns the text response entity body when responseType is either "" or "text". The allowed character set for response text is UTF-8. Otherwise returns an invalidStateError exception with either "Permission denied", "Position unavailable", or "Timeout expired".|
| | attribute	| readyState*	| Returns the current state, which is one of the readyState code shown below.|
| | method    | void open(ByteString method, DOMString url, boolean async = true, optional DOMString? username = null, optional DOMString? password = null)    | Sets the request method, request URL, and synchronous flag. Supported request method : GET, POST |
| | method    | void setRequestHeader(ByteString name, ByteString value)    | Combines a header in author request headers. |
| | attribute | upload | Returns an XMLHttpRequestUpload object that can be observed to monitor the progress of an upload. |
| | method    | void send(optional DOMString? body = null)    | Initiates the request. The optional 'data' argument allows only UTF-8 encoded string type. The argument is ignored if request method is GET. |
| | method    | void abort()    | Cancels any network activity. |
| | method    | ByteString getAllResponseHeaders()    | Returns a string that contains all response headers. |
| | method    | ByteString? getResponseHeader(ByteString name)    | Return the combined value given name and response’s header list. |
| | method    | overrideMimeType()    | overrideMimeType(mime) specifies a MIME type other than the one provided by the server to be used instead when interpreting the data being transferred in a request.|
| | attribute | withCredentials | If `true`, cross-origin requests carry credentials (cookies, HTTP auth). Defaults to `false`. |
| | attribute | responseXML | Returns the response as a `Document` when `responseType` is `""` or `"document"`. |
| | attribute | responseURL | **Not implemented** — always returns the empty string. |


\* The readyState code are as follows.

| readyStateCode | Description | Numeric Value |
|----------------|-------------|---------------|
| UNSENT         | The object has been constructed. | 0 |
| OPENED         | The open() method has been successfully invoked. | 1 |
| HEADERS_RECEIVED | All redirects (if any) have been followed and all HTTP headers of the final response have been received. | 2 |
| LOADING        | 	The response entity body is being received. | 3 |
| DONE           | The data transfer has been completed or something went wrong during the transfer (for example, infinite redirects). | 4 |

### EventSource
The EventSource interface is used to receive server-sent events. It connects to a server over HTTP and receives events in text/event-stream format without closing the connection.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [EventSource](https://html.spec.whatwg.org/multipage/comms.html#the-eventsource-interface) | constructor | EventSource() |  |
| | attribute | url | A DOMString representing the URL of the source. |
| | attribute | withCredentials | Boolean indicating whether the EventSource was instantiated with CORS credentials set. Pass `{withCredentials: true}` in the constructor's `EventSourceInit` to enable. |
| | attribute	| readyState	| A number representing the state of the connection. Possible values are CONNECTING (0), OPEN (1), or CLOSED (2). |
| | attribute | onopen    | An EventHandler called when an open event is received, that is when the connection was just opened. |
| | attribute | onmessage | An EventHandler called when a message event is received, that is when a message is coming from the source. |
| | attribute | onerror   | An EventHandler called when an error occurs and the error event is dispatched on an EventSource object. |
| | method    | void close() | Closes the connection, if any, and sets the readyState attribute to CLOSED. If the connection is already closed, the method does nothing. |


\* The readyState code are as follows.

| readyStateCode | Description | Numeric Value |
|----------------|-------------|---------------|
| CONNECTING     | The connection has not yet been established, or it was closed and the user agent is reconnecting. | 0 |
| OPEN           | The user agent has an open connection and is dispatching events as it receives them. | 1 |
| CLOSED         | The connection is not open, and the user agent is not trying to reconnect. Either there was a fatal error or the close() method was invoked. | 2 |

### Blob
Blob object is used by an XMLHTTPRequest object to retrieve binary data. Supported binary data are the resources supported by the lightweight web engine. When blob is used for other types of binary data, it is likely that the binary data is not recognized by the lightweight web engine.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Blob](https://w3c.github.io/FileAPI/#blob) | interface | Blob | A Blob object refers to a byte sequence |
| | constructor | Blob(optional sequence\<BlobPart\> blobParts = []) | |
| |	attribute |	size    | Returns the size of the byte sequence in number of bytes |
| |	attribute |	type	| The ASCII-encoded string in lower case representing the media type of the Blob |
| |	method	| Blob slice([Clamp] optional long long start = 0, [Clamp] optional long long end = size, optional DOMString contentType = "")	| Returns a new Blob object with bytes ranging from the optional start parameter up to but not including the optional end parameter, and with a type attribute that is the value of the optional contentType parameter. It must act as follows: |
| |	method	| Promise<USVString> text() | Returns a promise resolving with the blob's contents decoded as UTF-8. |
| |	method	| Promise<ArrayBuffer> arrayBuffer() | Returns a promise resolving with the blob's contents as an `ArrayBuffer`. |
| |	typedef | (BufferSource or Blob or DOMString) BlobPart | |
| [File](https://w3c.github.io/FileAPI/#dfn-file)            | interface | File           | `File` extends `Blob` with `name` and `lastModified` attributes. Exposed as a constructable global (`new File(parts, name, options)`). |
| [FileReader](https://w3c.github.io/FileAPI/#APIASynch)     | interface | FileReader     | Asynchronous reader over `Blob`/`File`. Standard `readAsText`/`readAsArrayBuffer`/`readAsDataURL` plus `result`/`onload`/`onerror` are exposed. |
| [FormData](https://xhr.spec.whatwg.org/#interface-formdata) | interface | FormData       | Constructable; supports `append`/`delete`/`get`/`getAll`/`has`/`set`. Accepted as the body of `XMLHttpRequest.send()` and `fetch()`. |

### BatteryManager

> **Build flag:** `BatteryManager` is gated by `STARFISH_ENABLE_BATTERY_STATUS` (set only on the `CUSTOM=unified_wearable` Tizen wearable variant). The default Linux/x64/EFL build does NOT define it; `BatteryManager` and `navigator.getBattery()` are `undefined` at runtime. Verified by `Battery.idl` extended attributes and runtime probe.

Extensions to the Navigator Object: The navigator is extended by the following attributes and methods.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [BatteryManager](https://w3c.github.io/battery/#the-batterymanager-interface)	| interface	| BatteryManager	| |
| |	attribute	| level	| Return the level of system battery. The level attribute MUST be set to 0 if the system's battery is depleted and the system is about to be suspended, and to 1.0 if the battery is full, the implementation is unable to report the battery's level, or there is no battery attached to the system |


### Geolocation
Extensions to the Navigator Object: The navigator is extended by the following attributes and methods.

| Interface            | Type   | Name                      | Description |
|----------------------|--------|---------------------------|-------------|
| [Navigator](https://html.spec.whatwg.org/#the-navigator-object)	| interface	| Navigator	| The navigator attribute of the Window interface must return an instance of the Navigator interface, which represents the identity and state of the user agent (the client), and allows Web pages to register themselves as potential protocol and content handlers |
| |	attribute	| geolocation	| Return geolocation interface. |
| |	attribute	| cookieEnabled	| Return true if the user agent attempts to handle cookies according to the cookie specification. |
| |	attribute	| language	| Return a string representing the language version as defined in BCP 47 (e.g. `ko_KR`). |
| |	attribute	| onLine	| Returns whether the user agent considers itself to be online. LWE always returns `true`. |
| |	method	| boolean javaEnabled() | Always returns `false` (Java applets are not supported). |
| |	misc	| **Unsupported in LWE** (`[Unimplemented]`) | `productSub`, `languages`, `plugins`, `mimeTypes`. |
| |	misc	| **Not exposed at all** | `mediaDevices`, `clipboard`, `share`, `permissions`, `bluetooth`, `usb`, `xr`, `maxTouchPoints`, `hardwareConcurrency`, `deviceMemory`, `connection`, `userAgentData`, `serviceWorker` (build-conditional under `STARFISH_ENABLE_SERVICE_WORKER`), `getBattery`/`battery` (Tizen wearable only). |
| [NavigatorID](https://html.spec.whatwg.org/multipage/#navigatorid) | interface | | NavigatorID is used for identifying Navigator. |
| | attribute | appCodeName | Returns the string "Mozilla". |
| | attribute | appName | Returns the string "Netscape". |
| | attribute | appVersion | Returns a string like "Mozilla/5.0 (like Gecko/54.0 Firefox/54.0) Starfish/<engine version>". |
| | attribute | platform | Returns either the empty string or a string representing the platform on which the MWE is executing. |
| | attribute | product | Returns the string "Gecko". |
| | attribute | userAgent | Returns a string like "Mozilla/5.0 (like Gecko/54.0 Firefox/54.0) Starfish/<engine version>". |
| | attribute | vendor | Returns the string "Samsung Electronics Co., Ltd.". |
| | attribute | vendorSub | Returns the empty string. |
| [Geolocation](https://dev.w3.org/geo/api/spec-source.html#geolocation) | interface	| Geolocation | The interface itself is `[NoInterfaceObject]` — `Geolocation`/`Coordinates`/`Geoposition`/`PositionError` are NOT exposed as constructable globals; they are reachable only via `navigator.geolocation` and the callback parameters. |
| | method   | void getCurrentPosition(PositionCallback successCallback, optional PositionErrorCallback errorCallback, optional PositionOptions options)	| Parameters are in following formats:<br>`successCallback`: `function(position) {}`<br>`errorCallback`: `function (positionError) {}`<br>`options`: `PositionOptions` |
| | method   | long watchPosition(PositionCallback successCallback, optional PositionErrorCallback errorCallback, optional PositionOptions options) | Returns a watch id that can be passed to `clearWatch()` to stop receiving position updates. |
| | method   | void clearWatch(long watchId) | Cancels an ongoing `watchPosition()` call. |
| | callback | PositionCallback = void (Position position) | |
| | callback | PositionErrorCallback = void (PositionError positionError) | |
| [Coordinates](https://dev.w3.org/geo/api/spec-source.html#coordinates_interface) | attribute | latitude | The latitude attribute is a geographic coordinate specified in decimal degrees. |
| | attribute | longitude | The longitude attribute is a geographic coordinate specified in decimal degrees. |
| | attribute |	altitude | The altitude attribute denotes the height of the position, specified in meters above the WGS84 ellipsoid.|
| | attribute | accuracy | The accuracy attribute denotes the accuracy level of the latitude and longitude coordinates. It is specified in meters, and is a non-negative real number.|
| | attribute |  altitudeAccuracy |	 Not supported by the lightweight web engine. Always returns null.|
| | attribute |	 heading | The heading attribute denotes the direction of travel of the hosting device and is specified in degrees, where 0° ≤ heading < 360°, counting clockwise relative to the true north.|
| | attribute |	 speed | The speed attribute denotes the magnitude of the horizontal component of the hosting device's current velocity and is specified in meters per second. The value of the speed attribute is a non-negative real number.|
| Geoposition	| interface	| Geoposition	| The Geoposition interface represents the position of the concerned device at a given time |
| | attribute	| coords | Returns a Coordinates object defining the current location. |
| |	attribute	| timestamp | Returns a DOMTimeStamp representing the time at which the location was retrieved. |
| [PositionError](https://dev.w3.org/geo/api/spec-source.html#position_error_interface) | attribute | code* |	 Returns the appropriate position error code |
| | message |	Returns an error message describing the details of the error encountered. | |
| |	constant |	PERMISSION_DENIED = 1 | |
| | constant |	POSITION_UNAVAILABLE = 2 | |
| | constant | TIMEOUT = 3 | | |


\* PositionError codes are as follows:

| Error Code | Description | Numeric Value |
|------------|-------------|---------------|
| PERMISSION_DENIED | The location acquisition process failed because the lightweight web engine does not have permission to use the Geolocation API. | 1 |
| POSITION_UNAVAILABLE | The position of the device could not be determined. | 2 |
| TIMEOUT | The length of time specified by the timeout property has elapsed before successfully acquiring a new Position object. | 3 |

### Fetch API

`fetch`, `Headers`, `Request`, `Response`, and the `Body` mixin are exposed. Verified by IDL `src/core/fetch/*.idl` and runtime probes.

| Interface | Status |
|-----------|--------|
| `fetch(input, optional RequestInit)` | Returns `Promise<Response>`. |
| `Headers` | Full WHATWG surface: constructor (seq-of-seq or string-record), `get`/`set`/`append`/`delete`/`has`/`forEach`/`entries`/`keys`/`values`/`for..of`. |
| `Request` | Constructor + `clone()`; properties `method`, `url`, `headers`, `mode`, `credentials`, `cache`, `redirect`, `referrer`, `referrerPolicy`, `destination`, `integrity`, `body`. **`Request.signal` is `undefined` (`[Unimplemented]`)** — `RequestInit.signal` is silently ignored. |
| `Response` | Constructor; instance methods `text()`, `json()`, `arrayBuffer()`, `blob()`, `formData()`, `clone()`. Static `Response.error()`, `Response.redirect(url, status)`. **`Response.json` static is NOT exposed.** |
| `Body` mixin | `body` (`ReadableStream`), `bodyUsed`, plus the consumers above. **`BodyInit` does not accept `FormData` or `URLSearchParams`** — only `Blob`/`BufferSource`/`USVString`/`ReadableStream`. |
| **Not exposed** | `AbortController`, `AbortSignal`. There is no way to cancel an in-flight `fetch()` from JS. |

### URL & URLSearchParams

| Interface | Status |
|-----------|--------|
| `URL` | Full WHATWG getter/setter surface (`href`, `protocol`, `host`, `hostname`, `port`, `pathname`, `search`, `hash`, `origin`, `username`, `password`, `searchParams`). `URL.createObjectURL`/`URL.revokeObjectURL` exposed. |
|  | **`new URL("invalid")` does NOT throw** — silently returns `about:blank`. WHATWG-compliant browsers throw `TypeError`. |
|  | **Not exposed:** `URL.canParse`, `URL.parse` (static), `URL.toJSON` (instance — `[Unimplemented]`, calling throws). |
| `URLSearchParams` | `append`/`delete`/`get`/`getAll`/`has`/`set`/`sort`/`toString`/iterable/`entries`/`keys`/`values`. |
|  | **Constructor accepts only `string` or `sequence<sequence<USVString>>`.** `new URLSearchParams({a:1, b:2})` (record/object form) silently produces an empty params object. |
|  | **`size` attribute is NOT exposed.** Use `Array.from(usp).length`. |

### Encoding (TextEncoder / TextDecoder)

| Interface | Status |
|-----------|--------|
| `TextEncoder` | Constructor + `encode(string)` → `Uint8Array`. |
|  | **`encodeInto` is `[Unimplemented]`** — `undefined`. |
|  | Does NOT throw on non-`utf-8` constructor labels; output is always UTF-8 regardless. |
| `TextDecoder` | Constructor accepts label + `{fatal, ignoreBOM}`. `decode(buffer, {stream})` works. |
|  | Supported labels include `utf-8`, `utf-16`, `latin1`, `iso-8859-1`. `fatal:true` correctly throws on bad sequences. |

### Streams

| Interface | Status |
|-----------|--------|
| `ReadableStream` | Constructor with `{start({enqueue, close})}` works. `cancel`, `getReader`, `locked` work. |
|  | **`ReadableStream.tee`, `pipeTo`, `pipeThrough`, `ReadableStream.from` are `[Unimplemented]` / not exposed.** |
| `ReadableStreamDefaultReader` | `read()`, `cancel()`, `releaseLock()`, `closed` exposed. The pull/enqueue path observed at runtime is **unreliable**: enqueued chunks do not always drain via `read()` in this build. **Prefer one-shot decoders (`response.text()`, `.arrayBuffer()`, `.blob()`) over streaming consumption.** |
| **Not exposed** | `WritableStream`, `TransformStream`, `ByteLengthQueuingStrategy`, `CountQueuingStrategy`, `ReadableStreamBYOBReader`. Globals are `undefined`. |
| **Blob** | `Blob.stream()` is NOT exposed. Use `await blob.text()` / `.arrayBuffer()`. |

### CSP (Content Security Policy)

CSP is enforced by `src/core/csp/`. The directive parser in `ContentSecurityPolicyDirectiveList.cpp` recognizes only this set:

| Recognized & enforced | Silently ignored |
|-----------------------|------------------|
| `base-uri`, `child-src`, `connect-src`, `default-src`, `form-action`, `frame-ancestors`, `frame-src` (deprecated → use `child-src`), `img-src`, `media-src`, `script-src`, `style-src` | `font-src`, `object-src`, `worker-src`, `manifest-src`, `prefetch-src`, `report-uri`, `report-to`, `require-trusted-types-for`, `trusted-types`, `upgrade-insecure-requests`, `block-all-mixed-content`, `sandbox` |

`frame-ancestors` is checked against every ancestor URL, has no `default-src` fallback, and is enforced only from a policy delivered with the framed response — where it applies, it overrides `X-Frame-Options`.

Apps relying on the right-hand list get **no protection** — the directive is parsed but no enforcement code path looks at it. The `securitypolicyviolation` event fires correctly on `document` for the recognized set, but the dispatched `SecurityPolicyViolationEvent` only populates `blockedURI` and `violatedDirective`; the other 8 attributes (`documentURI`, `referrer`, `effectiveDirective`, `originalPolicy`, `sourceFile`, `statusCode`, `lineNumber`, `columnNumber`) are `[Unimplemented]` and read back as `undefined`.

### WebGL

> **Build flag:** WebGL is gated by `-DWEBGL=1` (also requires `BACKEND=*_cairo_gl`) and is **off by default** — pass `-DWEBGL=1` at configure time. Headless backends without GL return `null` from `canvas.getContext('webgl')`.

| Interface | Notes |
|-----------|-------|
| `WebGLRenderingContext` | Full WebGL 1.0 surface (~190 methods) per [Khronos WebGL 1.0 spec](https://registry.khronos.org/webgl/specs/latest/1.0/). Obtain via `canvas.getContext('webgl')` or `'experimental-webgl'`. |
| `WebGL2RenderingContext` | WebGL 2.0 surface; obtain via `canvas.getContext('webgl2')`. Includes `WebGLQuery`, `WebGLSampler`, `WebGLSync`, `WebGLTransformFeedback`, `WebGLVertexArrayObject`. |
| Object handles | `WebGLBuffer`, `WebGLFramebuffer`, `WebGLRenderbuffer`, `WebGLTexture`, `WebGLProgram`, `WebGLShader`. |
| Value types | `WebGLActiveInfo`, `WebGLShaderPrecisionFormat`, `WebGLUniformLocation`, `WebGLContextAttributes`. |
| **Not exposed** | `WebGLContextEvent` typed event (use a generic `Event` listener for `webglcontextlost`/`webglcontextrestored`). |

**Extensions exposed via `getExtension(name)`** (subject to the underlying GL driver advertising the matching `GL_*` token; on Mesa llvmpipe / SwiftShader none may be advertised):

`OES_texture_float`, `OES_texture_half_float`, `OES_texture_float_linear`, `OES_standard_derivatives`, `OES_vertex_array_object`, `WEBGL_depth_texture`, `EXT_blend_minmax`, `EXT_texture_filter_anisotropic`.

**Common extensions NOT implemented** (will return `null`): `WEBGL_lose_context`, `WEBGL_debug_renderer_info`, `WEBGL_compressed_texture_*` (s3tc/etc1/astc/pvrtc), `OES_element_index_uint`, `EXT_color_buffer_float`, `ANGLE_instanced_arrays`, `OES_texture_half_float_linear`, `EXT_sRGB`, `KHR_parallel_shader_compile`.

### Performance

| Member | Status |
|--------|--------|
| `performance.now()`, `performance.timeOrigin` | Implemented. |
| `performance.mark(name)`, `performance.measure(name, start, end)`, `performance.clearMarks()`, `performance.clearMeasures()`, `performance.clearResourceTimings()`, `performance.toJSON()` | Implemented. |
| `performance.getEntries()`, `getEntriesByType()`, `getEntriesByName()` | Implemented. |
| `performance.timing` | **Quirk: typed as `PerformanceResourceTiming` in IDL (not the spec's `PerformanceTiming`).** 11 timestamp attributes work; `connectStart/End`, `domLoading`, `domInteractive`, `domComplete`, `redirectStart/End`, `unloadEventStart/End` are `[Unimplemented]` — read back as `undefined`. |
| `performance.navigation` | **Not exposed** (`undefined`). |
| `performance.memory` | **Not exposed** (Chrome-specific). |
| **Not exposed**: `PerformanceMark`, `PerformanceMeasure`, `PerformanceObserver`, `PerformanceNavigationTiming`, `PerformancePaintTiming`, `PerformanceLongTaskTiming`, `PerformanceServerTiming`, `PerformanceEventTiming` | Constructors are `undefined`. |

### Crypto

| Member | Status |
|--------|--------|
| `crypto.getRandomValues(typedArray)` | Implemented. |
| `crypto.randomUUID()` | **`[Unimplemented]`** — `undefined`. |
| `crypto.subtle` | **`[Unimplemented]`** — `undefined`. **The entire WebCrypto algorithm surface is missing**: `encrypt`, `decrypt`, `sign`, `verify`, `digest`, `generateKey`, `deriveKey`, `deriveBits`, `importKey`, `exportKey`, `wrapKey`, `unwrapKey` are all unavailable. |
| **Not exposed** | `SubtleCrypto`, `CryptoKey`, `CryptoKeyPair` constructors. Apps needing SHA-256, AES, HMAC, ECDSA, etc. must ship a JS polyfill. |

### Forms — runtime caveats

The HTML form-control IDLs in `src/core/dom/HTMLFormElement.idl`, `HTMLInputElement.idl`, etc. expose far less than the spec implies. Listing the **unimplemented surface** so authors don't reach for it:

| Surface | Status |
|---------|--------|
| Constraint validation API on every form control (`validity`, `validationMessage`, `willValidate`, `checkValidity()`, `reportValidity()`, `setCustomValidity()`) | **All `[Unimplemented]`.** Reading returns `undefined`; calling the methods raises `TypeError: Callee is not a function object`. There is no working `:invalid` runtime state, no `ValidityState` constructor, and no `RadioNodeList`. |
| `HTMLInputElement` selection/range/step API (`select()`, `setSelectionRange()`, `setRangeText()`, `selectionStart/End/Direction`, `stepUp()`, `stepDown()`, `valueAsDate`, `valueAsNumber`) | All `[Unimplemented]`. Calling throws. |
| `HTMLInputElement` other attrs (`accept`, `alt`, `autocomplete`, `dirName`, `formNoValidate`, `pattern`, `readOnly` (works on textarea, NOT input), `inputMode`, `height`, `width`, `src`, `useMap`, `align`, `indeterminate`, `files`, `list`) | `[Unimplemented]` — `undefined`. |
| `HTMLTextAreaElement` selection/wrap (`wrap`, `select()`, `selectionStart/End/Direction`, `setRangeText()`, `setSelectionRange()`, `inputMode`) | `[Unimplemented]`. |
| `HTMLOptGroupElement.label` | `[Unimplemented]` — `<optgroup label="X">` does not surface `label` via JS. |
| `HTMLFieldSetElement.elements` | `[Unimplemented]` — `undefined`. |
| `HTMLOutputElement.type` | Returns `""` (spec says `"output"`). |
| **Interfaces not exposed at all** | `HTMLDataListElement`, `HTMLProgressElement`, `HTMLMeterElement` — the corresponding `<datalist>`/`<progress>`/`<meter>` tags fall through to `HTMLUnknownElement`. `<input list>` autocomplete UI does not work. `RadioNodeList` is not exposed — `form.elements.namedItem('radio')` returns only the first matching radio. |
| `FormData` iteration | **`fd.entries`, `keys`, `values`, `forEach`, `[Symbol.iterator]` are all `undefined`** (IDL `iterable<>` is commented out). `for..of fd`, `Array.from(fd)`, `[...fd]` will throw or yield nothing. |
| `FormData` Blob/File overloads | `append(name, Blob, filename)` / `set(name, Blob, filename)` are not exposed; `FormDataEntryValue` is `USVString` only. |

### Selection API and editing — not available

The W3C Selection API and `document.execCommand` editing pipeline are entirely absent. There is no in-engine way to read, programmatically modify, or observe the user's selection. Authors who need a "selection" must implement it themselves using `Range` plus their own visual highlighting (e.g. wrap with `<span class="hl">`).

| Surface | Status |
|---------|--------|
| `Selection` interface | **Not exposed.** `typeof Selection === "undefined"`; no constructor and no prototype. |
| `window.getSelection()` / `document.getSelection()` | **Not in IDL.** Both are `undefined`; calling raises `TypeError: Callee is not a function object`. |
| `document.execCommand` and `queryCommand{Enabled,Indeterm,State,Supported,Value}` | **`[Unimplemented]`** in `Document.idl` — `undefined` at runtime (also noted in the Document table above). No editing-host pipeline exists; setting `contenteditable=true` and `document.designMode = "on"` produces no effect. |
| `document.onselectionchange` / `onselectstart` / `Element.onselectstart` | Not in IDL — `undefined`. The `selectionchange` and `selectstart` events are never dispatched. |
| `<input>` / `<textarea>` selection API (`select()`, `setSelectionRange()`, `setRangeText()`, `selectionStart/End/Direction`) | All `[Unimplemented]` (also covered in §Forms). Methods throw `TypeError`; attribute reads return `undefined`. **However, `selectionStart`/`selectionEnd`/`selectionDirection` are writable as plain expando properties** because the IDL `[Unimplemented]` setter does not throw — assignments succeed but have no effect on the rendered widget. Do not rely on this. |
| Caret/IME hooks (`getComposition`, `caretPositionFromPoint`) | `caretPositionFromPoint` is `[Unimplemented]`. There is no public caret-position API. |

**Recommended pattern.** For text-search-and-highlight or "click-to-mark" UIs, build on `Range` directly:

```js
function highlight(range, cls) {
  // surroundContents only works when the range does not split a non-Text node.
  // For a robust implementation walk the range with a TreeWalker and wrap each
  // contained Text node individually.
  const span = document.createElement('span');
  span.className = cls;
  range.surroundContents(span);
}
```

### Range edge cases

The Range table above (rows around line 1137) is correct at the interface level, but the runtime has these caveats authors should know:

| Edge case | Observed behavior |
|-----------|-------------------|
| `range.toString()` when start and end live in **different Text nodes** with intermediate descendant Text nodes | **Buggy: trailing endNode text is appended twice.** Example: with `<p>Hello <b>brave</b> world of <span>LWE</span></p>` and a Range from `(p.firstChild, 0)` to `(span.firstChild, 3)`, `toString()` returns `"Hello brave world of LWELWE"` instead of `"Hello brave world of LWE"`. Source: `src/core/dom/Range.cpp` `Range::toString()` — the descendant-walk loop already visits the end text node before the explicit "endNode prefix" append at the bottom of the function. Workaround: extract text by walking nodes manually with a `TreeWalker`. |
| `range.surroundContents(newParent)` whose endpoints span a non-Text node boundary | Throws `InvalidNodeTypeError` (the spec's "Invalid State" condition). The exception name LWE uses is **`InvalidNodeTypeError`** rather than the spec's `InvalidStateError`; check `e.name` against both if you must distinguish. |
| `range.surroundContents(newParent)` on a **collapsed** range | Succeeds and produces an empty wrapper element at the collapse point (e.g. `<mark></mark>`). This matches the spec's "wrap nothing" semantics but is rarely useful. |
| `range.collapse()` (no argument) | Collapses to **end** (`toStart` defaults to `false` per IDL). Matches the spec. `collapse(true)` collapses to start, `collapse(false)` to end. Both leave `collapsed === true` and equal start/end offsets. |
| `range.cloneContents()` | **`[Unimplemented]`** — `undefined`; calling throws `TypeError`. Use `extractContents()` and re-insert the original content if you need a copy. |
| `range.createContextualFragment(html)` | **`[Unimplemented]`** — `undefined`; calling throws `TypeError`. Use a temporary element with `innerHTML = …` and adopt its children instead. |
| `range.expand(unit)` | **`[Unimplemented]`** (non-standard WebKit-ism). Not available. |
| `range.getClientRects()` / `getBoundingClientRect()` | Implemented and return per-fragment rects. Verified across multi-line text. |

### Canvas 2D — additional details

The existing canvas mixin tables are incomplete. Adding the missing pieces:

| Mixin / Interface | Members |
|-------------------|---------|
| `CanvasCompositing` | `globalAlpha` (0.0–1.0, default 1.0), `globalCompositeOperation` (default `"source-over"`; accepts the full Porter-Duff set **and** the CSS blend-mode names). |
| `CanvasFilters` | **`filter` is `[Unimplemented]`** — `undefined` at runtime. CSS filter strings on the 2D context have no effect; use the CSS `filter` property on the parent. |
| `CanvasUserInterface` | All four members `[Unimplemented]`: `drawFocusIfNeeded(Element)`, `drawFocusIfNeeded(Path2D, Element)`, `scrollPathIntoView()`, `scrollPathIntoView(Path2D)`. |
| `CanvasTransform` | Adds `getTransform()` (returns a fresh `DOMMatrix`) and the `setTransform(DOMMatrix2DInit)` overload. |
| `Path2D` | Constructible: `new Path2D()` and `new Path2D(Path2D)`. Includes the full `CanvasPath` mixin. **`new Path2D(DOMString)` (SVG path-string overload) is not implemented** — engine logs a warning and returns an empty path. **`Path2D.addPath` is `[Unimplemented]`.** |
| `ImageBitmapRenderingContext` | Obtained via `canvas.getContext("bitmaprenderer")`. The context object exists but `transferFromImageBitmap` is `[Unimplemented]`, so the context cannot display anything. |
| `HTMLCanvasElement` extras | `toDataURL(type='image/png', quality)` works (PNG verified). **`toBlob(callback)` and `transferControlToOffscreen()` are `[Unimplemented]`.** `OffscreenCanvas` is NOT exposed. |
| `TextMetrics` | Only `width` is functional. **All 11 extended baseline metrics** (`actualBoundingBoxLeft/Right/Ascent/Descent`, `fontBoundingBoxAscent/Descent`, `emHeightAscent/Descent`, `hangingBaseline`, `alphabeticBaseline`, `ideographicBaseline`) are `[Unimplemented]` — read as `undefined`. |
| `ImageData` | Constructor overloads `new ImageData(sw, sh)` and `new ImageData(Uint8ClampedArray, sw, optional sh)` are exposed. **`colorSpace` is not exposed**; pixel data is always sRGB. |
| `ImageBitmap` | `close()` is implemented (releases the bitmap). |
| Additional enum values | `CanvasTextBaseline` accepts `"ideographic"` (in addition to the values listed earlier). `ImageSmoothingQuality` enum: `"low"|"medium"|"high"`. `CanvasFillRule`: `"nonzero"|"evenodd"`. |

### CSS Layout — Flexbox / Grid / Position runtime caveats

#### Flexbox

| Property | Supported | Not supported / silent fallback |
|----------|-----------|---------------------------------|
| `flex-direction` | `row`, `row-reverse`, `column`, `column-reverse` | invalid → `row` |
| `flex-wrap` | `nowrap`, `wrap`, `wrap-reverse` | — |
| `flex-flow` | `<dir>`, `<wrap>`, `<dir> <wrap>`, `<wrap>` alone | comma-separated forms |
| `flex` shorthand | `auto`/`none`/`initial`/`<n>`/`<g> <s> <b>` | — |
| `flex-grow`/`shrink`/`basis` | non-negative numbers / lengths / % / `auto` / `content` | negative values silently coerced to initial |
| `justify-content` | `flex-start`, `flex-end`, `start`, `end`, `center`, `space-between`, `space-around`, `stretch`, `normal` | **`space-evenly`, `left`, `right`** cause the declaration to be **dropped** (property falls back to `normal`). `start`/`end`/`stretch` are experimentally supported (layout may not match spec). |
| `align-items` / `align-self` | `flex-start`, `flex-end`, `start`, `end`, `center`, `baseline`, `stretch` | **`first baseline`, `last baseline`, `self-start`, `self-end`, `normal`** cause the declaration to be **dropped** (property falls back to `stretch`). `start`/`end` are experimentally supported (layout may not match spec). `align-self: auto` resolves to the parent’s `align-items` value. |
| `align-content` | `flex-start`, `flex-end`, `center`, `space-between`, `space-around`, `stretch` | **`space-evenly`, `start`, `end`, `normal`, baseline variants** cause the declaration to be **dropped** (property falls back to `stretch`). |
| `gap` / `row-gap` / `column-gap` | `<length-percentage>` | Applied. `FlexFormattingContext` maps `row-gap`/`column-gap` onto the main and cross gaps by `flex-direction`, so a wrapped flex container gets its cross-axis line gap. A percentage `row-gap` resolves against the block size. `gap` expands to both longhands. `calc()` is not accepted. |
| `order` | integer (incl. negative) | — |
| `justify-items` / `justify-self` | — | Recognized (`Style.h`) and applied. |
| `place-items` / `place-content` / `place-self` | — | **NOT recognized** by the trie — declarations dropped with `Unsupported css property`. |

#### Grid

| Property | Supported | Not supported / partial |
|----------|-----------|--------------------------|
| `display: grid` / `inline-grid` | both | — |
| `grid-template-rows` / `grid-template-columns` | `<length>` (px/em/%/vw/...), `<fr>`, `auto`, `min-content`, `max-content`, `minmax(min, max)`, `repeat(<int>, …)`, `repeat(auto-fill, …)`, `repeat(auto-fit, …)` | **`fit-content(<length>)`, line-name brackets `[name]`, `subgrid`** are not recognized — entire declaration is dropped. |
| `grid-template-areas` | string syntax | — |
| `grid-template` (shorthand) | Expands to `grid-template-rows`, `grid-template-columns`, and `grid-template-areas`. Supports `[ <string> <track-size>? ]+ / <track-list>` and `<track-list> / <track-list>` forms. | — |
| `grid-auto-flow` / `grid-auto-rows` / `grid-auto-columns` | — | **NOT recognized.** Auto-placement always uses default `row` flow with `auto` track sizes. |
| `grid-row-start/end`, `grid-column-start/end` | `auto`, `<integer>`, `<custom-ident>` (named lines), `span <integer>`, `span <custom-ident>` | Negative integers parse but layout effect (counting from end) is not guaranteed — prefer positive. The serialized `*-end` may come back empty in some shorthand expansions (cosmetic bug). |
| `grid-row` / `grid-column` (shorthand) | `<start> / <end>` | — |
| `grid-area` | `<name>` or `<line>{1,4}` | — |
| `gap` / `grid-gap` (single value) | layout uses the value | Both longhands are set. Grid honors only fixed-length gaps — a percentage gap computes to 0 (flex resolves it). |
| `justify-content` (grid) | `normal`, `start`, `center`, `end`, `stretch` | Other values (`space-between`/`space-around`/`space-evenly`/`flex-start`/`flex-end`/`left`/`right`) parse but log `unsupported justify-content value in grid` at layout — **no visual effect**. |
| `align-content` (grid) | parsed, **never applied** (`GridFormattingContext` has no `applyAlignContent`). | Use `align-items`/`align-self` per-item to position rows. |
| `align-items` / `align-self` (grid) | `start`, `center`, `end`, `stretch` | Other values emit `STARFISH_UNSUPPORTED` at layout. |
| `justify-items` / `justify-self` | — | Recognized and applied to grid items. |
| `place-items` / `place-content` / `place-self` | — | **NOT recognized.** |

#### Position / Float / Inset

| Property | Supported | Not supported / fallback |
|----------|-----------|---------------------------|
| `position` | `static`, `relative`, `absolute`, `fixed` | **`sticky` is silently treated as `static`** (no console warning). |
| `top` / `right` / `bottom` / `left` | `<length>`, `<percentage>`, `auto` | — |
| `inset` (shorthand) | 1–4 values (CSS-standard) | — |
| `inset-block-start/end`, `inset-inline-start/end` | — | **NOT supported** (`Unsupported css property` warning). |
| `float` | `none`, `left`, `right` | **`inline-start` / `inline-end` silently fall back to `none`** (no warning). |
| `clear` | `none`, `left`, `right`, `both` | **`inline-start` / `inline-end` silently fall back to `none`** (no warning). |
| `z-index` | `auto`, `<integer>` (incl. negative) | — |

> **Containing block & stacking context rules (audit-confirmed):** only an ancestor with `position != static` **OR** with `transform != none` establishes a containing block for absolutely-positioned descendants. **`will-change`, `filter`, `contain`, `perspective` do NOT establish a containing block in LWE.** Stacking contexts are created only by `position` + `z-index` (other than `auto`); `opacity < 1`, `transform`, `will-change`, `filter`, `mix-blend-mode`, and `isolation` do NOT create stacking contexts.

### CSS `display` and `visibility` — runtime caveats

| `display` value | Status |
|-----------------|--------|
| `block`, `inline`, `inline-block`, `none`, `flex`, `inline-flex`, `grid`, `inline-grid`, `table`, `inline-table`, `table-row`, `table-row-group`, `table-header-group`, `table-footer-group`, `table-cell`, `table-column`, `table-column-group`, `table-caption`, `list-item`, **`inline-list-item`** | Supported. (`list-item` and `inline-list-item` were missing from Spec.md.) |
| `-webkit-box`, `-webkit-inline-box` | Supported when `STARFISH_ENABLE_CSS_WEBKIT_BOX_PREFIX` is set (default for Linux/Android/Windows hosts; off on Tizen wearable builds). Primary use case is `-webkit-line-clamp`. |
| `-webkit-flex`, `-webkit-inline-flex` | Aliases under `STARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX` (already documented in Obsolete CSS). |
| `contents` | Supported ([css-display-3](https://drafts.csswg.org/css-display-3/#valdef-display-contents)). The element generates no box; its children and `::before`/`::after` render in its place and still inherit from it. Blockified to `block` on the root; on replaced elements and form controls (`img`, `iframe`, `br`, `object`, `input`, `textarea`, `select`, `video`/`audio`, `canvas`) and on every `svg` element it behaves as `none` per Appendix B; the rest of Appendix B.2 (hoisting inner `svg`/`g`/`use`/`tspan`, hiding other SVG elements) is not implemented. The UA sheet applies it to HTML `slot`. |
| `flow-root` | **NOT supported** — silently dropped. Float-clearing BFC creation does NOT happen. Use a float-clearing wrapper instead. |
| `run-in` | **NOT supported** — silently dropped. |
| `ruby`, `ruby-base`, `ruby-text`, `ruby-base-container`, `ruby-text-container` | **NOT supported** — silently dropped. |
| Multi-token CSS Display L3 syntax (`block flex`, `inline flow-root`, `flow-root list-item`, …) | **NOT supported.** The parser's `tokens.size() != 1` guard rejects all multi-token forms — declaration silently dropped (no `Unsupported css property` warning). |

> **Footgun:** rejected `display` values produce **no console warning**. Authors that try `display: flow-root` and rely on `getComputedStyle.display` for feature detection will see the property fall back silently to its initial value (`inline`/`block`). Verify visually or by checking layout (e.g., `offsetWidth` on a child).

| `visibility` value | Status |
|--------------------|--------|
| `visible`, `hidden` | Supported. |
| `collapse` | Parses; on non-table elements **computes to `hidden`** (per spec). On `<tr>`/`<tbody>` it parses but **does NOT actually collapse the row** — the row is hidden in place, height unchanged. Use `display: none` to actually remove rows. |

### CSS @keyframes / animations — runtime caveats

| Construct | Status |
|-----------|--------|
| `@keyframes` selectors (`from`, `to`, `0%`, `25%`, `50%`, `75%`, `100%`, comma list `0%, 100%`) | OK. |
| `animation-name` / `-duration` / `-timing-function` / `-delay` / `-iteration-count` / `-play-state: paused` | OK. (`paused` correctly halts progression.) |
| `animation-direction: alternate` / `alternate-reverse` | **Buggy:** end-of-cycle value is incorrect (animations end stuck near a middle frame instead of returning to the cycle endpoint). |
| `animation-iteration-count: 3` (or any finite > 1) | Cycles run, but intermediate samples may be skipped — not all iterations are observable. |
| `animation-fill-mode: forwards` | OK. |
| `animation-fill-mode: backwards` / `both` | **Bug:** the `backwards` phase (during `delay`) renders the *current* value instead of the keyframe-`from` value. |
| `animation-duration: 0s` | **Buggy:** the `to` value is NOT applied; animation stays at `from`. |
| `addEventListener('animationstart' / 'animationend' / 'animationcancel')` | Fire correctly. `animationName` and `elapsedTime` populated. |
| `addEventListener('animationiteration')` | **Never fires.** The `KeyFramesAnimationEventType` enum has no `AnimationIteration` value, and `m_animationiteration` static string does not exist. The dispatch site (`AnimationExecutor.cpp:589`) is gated on `isSVGAnimation`, so non-SVG keyframe animations cannot fire iteration events. |
| Multi-name `animation-name: a, b` with `animation-duration: 100ms, 100ms` | **Broken** — start events do not fire. |
| `getComputedStyle(el).animationName` | Includes literal quotes around the name. Non-standard. |
| `getComputedStyle(el).animationIterationCount: inf` | Should be `infinite` per spec. |

**Timing-function value parsing** (`Style.cpp::updateValueUnitTransitionTimingFunction` / `updateValueUnitAnimationTimingFunction`):

| Token | Status |
|-------|--------|
| `ease`, `linear`, `ease-in`, `ease-out`, `ease-in-out`, `step-start`, `step-end` | All seven keywords supported. |
| `cubic-bezier(x1, y1, x2, y2)` | Supported. **`x1` and `x2` must be in `[0, 1]`** (`y1`/`y2` may be any number); out-of-range x rejects the whole declaration. |
| `steps(<integer>, start \| end)` | Supported. `<integer>` must be > 0; the second argument defaults to `end`. |
| `steps(<n>, jump-start \| jump-end \| jump-none \| jump-both)` | **NOT recognized** — only `start`/`end` accepted as the 2nd arg. |
| `linear(<linear-stop-list>)` (CSS Easing 2) | **NOT recognized.** |

**`animation-direction`**: `normal`, `reverse`, `alternate`, `alternate-reverse` all parse. (At runtime, `alternate`/`alternate-reverse` end-of-cycle is buggy — see the table above.)

**`animation-fill-mode`**: `none`, `forwards`, `backwards`, `both` all parse.

**`animation-play-state`**: `running`, `paused`. Both work.

**`animation-iteration-count`**: any positive `<number>` or `infinite`. **`getComputedStyle(...).animationIterationCount` returns `inf` instead of the spec-required `infinite`** (already noted above).

> 🔥 **Critical crashes:**
> - **`@keyframes empty {}`** (empty body) → SIGSEGV when an animation referencing it starts.
> - **Animating an unsupported property** (e.g. `background` shorthand, `box-shadow`, `filter`, `font-weight`, `letter-spacing`, `clip-path`) crashes the engine in debug builds (`AnimatedValue.cpp:328` `STARFISH_UNIMPLEMENTED` → abort).
>
> **Safe-to-animate property whitelist** (verified via `AnimatedValue::create` switch): `color`, `background-color`, `border-*-color`, `caret-color`, `outline-color`, `text-decoration-color`, `width` / `min-width` / `max-width`, `height` / `min-height` / `max-height`, `margin-*`, `padding-*`, `border-*-width`, `left` / `right` / `top` / `bottom`, `font-size`, `background-position-x/y`, `background-size`, `opacity`, `transform`, `transform-origin`. **Anything else may abort the engine.**

### CSS pseudo-classes — runtime caveats

The pseudo-class enum lives in `src/StaticStrings.h:244-308`; the matcher is `StyleResolver::checkPseudoClass` (`Style.cpp:8504-8778`). Anything that hits `default:` logs `Unsupported css pseudo-element: <id>` and returns `false`.

| Pseudo | Status |
|--------|--------|
| `:hover`, `:active`, `:focus`, `:target`, `:link`, `:checked`, `:disabled`, `:enabled`, `:placeholder-shown`, `:root`, `:empty`, `:first-child`, `:last-child`, `:only-child`, `:nth-child(...)`, `:nth-last-child(...)`, `:first-of-type`, `:last-of-type`, `:only-of-type`, `:nth-of-type(...)`, `:nth-last-of-type(...)`, `:scope`, `:lang(...)`, `:dir(ltr|rtl)`, `:defined`, `:host`, `:host(...)`, `:fullscreen` | Implemented. |
| `:not(...)`, `:is(...)`, `:where(...)` | Implemented over a full complex-selector-list. `:not()` matches when no branch matches and is non-forgiving (an invalid branch drops the whole rule); `:is()`/`:where()` match when any branch does and are forgiving. |
| **Parses but never matches** (silent — always returns `false`; entry exists in the `STARFISH_ENUM_PSEUDO_SELECTORS` list at `src/StaticStrings.h:244` but no `case` in `checkPseudoClass`) | `:focus-visible`, `:focus-within`, `:required`, `:optional`, `:valid`, `:invalid`, `:in-range`, `:out-of-range`, `:read-only`, `:read-write`, `:default`, `:indeterminate`, `:any-link`, `:local-link`, `:visited`, `:target-within`, `:blank`, `:current`, `:drop`, `:future`, `:past`, `:paused`, `:playing`, `:user-invalid`. |
| **Hard `SyntaxError`** (entire selector dropped at parse) | `:has(...)`, `:popover-open`, `:modal`, `:nth-child(An+B of <selector>)`. Forgiving-selector-list rules don't apply — these break the whole stylesheet rule. |
| `:scope` | Matches `documentElement` even outside `querySelector(...)` calling context (non-spec). |

### CSS pseudo-elements — runtime caveats

LWE supports exactly **4** pseudo-elements for *style application*: `::before`, `::after`, `::first-line`, `::first-letter`. The `PseudoElementType` enum (`Style.h:3285-3297`) only has slots for those four (plus internal `FirstLineInherited`).

| Pseudo | Status |
|--------|--------|
| `::before`, `::after` | OK. `content: "string"`, `content: counter()`, `content: attr(x)`, `content: url()`, `content: ""` all work. |
| `::first-line`, `::first-letter` | OK at render time. |
| `::placeholder` | **NOT implemented** — logs `Unsupported css pseudo-element: 60`. |
| `::selection` | **NOT implemented** — logs `Unsupported css pseudo-element: 52`. |
| `::marker` | **NOT implemented**. |
| `::backdrop` | **NOT implemented** (and `<dialog>.showModal()` is also out of scope). |
| `::slotted(...)` | **Implemented** — matches a slot's assigned nodes from within the shadow tree that contains the slot. |
| `::file-selector-button`, `::target-text`, `::part(...)`, `::-webkit-*` | **Token not even in the parser enum** — silently dropped at parse time (no warning). |
| `::cue`, `::spelling-error`, `::grammar-error` | **NOT implemented** (logged warning). |
| `getComputedStyle(el, '::pseudo')` | **🐛 Always returns the host's computed style, even for the 4 implemented pseudos.** The Window binding ignores the second argument. Authors that probe pseudo support via CSSOM will get false negatives. |

### CSS Custom Properties (`--*` / `var()`) — runtime caveats

| Construct | Status |
|-----------|--------|
| `--name: value` declarations + cascade override + inheritance | OK. |
| `var(--name)` and `var(--name, fallback)` (incl. nested fallback) | OK. |
| `calc(var(--n) * 1px)` and other type coercion through `calc()` | OK. |
| `var()` inside shorthand declarations (`background: var(--bg)`, `font: var(--fs) sans-serif`, `transition: opacity var(--dur)`) | OK. |
| `var()` resolving to a `linear-gradient(...)` for `background-image` | OK. |
| `el.style.setProperty('--x', v)` / `getPropertyValue('--x')` / `removeProperty('--x')` | OK. |
| `getComputedStyle(child).getPropertyValue('--x')` (inheritance) | OK. |
| Direct self-cycle `--self: var(--self)` | Guarded — falls back to initial. |
| **Indirect cycle `--a: var(--b); --b: var(--a)`** | **🔥 Hangs / SIGSEGV** during `getComputedStyle`. The cycle guard at `CSSVariableSyntaxTreeBuilder.cpp:354-365` only checks one-step self-reference, not multi-step cycles. |
| `el.style.cssText` for declarations containing only custom properties | **Returns empty string** (`generateCSSText` does not iterate `m_cssCustomValues`). |
| `setProperty('--x', v, 'important')` + `getPropertyPriority('--x')` | **`!important` is silently dropped** — `setCustomProperty` has no priority parameter. `getPropertyPriority('--x')` always returns `""`. |
| `@property { ... }` at-rule | **Not parsed** — silently dropped. |
| `CSS.registerProperty(...)` | **Not exposed** — `undefined`. No typed custom properties. |

### Tables — runtime caveats

| Property/Construct | Status |
|--------------------|--------|
| `<table>` with `border-collapse`, `border-spacing` (single length), `table-layout`, `caption-side: top|bottom`, `empty-cells`, `vertical-align` on `<td>` | All work. `display: table | table-row | table-cell | table-row-group | table-caption` on non-table elements works (anonymous box wrapping). |
| `border-spacing: <h> <v>` (two-value form) | **🔥 SIGSEGV** — `getComputedStyle().borderSpacing` reads uninitialized `m_multiValue` (`ComputedStyleCSSStyleDeclaration.cpp:1750`). Use a single length only. |
| `caption-side: left | right` | **NOT supported** — silently dropped (the value list at parser-side has only `top`/`bottom`). |
| `<caption>` without an explicit CSS `width` | Hits `STARFISH_UNIMPLEMENTED` (`FrameTableBox.cpp:1336`); in debug this floods logs every layout pass and may abort. **Always set `width` on `<caption>`.** |
| `table-layout: fixed` with inline `<td width=>` | Does NOT actually constrain column widths — falls back to `auto` layout. If you need fixed table layout, use `<col>` with explicit widths. |

### Multi-column — runtime caveats

`column-count`, `column-width`, `columns` (shorthand), `column-rule[-style|-width|-color]`, `column-span`, `column-fill`, `break-before`/`-after`/`-inside` are **entirely unsupported** — none of these properties are in the parser's lookup trie, and there is no `FrameMultiColumnBox` / fragmentation engine. Each declaration logs `Unsupported css property:` and is dropped.

`column-gap` IS recognized but only as the **unified flex/grid `gap` property** — it has no effect on a non-flex/non-grid container. To emulate columns, use `display: grid; grid-template-columns: repeat(N, 1fr); gap: <length>`.

### Page / Print / Break — runtime caveats

LWE has **zero** support for CSS Paged Media:

| Construct | Status |
|-----------|--------|
| `@page` at-rule (incl. `:first`/`:left`/`:right`) | **Not parsed** — entire rule silently dropped from `cssRules`. No `CSSPageRule` IDL. |
| `page-break-before` / `page-break-after` / `page-break-inside` | **Unsupported** — `Unsupported css property` warning, dropped. |
| `break-before` / `break-after` / `break-inside` (modern) | **Unsupported.** |
| `orphans`, `widows` | **Unsupported.** |
| `page` (shorthand) | **Unsupported.** |
| `@media print` | The engine's `MediaQueryEvaluator` is permanently `"screen"` (`Style.cpp:10271-10277`). `matchMedia('print').matches === false` always. **Rules inside `@media print` are statically unreachable.** |
| `window.print()` | **Not exposed** (`undefined`). No print pipeline of any kind. |

### Containment / will-change / @container — runtime caveats

| Construct | Status |
|-----------|--------|
| `will-change` | Parses. **Tokens `transform` and `opacity` DO create a stacking context AND set `m_needsGraphicsBuffer`** (`Frame.cpp:1598-1610`) — earlier docs that called this a no-op were wrong. Other tokens (`scroll-position`, `contents`, custom-ident) are stored but inert. **Bug:** `getPropertyValue('will-change')` returns empty string for `auto` (should be `"auto"`). **Bug:** `CSS.supports('will-change', 'transform')` returns `false` despite the property being supported. |
| `contain` (any value: `none`/`layout`/`paint`/`size`/`style`/`content`/`strict`/`inline-size`/`block-size`) | **NOT recognized** — declaration silently dropped. No layout/paint isolation available. |
| `content-visibility: visible / hidden / auto` | **NOT recognized.** `content-visibility: hidden` does NOT hide the subtree. Use `display: none`. |
| `contain-intrinsic-size` | **NOT recognized.** Cannot reserve space for skipped subtrees. |
| `container-type` / `container-name` / `container` (shorthand) | **NOT recognized.** |
| `@container` at-rule | **Not parsed** — entire rule silently discarded. **Container Queries do not work at all.** Use `@media` (viewport) + JS `resize` polling for width-based logic. |
| `@starting-style` at-rule | **Not parsed** — silently discarded. Workaround: set the starting value, force layout (`offsetHeight`), then change to the end value, OR use double `requestAnimationFrame`. |

### CSS-wide keywords / `all` shorthand — runtime caveats

| Construct | Status |
|-----------|--------|
| `all: initial` / `inherit` / `unset` / `revert` (the CSS Cascade `all` shorthand) | **Never applies.** `Style.cpp::updateValueAll` is a TODO stub that returns `false` for every input — declarations like `all: unset` are silently dropped. |
| Per-property `<prop>: initial` | Works at the cascade level — the property reverts to its initial value. |
| Per-property `<prop>: inherit` | Works for inheritable properties (and at the cascade level for non-inheritable). |
| Per-property `<prop>: unset` / `revert` / `revert-layer` | Behavior is partial — `unset` is interpreted as either `initial` or `inherit` per spec for known properties, but `revert` and `revert-layer` are NOT understood by `applyProperty` and may fall back silently. |

### CSS Houdini — runtime caveats

CSS Houdini support is **essentially absent**. Only a cosmetic Typed-OM façade is exposed.

| Surface | Status |
|---------|--------|
| `CSS.registerProperty(...)`, `CSS.paintWorklet`, `CSS.layoutWorklet`, `CSS.animationWorklet`, `Worklet`, `PaintWorkletGlobalScope`, `CSSPaintCallback` | **All `undefined`.** No worklets, no Properties & Values API. |
| `paint(<name>)` CSS function | Silently dropped — `background-image: paint(...)` resolves to `none`. |
| `el.attributeStyleMap`, `el.computedStyleMap()`, `StylePropertyMap`, `StylePropertyMapReadOnly` | **All absent** — use `el.style.*` and `getComputedStyle(el).*`. |
| `CSSStyleValue`, `CSSKeywordValue`, `CSSUnitValue`, `CSSNumericValue` constructors | Exposed as cosmetic stubs. `value`/`unit` round-trip; **arithmetic methods (`add`/`sub`/`mul`/`div`/`min`/`max`/`equals`/`to`/`toSum`/`type`) are commented out** in IDL — `undefined` at runtime. |
| `CSSImageValue`, `CSSTransformValue`, `CSSMathValue`, `CSSURLImageValue` | **NOT exposed.** |
| `CSSStyleValue.parse(prop, cssText)` | **🔥 Crashes the engine** — generated binding asserts `result != nullptr` and the C++ stub returns `nullptr` (`CSSStyleValue.cpp:53`); SIGABRT on call. **Do NOT call.** |
| `CSSStyleValue.parseAll(...)` | Returns empty array (safe). |
| `CSS.escape(ident)` | **`undefined`** (declared `[Unimplemented]`). Use a polyfill or manual `\` escaping. |
| `CSS.supports(prop, value)` / `CSS.supports(condition)` | Function exists but **returns `false` for valid declarations** including `color: red`, `display: grid`, `aspect-ratio: 1`, `--x: 1`, all gradient functions including the working `linear-gradient`. **Treat negative results as inconclusive** — feature-detect via setting an inline value and reading `getComputedStyle` instead. |

### CSS Image functions — runtime caveats

Of the function set in CSS Images L4, only `linear-gradient(...)` and `radial-gradient(...)` work. Everything else silently resolves the entire declaration to `none` (no warning, no comma-list fallback).

| Function | Status |
|----------|--------|
| `linear-gradient`, `radial-gradient` | OK. |
| `conic-gradient`, `repeating-linear-gradient`, `repeating-radial-gradient` | **Silently → `none`.** |
| `image-set(...)` (CSS3 url-quoted form, CSS4 bare-string form, with `type(...)`) | **Silently → `none`.** No DPR-based image picker — use a single `url()` (typically @2x) or branch via `@media (min-resolution: 2dppx)`. |
| `cross-fade(...)` | **Silently → `none`.** Use a layered overlay with separate `<img>` + opacity transitions. |
| `element(#id)` | **Silently → `none`.** No live source painter. |
| `image(...)` (with directional / fallback color) | **Silently → `none`.** |
| `paint(<name>)` | **Silently → `none`** (Paint Worklet absent). |
| `border-image-source` | **Only `url(...)` works** — even gradients are rejected at apply-time (`STARFISH_UNSUPPORTED("css property: gradient")`). |

> **Trap:** an unsupported function in a comma-list of background-images fails the **entire** declaration. `background-image: image-set(...), url('fallback.png')` produces `none`, not the fallback. Put fallbacks in a separate earlier rule (cascade) instead.

### Gradient syntax detail — runtime caveats

`linear-gradient(...)` and `radial-gradient(...)` are the only gradient functions parsed. Within them:

| Syntax | Status |
|--------|--------|
| `linear-gradient(<angle>, c1, c2, ...)` (e.g. `45deg`, `0.25turn`) | OK. Negative angles allowed. |
| `linear-gradient(to <side-or-corner>, ...)` (`to top`, `to bottom right`, …) | OK. The four sides + four corner combinations parse. |
| `linear-gradient(c1, c2)` (no angle/side ⇒ default `to bottom`) | OK. |
| `radial-gradient(<shape> <size> at <position>, ...)` | OK. Shapes: `circle`, `ellipse`. Sizes: `closest-side`, `closest-corner`, `farthest-side`, `farthest-corner` (default), explicit length(s). |
| `radial-gradient(at <position>, ...)` (no shape/size) | OK. |
| Color-stop position (`red 50%`, `blue 200px`) | OK. |
| **Color hint** between two stops (`red, 30%, blue`) — single bare percentage between two color stops | Parser accepts the syntax but the **interpolation hint behavior is approximate**; treat as a smoothing nudge only. |
| **Multi-position color stop** (`red 0% 25%`, two positions on one stop) | **NOT supported** — only the first position is parsed, the second is dropped silently. |
| Modern color spaces in stops (`linear-gradient(in oklch, ...)`) | **NOT supported** — `in <colorspace>` clause is not recognized; the `in` token aborts gradient parsing. |
| `conic-gradient(...)`, `repeating-linear-gradient(...)`, `repeating-radial-gradient(...)` | **Silently → `none`** (already documented). |

### Writing-mode + isolation — runtime caveats

| Property | Status |
|----------|--------|
| `writing-mode` (`horizontal-tb`/`vertical-rl`/`vertical-lr`/`sideways-rl`/`sideways-lr`) | **NOT recognized** by the parser — only the legacy `direction: ltr/rtl` controls bidi. Vertical text layout is unavailable. |
| `text-orientation` | **NOT recognized** (already documented under text properties). |
| `isolation` (`auto`/`isolate`) | **NOT recognized** — no CSS-driven stacking-context isolation. Use `position` + `z-index` instead. |
| `image-orientation` | **NOT recognized** (already documented). |
| `background-blend-mode` | **NOT recognized** (already documented in main background row). |

### 3D transform context — runtime caveats

3D transform functions (`matrix3d`, `translate3d`, `translateZ`, `scale3d`, `scaleZ`, `rotate3d`, `perspective`) parse via the `transform` property (already documented in the main table). However, the CSS properties that establish or control the 3D rendering context are **not** in the parser trie:

| Property | Status |
|----------|--------|
| `perspective` (as a CSS property, e.g. `perspective: 800px`) | NOT recognized as a property — only as a `transform: perspective(...)` function. Without the property, an ancestor cannot establish a 3D viewing distance for descendants. |
| `perspective-origin` | NOT recognized. |
| `backface-visibility` (`visible`/`hidden`) | NOT recognized — back faces of `rotate3d`-flipped elements cannot be hidden by CSS; manage via JS or pre-rendered alternatives. |
| `transform-style` (`flat`/`preserve-3d`) | NOT recognized — children of 3D-transformed elements flatten by default and there is no way to opt into a single 3D rendering context. |
| `transform-box` (`view-box`/`fill-box`/`stroke-box`/`border-box`) | NOT recognized — uses the spec-default reference box only. |

> **Practical guidance:** treat all 3D transforms as best-effort 2D-projection cosmetic effects. Cards that flip/spin in 3D will work for simple single-element rotations but cascading 3D layouts (parent perspective, preserved-3d nested children, hidden back faces) are not available.

### Logical sizing properties — runtime caveats

The logical-direction sizing properties from CSS Logical Properties 1 are **not** in the parser trie:

| Property | Status |
|----------|--------|
| `inline-size`, `block-size` | NOT recognized — use `width`/`height` directly. |
| `min-inline-size`, `min-block-size`, `max-inline-size`, `max-block-size` | NOT recognized. |
| `inset-block`, `inset-inline`, `inset-block-start`, `inset-block-end`, `inset-inline-start`, `inset-inline-end` | NOT recognized. **Note:** physical `inset` shorthand IS supported (already documented in the add-on table above). |
| `padding-block-start`/`-end`, `padding-inline-start`/`-end`, `padding-block`, `padding-inline` | **Recognized** (already in main + add-on tables). |
| `margin-block-start`/`-end`, `margin-inline-start`/`-end`, `margin-block`, `margin-inline` | **Recognized**. |
| `border-block-start`/`-end`, `border-inline-start`/`-end` (incl. `-color`/`-style`/`-width`) | **Recognized**. |
| `border-start-start-radius`, `border-start-end-radius`, `border-end-start-radius`, `border-end-end-radius` | NOT recognized. Use the four physical radius properties. |

> **Practical guidance:** physical longhands cover the typical needs. The audit-added logical entries are mostly margin/padding/border shorthands; modern `inline-size`/`block-size` and `inset-*-*` longhands are absent.

### CSS Text 4 wrapping — modern surface absent

The CSS Text Module Level 4 wrapping/whitespace shorthands are unavailable. Stick to `white-space` + `word-break` + `overflow-wrap`/`word-wrap`.

| Construct | Status |
|-----------|--------|
| `white-space-collapse` (longhand) | NOT recognized — only the legacy combined `white-space` property parses. |
| `white-space: break-spaces` | NOT recognized — only `normal`/`nowrap`/`pre`/`pre-wrap`/`pre-line` parse. |
| `text-wrap` (`wrap`/`nowrap`/`balance`/`pretty`/`stable`) | NOT recognized. |
| `text-wrap-mode`, `text-wrap-style` | NOT recognized. |
| `wrap-before`, `wrap-after`, `wrap-inside` | NOT recognized. |
| `line-clamp` (unprefixed) | NOT recognized — use `-webkit-line-clamp` (already documented). |
| `text-spacing` | NOT recognized. |

### Text properties — modern surface gaps

| Property | Status |
|----------|--------|
| `quotes` | NOT recognized — `<q>`/`<blockquote>` use UA defaults; cannot customize quote characters. |
| `text-emphasis`, `text-emphasis-color`, `text-emphasis-position`, `text-emphasis-style` | NOT recognized. East-Asian emphasis marks unsupported. |
| `text-justify` | NOT recognized. |
| `text-align-last` | NOT recognized. |
| `text-orientation` (`mixed`/`upright`/`sideways`) | NOT recognized. |
| `text-combine-upright`, `text-spacing-trim`, `text-autospace` | NOT recognized. |
| `hanging-punctuation` | NOT recognized. |
| `text-decoration-thickness`, `text-underline-offset` | NOT recognized — only `text-decoration-line/style/color` and `text-underline-position` parse. |
| `text-emphasis-skip`, `text-skip-ink` | NOT recognized. |

### Aspect-ratio + intrinsic sizing — runtime caveats

| Construct | Status |
|-----------|--------|
| `aspect-ratio: <ratio>` (CSS Sizing 4 box property, e.g. `aspect-ratio: 16 / 9`) | **NOT recognized.** No entry in `CSSStyleLookupTrie`. The `aspect-ratio` token IS a recognized `@media` feature, but the box-level property is silently dropped — declarations log `Unsupported css property: aspect-ratio` and produce no constraint. |
| `aspect-ratio: auto <ratio>` (the two-value form preserving intrinsic ratio fallback) | **NOT recognized.** |
| Implicit aspect ratio from `<img width=H height=W>` | Honored by layout (intrinsic ratio used during image load placeholder phase), but only via the HTML attributes, not via CSS. |
| `width: min-content`/`max-content`/`fit-content`/`available` | Parsed (already documented in main width/height table); honored by layout. **`fit-content(<length>)` function form is NOT recognized** — only the bare keyword. |
| `contain-intrinsic-size` | NOT recognized (already documented under containment). |

### Object-fit / Object-position / Image rendering — runtime caveats

| Property | Status |
|----------|--------|
| `object-fit` (`fill`/`contain`/`cover`/`none`/`scale-down`) | **Apply to `<img>` ONLY.** `<video>`, `<canvas>`, `<object>`, `<iframe>`, SVG-as-replaced ignore the property — `computeObjectFit()` is invoked only from `FrameReplacedImage.cpp`. Workaround: wrap the element in `overflow:hidden` and size the inner element manually. |
| `object-position` (1/2/4-token forms, keywords/`%`/length) | Same constraint — `<img>` only. |
| `image-rendering` | Only **`auto`**, **`crisp-edges`**, **`pixelated`** parse. **`smooth`, `high-quality`, `optimizeSpeed`, `optimizeQuality`, `-webkit-optimize-contrast`** are silently rejected at value parse. |
| `image-orientation` | **NOT in the CSS trie at all** — every value silently dropped (incl. `from-image`, `none`, angles, `<angle> flip`). EXIF auto-rotation is NOT honored on `<img>`. (`ImageBitmapOptions.imageOrientation` for `createImageBitmap()` is a separate API and is supported with `none`/`flipY`.) |

### Line clamp — runtime caveats

| Construct | Status |
|-----------|--------|
| `display: -webkit-box; -webkit-box-orient: vertical; -webkit-line-clamp: <n>; overflow: hidden` | **Works** (gated by `STARFISH_ENABLE_CSS_WEBKIT_LINE_PREFIX`). |
| `-webkit-line-clamp: none` | Disables clamp. |
| Unprefixed `line-clamp: <n>` (CSS Overflow 4) | **NOT in the parser trie** — entire declaration dropped with `Unsupported css property: line-clamp` warning. Use `-webkit-line-clamp` only. |
| `-webkit-line-clamp` on a non-`-webkit-box` ancestor / inline / replaced / fixed-height items | Silently no-op (no clamp, no warning). |
| `-webkit-line-clamp` with `direction: rtl` | **Silently skipped** — `FrameFlexibleBox.cpp:1643` excludes RTL. |
| `getComputedStyle(el).webkitLineClamp` | **Always `undefined`** (bug — `ComputedStyleCSSStyleDeclaration.cpp:918-922` builds the value but never calls `addValuePair`). JS introspection unreliable until fixed. |

### SVG presentation properties

The following CSS properties are recognized by the parser (entries exist in `CSSStyleLookupTrie` and `Style.h:FOR_EACH_STYLE_ATTRIBUTE_BASIC`) and have full `updateValue*` implementations. They are **valid CSS** at the cascade and computed-style level, round-trip through `getComputedStyle`, and **are painted** on SVG elements. See the [SVG](#svg) section for the full SVG rendering surface.

| Property | Parsed values | Note |
|----------|--------------|------|
| `fill` | `<color>` &#124; `none` &#124; `url(#id)` | Already listed for `<canvas>`, but as a CSS property targets SVG `<path>`/`<circle>`/etc. |
| `fill-opacity`, `fill-rule` | `<number>` (0–1); `nonzero` &#124; `evenodd` | |
| `stroke`, `stroke-opacity`, `stroke-width` | `<color>`/`url(#id)`; `<number>`; `<length>` | |
| `stroke-linecap`, `stroke-linejoin`, `stroke-miterlimit` | `butt`/`round`/`square`; `miter`/`round`/`bevel`; `<number>` | |
| `stroke-dasharray`, `stroke-dashoffset` | `<dasharray>`; `<length>` | |
| `stop-color`, `stop-opacity` | `<color>`; `<number>` | For SVG `<stop>`. |

### CSS Shapes — runtime caveats

| Construct | Status |
|-----------|--------|
| `shape-outside`, `shape-margin`, `shape-image-threshold` | **NOT in the trie at all** — silently dropped. No layout integration; floats wrap with rectangular margin boxes only. Computed-style getters return `undefined`. |
| `clip-path: url(#id)` | Parses, **but applied only on SVG elements** (`SVGElement::clipPathElement` is the sole consumer). On HTML boxes the value is parsed but never used during paint. |
| `clip-path: inset() / circle() / ellipse() / polygon() / path() / shape()` | **Silently rejected by `updateValueClipPath`** (which accepts only `url(...)`). `getComputedStyle().clipPath` returns `url("")`. Earlier audit (Iter 41) was incorrect — basic-shape clip-paths do NOT work on HTML elements. |

### CSS Anchor Positioning + View Transitions — runtime caveats

**Both feature sets are ENTIRELY unsupported.**

| Surface | Status |
|---------|--------|
| `anchor-name`, `position-anchor`, `inset-area`, `position-try-options`, `position-try-fallbacks`, `position-visibility` | All NOT recognized — `Unsupported css property` warnings, declarations dropped. |
| `top: anchor(--name bottom)`, `left: anchor(--name right, fallback)` | Parser doesn't know `anchor()` function; the property may store a partial length and produce **meaningless layout** (target ends near anchor's wrong edge with no fallback applied). |
| `@position-try` at-rule | Not parsed. |
| `view-transition-name` | NOT recognized. |
| `@view-transition` at-rule | Silently dropped from `cssRules`. |
| `::view-transition`, `::view-transition-group`, `::view-transition-image-pair`, `::view-transition-old`, `::view-transition-new` | **Not in pseudo-element enum** — silently dropped. |
| `document.startViewTransition(callback)` | **`undefined`** — not even a stub. |

### Modern color functions — runtime caveats

The color parser (`CSSPropertyParser::parseNonNamedColor`) recognizes only legacy notations: `#hex` (3/4/6/8), `rgb()`/`rgba()`, `hsl()`/`hsla()`, named colors, `transparent`, `currentColor`. Everything else is **silently dropped at parse time** — the entire declaration is rejected (the resulting fallback is the *initial* value of the property, NOT `rgb(0,0,0)` as previously stated).

| Function | Status |
|----------|--------|
| `hwb(H W% B% [/A])` | **NOT supported** — declaration dropped. |
| `lab(...)`, `lch(...)`, `oklab(...)`, `oklch(...)` | **NOT supported.** |
| `color(<colorspace> ...)` (all spaces incl. `srgb`, `display-p3`, `rec2020`) | **NOT supported.** |
| `color-mix(in <space>, c1, c2)` | **NOT supported.** |
| `color-contrast(...)` | **NOT supported.** |
| Relative color syntax `rgb(from red r g b)` etc. | **NOT supported.** |
| `light-dark(c1, c2)` | **NOT supported** (use a hard-coded color or branch in JS). |
| System colors (`AccentColor`, `Canvas`, `CanvasText`, `LinkText`, `VisitedText`, `ButtonFace`, `ButtonText`, etc.) | **NOT in the named-color table** — silently dropped. |

> No `Unsupported css ...` warning fires for unknown color *values* (the warning only fires for unknown *property names*). Authors get no diagnostic; the only signal is that the property reverts to its initial value.

### CSS Math functions — runtime caveats

| Function | Status |
|----------|--------|
| `calc(...)`, nested `calc()`, `calc(var(--x) * 2)` | OK. |
| `min(a, b)` / `max(a, b)` | **Hard-capped at exactly 2 arguments.** `min(a, b, c)` → declaration rejected (silent — falls back to initial). |
| `clamp(a, b, c)` | OK (exactly 3 args required). |
| `round()`, `mod()`, `rem()` | **NOT recognized** at top level — declarations rejected. |
| `abs()`, `sign()` at top level | **NOT recognized.** |
| `pow()`, `sqrt()`, `hypot()`, `log()`, `exp()`, `sin()`, `cos()`, `tan()`, `asin()`, `acos()`, `atan()`, `atan2()` at top level | **NOT recognized.** |
| **Inside `calc(...)` context** the parser silently swallows unknown function names, treating them as parenthesized sub-expressions. **This produces wrong numeric results without any error.** Examples observed: |
| `calc(sqrt(16) * 10px)` | Returns `160px` (treats `sqrt(16)` as `16` — no real sqrt). |
| `calc(sign(-5) * 50px)` | Returns `-250px` (treats as `-5*50` — no real sign). |
| `calc(log(2.718) * 10px)` | Returns `27.0312px` (treats as `2.718 * 10` — no real log). |
| `calc(sin(45deg) * 1deg)` | Treats `sin(...)` as the inner angle; produces garbage matrix. |
| Constants `pi`, `e`, `infinity`, `-infinity`, `NaN` | **NOT supported** anywhere — declaration rejected. |
| `calc()` inside `rgb()`/`hsl()` channels | **NOT supported** — `rgb(calc(255/2), 0, 0)` resolves to `rgba(0, 0, 0, 0)`. (Note: `var()` inside `rgb()` channels DOES work.) |

> ⚠️ **The silent-success failure mode is the worst trap.** The engine accepts `sqrt`/`pow`/`sin`/`abs` syntax inside `calc()` and returns numerically wrong values — no warning, no error. Static lint is the only practical guard.

### Font properties — modern surface absent

The following font-related CSS properties are **not in the parser trie** at all (`CSSStyleLookupTrie.cpp` has no entry; declarations log `Unsupported css property: <name>` and are dropped). All variable-font / OpenType / locale-extension surfaces are unavailable:

| Property / descriptor | Status |
|-----------------------|--------|
| `font-stretch` (`condensed`/`expanded`/`<percentage>`) | NOT recognized. |
| `font-variant` (and `font-variant-caps`/`-numeric`/`-ligatures`/`-east-asian`) | NOT recognized. |
| `font-feature-settings` (`"liga"`, `"dlig"`, …) | NOT recognized. |
| `font-variation-settings` (`"wght"`, `"wdth"`, …) | NOT recognized — variable-font axis tuning is unavailable. |
| `font-size-adjust` | NOT recognized. |
| `font-synthesis` (`weight`/`style`/`small-caps`/`none`) | NOT recognized — the engine cannot opt out of synthesizing missing weights/italics. |
| `font-optical-sizing` | NOT recognized. |
| `font-language-override` | NOT recognized. |
| `font-palette`, `@font-palette-values` | NOT recognized. |
| `@font-face` `unicode-range` descriptor | NOT recognized — entire `unicode-range:` declaration inside `@font-face` is silently dropped, so all glyphs from a face apply unconditionally. |
| `@font-face` `font-display` descriptor | NOT recognized — there is no FOIT/FOUT control. |
| `@font-face` `font-stretch`/`font-variant`/`font-feature-settings`/`font-variation-settings` descriptors | NOT recognized. |
| Generic family keywords | `serif`, `sans-serif`, `monospace`, `cursive`, `fantasy` parse. **`system-ui`, `ui-serif`, `ui-sans-serif`, `ui-monospace`, `ui-rounded`, `emoji`, `math`, `fangsong`** are NOT recognized as keywords; they fall back to family-name lookup which usually fails. |

> **Practical guidance:** authors targeting LWE should not assume any modern OpenType feature/variation control is available. Pre-pick a fixed family + weight stack and ship a separate `@font-face` per weight/style if needed.

### Form-control styling — runtime caveats

| Property/Pseudo | Status |
|-----------------|--------|
| `appearance: auto`, `appearance: none` | Supported, only on `FrameInputBox` (text inputs/buttons) — suppresses background/border/content paint. |
| `appearance: button / checkbox / radio / menulist / textfield / slider-horizontal / progress-bar / scrollbar* / etc.` | **Silently rejected** — declaration dropped, computed falls back to `auto`. |
| `-webkit-appearance` / `-moz-appearance` | **NOT recognized** — full declaration dropped with `Unsupported css property` warning. |
| `accent-color`, `color-scheme`, `forced-color-adjust` | **NOT recognized** — silently dropped. No way to tint native form widgets or signal dark-mode preference. |
| `:placeholder-shown` (pseudo-class) | **WORKS** — both `Element.matches()` and selector matching work (correction to earlier audit). |
| `::placeholder` (pseudo-element) | **NOT supported** — `checkPseudoElement` lacks the case (logs `Unsupported css pseudo-element: 60`). Style placeholder color via `:placeholder-shown { color: ... }` on the input itself. |
| Native `<input type=checkbox/radio>` chrome | LWE has no native checkbox/radio painter; `appearance: none` does NOT change the box dimensions. |

### Scroll-driven animations — runtime caveats

**Entirely absent** in LWE — both CSS surface and JS surface.

| Surface | Status |
|---------|--------|
| `animation-timeline`, `scroll-timeline`, `scroll-timeline-name`, `scroll-timeline-axis`, `view-timeline`, `view-timeline-name`, `view-timeline-axis`, `view-timeline-inset`, `animation-range`, `animation-range-start`, `animation-range-end`, `timeline-scope` | **NOT recognized** — declarations rejected with `Unsupported css property`. |
| `ScrollTimeline`, `ViewTimeline`, `AnimationTimeline`, `DocumentTimeline` constructors | **All `undefined`.** `new ScrollTimeline(...)` throws `ReferenceError`. |
| `document.timeline` | **`undefined`.** |
| `CSS.supports(...)` for any timeline property | Returns `false`. |
| **Workaround** | Drive via `scroll` event + `requestAnimationFrame` + manual `transform` updates. |

### CSS Nesting (`&` selector) — runtime caveats

**Entirely unsupported.** No `&` selector handler in `CSSParser::getSimpleSelector` (falls into `m_failedParsing = true`). No nested-rule entry in `parseStyleRule`. No `@nest` at-rule branch. `CSSStyleRule.cssRules` getter doesn't exist.

| Form | Status |
|------|--------|
| `.parent { & .child { … } }` (Level 1 with `&`) | **Inner rule silently dropped.** Outer declarations *before* the inner block survive; outer declarations after may be lost. |
| `.parent { .child { … } }` (relaxed form, no `&`) | **🔥 Dangerous: can wipe the entire parent rule's body** — `.a { .b { color: red } }` ends up with an empty body for `.a` (CSSOM `cssText === ""`). Looks like "CSS just didn't load." |
| `.btn { &:hover { … } }`, `.btn { &.primary { … } }`, `.foo { & + & { … } }` | All silently dropped. |
| `.box { & @media (min-width:100px) { … } }` (nested `@media`) | Inner block rejected; nested at-rule not recognized. |
| `@nest .child & { … }` (legacy form) | `@nest` treated as unknown at-rule, swallowed (and may swallow following rules). |
| Multi-level nesting (`.a { .b { .c { … } } }`) | Entire outer rule emptied. |
| **Workaround** | Use a build-time preprocessor (PostCSS-nesting / Sass / Lightning CSS) to flatten nested rules to plain CSS before shipping to LWE. |

### UI properties — runtime caveats

| Property | Status |
|----------|--------|
| `cursor` | Parsed into an inherited computed-style value (keyword form only — `url()` image cursors unsupported). Tracked keywords: `auto`, `default`, `pointer`, `none`; all other valid keywords collapse to `CursorOtherValue` (serializes to `auto` via `getComputedStyle` since the concrete keyword is not retained). The `pointer` value drives the tap-sound (link effect) feedback on Tizen. No cursor is rendered. (See dedicated note in earlier sections.) |
| `user-select` | Only **`auto`** and **`none`** parse. `text` / `all` / `contain` are rejected at parse time (the C++ assigns the enum then returns `false`, so the value is dropped). UA wildcard `* { user-select: none }` makes the practical default `none`. |
| `-webkit-user-select` | **NOT recognized.** |
| `user-modify` | **NOT recognized.** |
| `resize` | Only **`none`** parses. `both` / `horizontal` / `vertical` / `block` / `inline` are rejected. **No resize-grip painter exists** anyway — even if accepted, no widget would render. |
| `pointer-events` | All 10 values (`auto`/`none`/`visiblePainted`/`visibleFill`/`visibleStroke`/`visible`/`painted`/`fill`/`stroke`/`all`) parse and round-trip via `getComputedStyle`. **However:** the value is NOT consulted in hit-testing — `core/event/`, `core/page/` never read `pointerEventsValue()`. **`pointer-events: none` does NOT block click delivery** for arbitrary elements in this build. (Anchor disabled-state may behave differently via UA pseudo-class logic.) |
| `touch-action` | **NOT recognized.** |
| `caret-color: <color> / transparent` | Parses and stores. Visually relevant only when LWE paints a caret (limited). |
| `caret-color: currentcolor` | **🐛 Bug** — apply path copies the *parent*'s `caret-color` rather than the element's own resolved `color` (`Style.cpp:7634`). Use a literal color instead. |
| `caret-shape` | **NOT recognized.** |
| `accent-color` | **NOT recognized.** |

### Cascade & specificity — runtime caveats

| Construct | Status |
|-----------|--------|
| Standard 3-tuple specificity (ID × 0x10000 + class/attr/pseudo-class × 0x100 + tag/pseudo-element × 1) | Implemented per `Style.cpp::specificityForOneSelector`. |
| `:host` specificity | **Returns `0`** (not the spec-required pseudo-class weight). |
| `:where(...)` (zero-specificity wrapper) | Implemented — contributes zero specificity, per Selectors 4. |
| `:is(...)`, `:not(...)` | Implemented — contribute the specificity of their most specific branch, per Selectors 4. |
| `::slotted(...)` | Implemented — contributes its own pseudo-element unit plus its argument compound. |
| `:has(...)` | Parse-fail. |
| `<style>` element vs inline `style=""` | Inline declarations win against same-specificity rules per spec. **`!important` from `<style>` elements correctly overrides inline non-important.** |
| `!important` on a normal property | Honored (`setFlagImportant(true)` on the value pair). |
| `!important` on a CSS custom property (`--x: 1 !important;`) | **Silently dropped** — `setCustomProperty` has no priority parameter; `getPropertyPriority('--x')` always returns `""`. (Already noted under Custom Properties.) |
| Computed-style `unset` reduction | **Partial.** `unset` is applied as either `inherit` or `initial` per the property's inheritance attribute by `applyProperty`. |
| Computed-style `revert` / `revert-layer` reduction | **Not implemented** — falls through `applyProperty`'s `default:` and may behave like `unset`. |
| `@import` cascade ordering | `@import` rules are parsed and inlined; resulting cascade is in source order. **Cyclic `@import` detection is partial** — a stylesheet that imports itself triggers an infinite-load attempt that is bounded by the network layer's redirect/depth limit. |

### CSSOM — additional details

| Interface | Detail |
|-----------|--------|
| `CSSStyleDeclaration` | Adds `removeProperty(name)`, `getPropertyPriority(name)`, indexed getter (`style[i]` returns property name), camelCase named getter/setter (`style.color = ...`). CSS custom properties (`--*`) are supported via `setProperty`/`getPropertyValue`. |
|  | **Quirk:** `getComputedStyle()` returns a writable `CSSStyleDeclaration` (not frozen), `length === 0`, mutations don't throw. Iterate via known property names with `getPropertyValue("…")` instead of indexed `length`. |
| `CSSRule` | The `cssText` setter is **silently ignored** despite being writable in IDL. |
| `CSSStyleSheet` | **Constructable** — `new CSSStyleSheet(options)` works, with `replace()` (returns a `Promise`) and `replaceSync()`. `adoptedStyleSheets` is exposed on both `Document` and `ShadowRoot` as an observable array. `rules === cssRules` (legacy alias is the same live list). `insertRule` throws `SyntaxError` on bad text and `IndexSizeError` on out-of-range index; `deleteRule` throws `IndexSizeError` on out-of-range. |
| `CSSFontFaceRule` | The `style` accessor **crashes the engine in debug builds** (`Assertion 'isCSSStyleRule()' failed.`). Treat `CSSFontFaceRule.style` as unsupported. |
| `CSSNamespaceRule`, `CSSCounterStyleRule` | Produced by the parser. `CSSNamespaceRule` exposes `prefix` and `namespaceURI`. |
| `CSSPageRule` | The CSSRule-type constant exists but the parser never produces a rule of this type. |
| `MediaList` | `mediaText` is also a stringifier (`String(ml)` serializes). Indexed getter `ml[i]` works. `deleteMedium(name)` throws `NotFoundError` if the medium isn't in the list. |
| `MediaQueryList` | Inherits `EventTarget` (`addEventListener('change', ...)` works). **The engine does NOT auto-dispatch `change` on viewport changes** — listeners only fire if app code calls `dispatchEvent` manually. `matchMedia(invalidQuery)` returns `MediaQueryList` with `media === "not all"`; does not throw. |
| `StyleSheet` | `href` returns empty string `""` for inline `<style>` sheets (IDL nullable; engine never returns `null`). |
| Loader | `<link rel=stylesheet href="data:text/css,...">` is **not loaded** — `link.sheet` is `null`. `@import url("data:text/css,...")` does not produce a `CSSImportRule`. |
| `CSS` namespace | `CSS.supports(...)` is implemented. **`CSS.escape` is NOT implemented** — `undefined`. |
| CSS Typed OM | `CSSStyleValue`, `CSSKeywordValue`, `CSSUnitValue`, `CSSNumericValue` constructors exposed but the API surface is essentially empty (most operations are commented out in IDL). Treat as experimental — do not use in webapps. |

### HTML head & embedded elements — runtime caveats

| Element | Detail |
|---------|--------|
| `HTMLLinkElement` | Adds `sheet` (via `LinkStyle` mixin — `null` until stylesheet load completes; only resolved for `rel="stylesheet"`). **`[Unimplemented]` (read as `undefined`):** `as`, `integrity`, `sizes`, `disabled`, `imageSrcset`, `imageSizes`, `scope`, `workerType`, `useCache`. Subresource Integrity is not enforced. |
| `HTMLStyleElement` | Adds `nonce`, `sheet`. **`disabled` is reflected but does NOT detach the stylesheet from the cascade** — toggle by removing the element instead. |
| `HTMLScriptElement` | Adds `async`, `defer`, `nonce`. **`[Unimplemented]`:** `integrity`. **Not in IDL at all:** `referrerPolicy`, lowercase `script.nomodule` alias (only camelCase `noModule` works). LWE does not implement `type="module"` / import maps. |
| `HTMLMetaElement` | Adds row to DOM table. Implemented: `name`, `content`, `httpEquiv`. **Not in IDL at all:** `charset`, `scheme`, `media` — read as `undefined`. The `<meta charset>` content attribute IS honored by the parser (read it via `meta.getAttribute('charset')`), and `<meta http-equiv="content-security-policy">` and `="content-type"` are honored; other pragmas (`refresh`, `default-style`, `x-ua-compatible`) are inert. |
| `HTMLAnchorElement` | Implements full `HTMLHyperlinkElementUtils` (URL accessors and setters). **`[Unimplemented]`:** `download`, `ping`. Default click handler ignores `download`, `ping`, and `rel="noopener\|noreferrer"` semantics. |
| `HTMLAreaElement` | Same hyperlink-utils surface plus `coords`, `shape`, `noHref`. **`[Unimplemented]`:** `alt`, `download`, `ping`. |
| `HTMLImageElement` | Adds `naturalWidth`, `naturalHeight`, `complete`, `referrerPolicy`, `useMap`, `name`. **`[Unimplemented]`:** `alt`, `srcset`, `sizes`, `isMap`, `currentSrc`, `lowsrc`. **Not in IDL at all:** `loading`, `decoding`, `fetchPriority`, `decode()` Promise method. **Bug:** setting `el.crossOrigin` to any value other than `"use-credentials"` rewrites the DOM attribute to `"anonymous"`. **Bug:** intrinsic-aspect width/height fallback uses integer division. |
| `<picture>` | **NOT supported by the parser.** `<picture>` is rejected with `HTMLDocument: invalid (or unsupported) element: picture`; element falls through to `HTMLUnknownElement`. Source-set selection unavailable. |
| `HTMLIFrameElement` | Adds `srcdoc` (works; serialized to a `data:` URL internally). **`[Unimplemented]`:** `sandbox`, `allow`, `allowFullscreen`, `loading`, `csp`, `align`, `frameBorder`, `longDesc`, `marginHeight`, `marginWidth`, `getSVGDocument()`. **The `frameborder` HTML attribute IS honored by layout** but cannot be read/written via JS property. **Bugs:** `contentWindow` always throws `SecurityError` (even same-origin file:// → file://); `contentDocument` does NOT enforce same-origin (cross-origin `data:` child docs are readable). |
| `HTMLObjectElement` | **Only legacy reflectors implemented**: `align`, `archive`, `code`, `declare`, `standby`, `codeBase`, `codeType`, `border`. **All modern surface `[Unimplemented]`:** `data`, `type`, `name`, `typeMustMatch`, `useMap`, `width`, `height`, `form`, `contentDocument`, `contentWindow`, validation API. `<object>` renders as a sized blank box; resource loading happens only on `STARFISH_ENABLE_AVPLAY` builds via `type="application/avplayer"`. |
| `<embed>`, `<frame>`, `<frameset>` | **NOT registered with `HTMLDocument::createHTMLElement`** — all three become `HTMLUnknownElement`. The constructors `HTMLFrameElement` / `HTMLFrameSetElement` exist but `new` throws `"Illegal constructor"`; no `HTMLEmbedElement` IDL exists at all. Treat the entire frame-family as unsupported. |

### Console & error handling — runtime caveats

The `console` global is hand-written (not an IDL interface). The `CONSOLE_APIS` X-macro at `src/core/extra/Console.h` fixes the method set:

| Status | Methods |
|--------|---------|
| **Supported** | `log`, `info`, `warn`, `error`, `debug`, `assert(cond, ...)` (warn-level on false; never throws), `group`, `groupCollapsed`, `groupEnd`, `time(label)`, `timeLog(label, ...)`, `timeEnd(label)`. |
| **Not supported (TypeError on call)** | `trace`, `dir`, `dirxml`, `table`, `count`, `countReset`, `clear`, `profile`, `profileEnd`, `timeStamp`. Feature-detect with `typeof console.X === 'function'` before calling. |

`console.log` does **NOT** support `%s`/`%d`/`%o` formatter substitution (the spec's "Formatter" algorithm is not implemented); arguments are stringified individually and joined with spaces.

| Surface | Detail |
|---------|--------|
| `ErrorEvent` constructor | Fully implemented. `new ErrorEvent('error', {message, filename, lineno, colno, error})` round-trips all five fields, including `error` as an `Error` instance. |
| `document.createEvent('ErrorEvent')` | **Broken** — returns a plain `Event` (logs `STARFISH_UNSUPPORTED`). Use `new ErrorEvent(...)` instead. |
| `window.onerror` (attribute-style) | Receives 5 args: `(message, source, lineno, colno, error)`. `error` is the original thrown value. Spec-compliant. |
| `addEventListener('error', fn)` | Receives a single `ErrorEvent` argument. Spec-compliant. |
| `unhandledrejection` / `rejectionhandled` events | **Not implemented.** `PromiseRejectionEvent` constructor not exposed (`undefined`). `Promise.reject(...)` with no `.catch` is silently dropped — there is no `HostPromiseRejectionTracker` wiring. Always attach a `.catch` to top-level promise chains in LWE webapps. |

### Pointer / Keyboard / Touch / Drag events — runtime caveats

| Event interface | Detail |
|-----------------|--------|
| `KeyboardEvent` | Implements `key`, `code`, `keyCode`, `charCode`, `which` (via UIEvent), `ctrlKey`, `shiftKey`, `altKey`, `metaKey`, `repeat`, plus `DOM_KEY_LOCATION_*` constants. **`[Unimplemented]`:** `location`, `isComposing`, `getModifierState(key)` — read as `undefined`. The C++ already stores `location`/`isComposing`; the IDL annotation is stale. |
| `MouseEvent` | Adds `pageX`/`pageY` (read 0 on synthetic events because `MouseEventInit` does not surface them). **`[Unimplemented]`:** `offsetX`, `offsetY`, `movementX`, `movementY`, `x`, `y`, `layerX`, `layerY`, `getModifierState(key)`. **Bug:** `new MouseEvent('click', {altKey:true})` does NOT copy modifier keys from the init dict. |
| `PointerEvent` | Exposed; inherits MouseEvent. Implements only `pointerId`, `pointerType` on the prototype. **`[Unimplemented]`:** `width`, `height`, `pressure`, `tangentialPressure`, `tiltX`, `tiltY`, `twist`, `altitudeAngle`, `azimuthAngle`, `isPrimary`, `getCoalescedEvents()`, `getPredictedEvents()`. **Bug:** `new PointerEvent` ctor ignores `pointerId`/`pointerType` from init dict. **Bug:** `document.createEvent('PointerEvent')` throws "operation is not supported"; use `new PointerEvent(...)`. |
| `WheelEvent` | **Does NOT exist in LWE** — `WheelEvent === undefined`. `el.onwheel` accepts assignment but never fires. |
| `TouchEvent` / `Touch` / `TouchList` | All three constructors exposed (`typeof === 'function'`), but **`new TouchEvent(...)` throws `Illegal constructor`** — there is no JS-side way to construct/dispatch a TouchEvent. Real touch events fire only on touchscreen-capable shells (not on glfw/EFL desktop). `Touch` is constructible (`target`, `screenX/Y`, `clientX/Y` only); `TouchEvent.touches` is on the prototype but `targetTouches`/`changedTouches`/modifier flags are not. |
| `DragEvent` / `DataTransfer` / `DataTransferItem` / `DataTransferItemList` | **All four are absent** — no IDL, no C++. Globals are `undefined`. `HTMLElement.draggable` is `[Unimplemented]` — reflector returns `undefined`. All `ondrag*` handler slots accept assignment but never fire. |
| `InputEvent` | Constructible. Implements `data`, **`inputType`** (Spec.md previously didn't list this). **`[Unimplemented]`:** `dataTransfer`, `isComposing`, `getTargetRanges()`. |
| `CompositionEvent` | Constructible. Implements `data` only. **Not in IDL:** `locale`. `initCompositionEvent()` is `[Unimplemented]`. |

### Animation / Transition events — runtime caveats

| Surface | Status |
|---------|--------|
| `AnimationEvent` constructor | Works; `animationName`, `elapsedTime` round-trip. **`pseudoElement` is `[Unimplemented]`** — read as `undefined`. |
| `TransitionEvent` constructor | Works; `propertyName`, `elapsedTime`, `pseudoElement` round-trip. (Spec.md previously had no TransitionEvent row.) |
| `document.createEvent('AnimationEvent')` / `createEvent('TransitionEvent')` | **Broken** — both silently return a plain `Event` (logs `STARFISH_UNSUPPORTED`). Use `new AnimationEvent(...)` / `new TransitionEvent(...)` instead. |
| `addEventListener('animationstart' / 'animationend')` | Fires correctly. `event.animationName` and `event.elapsedTime` are populated. |
| `addEventListener('animationiteration')` | **NEVER fires.** No dispatch site for iteration boundaries; even animations with `animation-iteration-count: 3` do not emit this event. |
| `addEventListener('animationcancel')` | Implemented. |
| `addEventListener('transitionstart' / 'transitionend')` | Fires correctly. **Bug:** `event.elapsedTime` is always `0` for transition events (the C++ `fireTransition*Event` paths never call `init.setElapsedTime()`). |
| `addEventListener('transitionrun')` | **NEVER fires.** LWE collapses `transitionrun` into `transitionstart` (fired immediately on creation, regardless of `transition-delay`). |
| `addEventListener('transitioncancel')` | Implemented. |
| On-handler attributes (`onanimationstart`/`end`/`iteration`/`cancel`, `ontransitionstart`/`end`/`run`/`cancel`) | **None exist** as IDL attributes. Use `addEventListener` for all animation/transition events. |
| `getComputedStyle(el).opacity` during a running keyframe animation | Returns the **declared** value, not the interpolated value. The render output animates correctly but `getComputedStyle` is not animation-aware. |

### Web Animations API — runtime caveats

| Surface | Status |
|---------|--------|
| `Element.animate(keyframes, options)` | Works fire-and-forget — drives a CSS-animation pipeline. Returns an `Animation` instance (a thin `EventTarget` wrapper, no live link to the running animation). |
| `Animation` instance members | **All `[Unimplemented]`** — `id`, `effect`, `timeline`, `startTime`, `currentTime`, `playbackRate`, `playState`, `pending`, `ready`, `finished`, `replaceState`, `play()`, `pause()`, `cancel()`, `finish()`, `reverse()`, `updatePlaybackRate()`, `commitStyles()`, `persist()`. Reads return `undefined`; writes are silently kept as JS expandos with **no effect on the running animation**. |
| `Animation` event handlers | No `finish`/`cancel`/`remove` events are ever dispatched. Listeners attached via `addEventListener` never fire. |
| `new Animation()` | Throws `TypeError: Illegal constructor`. |
| `Element.getAnimations()` / `Document.getAnimations()` | **`undefined`** on the receivers — calling **throws `TypeError`** (not just returns undefined). |
| `document.timeline` | `undefined`. |
| `KeyframeEffect`, `AnimationEffect`, `AnimationTimeline`, `DocumentTimeline`, `AnimationPlaybackEvent` | **All `undefined`** — no IDL, no constructor exposed. |
| **Recommended LWE pattern** | Use `el.animate(...)` for one-shot effects, or define CSS `@keyframes` and toggle the `animation` shorthand. For controllable animation, drive via `requestAnimationFrame` + inline-style writes. |

### SVG family — runtime caveats

64 `SVG*.idl` files exist under `src/core/dom/svg/`; 45 element subclasses are registered in `SVGDocument::createSVGElement`. Inline `<svg>...</svg>` and `createElementNS('http://www.w3.org/2000/svg', tag)` produce correctly namespaced `SVG*Element` instances (NOT `HTMLUnknownElement`). `SVGAnimatedLength.baseVal.value` reads parsed attribute values; presentation attributes map to CSS (`getComputedStyle(rect).fill === 'rgb(255,0,0)'`); `<svg width/height>` allocates a real layout box (`getBoundingClientRect()` returns it). SVG elements are rendered — see the [SVG](#svg) section for the full supported surface.

**Known limitations** (methods authors typically expect but are missing):

| Surface | Status |
|---------|--------|
| `getBBox()`, `getCTM()`, `getScreenCTM()`, `getTotalLength()`, `getPointAtLength()`, `pathLength` | **Not on the prototype at all** — not even `[Unimplemented]`. Calling throws `TypeError`. |
| `SVGPoint`, `SVGRect`, `SVGMatrix` | **Not exposed** as constructable globals. |
| `SVGGraphicsElement`, `SVGGeometryElement` | **Not exposed** — every SVG element inherits directly from `SVGElement` without the SVG2 graphics-element layer. |
| `SVGSVGElement.createSVGRect()` / `createSVGPoint()` / `createSVGMatrix()` | Throw `TypeError` (`[Unimplemented]`). `createSVGLength()`, `createSVGNumber()`, `createSVGAngle()`, `createSVGTransform()` work. |
| `<foreignObject>` | Falls through to the generic `SVGElement` base (no `SVGForeignObjectElement` class). Crash-safety is covered by WPT crashtests, but no dedicated rendering. |

### ECMAScript engine (Escargot) — additional details

The JavaScript runtime is [Escargot](https://github.com/Samsung/escargot). Verified surface (against `./Starfish` glfw debug build):

**Language level — ES2024 + most ES2025 supported.** Operators (`?.`, `??`, `??=`/`||=`/`&&=`, `**`, `1_000_000`), classes (public/private fields, private methods, static initialization blocks, accessor pairs), async (`async`/`await`, `async function*`, `for await`), generators, regex flags (`g i m s u y d v` + named groups + lookbehind + `\p{…}`).

**Known gaps:**

| Feature | Status |
|---------|--------|
| Decorators | **Syntax error** (Stage-3 not landed in Escargot). |
| `Temporal` | **`undefined`** (Escargot built without `ESCARGOT_TEMPORAL`). |
| `ShadowRealm` | **`undefined`** (Escargot built without `ESCARGOT_SHADOWREALM`). |
| `Array.prototype.group` | **Not implemented** (replaced by `Object.groupBy`/`Map.groupBy` which both work). |
| `<script type="module">` and dynamic `import()` | **Not wired into LWE.** Author code as classic scripts only. Top-level `await` is therefore unavailable (it requires modules). |

**Built-ins verified present:** `BigInt`, `BigInt64Array`, `BigUint64Array`, `WeakRef`, `FinalizationRegistry`, `Atomics`, `SharedArrayBuffer`, `Proxy`, `Reflect`, `Symbol` (incl. `iterator`/`asyncIterator`/`hasInstance`), `globalThis`, `structuredClone`, `queueMicrotask`, `Iterator` (with helpers). `Promise.allSettled`/`any`/`finally`/`try`/`withResolvers`. Array `at`/`flat`/`flatMap`/`findLast`/`findLastIndex`/`toSorted`/`toReversed`/`toSpliced`/`with`. `Object.groupBy`, `Map.groupBy`, `Object.fromEntries`/`hasOwn`. `Set.prototype.union`/`intersection`/`difference`/`isSubsetOf`. `Math.f16round`, `Math.sumPrecise`.

`WebAssembly` is **not** part of that list: it is gated by `ENABLE_WASM=1`, which is off by default and is what turns on Escargot's `ESCARGOT_WASM` (`build/third_party.cmake`). Check `typeof WebAssembly` against your own build before depending on it.

### Media — additional details

| Interface | Detail |
|-----------|--------|
| `HTMLMediaElement` | Adds: `error` (readonly `MediaError?`), `defaultPlaybackRate`, `playbackRate`. **`volume` setter throws `IndexSizeError` outside `[0, 1]`.** |
|  | Build-conditional: `srcObject` is `[STARFISH_ENABLE_WEBRTC]` — only exposed when `WEBRTC=1`. |
|  | **`[Unimplemented]`:** `fastSeek`, `getStartDate`, `defaultMuted`, `audioTracks`, `videoTracks`. |
|  | Note: `canPlayType` returns a static answer (`"probably"`/`"maybe"`/`""`); when built with `STARFISH_USE_MOCK_MEDIAPLAYER` (the default Linux non-ffmpeg path), it still returns `"probably"` for mp4 even though no real decoding occurs. |
| `HTMLVideoElement` | Adds: `poster`. **`[Unimplemented]`:** `playsInline`, `getVideoPlaybackQuality()`. `VideoPlaybackQuality` interface not exposed. |
| `HTMLAudioElement` | Named constructor `new Audio(optional src)` exposed. |
| `HTMLSourceElement` | **`[Unimplemented]`:** `srcset`, `sizes`, `media` — `<picture>`-style source selection is not supported. |
| `MediaError` | Spec.md previously omitted this. Constants: `MEDIA_ERR_ABORTED`(1), `MEDIA_ERR_NETWORK`(2), `MEDIA_ERR_DECODE`(3), `MEDIA_ERR_SRC_NOT_SUPPORTED`(4). Attributes: `code`, `message`. **Direct construction (`new MediaError()`) is allowed but yields `code=0`/`message=""` (the C++ getters log `UNIMPLEMENTED`).** Only `videoEl.error` returns a meaningful instance. |
| `TextTrackCue` | **Cannot be constructed directly** — `new TextTrackCue(...)` throws `TypeError: Illegal constructor`. Use `new VTTCue(startTime, endTime, text)` instead. `pauseOnExit` is `[Unimplemented]`. |
| `TextTrackCueList.getCueById` | `[Unimplemented]`. |
| `TextTrackList` | `onchange`/`onaddtrack`/`onremovetrack` are `[Unimplemented]`. |
| `TimeRanges` | Cannot be constructed directly (no `Constructor` extended attribute — `new TimeRanges()` throws `Illegal constructor`). Obtain via `videoEl.buffered`/`played`/`seekable`. |
| `MediaSource` | Adds: static `MediaSource.isTypeSupported(type)`, `addSourceBuffer(type)`, `removeSourceBuffer(buffer)`. `onsourceopen`/`onsourceended`/`onsourceclose`, `setLiveSeekableRange`/`clearLiveSeekableRange` are `[Unimplemented]`. |
| `SourceBuffer` | Adds: `timestampOffset` (R/W, throws on bad input), `abort()`, `changeType(type)`. `audioTracks`/`videoTracks` are `[Unimplemented]`. |

### JavaScript engine — Intl & locale

The JS engine is **Escargot** built with `-DESCARGOT_LIBICU_SUPPORT=ON` (the LWE default). On the verified Linux/x64/EFL release build the full ES2020+ `Intl` namespace is present and locale-aware prototype methods on `String`/`Date`/`Number`/`Array`/`BigInt` work. None of this is gated by a Starfish IDL or a `STARFISH_*` macro — it is purely a property of how Escargot was compiled. Webapps may rely on it on stock LWE builds; if a downstream variant ships Escargot with `LIBICU_SUPPORT=OFF`, every API in the table below disappears or degrades to a "C" locale.

| `Intl` member | Status on stock LWE | Verified call |
|---------------|---------------------|-----------------------------------------------------------------|
| `Intl` (namespace object) | `typeof Intl === 'object'` | — |
| `Intl.Collator` | constructor exposed | `new Intl.Collator('ko').compare('가','나')` → `-1` |
| `Intl.DateTimeFormat` | constructor exposed | `new Intl.DateTimeFormat('ko-KR',{year:'numeric',month:'long',day:'numeric'}).format(new Date(0))` → `1970년 1월 1일` |
| `Intl.NumberFormat` | constructor exposed; `style:'currency'` works | `new Intl.NumberFormat('ko-KR').format(1234567)` → `1,234,567`; `... 'en-US',{style:'currency',currency:'USD'}` → `$1,234.56` |
| `Intl.PluralRules` | constructor exposed | `.select(1)` → `'one'`, `.select(2)` → `'other'` |
| `Intl.RelativeTimeFormat` | constructor exposed | `new Intl.RelativeTimeFormat('en').format(-1,'day')` → `'1 day ago'` |
| `Intl.ListFormat` | constructor exposed | `new Intl.ListFormat('en').format(['a','b','c'])` → `'a, b, and c'` |
| `Intl.Locale` | constructor exposed | `new Intl.Locale('ko-KR').toString()` → `'ko-KR'` |
| `Intl.Segmenter` | constructor exposed; iterator + `Symbol.iterator` works | first word of `'Hello world'` → `'Hello'` |
| `Intl.DisplayNames` | constructor exposed | `new Intl.DisplayNames(['en'],{type:'region'}).of('KR')` → `'South Korea'` |
| `Intl.DurationFormat` | **constructor exposed** (Stage-4 / ES2025; bonus on top of Spec.md's old Intl coverage) | — |
| `Intl.getCanonicalLocales` | function exposed | `Intl.getCanonicalLocales(['EN-us','Ko-kr'])` → `['en-US','ko-KR']` |
| `Intl.supportedValuesOf` | function exposed | `Intl.supportedValuesOf('calendar').slice(0,5)` → `['buddhist','chinese','coptic','dangi','ethioaa']` |

**Locale-aware prototype methods** — all `function`, all spec-conformant on the verified build:

| Method | Verified result |
|--------|------------------|
| `String.prototype.localeCompare(target [, locales [, options]])` | `'a'.localeCompare('b')` → `-1`; `'한'.localeCompare('가','ko')` → `1`; `'a'.localeCompare('A',undefined,{sensitivity:'base'})` → `0` |
| `Date.prototype.toLocaleString(locales, options)` | `(new Date(0)).toLocaleString('en-US')` → `1/1/1970, 9:00:00 AM` (TZ-dependent — the host TZ leaks through; tests should freeze TZ) |
| `Date.prototype.toLocaleDateString` / `toLocaleTimeString` | working |
| `Number.prototype.toLocaleString(locales, options)` | `(1234567).toLocaleString('ko-KR')` → `'1,234,567'`; `(1234.5).toLocaleString('en-US',{style:'currency',currency:'USD'})` → `'$1,234.50'` |
| `Array.prototype.toLocaleString` | working — but **note**: separator is the locale's number-group separator joined with `,`, not the locale list separator. `[1000,2000,3000].toLocaleString('en-US')` returns `'1,000,2,000,3,000'` (per ES spec — ES `Array.prototype.toLocaleString` does not use `Intl.ListFormat`). For human-friendly lists use `Intl.ListFormat` explicitly. |
| `BigInt.prototype.toLocaleString` | working: `(123456789012345678901234567890n).toLocaleString('en-US')` → `'123,456,789,012,345,678,901,234,567,890'` |

> **Caveat — TZ data:** `Date.prototype.toLocaleString` uses the host's IANA TZ via libICU; on a TV/STB without a configured TZ the output may differ from a desktop dev box.

> **Caveat — locale data weight:** the full ICU data file (`icudt*.dat`) is ~10 MB and is linked into Escargot. Wearable/`SMALL_CONFIG` Escargot builds may strip locale data; if you ship LWE with `-DESCARGOT_SMALL_CONFIG=ON` re-verify the table above before relying on it.

### Inspector / Debugger / Profiler / Memory APIs — runtime caveats

**Bottom line:** LWE has no DevTools-style introspection surface available to JavaScript. The C++ `Inspector` class (`src/core/inspector/Inspector.{h,cpp}`) is a build-conditional **out-of-process console-message bridge** (nanomsg pair socket on `ws://0.0.0.0:23888`) — *not* a Chrome DevTools Protocol implementation, *not* attached to a JS interface, and *not* a heap/CPU profiler. Web pages cannot detect it, drive it, or observe a debugger. JS-side memory introspection (`performance.memory`, `measureUserAgentSpecificMemory`, `console.profile`, etc.) is entirely absent.

| Surface | Status (verified) |
|---------|-------------------|
| `Inspector`, `Debugger`, `Profiler` JS globals | **All `undefined`.** No IDL exists; the C++ `Starfish::Inspector` class is not exposed to script. |
| `console.profile`, `console.profileEnd`, `console.timeStamp` | **`undefined`.** Not in the `CONSOLE_APIS` X-macro. |
| `console.count`, `countReset`, `trace`, `dir`, `dirxml`, `table`, `clear`, `context` | **`undefined`** — there is no profiler-flavored console method either. |
| `performance.memory` (Chrome `MemoryInfo`) | **`undefined`.** Already documented in §Performance; re-confirmed. |
| `performance.measureUserAgentSpecificMemory()` | **`undefined`.** No IDL, no C++. |
| `performance.navigation` | **`undefined`** (§Performance). |
| `PerformanceObserver`, `PerformanceObserverEntryList` | **`undefined`** — long-tasks / paint-timing observation impossible. |
| `ReportingObserver`, `Report`, `ReportBody`, `DeprecationReport`, `InterventionReport` | **`undefined`.** No Reporting API. |
| `navigator.sendBeacon(url, data)` | **`undefined`.** No CrashReporting/beaconing path; use `fetch(url, {keepalive:true})` instead — but note `keepalive` itself is not validated by LWE (see Fetch caveats). |
| `globalThis.gc()`, `Memory`, `MemoryInfo` constructors | **`undefined`.** Escargot is built without a `--expose-gc` style hook, and BDWGC is not surfaced to JS. |
| `__DevToolsHost`, `InspectorFrontendHost`, `InspectorBackend`, `CDP` | **`undefined`.** No DevTools/CDP runtime polyfills are exposed to page script. (The engine does ship a CDP *server* under `STARFISH_ENABLE_CDP`, off by default — it is driven out-of-process over a WebSocket, not from the page. See `docs/CDP.md`.) |
| `debugger;` statement | **No-op** when built with default `-DENABLE_DEBUGGER=0`. Does not throw, does not pause; control flow continues. With `-DENABLE_DEBUGGER=1`, the statement enters Escargot's debugger protocol — see below. |
| `performance.mark()` / `performance.measure()` / `getEntriesByType('measure')` | **Working** (§Performance). Verified: `mark a; mark b; measure m,a,b` returns one entry with non-zero `duration`. This is the only timing-instrumentation primitive available; build dashboards on top of `getEntries()`, not on a debugger. |

**Build-time debugger (Escargot, not Inspector):**

LWE exposes Escargot's JS debugger protocol via `cmake -DENABLE_DEBUGGER=1` (defines `STARFISH_ENABLE_DEBUGGER`; see `build/config.cmake:322`). When enabled:
- `LWEWebView::RegisterDebuggerShouldInitHandler(cb)` and `RegisterDebuggerShouldContinueWaitingHandler(cb)` (declared in `inc/LWEWebView.h:379`/`:866`) let the host decide per-URL whether to start the debug server and whether to keep waiting for a client to attach.
- The protocol is the one consumed by [escargot-vscode-extension](https://github.com/Samsung/escargot-vscode-extension).
- It is a **JS-source debugger** (breakpoints, step, eval), **not** a DOM/CSS/Network inspector. The separate CDP server (`STARFISH_ENABLE_CDP`, off by default) does implement `DOM.*`, `CSS.*`, `Network.*`, `Runtime.*`, `Page.*` and `Log.*` for out-of-process clients such as Puppeteer — see `docs/CDP.md` and the per-domain status table in `docs/CDP_DOMAINS.md`. Neither surface is reachable from page script, and `Profiler.*`/`HeapProfiler.*` are stubs there too, because Escargot exposes no CPU sampler or heap-snapshot serializer.
- The default LWE/Starfish builds ship with `ENABLE_DEBUGGER=0`. Webapps must not feature-detect a debugger from JS — the only observable signal is that `debugger;` is a no-op.

**Build-time `STARFISH_ENABLE_INSPECTOR` (not the same thing):**

`STARFISH_ENABLE_INSPECTOR` (auto-on for x64, see `build/config.cmake:97`) compiles in `Starfish::Inspector`, but the class is **only instantiated if a host calls `WebView::setupInspector(port=23888)`** — and no public `LWE*` API calls it. The shipped `LWEWebContainer` / `LWEWebView` never invoke `setupInspector`, so on every default build the class is dead code at runtime. When wired up (custom shell), it accepts JSON commands `{"command":"ping"|"eval", "content":"…"}` over a nanomsg `NN_PAIR` socket and replays `console.{log,info,warn,error,debug}` content as `{"command":"console-…"}` JSON frames; this is what `src/core/extra/Console.cpp` guards on `STARFISH_ENABLE_INSPECTOR`. Treat it as a remote-console feed, not an inspector.

**`StarfishGCMemoryLogger` log spam:**

`src/public/delegate/LWEDelegate.cpp:52` registers `StarfishGCMemoryLogger` as a `RECLAIM_END` listener on `Escargot::Memory`. This emits `LWEDelegate.cpp: StarfishGCMemoryLogger(54) > Done GC: HeapSize: [<used MB>, <heap MB>]` to stderr (via `STARFISH_LOG_INFO`) on **every** BDWGC sweep, regardless of build flags or `-DCMAKE_BUILD_TYPE`. There is no JS, public-API, or env-var off-switch in the engine source — the listener is unconditionally added in `LWE::Initialize` after a defensive `removeGCEventListener`. Embedders who want quiet stderr must filter the prefix downstream. The output is the only memory-usage signal a host can observe without a debugger, and matches BDWGC's `GC_get_memory_use()` (live) and `GC_get_heap_size()` (committed).

**Authoring guidance for LWE webapps:**

- Do not write code that reads `performance.memory.usedJSHeapSize` / `…jsHeapSizeLimit`. There is no fallback — guard with `if (performance && performance.memory) { … }`.
- For perf timing, stick to `performance.now()` + `performance.mark()` / `performance.measure()`. They are the only primitives that exist.
- For "is a debugger attached?" feature detection: there is no reliable signal. Apps that gate behavior on devtools presence should treat LWE as "always production".
- `console.profile()`, `console.timeStamp()`, `console.count()` calls **throw `TypeError: Callee is not a function object`** (§Console). Feature-detect with `typeof console.profile === 'function'`.

## Web Device API
The following describes Web device APIs supported by lightweight web engine. Supported interfaces and methods are generally the same as the interfaces and methods supported by Tizen API, respectively. If there are exceptions, they are explicitly mentioned below.

> **Build flag:** the Web Device API (`window.tizen`) is exposed only when `CMAKE_SYSTEM_NAME=Tizen` and the `TIZEN_DEVICE_API` macro is defined. On `CMAKE_SYSTEM_NAME=Linux`/`CMAKE_SYSTEM_NAME=Windows`/the Android build `window.tizen` is `undefined`.

| API            | Description | Note |
|----------------|-------------|------|
| [Application](https://developer.tizen.org/dev-guide/4.0.0/org.tizen.web.apireference/html/device_api/tv/tizen/application.html) | The Application API provides a way to launch other applications and access application management. | |
| [MessagePort](https://developer.tizen.org/dev-guide/4.0.0/org.tizen.web.apireference/html/device_api/tv/tizen/messageport.html) | The MessagePort API provides the functionality for communicating with other applications. | |

## Accessible Rich Internet Applications (WAI-ARIA)
The following describes WAI-ARIA supported by lightweight web engine. Please, see [here](https://www.w3.org/TR/wai-aria/) for more information about WAI-ARIA.

> **Audit note:** ARIA support is **parser-level only**. The four attributes below round-trip via `getAttribute`/`setAttribute` and feed the engine's TTS accessible-name algorithm (`src/core/modules/tts/TextAlternativeHelper.cpp`). LWE does **NOT** implement:
> - The `role` attribute as a semantic role — `<div role="button">` does not affect tab-focus, click-as-Enter, or any layout/event behavior.
> - The `ARIAMixin` IDL accessors (`Element.role`, `Element.ariaLabel`, `Element.ariaPressed`, …, ~45 properties). `el.role = 'checkbox'` is silently kept as a JS expando, not a reflected attribute. **Always use `setAttribute('role', ...)` / `setAttribute('aria-*', ...)`.**
> - `Element.attachInternals()` / `ElementInternals` (Custom Elements ARIA semantics).
> - Any accessibility tree exposure to JavaScript or DevTools.
>
> Other `aria-*` attributes (`aria-pressed`, `aria-expanded`, `aria-valuenow`, etc.) are stored as plain string attributes; author scripts must read them with `getAttribute` and act on them manually.

| Interface | Type | Name | Description | Note |
|-----------|------|------|-------------|------|
| [WAI-ARIA](https://www.w3.org/TR/wai-aria/) | property | [aria-label](https://www.w3.org/TR/wai-aria/#aria-label) | Defines a string value that labels the current element. See related aria-labelledby.| |
| | property | [aria-labelledby](https://www.w3.org/TR/wai-aria/#aria-labelledby) | Identifies the element (or elements) that labels the current element. See related aria-describedby. | |
| | property | [aria-describedby](https://www.w3.org/TR/wai-aria/#aria-describedby)| Identifies the element (or elements) that describes the object. See related aria-labelledby.| |
| | state | [aria-hidden](https://www.w3.org/TR/wai-aria/#aria-hidden) | Indicates whether the element is exposed to an accessibility API. See related aria-disabled. | |

## Web Speech APIs
The following describes Web Speech APIs supported by lightweight web engine. Please, see [here](https://w3c.github.io/speech-api/#tts-section) for more information.

| Interface | Type | Name | Description | Note |
|-----------|------|------|-------------|------|
| [SpeechSynthesis](https://w3c.github.io/speech-api/#speechsynthesis) | interface | SpeechSynthesis | The controller interface for the speech service. | |
| | attribute | pending | This attribute is true if the queue for the global SpeechSynthesis instance contains any utterances which have not started speaking.| |
| | attribute | speaking | This attribute is true if an utterance is being spoken. | |
| | attribute | paused| This attribute is true when the global SpeechSynthesis instance is in the paused state.| |
| | method | void speak(SpeechSynthesisUtterance utterance) | This method appends the SpeechSynthesisUtterance object utterance to the end of the queue for the global SpeechSynthesis instance. | |
| | method | void cancel() | This method removes all utterances from the queue. If an utterance is being spoken, speaking ceases immediately. | |
| | method | void pause() | This method puts the global SpeechSynthesis instance into the paused state. | |
| | method | void resume() | This method puts the global SpeechSynthesis instance into the non-paused state. | |
| | method | sequence<SpeechSynthesisVoice> getVoices() | This method returns the available voices. It is user agent dependent which voices are available. | |
| [SpeechSynthesisUtterance](https://w3c.github.io/speech-api/#speechsynthesisutterance) | interface | SpeechSynthesisUtterance | Represents a speech request. It contains the content the speech service should read and information about how to read it. | |
| | attribute | text  | This attribute specifies the text to be synthesized and spoken for this utterance.| |
| | attribute | lang  | This attribute specifies the language of the speech synthesis for the utterance | |
| | attribute | voice| This attribute specifies the speech synthesis voice that the web application wishes to use.| |
| | attribute | rate  | This attribute specifies the speaking rate for the utterance. It is relative to the default rate (1) for this voice. | |
| | attribute | onstart  | Fired when this utterance has begun to be spoken. | |
| | attribute | onend| Fired when this utterance has completed being spoken. | |
| | attribute | onerror  | Fired if there was an error that prevented successful speaking of this utterance. | |
| | attribute | onpause  | Fired when and if this utterance is paused mid-utterance. | |
| | attribute | onresume| Fired when and if this utterance is resumed after being paused mid-utterance. | |
| [SpeechSynthesisEvent](https://w3c.github.io/speech-api/#speechsynthesisevent) | interface | SpeechSynthesisEvent | Contains information about the current state of SpeechSynthesisUtterance objects that have been processed in the speech service. | |
| | attribute | utterance  | This attribute contains the SpeechSynthesisUtterance that triggered this event.| |
| | attribute | elapsedTime | This attribute indicates the time, in seconds, that this event triggered, relative to when this utterance has begun to be spoken.| |
| [SpeechSynthesisVoice](https://w3c.github.io/speech-api/#speechsynthesisvoice) | interface | SpeechSynthesisVoice | Represents a voice that the system supports. | |
| | attribute | voiceURI  | The voiceURI attribute specifies the speech synthesis voice and the location of the speech synthesis service for this voice.| |
| | attribute | name | This attribute is a human-readable name that represents the voice.| |
| | attribute | lang  | This attribute is a BCP 47 language tag indicating the language of the voice.| |
| | attribute | localService | This attribute is true for voices supplied by a local speech synthesizer, and is false for voices supplied by a remote speech synthesizer service.| |
| | attribute | default  | This attribute is true for at most one voice per language.| |

## WebRTC
The following describes WebRTC APIs supported by lightweight web engine. Please, see [WebRTC Spec](https://w3c.github.io/webrtc-pc/) for more information.
The WebRTC support is in an early stage.

> **Build flag:** WebRTC is gated by `-DWEBRTC=1` (which also turns on `STARFISH_ENABLE_WEBRTC`/`STARFISH_ENABLE_WEBAUDIO`/`STARFISH_ENABLE_WEBSOCKET`/`STARFISH_ENABLE_MULTIMEDIA`). The default Linux/EFL release build ships with `-DWEBRTC=0`, so `RTCPeerConnection`, `MediaStream`, `MediaStreamTrack`, `navigator.mediaDevices`, and the rest of the interfaces in this section are absent at runtime. Verified by IDL inspection (`[STARFISH_ENABLE_WEBRTC]` extended attributes) — see [Build-Conditional Surface](#build-conditional-surface).

| Interface | Type | Name | Description | Note |
|-----------|------|------|-------------|------|
| [RTCPeerConnection](https://w3c.github.io/webrtc-pc/#rtcpeerconnection-interface) | interface | RTCPeerConnection | The main interface for WebRTC | |
| | constructor | constructor | Calling new RTCPeerConnection(configuration) creates an RTCPeerConnection object. | |
| | attribute | localDescription (of type RTCSessionDescription, readonly, and nullable) | This attribute returns PendingLocalDescription if it is not null and otherwise it returns CurrentLocalDescription. | |
| | attribute | currentLocalDescription (of type RTCSessionDescription, readonly, nullable) | This attribute returns CurrentLocalDescription. | |
| | attribute | pendingLocalDescription (of type RTCSessionDescription, readonly, nullable) | This attribute returns PendingLocalDescription. | |
| | attribute | remoteDescription (of type RTCSessionDescription, readonly, nullable) | This attribute returns PendingRemoteDescription if it is not null and otherwise it returns CurrentRemoteDescription. | |
| | attribute | currentRemoteDescription (of type RTCSessionDescription, readonly, nullable) | This attribute returns CurrentRemoteDescription. | |
| | attribute | pendingRemoteDescription (of type RTCSessionDescription, readonly, nullable) | This attribute returns PendingRemoteDescription. | |
| | attribute | signalingState (of type RTCSignalingState, readonly) | This attribute returns the RTCPeerConnection object's signaling state. | |
| | method |  Promise<RTCSessionDescriptionInit> createOffer(optional RTCOfferOptions options = {}) | The createOffer method generates a blob of SDP that contains an RFC 3264 offer with the supported configurations for the session, including descriptions of the local MediaStreamTracks attached to this RTCPeerConnection, the codec/RTP/RTCP capabilities supported by this implementation, and parameters of the ICE agent and the DTLS connection. The options parameter may be supplied to provide additional control over the offer generated. | |
| | method |  Promise<RTCSessionDescriptionInit> createAnswer(optional RTCAnswerOptions options = {}) | The createAnswer method generates an [SDP] answer with the supported configuration for the session that is compatible with the parameters in the remote configuration. | |
| | method | Promise<void> setLocalDescription(optional RTCSessionDescriptionInit description = {})  | The setLocalDescription method instructs the RTCPeerConnection to apply the supplied RTCSessionDescriptionInit as the local description. | |
| | method | Promise<void> setRemoteDescription(optional RTCSessionDescriptionInit description = {}) | The setRemoteDescription method instructs the RTCPeerConnection to apply the supplied RTCSessionDescriptionInit as the remote offer or answer. This API changes the local media state. | |
| | method | Promise<void> addIceCandidate(optional RTCIceCandidateInit candidate = {}) | The addIceCandidate method provides a remote candidate to the ICE Agent. | |
| | method | RTCConfiguration getConfiguration() | Returns an RTCConfiguration object representing the current configuration of this RTCPeerConnection object. | |
| | method | void setConfiguration(RTCConfiguration configuration) | The setConfiguration method updates the configuration of this RTCPeerConnection object. | |
| | method | void close() | Closes the connection. | |
| | method | sequence<RTCRtpTransceiver> getTransceivers() | Returns a sequence of RTCRtpTransceiver objects representing the RTP transceivers that are currently attached to this RTCPeerConnection object. | | |
| | method | RTCRtpSender addTrack(MediaStreamTrack track, MediaStream... streams) | Adds a new track to the RTCPeerConnection, and indicates that it is contained in the specified MediaStreams. | |
| | method | void removeTrack(RTCRtpSender sender) | Stops sending media from sender. | |
| [RTCConfiguration](https://w3c.github.io/webrtc-pc/#rtcconfiguration-dictionary) | dictionary | RTCConfiguration | The RTCConfiguration defines a set of parameters to configure how the peer-to-peer communication established via RTCPeerConnection is established or re-established. | |
| | attribute | iceServers (of type sequence\<RTCIceServer\>)| An array of objects describing servers available to be used by ICE, such as STUN and TURN servers. | |
| | attribute | iceTransportPolicy (of type RTCIceTransportPolicy) | Indicates which candidates the ICE Agent is allowed to use. | |
| | attribute | bundlePolicy (of type RTCBundlePolicy. | Indicates which media-bundling policy to use when gathering ICE candidates. | |
| | attribute | rtcpMuxPolicy (of type RTCRtcpMuxPolicy) | Indicates which rtcp-mux policy to use when gathering ICE candidates. | |
| | attribute | peerIdentity (of type DOMString) | Sets the target peer identity for the RTCPeerConnection. | |
| | attribute | sequence\<RTCCertificate\> certificates | A set of certificates that the RTCPeerConnection uses to authenticate. |
| | attribute | iceCandidatePoolSize (of type octet, defaulting to 0) | Size of the prefetched ICE pool as defined in [JSEP] (section 3.5.4. and section 4.1.1.). | |
| [RTCDataChannelState](https://w3c.github.io/webrtc-pc/#dom-rtcdatachannelstate) | enum | | |
| | value | "connecting " | The user agent is attempting to establish the underlying data transport |
| | value | "open" | The underlying data transport is established and communication is possible. |
| | value | "closing" | The procedure to close down the underlying data transport has started. |
| | value | "closed" | The underlying data transport has been closed or could not be established. |
| [RTCDataChannelInit](https://w3c.github.io/webrtc-pc/#dom-rtcdatachannelinit) | dictionary | | |
| | value | boolean ordered = true | If set to false, data is allowed to be delivered out of order. |
| | value | unsigned short maxPacketLifeTime | Limits the time (in milliseconds) during which the channel will transmit or retransmit data if not acknowledged. |
| | value | unsigned short maxRetransmits | Limits the number of times a channel will retransmit data if not successfully delivered. |
| | value | USVString protocol = "" | Subprotocol name used for this channel. |
| | value | boolean negotiated = false; | The default value of false tells the user agent to announce the channel in-band and instruct the other peer to dispatch a corresponding RTCDataChannel object. |
| | value | unsigned short id | Sets the channel ID when negotiated is true. |
| [RTCDataChannel](https://w3c.github.io/webrtc-pc/#dom-rtcdatachannel) | interface | | |
| | readonly attribute | USVString label | The label attribute represents a label that can be used to distinguish this RTCDataChannel object from other RTCDataChannel objects. |
| | readonly attribute | boolean ordered | The ordered attribute returns true if the RTCDataChannel is ordered, and false if out of order delivery is allowed. |
| | readonly attribute | unsigned short? maxPacketLifeTime | The maxPacketLifeTime attribute returns the length of the time window (in milliseconds) during which transmissions and retransmissions may occur in unreliable mode |
| | readonly attribute | unsigned short? maxRetransmits | The maxRetransmits attribute returns the maximum number of retransmissions that are attempted in unreliable mode. |
| | readonly attribute | USVString protocol | The protocol attribute returns the name of the sub-protocol used with this RTCDataChannel. |
| | readonly attribute | boolean negotiated | The negotiated attribute returns true if this RTCDataChannel was negotiated by the application, or false otherwise. |
| | readonly attribute | unsigned short? id | The id attribute returns the ID for this RTCDataChannel. |
| | readonly attribute | RTCDataChannelState readyState | The readyState attribute represents the state of the RTCDataChannel object. |
| | attribute | EventHandler onopen | The event type of this event handler is open. |
| | attribute | EventHandler onbufferedamountlow | The event type of this event handler is bufferedamountlow. |
| | attribute | EventHandler onerror | The event type of this event handler is RTCErrorEvent. errorDetail contains "sctp-failure", sctpCauseCode contains the SCTP Cause Code value, and message contains the SCTP Cause-Specific-Information, possibly with additional text. |
| | attribute | EventHandler onclosing | The event type of this event handler is Event. |
| | attribute | EventHandler onclose | The event type of this event handler is Event. |
| | attribute | EventHandler onmessage | The event type of this event handler is message. |
| | attribute | DOMString binaryType | The binaryType attribute MUST, on getting, return the value to which it was last set. |
| | method | void send(USVString data) | Run the steps described by the send() algorithm with argument type string object. |
| [RTCDataChannelEvent](https://w3c.github.io/webrtc-pc/#dom-rtcdatachannelevent)| interface | | |
| | readonly attribute | RTCDataChannel channel | The channel attribute represents the RTCDataChannel object associated with the event. |
| [RTCIceTransportPolicy](https://w3c.github.io/webrtc-pc/#dom-rtcicetransportpolicy) | enum | RTCIceTransportPolicy | | |
| | value | relay | The ICE Agent uses only media relay candidates such as candidates passing through a TURN server. | |
| | value | all | The ICE Agent can use any type of candidate when this value is specified. | |
| [RTCBundlePolicy](https://w3c.github.io/webrtc-pc/#dom-rtcbundlepolicy) | enum | RTCBundlePolicy | | |
| | value | balanced | Gather ICE candidates for each media type in use (audio, video, and data). | |
| | value | max-compat | Gather ICE candidates for each track.  | |
| | value | max-bundle | Gather ICE candidates for only one track. | |
| [RTCRtcpMuxPolicy](https://w3c.github.io/webrtc-pc/#dom-rtcrtcpmuxpolicy) | enum | RTCRtcpMuxPolicy | |
| | value | negotiate | Gather ICE candidates for both RTP and RTCP candidates.  | |
| | value | require | Gather ICE candidates only for RTP and multiplex RTCP on the RTP candidates. | |
| [RTCSessionDescriptionInit](https://w3c.github.io/webrtc-pc/#dom-rtcsessiondescriptioninit) | dictionary | RTCSessionDescriptionInit | | |
| | value | type (of type RTCSdpType) | The type of this description. If not present, then setLocalDescription will infer the type based on the RTCPeerConnection's signaling state, whereas setRemoteDescription and the RTCSessionDescription constructor will throw a TypeError, because they require the argument. | |
| | value | sdp (of type DOMString) | The string representation of the SDP; if type is "rollback", this member is unused. | |
| [RTCSessionDescription](https://w3c.github.io/webrtc-pc/#dom-rtcsessiondescription) | interface | RTCSessionDescription | | |
| | constructor |  constructor(optional RTCSessionDescriptionInit descriptionInitDict = {}) | The RTCSessionDescription() constructor takes a dictionary argument, description, whose content is used to initialize the new RTCSessionDescription object. | |
| | attribute | type (of type RTCSdpType, readonly) | The type of this RTCSessionDescription. | |
| | attribute | sdp (of type DOMString, readonly) | The string representation of the SDP. | |
| [RTCSdpType](https://w3c.github.io/webrtc-pc/#dom-rtcsdptype) | enum | RTCSdpType | | |
| | value | offer | An RTCSdpType of offer indicates that a description MUST be treated as an SDPoffer. | |
| | value | pranswer | An RTCSdpType of pranswer indicates that a description MUST be treated as an SDP answer, but not a final answer.  | |
| | value | answer | An RTCSdpType of answer indicates that a description MUST be treated as an SDP final answer, and the offer-answer exchange MUST be considered complete.  | |
| [RTCIceCandidateInit](https://w3c.github.io/webrtc-pc/#dom-rtcicecandidateinit) | dictionary | RTCIceCandidateInit | | |
| | attribute | candidate of type DOMString, defaulting to "" | This carries the candidate-attribute as defined in section 15.1 of [ICE]. If this represents an end-of-candidates indication, candidate is an empty string. | |
| | attribute | sdpMid of type DOMString, nullable, defaulting to null | If not null, this contains the media stream "identification-tag" defined in [RFC5888] for the media component this candidate is associated with. | |
| | attribute | sdpMLineIndex of type unsigned short, nullable, defaulting to null | If not null, this indicates the index (starting at zero) of the media description in the SDP this candidate is associated with. | |
| | attribute | usernameFragment of type DOMString, nullable, defaulting to null | If not null, this carries the ufrag as defined in section 15.4 of [ICE]. | |
| [RTCIceCandidate](https://w3c.github.io/webrtc-pc/#rtcicecandidate-interface) | interface | RTCIceCandidate | | |
| | constructor |  constructor(optional RTCIceCandidateInit candidateInitDict = {}) | The RTCIceCandidate() constructor takes a dictionary argument, candidateInitDict, whose content is used to initialize the new RTCIceCandidate object. | |
| | attribute | candidate of type DOMString, readonly | This carries the candidate-attribute as defined in section 15.1 of [ICE]. | |
| | attribute | sdpMid of type DOMString, readonly, nullable | If not null, this contains the media stream "identification-tag" defined in [RFC5888] for the media component this candidate is associated with. | |
| | attribute | sdpMLineIndex of type unsigned short, readonly, nullable | If not null, this indicates the index (starting at zero) of the media description in the SDP this candidate is associated with. | |
| [RTCSignalingState](https://w3c.github.io/webrtc-pc/#dom-rtcsignalingstate) | enum | RTCSignalingState | | |
| | value | stable | There is no offer/answer exchange in progress. This is also the initial state, in which case the local and remote descriptions are empty. | |
| | value | have-local-offer| A local description, of type "offer", has been successfully applied. | |
| | value | have-remote-offer | A remote description, of type "offer", has been successfully applied. | |
| | value | have-local-pranswer | A remote description of type "offer" has been successfully applied and a local description of type "pranswer" has been successfully applied. | |
| | value | have-remote-pranswer | A local description of type "offer" has been successfully applied and a remote description of type "pranswer" has been successfully applied. | |
| | value | closed | The RTCPeerConnection has been closed; its [[IsClosed]] slot is true. | |
| [RTCRtpSender](https://w3c.github.io/webrtc-pc/#dom-rtcrtpsender) | interface | RTCRtpSender | | |
| | attribute | track (of type MediaStreamTrack, readonly, nullable) | The track attribute is the track that is associated with this RTCRtpSender object.  | |
| [RTCRtpTransceiver](https://w3c.github.io/webrtc-pc/#dom-rtcrtptransceiver) | interface | RTCRtpTransceiver | | |
| | attribute | mid (of type DOMString, readonly, nullable) | The mid attribute is the mid negotatiated and present in the local and remote descriptions as defined in [JSEP] (section 5.2.1. and section 5.3.1.). | |
| [MediaStream](https://w3c.github.io/mediacapture-main/#mediastream) | interface | MediaStream | | |
| | method | sequence<MediaStreamTrack> getVideoTracks() | Returns a sequence of MediaStreamTrack objects representing the video tracks in this stream. | |
| | method | sequence<MediaStreamTrack> getTracks() | Returns a sequence of MediaStreamTrack objects representing all the tracks in this stream. | |
| | method | void addTrack(MediaStreamTrack track) | Adds the given MediaStreamTrack to this MediaStream. | |
| [MediaStreamTrack](https://w3c.github.io/mediacapture-main/#mediastreamtrack) | interface | MediaStreamTrack | | |
| | attribute | kind of type DOMString, readonly | The kind attribute MUST return the string "audio" if this object represents an audio track or "video" if this object represents a video track. | |

## WebAudio
The following describes WebAudio APIs supported by lightweight web engine. Please, see [WebAudio Spec](https://webaudio.github.io/web-audio-api/) for more information.
The WebAudio support is in an early stage.

> **Build flag:** WebAudio is gated by `STARFISH_ENABLE_WEBAUDIO` (default on for `CMAKE_SYSTEM_PROCESSOR=x86_64`; also implicitly enabled when `WEBRTC=1`). Without it, `AudioContext`/`BaseAudioContext`/`AudioBuffer*`/`AudioNode` globals are not exposed. See [Build-Conditional Surface](#build-conditional-surface).

| Interface | Type | Name | Description | Note |
|-----------|------|------|-------------|------|
| [BaseAudioContext](https://webaudio.github.io/web-audio-api/#BaseAudioContext) | interface | BaseAudioContext | | |
| | callback | DecodeErrorCallback = void (DOMException error); | | |
| | callback | DecodeSuccessCallback = void (AudioBuffer decodedData); | | |
| | attribute | readonly AudioDestinationNode destination | An AudioDestinationNode with a single input representing the final destination for all audio. | |
| | attribute | readonly attribute AudioContextState state | Describes the current state of the AudioContext. | |
| | attribute | attribute EventHandler onstatechange; | A property used to set the EventHandler for an event that is dispatched to BaseAudioContext when the state of the AudioContext has changed (i.e. when the corresponding promise would have resolved). | |
| | method | AudioBufferSourceNode createBufferSource(); | Factory method for a AudioBufferSourceNode. | |
| | method | Promise<AudioBuffer> decodeAudioData (ArrayBuffer audioData, optional DecodeSuccessCallback? successCallback, optional DecodeErrorCallback? errorCallback); | Asynchronously decodes the audio file data contained in the ArrayBuffer. | |
| [AudioContext](https://webaudio.github.io/web-audio-api/#AudioContext) | interface | AudioContext | | |
| | constructor | constructor (optional AudioContextOptions contextOptions = {}); | | |
| | method | Promise<void> close (); | Closes the AudioContext, releasing the system resources being used. | |
| [AudioBufferOptions](https://webaudio.github.io/web-audio-api/#dictdef-audiobufferoptions) | dictionary | AudioBufferOptions | | |
| | attribute | long numberOfChannels = 1; | The number of channels for the buffer.  | |
| | attribute | unsigned long length; | The length in sample frames of the buffer. | |
| | attribute | float sampleRate; | The sample rate in Hz for the buffer. | |
| [AudioBuffer](https://webaudio.github.io/web-audio-api/#AudioBuffer) | interface | AudioBuffer | | |
| | constructor | constructor (AudioBufferOptions options); | | |
| | attribute | readonly attribute float sampleRate; | The sample-rate for the PCM audio data in samples per second. | |
| | attribute | readonly attribute unsigned long length; | Length of the PCM audio data in sample-frames.  | |
| | attribute | readonly attribute double duration; | Duration of the PCM audio data in seconds. | |
| | attribute | readonly attribute unsigned long numberOfChannels; | The number of discrete audio channels. | |
| [ChannelCountMode](https://webaudio.github.io/web-audio-api/#enumdef-channelcountmode) | enum | ChannelCountMode | | |
| | value | "max" | computedNumberOfChannels is the maximum of the number of channels of all connections to an input. | |
| | value | "clamped-max" | computedNumberOfChannels is determined as for "max" and then clamped to a maximum value of the given channelCount. | |
| | value | "explicit" | computedNumberOfChannels is the exact value as specified by the channelCount. | |
| [ChannelInterpretation](https://webaudio.github.io/web-audio-api/#enumdef-channelinterpretation) | enum | ChannelInterpretation | | |
| | value | "speakers" | use up-mix equations or down-mix equations. | |
| | value | "discrete" | Up-mix by filling channels until they run out then zero out remaining channels. | |
| [AudioNode](https://webaudio.github.io/web-audio-api/#audionode) | interface | AudioNode | | |
| | attribute | readonly BaseAudioContext context;| The BaseAudioContext which owns this AudioNode. | |
| | method | AudioNode connect (AudioNode destinationNode, optional unsigned long output = 0, optional unsigned long input = 0); | There can only be one connection between a given output of one specific node and a given input of another specific node. | |
| [AudioScheduledSourceNode](https://webaudio.github.io/web-audio-api/#AudioScheduledSourceNode) | interface | AudioScheduledSourceNode | | |
| | attribute | EventHandler onended; | A property used to set the EventHandler (described in HTML[HTML]) for the ended event that is dispatched for AudioScheduledSourceNode node types. | |
| | method | void stop(optional double when = 0); | Schedules a sound to stop playback at an exact time. | Only when = 0 is supported at the moment. |
| [AudioBufferSourceOptions](https://webaudio.github.io/web-audio-api/#AudioBufferSourceNode) | dictionary | AudioBufferSourceOptions | | |
| | attribute | AudioBuffer? buffer; | Represents the audio asset to be played. | |
| [AudioBufferSourceNode](https://webaudio.github.io/web-audio-api/#AudioBufferSourceNode) | interface | AudioBufferSourceNode | | |
| | constructor | constructor (BaseAudioContext context, optional AudioBufferSourceOptions options = {}); | | |
| | method | void start (optional double when = 0, optional double offset, optional double duration); | Schedules a sound to playback at an exact time. | Only when = 0 is supported at the moment. |
| [AudioDestinationNode](https://webaudio.github.io/web-audio-api/#AudioDestinationNode) | interface | AudioDestinationNode | | |

## WebSocket
The following describes WebSocket APIs supported by lightweight web engine. Please, see [WebSocket Spec](https://html.spec.whatwg.org/multipage/web-sockets.html/) for more information.
The Websocket is limitedly supported.

> **Build flag:** WebSocket is gated by `STARFISH_ENABLE_WEBSOCKET` (turned on automatically for `CMAKE_SYSTEM_PROCESSOR=x86_64` and whenever `WEBRTC=1`). Builds without it will not expose the `WebSocket` global. See the [Build-Conditional Surface](#build-conditional-surface) section.

| Interface | Type | Name | Description | Note |
|-----------|------|------|-------------|------|
| [WebSocket](https://html.spec.whatwg.org/multipage/web-sockets.html) | interface | WebSocket | | |
| | constructor | constructor (USVString url, optional DOMString protocols); | LWE accepts a single subprotocol string only; the WHATWG `sequence<DOMString>` form is **not** parsed by `WebSocket.idl`. Pass a comma-separated string if you need to advertise multiple protocols (or wrap and call repeatedly). |
| | attribute | readonly USVString url | Returns the URL that was used to establish the WebSocket connection. | |
| | value | "CONNECTING" | The connection has not yet been established. |
| | value | "OPEN" | The WebSocket connection is established and communication is possible. |
| | value | "CLOSING" | The connection is going through the closing handshake, or the close() method has been invoked. |
| | value | "CLOSED" | The connection has been closed or could not be opened. |
| | attribute | unsigned short readyState | Returns the state of the WebSocket object's connection. It can have the values described below. | |
| | attribute | readonly unsigned long long bufferedAmount | Returns the number of bytes of application data (UTF-8 text and binary data) that have been queued using send() but not yet been transmitted to the network. | |
| | attribute | EventHandler onopen | Fired at networking-related objects when a connection is established. | |
| | attribute | EventHandler onerror | Fired when unexpected errors occur. | |
| | attribute | EventHandler onclose | Fired when WebSocket elements when the connection is terminated. | |
| | method | void close(optional [Clamp] unsigned short code, optional USVString reason); | Closes the WebSocket connection. | |
| | attribute | EventHandler onmessage | Fired at an object when it receives a message. | |
| | attribute | BinaryType binaryType | Returns a string that indicates how binary data from the WebSocket object is exposed to scripts. | |
| | method | send(USVString data); | Transmits string data using the WebSocket connection. | |
| | method | send(Blob data); | Transmits Blob data using the WebSocket connection. | |
| | method | send(ArrayBuffer data); | Transmits ArrayBuffer data using the WebSocket connection. | |

### Final cleanup — remaining surfaces

A final pass of runtime probes (see also [Build-Conditional Surface](#build-conditional-surface)) on a default-flagged build (`CMAKE_SYSTEM_NAME=Linux SHELL=glfw BACKEND=uv_cairo_gl WEBGL=1`, all other features off) confirmed the following gaps. None are tracked elsewhere in this document at the API-shape level.

**Not exposed as globals (constructor / namespace returns `undefined`):**

| API | Notes / alternative in LWE |
|---|---|
| `BroadcastChannel` | Not implemented. For same-origin tab-to-tab signalling, LWE webapps run as a single document context anyway; use direct in-page events. |
| `URLPattern` | Not implemented. Use manual `URL` parsing + `RegExp` against `pathname`/`search`. |
| `CompressionStream` / `DecompressionStream` | Not implemented. No built-in gzip/deflate in JS; bundle a JS-side library (e.g. `pako`) if compression is required, or rely on HTTP `Content-Encoding`. |
| `FileSystem`, `FileSystemHandle`, `FileSystemFileHandle`, `FileSystemDirectoryHandle`, `FileSystemWritableFileStream` | Modern File System Access API not implemented. |
| `window.showOpenFilePicker` / `showSaveFilePicker` / `showDirectoryPicker` | Not implemented. Use `<input type="file">` with `change` for read access (subject to `FileReader`/`Blob`, which are supported). |
| `window.requestFileSystem` / `webkitRequestFileSystem` | Legacy FileSystem API not implemented. |
| `navigator.storage` | StorageManager (quota / persist) not exposed; the build flag block at the top of this section already lists the related quota-estimate gap. |
| `Notification`, `PushManager`, `PushSubscription` | Listed under SERVICE_WORKER build-flag deps; in default build they are absent. |
| `Cache`, `CacheStorage`, `caches` | Listed under SERVICE_WORKER build-flag deps; absent in default build. |

**Per-interface gaps (interface exists, specific member missing):**

| Interface.member | Status | Notes |
|---|---|---|
| `Event.prototype.composedPath()` | Implemented (`src/core/dom/Event.idl`) | Returns the event path, honoring closed-tree visibility. |
| `Event.prototype.composed` (attribute) | Supported (read-only, default `false`) | Useful as a no-op flag for code that copies events; no Shadow-DOM observable effect. |
| `HTMLFormElement.prototype.requestSubmit()` | Not exposed | Use `form.dispatchEvent(new Event('submit', { cancelable: true, bubbles: true }))` followed by `form.submit()` if the dispatch wasn't cancelled, or wire your own click handler on a `<button type="submit">`. |
| `Node.prototype.getRootNode()` | Supported | Returns the containing `ShadowRoot` inside a shadow tree, or the `Document`; `{composed: true}` returns the shadow-including root. |

**Confirmed working (called out because they're often assumed missing on embedded engines):**

- `MessageChannel` / `MessagePort` — constructible; `port1.postMessage` is callable. Already documented in the DOM table.
- `fetch()` — returns a thenable (`Promise`). The Promise integration with Web APIs is the standard one supplied by Escargot's microtask queue.
- `Event.prototype.stopImmediatePropagation()` — present.
- `<input>.autocomplete` — DOM property reflects the attribute and round-trips values like `"name"`, `"email"`, `"off"`. The visual effect (browser-managed autofill UI) does not exist in LWE; the attribute is purely informational for any host-side autofill bridge.
| | method | send(ArrayBufferView data); | Transmits ArrayBufferView data using the WebSocket connection. | | |