# Starfish CDP MVP — 상세 설계 문서

대상: starfish C++ 웹엔진에 Chrome DevTools Protocol(CDP) MVP를 구현하는 에이전트.
범위(MVP): **Target / Page / Runtime / DOM / Log** 5개 도메인 + WebSocket transport.
참조 구현: `/home/bwikbs/workspace/work_lwe/work1/browser/src/cdp/` (Zig, Lightpanda).
신규 코드는 전부 `src/core/cdp/` 아래. 기존 코드 수정은 `STARFISH_ENABLE_CDP` 가드로 격리.

> **갱신(MVP 이후)**: 본 문서는 5개 도메인 MVP 설계를 기술한다. 서버는 이후
> **49개 도메인**으로 확장됐다(§12). 도메인별 구현 상태의 권위 있는 표는
> `docs/CDP_DOMAINS.md`, 메서드 단위 표는 `docs/CDP.md` 를 본다.

> 본 문서의 모든 코드 블록은 **인터페이스 스케치**다. 구현 본문은 포함하지 않는다.
> `[추측]` 표시는 실제 파일에서 확인하지 못해 합리적으로 가정한 부분이다.

---

## 0. 근거 자료 (실제 파일 확인 결과)

| 항목 | 출처 | 확인 내용 |
|---|---|---|
| IO 스레드 + 메인 위임 패턴 | `src/core/inspector/Inspector.cpp:197-266, 248` | `Thread::run(messageLoop, worker, this)` 로 IO 스레드 기동, `addIdlerWithNoGCRootingInOtherThread(nullptr, fn, data)` 로 메인 위임 |
| 메인 위임 콜백 시그니처 | `MessageLoopInterface.h:26-32` | `void(*)(size_t handle, void*)` 및 `void(*)(size_t, void*, void*)` |
| 비동기 람다 위임 | `MessageLoop.h:71` | `void runOnMainThreadAsync(const std::function<void()>&)` (단, GC 루팅 주의 — 아래 §7) |
| JS 평가(타입 보존) | `ScriptWrappable.cpp:1471`, `.h:234` | `ScriptValue evaluateString(ScriptBindingInstance*, String*, String* fileName, bool* result)` → `ScriptValue == Escargot::ValueRef*` |
| WebView JS 진입점(우회 대상) | `WebView.cpp:` `evaluateJavaScript` | `toBrowserString(...evaluateString(...))` 로 문자열 평탄화 → CDP에서는 **사용 안 함** |
| ScriptBindingInstance | `BrowsingContext.h:80,87`; `ScriptBindingInstance.h:107,141,146` | `bc->scriptBindingInstance()`, `->isScriptingEnabled()`, `->scriptContext()` (`Escargot::ContextRef*`), `->engineInstance()` |
| ValueRef API | `EscargotPublic.h:1000-1091` | `isBoolean/isNumber/isNull/isUndefined/isString/isObject/isSymbol/isBigInt/isCallable/isFunctionObject/isArrayObject/isErrorObject/...` 와 `asBoolean()/asNumber()/asString()/asObject()`, `toStringWithoutException(ContextRef*)` |
| WS 핸드셰이크/프레이밍 | `browser .../Connection.zig:391-556` | SHA1(key+GUID)→base64, 프레임 헤더 인코딩, 마스킹 |
| SHA1/base64 출처 | `third_party/` 탐색 결과 | **§2.1 참조** — libwebsockets에만 존재(의존 금지). starfish 본체엔 없음 → 최소 자체 구현 |
| Node 접근자 | `Node.h:204-364` | `nodeType()`, `nodeName()`, `localName()`, `nodeValue()→Optional<String*>`, `firstChild()`, `nextSibling()`, `isDocument()`, `asElement()` |
| Element 속성 | `Element.h:188,193,220,229`; `NamedNodeMap.h:38-42`; `Attr.h:58,88` | `hasAttributes()`, `attributes()→NamedNodeMap*`, `NamedNodeMap::length()/item(i)`, `Attr::name()/value()` |
| Document | `Document.h:281` | `Element* documentElement()` |
| GLOB 자동 빌드 | `build/starfish.cmake:51` | `FILE(GLOB_RECURSE STARFISH_SRC ${ROOT}/src/*.cpp)` → `src/core/cdp/*.cpp` 자동 포함 |
| 플래그 정의 위치 | `build/config.cmake:42` | `STARFISH_ENABLE_INSPECTOR` 와 동일 패턴으로 `STARFISH_ENABLE_CDP` 추가 |
| Console 훅 | `Console.cpp:38-101` | `log/info/error/warn/debug` 각각 `#if INSPECTOR` 블록 존재 → CDP 분기 추가 지점 |
| 버전 응답 값 | `browser Server.zig buildJSONVersionResponse`, `domains/browser.zig:23-36` | Protocol-Version `1.3`, 아래 §2.2 |
| id 발급 규칙 | `browser cdp/id.zig:29-62` | `FID-%010d`, `LID-%010d`; session/target/browserContext는 prefix+증가 정수 |

---

## 1. 파일 구조 & 클래스 헤더 스케치

```
src/core/cdp/
  CDPServer.h / .cpp          // accept 루프 소유, 연결 수락
  CDPConnection.h / .cpp      // 연결 1개: WS 핸드셰이크/프레이밍/송수신
  CDPDispatcher.h / .cpp      // method 문자열 → 도메인 라우팅, Command 생성
  CDPSession.h / .cpp         // CDP 세션 상태(enable 플래그, id들)
  CDPCommand.h / .cpp         // 핸들러에 전달되는 요청 컨텍스트 (sendResult/Error/Event)
  NodeRegistry.h / .cpp       // Node* <-> nodeId
  RemoteObject.h / .cpp       // ValueRef* -> CDP RemoteObject JSON
  Base64.h / .cpp             // 최소 base64 인코더 (§2.1)
  Sha1.h / .cpp               // 최소 SHA1 (§2.1)
  domains/
    TargetDomain.h / .cpp
    PageDomain.h / .cpp
    RuntimeDomain.h / .cpp
    DOMDomain.h / .cpp
    LogDomain.h / .cpp
```

> **확장 관례(§12)**: 행위가 큰 도메인만 `domains/*Domain.cpp` 파일을 가진다.
> synthetic/stub 도메인은 별도 파일 없이 `CDPDispatcher.cpp` 의 `route()`
> else-if 체인에 **인라인**으로 추가된다(예: Profiler/CacheStorage/WebAuthn).

기존 수정:
- `src/core/page/WebView.h/.cpp` — `setupCDPServer()`, `m_cdpServer`, GC 등록.
- `src/core/extra/Console.cpp` — Log 도메인 분기 1곳.
- `build/config.cmake` — `STARFISH_ENABLE_CDP` 정의.
- `inc/LWEWebView.h` — `StartDevTools(port)` 공개 진입(선택).

### 1.1 컨벤션 정리 (실제 코드 기준)

- 네임스페이스 `Starfish`.
- GC 객체는 `: public gc` 상속 (예: `Inspector : public gc`, `MessageLoop : public gc`).
- 문자열은 `String*` (GC). UTF-8 변환은 `s->toUTF8NonGCString()` (`std::string` 반환).
- JSON 직렬화는 RapidJSON (`rapidjson/document.h`, `writer.h`) — Inspector.cpp 와 동일.
- **GC 객체를 보유하는 클래스는 GC 상속**, IO 스레드 전용/raw 바이트만 다루는 클래스는 GC 비상속(일반 heap)으로 둔다.

### 1.2 CDPServer

소유관계: `WebView` → `CDPServer`(1) → `CDPConnection`(0..1; MVP는 단일 연결).
GC: **비상속**. 멤버에 GC 객체 없음(메인 스레드 객체 접근은 WebView 포인터 경유 위임으로만).

```cpp
// CDPServer.h  (guarded by STARFISH_ENABLE_CDP)
namespace Starfish {
class WebView;
class CDPDispatcher;
class CDPConnection;

class CDPServer {
public:
    CDPServer(WebView* webView, uint16_t port);
    ~CDPServer();

    void start();      // 메인 스레드: accept 스레드 기동
    void stop();       // 메인 스레드: 루프 종료 + join

    // IO 스레드에서 디코드된 텍스트 메시지를 메인으로 넘길 때 사용.
    CDPDispatcher* dispatcher() { return m_dispatcher; }
    WebView* webView() { return m_webView; }
    uint16_t port() const { return m_port; }

    // CDPConnection 이 송신할 raw 바이트(프레임 포함)를 IO 스레드 송신 큐에 넣음.
    void enqueueSend(const std::string& frame);

private:
    static void* acceptLoop(void* self);    // IO 스레드 진입 (Inspector::worker 패턴)

    WebView*        m_webView;       // 메인 스레드 객체. IO 스레드는 messageLoop 위임에만 사용.
    CDPDispatcher*  m_dispatcher;    // non-GC. 메인 스레드에서만 핸들러 실행.
    CDPConnection*  m_conn;          // 현재 연결(단일). non-GC.
    Thread*         m_ioThread;      // WebView::threadPool() 기반 (Inspector 와 동일)
    int             m_listenFd;      // POSIX listen 소켓
    uint16_t        m_port;
    volatile bool   m_isRunning;
};
} // namespace Starfish
```

생성/소멸: `WebView::setupCDPServer(port)` 에서 `new CDPServer(this, port); m_cdpServer->start();`.
소멸은 `WebView::destroy()` 경로에서 `stop()` 후 `delete`.

### 1.3 CDPConnection

소유관계: `CDPServer` → `CDPConnection`. GC: **비상속**(IO 스레드 전용, raw 바이트만).
역할: 한 소켓의 HTTP 디스커버리 → WS 업그레이드 → 프레임 디코드/인코드.

```cpp
// CDPConnection.h
namespace Starfish {
class CDPServer;

class CDPConnection {
public:
    enum class State { Handshaking, Live, Closed };

    CDPConnection(CDPServer* server, int fd);
    ~CDPConnection();

    // IO 스레드 루프에서 호출. recv → 상태에 따라 처리.
    // 반환: false면 연결 종료.
    bool pump();

    // 송신: 텍스트 프레임으로 마스킹 없이(server→client) 인코딩 후 write.
    // 메인 스레드가 만든 JSON 문자열을 IO 스레드가 호출 (송신 큐 경유).
    void sendText(const std::string& utf8json);

    State state() const { return m_state; }
    int fd() const { return m_fd; }

private:
    bool doHandshake(const std::string& request);     // HTTP/WS 업그레이드 (§2.3)
    bool handleHttpDiscovery(const std::string& url);  // /json/version, /json/list
    bool readFrames();                                 // WS 프레임 디코드 → dispatch 위임 (§2.4)
    void writeAll(const char* data, size_t len);       // partial write 루프

    CDPServer*   m_server;
    int          m_fd;
    State        m_state;
    std::string  m_recvBuf;     // 누적 수신 버퍼 (프레임 경계 처리용)
};
} // namespace Starfish
```

### 1.4 CDPDispatcher

소유관계: `CDPServer` → `CDPDispatcher`. **메인 스레드에서만 핸들러 실행.** GC: 비상속(보유 GC 객체 없음; WebView 경유 접근).

```cpp
// CDPDispatcher.h
namespace Starfish {
class WebView;
class CDPSession;
class CDPCommand;
class NodeRegistry;
class TargetDomain; class PageDomain; class RuntimeDomain;
class DOMDomain; class LogDomain;

class CDPDispatcher {
public:
    CDPDispatcher(CDPServer* server, WebView* webView);
    ~CDPDispatcher();

    // IO 스레드가 호출. raw UTF-8 문자열만 받는다. 내부에서 메인으로 위임.
    void onMessageFromIO(std::string&& rawJson);

    // 메인 스레드 실제 처리 (위임 타겟). rapidjson 파싱 + 라우팅.
    void dispatchOnMain(const std::string& rawJson);

    CDPSession*   session()      { return m_session; }
    NodeRegistry* nodeRegistry() { return m_nodeRegistry; }
    WebView*      webView()      { return m_webView; }
    CDPServer*    server()       { return m_server; }

private:
    void route(CDPCommand& cmd, const std::string& domain,
               const std::string& method);

    CDPServer*    m_server;
    WebView*      m_webView;
    CDPSession*   m_session;       // 단일 세션 (MVP)
    NodeRegistry* m_nodeRegistry;
    TargetDomain*  m_target;
    PageDomain*    m_page;
    RuntimeDomain* m_runtime;
    DOMDomain*     m_dom;
    LogDomain*     m_log;
};
} // namespace Starfish
```

> **메인/IO 경계 메모**: `m_session`, `m_nodeRegistry`, 도메인 객체는 모두 메인 스레드 전용 상태를 갖는다.
> `onMessageFromIO()` 만 IO 스레드에서 호출되며, 그 안에서 즉시 메인으로 위임한다(§7).

### 1.5 CDPSession (CDP 상태 머신)

GC: 비상속(plain 멤버만). 메인 스레드 전용.

```cpp
// CDPSession.h
namespace Starfish {
class CDPSession {
public:
    CDPSession();

    // enable 플래그
    bool pageEnabled = false;
    bool runtimeEnabled = false;
    bool domEnabled = false;
    bool logEnabled = false;
    bool targetDiscoverEnabled = false;
    bool targetAutoAttach = false;
    bool lifecycleEventsEnabled = false;

    // 고정 식별자 (단일 타겟 MVP — startup 시 발급)
    std::string sessionId;        // "SID-..."   attach 시 발급
    std::string targetId;         // "TID-..." == frameId
    std::string browserContextId; // "BID-..."
    std::string frameId;          // == targetId
    std::string loaderId;         // "LID-0000000001"
    bool attached = false;

    // 단조 증가 카운터(설명/objectId 등)
    uint32_t executionContextId = 1;   // 단일 컨텍스트
};
} // namespace Starfish
```

### 1.6 CDPCommand

핸들러에 전달되는 요청 컨텍스트. **메인 스레드에서만 생성/사용.** GC: 비상속(스택/단명).

```cpp
// CDPCommand.h
namespace Starfish {
class CDPDispatcher;

class CDPCommand {
public:
    CDPCommand(CDPDispatcher* d,
               Optional<int64_t> id,            // 요청 id (이벤트엔 없음)
               const std::string& sessionId,    // 빈 문자열이면 미부여
               rapidjson::Value* params);        // 요청 params (nullable)

    CDPDispatcher* dispatcher() { return m_dispatcher; }
    rapidjson::Value* params()  { return m_params; }   // nullable

    // 응답: {"id":id,"result":{...}, "sessionId"?:...}
    void sendResult(rapidjson::Value& result);
    void sendResultEmpty();                              // result:{}

    // 오류: {"id":id,"error":{"code":code,"message":msg}, "sessionId"?:...}
    void sendError(int code, const char* message);

    // 이벤트: {"method":m,"params":{...}, "sessionId"?:...}  (id 없음)
    void sendEvent(const char* method, rapidjson::Value& params);

private:
    void emit(rapidjson::Document& doc);   // 직렬화 + dispatcher->server->enqueueSend

    CDPDispatcher*   m_dispatcher;
    Optional<int64_t> m_id;
    std::string      m_sessionId;
    rapidjson::Value* m_params;
};
} // namespace Starfish
```

`Optional<T>` 는 starfish 기존 타입(예: `Node::nodeValue()→Optional<String*>`)을 사용.

### 1.7 도메인 클래스 공통 형태

각 도메인은 GC 비상속. 메인 스레드 전용. `processMessage` 진입점.

```cpp
// e.g. RuntimeDomain.h
namespace Starfish {
class CDPCommand;
class RuntimeDomain {
public:
    RuntimeDomain(CDPDispatcher* d) : m_dispatcher(d) {}
    // method = "evaluate" 등 (도메인 prefix 제거된 부분)
    void processMessage(CDPCommand& cmd, const std::string& method);
private:
    CDPDispatcher* m_dispatcher;
};
}
```

---

## 2. WebSocket Transport 설계

### 2.1 SHA1 / base64 출처 결정

탐색 결과:
- `third_party/libwebsockets/` 에 SHA1/base64 존재(`lws-sha1-base64.h` 등). **그러나 계획상 libwebsockets 의존 금지** → 사용 안 함.
- escargot/third_party 및 starfish 본체에 재사용 가능한 독립 SHA1/base64 공개 API는 발견되지 않음.

**결정**: `src/core/cdp/Sha1.{h,cpp}`, `src/core/cdp/Base64.{h,cpp}` 에 **최소 자체 구현**.
- 외부 의존 0. 입력은 ≤ 60바이트(WS key 24바이트 + GUID 36바이트)뿐이라 단순 RFC 3174 SHA1 + 표준 base64 인코더로 충분.
- 시그니처(인터페이스만):
```cpp
// Sha1.h
namespace Starfish { void cdpSha1(const uint8_t* data, size_t len, uint8_t out[20]); }
// Base64.h
namespace Starfish { std::string cdpBase64Encode(const uint8_t* data, size_t len); }
```
> 참고: Connection.zig:471-477 은 `Sha1.init → update(key) → update(GUID) → final(20B)` 후 `base64.standard.Encoder.encode(28B)`. 동일 절차.

### 2.2 accept 루프 / 연결당 처리 (IO 스레드)

`Inspector::worker`(Inspector.cpp:197) 패턴을 차용하되 nanomsg 대신 POSIX:

1. `m_listenFd = socket(AF_INET, SOCK_STREAM, 0)`; `setsockopt(SO_REUSEADDR)`; `bind(0.0.0.0:port)`; `listen(1)`.
2. `while (m_isRunning)`:
   - `int fd = accept(m_listenFd, ...)`. (블로킹 또는 `poll` 후 accept)
   - `m_conn = new CDPConnection(m_server, fd);`
   - `while (m_conn->pump()) {}`  — 한 연결을 끝까지 처리(MVP 단일 연결).
   - `delete m_conn; m_conn = nullptr;`
3. 종료 시 `close(m_listenFd)`.

기동은 `Inspector::run` 과 동일:
```cpp
m_ioThread = new Thread(m_webView->threadPool());
m_ioThread->run(m_webView->messageLoop(), CDPServer::acceptLoop, this);
```
종료는 `Inspector::stop`: `m_isRunning=false; m_ioThread->joinIfNeeds();` (메인 스레드, `STARFISH_ASSERT(isMainThread())`).
> 주의: blocking `accept` 를 join 으로 깨우려면 stop 시 `shutdown(m_listenFd, SHUT_RDWR)` 후 join. (Inspector 는 nanomsg recv 가 깨지길 기대; 여기선 명시적 shutdown 필요.)

### 2.3 HTTP 디스커버리 + WS 업그레이드 (`CDPConnection::doHandshake`)

수신 버퍼가 `\r\n\r\n` 으로 끝날 때까지 누적(Connection.zig:231).

라우팅(요청 라인 `GET <url> HTTP/1.1`):

- `GET /json/version` → 아래 응답 후 `Connection: Close` 로 종료:
```
HTTP/1.1 200 OK\r\n
Content-Length: <len>\r\n
Connection: Close\r\n
Content-Type: application/json; charset=UTF-8\r\n\r\n
{"Browser":"Starfish/1.0","Protocol-Version":"1.3","User-Agent":"Starfish/1.0","webSocketDebuggerUrl":"ws://<host>:<port>/"}
```
  값 근거: browser `Server.zig buildJSONVersionResponse` + `Protocol-Version "1.3"`.

- `GET /json` | `/json/list` | `/json/list/` → 단일 타겟 목록 반환(Puppeteer가 직접 연결할 수 있게):
```
HTTP/1.1 200 OK\r\n Content-Length:<len>\r\n Connection: Close\r\n
Content-Type: application/json; charset=UTF-8\r\n\r\n
[{"description":"","id":"<TID>","title":"Starfish","type":"page","url":"<currentURL>",
  "webSocketDebuggerUrl":"ws://<host>:<port>/devtools/page/<TID>"}]
```
  > browser 는 `[]` 만 반환(Connection.zig:300)하지만, Puppeteer 의 `connect()` 호환을 위해 단일 항목 채움을 권장. `[추측]` — Puppeteer launch 경로는 `/json/version` 의 `webSocketDebuggerUrl` 만 쓰므로, 최소 구현은 `[]` 로도 동작 가능.

- `GET /` 또는 `GET /devtools/page/<TID>` (URL 무관, 헤더로 판정) → **WS 업그레이드**:
  필수 헤더 4종 확인(Connection.zig:418-438): `Upgrade: websocket`, `Sec-WebSocket-Version: 13`, `Connection: ...upgrade...`, `Sec-WebSocket-Key: <key>`.
  응답:
```
HTTP/1.1 101 Switching Protocols\r\n
Upgrade: websocket\r\n
Connection: Upgrade\r\n
Sec-WebSocket-Accept: <base64(sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))>\r\n\r\n
```
  성공 시 `m_state = Live`.

### 2.4 프레임 디코드 (`readFrames`, client→server, 마스킹됨)

WS 프레임(RFC 6455):
- byte0: `FIN(1) | RSV(3) | opcode(4)`. opcode: 0x1 text, 0x2 binary, 0x8 close, 0x9 ping, 0xA pong.
- byte1: `MASK(1) | payloadLen(7)`. client→server 는 **MASK=1 필수**.
- payloadLen==126 → 다음 2바이트(BE) 길이; ==127 → 다음 8바이트(BE) 길이.
- MASK=1 → 4바이트 masking-key.
- payload[i] ^= maskKey[i % 4] 로 언마스킹.

처리:
- text(0x1): UTF-8 JSON → `m_server->dispatcher()->onMessageFromIO(std::move(payload))`.
- ping(0x9): pong(0xA) 으로 동일 payload 회신(서버 송신은 마스킹 없음).
- close(0x8): close 프레임 회신 후 `m_state=Closed`, `pump()` false.
- 프래그멘테이션: MVP 는 단일 프레임 가정. `[추측]` — FIN=0 멀티프레임은 누적 후 처리 권장하나, DevTools 클라이언트는 보통 단일 프레임 송신.

### 2.5 프레임 인코드 (`sendText`, server→client, 마스킹 없음)

Connection.zig:528-556 `websocketHeader` 와 동일:
- byte0 = `0x80 | 0x1` (FIN + text).
- len ≤ 125: byte1 = len. (2바이트 헤더)
- 126 ≤ len < 65536: byte1 = 126, 다음 2바이트 BE 길이. (4바이트)
- len ≥ 65536: byte1 = 127, 다음 8바이트 BE 길이. (10바이트)
- MASK=0 (서버는 마스킹 안 함). payload 그대로.
- `writeAll` partial-write 루프(Connection.zig:92-115).

### 2.6 송신 큐와 스레드 경계

- **핸들러는 메인 스레드에서 실행** → JSON 문자열(`std::string`)을 만든다(GC 객체 미포함).
- 그 문자열을 IO 스레드로 넘겨 소켓 write 해야 한다. 두 가지 방안:
  - (A) **권장**: `CDPServer::enqueueSend(const std::string&)` 가 내부 mutex 보호 큐에 push. IO accept 루프가 `pump()` 사이마다 큐를 flush. 단, blocking accept/recv 와 공존하려면 self-pipe 또는 짧은 `poll` timeout 로 깨움.
  - (B) **단순화 MVP**: 메인 스레드가 직접 `m_conn->sendText()` 호출(소켓 write 는 스레드 안전하지 않으나, write 는 메인에서만, read 는 IO 에서만 → 분리되면 OK). browser 도 송신은 blocking write 로 단순화함(Connection.zig:79-116). **MVP 는 (B) 채택**: read=IO, write=메인. 단, 메인 write 중 IO read 와의 동시성은 fd 동일이지만 방향 분리라 안전.
  - 결론: **write 는 메인 스레드, read 는 IO 스레드.** `CDPConnection::sendText` 는 메인에서 호출되도록 한다. `m_conn` 포인터 수명은 IO 가 소유하므로, 연결 종료 시 메인 송신과의 경합을 막기 위해 `m_connAlive` 원자 플래그로 가드. `[추측]` — MVP 단일 연결·짧은 세션에서 충분.

---

## 3. 메시지 라우팅 설계

### 3.1 수신 파싱 (메인 스레드, `dispatchOnMain`)

RapidJSON 으로 `rawJson` 파싱(Inspector.cpp:221 패턴):
```
{ "id": <int?>, "sessionId": <string?>, "method": "Domain.method", "params": {<obj?>} }
```
- `id`: 없으면(이벤트 송신 케이스 아님; 요청엔 항상 있음) `Optional` empty.
- `method` 없으면 무시/에러.
- `sessionId`: 없거나 빈 문자열이면 미부여. `"STARTUP"` 특수 처리(§9).
- `params`: 없으면 nullptr.

`method` 를 첫 `.` 으로 분리 → `domain`, `methodName` (CDP.zig:341-348 패턴).

### 3.2 디스패치 테이블 (`CDPDispatcher::route`)

문자열 비교로 단순 분기(starfish 스타일; CDP.zig 의 uint bitcast 최적화는 불필요):
```cpp
if (domain == "Target")  m_target->processMessage(cmd, methodName);
else if (domain == "Page")    m_page->processMessage(cmd, methodName);
else if (domain == "Runtime") m_runtime->processMessage(cmd, methodName);
else if (domain == "DOM")     m_dom->processMessage(cmd, methodName);
else if (domain == "Log")     m_log->processMessage(cmd, methodName);
else cmd.sendError(-32601, "'<method>' wasn't found");   // method not found
```

각 도메인 `processMessage` 내부도 `if (method == "enable") ...` 식 분기.

### 3.3 핸들러 시그니처

```cpp
void XxxDomain::processMessage(CDPCommand& cmd, const std::string& method);
```
개별 메서드는 private 멤버 함수 `void cmd_<name>(CDPCommand&)` 로 구현. (browser 의 `fn enable(cmd)` 대응)

### 3.4 에러 코드 (JSON-RPC + CDP 관례)

| code | 의미 | 사용처 |
|---|---|---|
| -32700 | Parse error | JSON 파싱 실패 |
| -32600 | Invalid Request | id/method 누락 |
| -32601 | Method not found | 미지원 domain/method |
| -32602 | Invalid params | params 누락/형식 오류 |
| -32000 | Server error (generic) | "Could not find node with given id" 등 (dom.zig:284) |
| -32001 | Unknown sessionId | 알 수 없는 sessionId (CDP.zig:305) |

응답 형식: `{"id":id,"error":{"code":code,"message":msg},"sessionId"?:...}`.

---

## 4. 도메인별 메서드 스펙 (MVP 최소 필드)

> 모든 result/event 는 `sessionId` 가 부여된 세션이면 함께 실린다(Command 의 input.sessionId 전달).
> 좌표/시간은 `timestamp`(double, monotonic ms) 사용.

### 4.1 Target

| method | params | result | 비고 |
|---|---|---|---|
| `setDiscoverTargets` | `{discover:bool}` | `{}` | `targetDiscoverEnabled=discover`. true 면 즉시 `Target.targetCreated`(아래) 발사 |
| `setAutoAttach` | `{autoAttach:bool, waitForDebuggerOnStart:bool, flatten?:bool}` | `{}` | `targetAutoAttach=autoAttach` |
| `getTargets` | `{}` | `{targetInfos:[TargetInfo]}` | 단일 타겟 배열 |
| `getTargetInfo` | `{targetId?:string}` | `{targetInfo:TargetInfo}` | |
| `attachToTarget` | `{targetId:string, flatten?:bool}` | `{sessionId:string}` | `doAttach` 후 sessionId 발급. **`Target.attachedToTarget` 이벤트 선행** |
| `createTarget` | `{url:string,...}` | `{targetId:string}` | MVP: 기존 단일 타겟 반환 또는 navigate. `[추측]` 단순화 |
| `closeTarget` | `{targetId:string}` | `{success:true}` | |
| `detachFromTarget` | `{sessionId?:string}` | `{}` | |

**TargetInfo** (target.zig:`TargetInfo`):
```json
{"targetId":"TID-...","type":"page","title":"","url":"about:blank",
 "attached":true,"canAccessOpener":false,"browserContextId":"BID-..."}
```
**Target.attachedToTarget** 이벤트 (target.zig:258):
```json
{"sessionId":"SID-...","targetInfo":{<TargetInfo, attached:true>},"waitingForDebugger":false}
```
**Target.targetCreated** 이벤트 (target.zig:208):
```json
{"targetInfo":{<TargetInfo, attached:false, url:"about:blank">}}
```
**Target.detachedFromTarget** (target.zig:299):
```json
{"sessionId":"SID-...","targetId":"TID-..."}
```

### 4.2 Page

| method | params | result | 비고 |
|---|---|---|---|
| `enable` | `{}` | `{}` | `pageEnabled=true` |
| `disable` | `{}` | `{}` | |
| `getFrameTree` | `{}` | `{frameTree:{frame:Frame}}` | startup graceful(§9) |
| `setLifecycleEventsEnabled` | `{enabled:bool}` | `{}` | `lifecycleEventsEnabled=enabled` |
| `navigate` | `{url:string,...}` | `{frameId, loaderId}` | 실제 navigate 위임(§7). result 는 frameNavigated 시점 |
| `reload` | `{ignoreCache?:bool}` | `{}` | |
| `getNavigationHistory` | `{}` | `{currentIndex:0,entries:[...]}` | MVP 최소 |
| `getLayoutMetrics` | `{}` | 아래 | viewport 크기 |
| `addScriptToEvaluateOnNewDocument` | `{source}` | `{identifier:"1"}` | MVP: noop, id 반환 |
| `createIsolatedWorld` | `{frameId}` | `{executionContextId:N}` | MVP: 단일 컨텍스트 id 반환 |

**Frame** (page.zig `CDPFrame`):
```json
{"id":"TID-...","loaderId":"LID-0000000001","url":"<url>","domainAndRegistry":"",
 "securityOrigin":"<origin>","mimeType":"text/html","secureContextType":"InsecureScheme",
 "crossOriginIsolatedContextType":"NotIsolated","gatedAPIFeatures":[]}
```
**getLayoutMetrics** result(최소):
```json
{"layoutViewport":{"pageX":0,"pageY":0,"clientWidth":W,"clientHeight":H},
 "visualViewport":{"offsetX":0,"offsetY":0,"pageX":0,"pageY":0,"clientWidth":W,"clientHeight":H,"scale":1},
 "cssLayoutViewport":{"pageX":0,"pageY":0,"clientWidth":W,"clientHeight":H},
 "contentSize":{"x":0,"y":0,"width":W,"height":H}}
```
W/H = `WebView::screenInfo()` 또는 mainBrowsingContext 뷰포트.

**이벤트** (lifecycle 켜졌을 때):
- `Page.frameNavigated` (page.zig:652): `{"frame":{<Frame>}}`
- `Page.lifecycleEvent` (page.zig:566): `{"frameId":"FID-...","loaderId":"LID-...","name":"init|DOMContentLoaded|load|networkIdle","timestamp":<double>}`
- `Page.frameStartedLoading` / `Page.frameStoppedLoading`: `{"frameId":"FID-..."}`
- `Page.domContentEventFired` (page.zig:736): `{"timestamp":<double>}`
- `Page.loadEventFired` (page.zig:761): `{"timestamp":<double>}`

### 4.3 Runtime

> **핵심**: `evaluate` 는 `evaluateString`(ScriptWrappable.cpp:1471) 을 직접 호출해 `ValueRef*` 보존 → RemoteObject 직렬화(§6). WebView::evaluateJavaScript 우회.

| method | params | result | 비고 |
|---|---|---|---|
| `enable` | `{}` | `{}` | `runtimeEnabled=true`. **`Runtime.executionContextCreated` 발사** |
| `disable` | `{}` | `{}` | |
| `runIfWaitingForDebugger` | `{}` | `{}` | noop (runtime.zig:41) |
| `evaluate` | `{expression:string, returnByValue?:bool, awaitPromise?:bool, ...}` | `{result:RemoteObject, exceptionDetails?:...}` | §6 |
| `callFunctionOn` | `{functionDeclaration, objectId?, arguments?, returnByValue?}` | `{result:RemoteObject}` | MVP: objectId 해석은 §6.4 |
| `getProperties` | `{objectId:string, ownProperties?:bool}` | `{result:[PropertyDescriptor]}` | MVP: objectId 핸들 테이블 조회 |
| `releaseObject` | `{objectId:string}` | `{}` | 핸들 테이블에서 제거 |

**Runtime.executionContextCreated** 이벤트:
```json
{"context":{"id":1,"origin":"<origin>","name":"","uniqueId":"1",
  "auxData":{"isDefault":true,"type":"default","frameId":"FID-..."}}}
```
**Runtime.executionContextsCleared** (navigate 시): params 없음.

**Runtime.consoleAPICalled** (Log/Console 경유, runtime.zig:159) — §4.5 참조.

**exceptionDetails** (evaluate 실패 시):
```json
{"exceptionId":1,"text":"Uncaught","lineNumber":0,"columnNumber":0,
 "exception":{<RemoteObject of error>},"stackTrace":{...optional}}
```
근거: `evaluateString` 내부 `Evaluator::execute` 의 `sbresult.error/stackTrace`(ScriptWrappable.cpp:1529-1545). MVP 는 `Evaluator::execute` 를 도메인에서 직접 호출하거나, `evaluateString` 의 `bool* result` 로 성공 여부만 받아 최소 exceptionDetails 구성. `[추측]` — exception 객체 직렬화는 `toStringWithoutException` 으로 text 만 채워도 Puppeteer 동작.

### 4.4 DOM

| method | params | result | 비고 |
|---|---|---|---|
| `enable` | `{}` | `{}` | `domEnabled=true` |
| `disable` | `{}` | `{}` | |
| `getDocument` | `{depth?:int=3, pierce?:bool}` | `{root:Node}` | document 노드 직렬화(§5) |
| `requestChildNodes` | `{nodeId:int, depth?:int}` | `{}` | **`DOM.setChildNodes` 이벤트 발사** |
| `querySelector` | `{nodeId:int, selector:string}` | `{nodeId:int}` | 0 이면 미발견 |
| `querySelectorAll` | `{nodeId:int, selector:string}` | `{nodeIds:[int]}` | |
| `describeNode` | `{nodeId?/backendNodeId?/objectId?, depth?}` | `{node:Node}` | |
| `getOuterHTML` | `{nodeId?/...}` | `{outerHTML:string}` | |
| `resolveNode` | `{nodeId?/backendNodeId?}` | `{object:RemoteObject}` | DOM→Runtime 연계. MVP 최소 |

**DOM.setChildNodes** 이벤트 (dom.zig:227):
```json
{"parentId":<int>,"nodes":[<Node>, ...]}
```
**DOM.documentUpdated** (navigate 시, page.zig:727): params 없음.

### 4.5 Log

| method | params | result |
|---|---|---|
| `enable` | `{}` | `{}` (logEnabled=true) |
| `disable` | `{}` | `{}` |
| `clear` | `{}` | `{}` |

**Log.entryAdded** 이벤트 (console 메시지 브리지):
```json
{"entry":{"source":"console-api","level":"info|error|warning|verbose","text":"<msg>","timestamp":<double>,"url":""}}
```
> Console.cpp 훅에서 발사. **추가로/대안으로** `Runtime.consoleAPICalled` (runtime.zig:159) 을 보내면 Puppeteer 의 `page.on('console')` 가 동작한다. MVP 권장: Log 도메인 enable 이면 `Log.entryAdded`, Runtime enable 이면 `Runtime.consoleAPICalled` 둘 다 지원.

**Runtime.consoleAPICalled** (runtime.zig:159):
```json
{"type":"log|info|error|warning|debug","timestamp":<double>,"executionContextId":1,
 "args":[{<RemoteObject>}, ...]}
```
MVP: `Console::log/info/...` 가 받는 인자는 이미 평탄화된 `String*` 1개이므로, args 는 `[{"type":"string","value":"<text>"}]` 단일로 채워도 충분.

---

## 5. NodeRegistry 설계

GC: **상속 안 함이 곤란** — `Node*` 를 보관해야 GC 가 노드를 살려두지 않으면 dangling 위험. 그러나 NodeRegistry 가 노드 수명을 강제로 늘리면 안 됨(DOM 이 주인). **결론**: NodeRegistry 는 `Node*` 를 **약하게** 보관하되, navigate 시 reset 으로 stale 제거. starfish 의 GC 맵은 `GCUnorderedMap` 사용 가능하나, 여기선 nodeId→Node 조회 시 노드가 살아있는지 보장이 어렵다.

**MVP 정책 (browser 와 동일)**: 등록은 `register(Node*)` 호출 시점에만. 조회 실패(또는 navigate 후 reset)는 `-32000 "Could not find node with given id"` (dom.zig:284). NodeRegistry 자체를 `gc` 상속시키고 `GCUnorderedMap<Node*,int>` + `GCUnorderedMap<int,Node*>` 를 멤버로 두어 GC 가 추적하게 한다(메인 스레드 전용이므로 GC 안전).

```cpp
// NodeRegistry.h
namespace Starfish {
class Node;
class NodeRegistry : public gc {
public:
    NodeRegistry();

    int  getOrCreate(Node* node);          // 없으면 새 id 발급(1부터), 등록 후 반환
    Node* lookup(int nodeId);              // 없으면 nullptr
    bool  has(Node* node) const;
    void  reset();                          // navigate 시: 모든 매핑 제거, m_next=1

    // CDP Node JSON 직렬화 (depth: 0=자식없음, N=N단계, <0=전체)
    void serializeNode(Node* node, int depth, rapidjson::Value& out,
                       rapidjson::Document::AllocatorType& alloc);

private:
    GCUnorderedMap<Node*, int> m_nodeToId;
    GCUnorderedMap<int, Node*> m_idToNode;
    int m_next;   // 1부터 증가. nodeId == backendNodeId (MVP 동일값)
};
}
```

> `GCUnorderedMap` 은 WebView.h:623 에서 사용 확인(`GCUnorderedMap<std::pair<...>, ...>`). 동일 타입 사용.

### 5.1 노드 직렬화 (`serializeNode`) — Node.zig:270-321 매핑

각 노드 객체 필드(CDP `Node`):
```json
{
  "nodeId":        <getOrCreate(node)>,
  "backendNodeId": <동일값>,
  "parentId":      <부모 등록 id, 부모 있으면>,
  "nodeType":      node->nodeType(),                 // Node.h:204
  "nodeName":      node->nodeName()->toUTF8...,       // Node.h:205
  "localName":     element? element->localName() : "",// Node.h:325 / Element.h:168
  "nodeValue":     node->nodeValue().hasValue()? value : "",  // Node.h:364, Optional<String*>
  "childNodeCount":<자식 수>,                          // firstChild→nextSibling 카운트
  "attributes":    [n0,v0,n1,v1,...],  // Element 이고 hasAttributes() 일 때만
  "children":      [<Node>,...]        // depth 허용 시
}
```

- **자식 수 계산**: `Node.h` 에 직접 childNodeCount 접근자 없음 → `firstChild()`(Node.h:337) 부터 `nextSibling()`(Node.h:357) 으로 순회 카운트.
- **attributes** (Element.zig:285-295 대응): `node->isElement()` 또는 `nodeType()==ELEMENT_NODE` → `asElement()`(Node.h:308). `Element::hasAttributes()`(Element.h:188) 면 `attributes()`(NamedNodeMap.h) 순회: `for i in [0,length()) { Attr* a = item(i); push(a->name()); push(a->value()); }` (NamedNodeMap.h:38-39, Attr.h:58,88).
- **localName**: Element 면 `element->localName()`, 아니면 `""` (Node.zig:297-302).
- **children**: `depth` 정책 — Node.zig:251 `full_child = self.depth < 0 || self.depth < depth`. `getDocument` 기본 depth=3.
- **documentURL/baseURL/xmlVersion** 등 부가 필드는 MVP 생략 가능(browser 는 null 채움). Puppeteer 필수 아님.

### 5.2 무효화 정책

- `navigate` (cross-document) 시 메인 스레드에서 `nodeRegistry()->reset()` 호출 → 모든 nodeId 무효.
- 동시에 `DOM.documentUpdated` 이벤트(domEnabled 면) + `Runtime.executionContextsCleared`(runtimeEnabled 면) 발사(page.zig:641,727).
- reset 후 클라이언트가 다시 `DOM.getDocument` 부터 재시작(표준 동작).

---

## 6. RemoteObject 설계 (ValueRef* → CDP RemoteObject)

근거 API: `EscargotPublic.h:1000-1091`, `ScriptBindingInstance.h:141,146`.

직렬화에 필요한 핸들:
- `ScriptBindingInstance* sbi = bc->scriptBindingInstance();`
- `Escargot::ContextRef* ctx = sbi->scriptContext();`  → `toStringWithoutException(ctx)` 에 사용(예외 안전).

### 6.1 타입 매핑 규칙

| 판정 (ValueRef 메서드) | type | subtype | value/description |
|---|---|---|---|
| `isUndefined()` | "undefined" | — | description="undefined" |
| `isNull()` | "object" | "null" | value=null |
| `isBoolean()` | "boolean" | — | value = `asBoolean()` |
| `isNumber()` | "number" | — | value = `asNumber()` (NaN/Inf 은 description 만: "NaN"/"Infinity") |
| `isString()` | "string" | — | value = `asString()->...` (StringRef→UTF8) |
| `isSymbol()` | "symbol" | — | description = `toStringWithoutException(ctx)` |
| `isBigInt()` | "bigint" | — | description = `toStringWithoutException(ctx)` |
| `isCallable()`/`isFunctionObject()` | "function" | — | className="Function", description = 소스 또는 "function" |
| `isArrayObject()` | "object" | "array" | description="Array(n)", objectId 발급 |
| `isErrorObject()` | "object" | "error" | description = error message |
| `isObject()` (그 외) | "object" | — | className, objectId 발급 |

> `is/as` 매크로(EscargotPublic.h:1052-1058, `ESCARGOT_POINTERVALUE_CHILD_REF_LIST`) 가 `isString/isObject/isArrayObject/isFunctionObject/isErrorObject/isSymbol/isBigInt/isPromiseObject/isDateObject/isRegExpObject` 등을 제공함을 확인.

### 6.2 RemoteObject JSON 형태

```json
{"type":"<type>", "subtype"?:"<subtype>", "className"?:"<class>",
 "value"?:<json primitive>, "description"?:"<string>", "objectId"?:"<handle>"}
```
- `returnByValue:true` 이고 객체이면 → 객체를 JSON.stringify 후 `value` 로(또는 MVP: primitive 만 value, 객체는 description+objectId). `[추측]` — MVP 는 primitive 만 value 직렬화, 객체는 objectId.

### 6.3 description 생성

- primitive: 위 표대로.
- object/function: `value->toStringWithoutException(ctx)` 결과를 UTF-8 로. (예외 없이 안전, EscargotPublic.h:1068)

### 6.4 objectId (객체 핸들) — MVP 처리

- 객체(`isObject()` 이고 primitive 아님)에는 `objectId` 발급 필요(`getProperties`/`releaseObject`/`callFunctionOn` 대상).
- **핸들 테이블**: `RemoteObjectStore`(NodeRegistry 와 유사, `gc` 상속) 에 `ObjectRef*`(=ValueRef→asObject) 보관, `int → ObjectRef*` 매핑. objectId 문자열은 `"{\"injectedScriptId\":1,\"id\":<n>}"` 형식(Chrome 호환) 또는 단순 `"OBJ-<n>"`.
  - browser 는 inspector(V8) 에 위임하지만 starfish 는 자체 테이블 필요.
  - **MVP 최소**: objectId 발급만 하고, `getProperties` 는 `ObjectRef::ownPropertyKeys`/`get` 으로 1단계 속성만 직렬화. `callFunctionOn` 은 `objectId` 해석 후 `ValueRef::call`. 미구현 시 `-32000` 반환도 허용(Puppeteer 의 단순 evaluate 시나리오는 objectId 불필요).
- objectId 수명: navigate(`executionContextsCleared`) 시 테이블 reset.

```cpp
// RemoteObject.h
namespace Starfish {
class RemoteObjectStore : public gc {
public:
    int store(Escargot::ObjectRef* obj);      // objectId(n) 발급
    Escargot::ObjectRef* lookup(int id);
    void release(int id);
    void reset();                              // navigate 시
private:
    GCUnorderedMap<int, Escargot::ObjectRef*> m_idToObj;
    int m_next;
};

// 자유 함수: ValueRef* → RemoteObject JSON
void serializeRemoteObject(ScriptBindingInstance* sbi,
                           RemoteObjectStore* store,
                           Escargot::ValueRef* value,
                           bool returnByValue,
                           rapidjson::Value& out,
                           rapidjson::Document::AllocatorType& alloc);
}
```

### 6.5 evaluate 실행 (타입 보존 경로)

```cpp
// RuntimeDomain::cmd_evaluate 내부 (메인 스레드, 의사코드)
ScriptBindingInstance* sbi = bc->scriptBindingInstance();
bool ok = false;
ScriptValue v = evaluateString(sbi, expression, /*fileName*/ "<cdp>", &ok);
// v == Escargot::ValueRef* (타입 보존됨)
// ok==false 이면 exceptionDetails 구성, true 이면 result=serializeRemoteObject(v)
```
> 주의: `evaluateString`(ScriptWrappable.cpp:1471) 은 내부에서 예외 시 window 에 error 이벤트를 dispatch 하고 `*result=true`(존재 여부) 로 둔다. exceptionDetails 의 메시지/스택을 정밀히 얻으려면 `Evaluator::execute` 패턴(ScriptWrappable.cpp:1514)을 도메인에서 직접 사용하는 것이 정확. **MVP 는 `evaluateString` 사용 + ok 플래그로 최소 exceptionDetails** 권장. `[추측]`

---

## 7. 스레드 경계 계약

| 동작 | 스레드 | 근거/메커니즘 |
|---|---|---|
| `accept`, `recv`, 프레임 디코드, ping/pong | **IO** | `CDPServer::acceptLoop` (Inspector.cpp:197 패턴) |
| 소켓 `write`(송신 프레임 인코드 포함) | **메인** | `CDPCommand::emit → server->...→ conn->sendText` (read=IO/write=메인 분리, §2.6) |
| JSON 파싱(rapidjson) | **메인** | `dispatchOnMain` 에서. (browser 는 IO 에서 파싱하나 starfish 는 GC/단순화 위해 메인) |
| 도메인 핸들러 실행 | **메인** | DOM/JS/Node* 접근 |
| `evaluateString`, ValueRef 직렬화 | **메인** | Escargot 컨텍스트는 메인 스레드 |
| NodeRegistry/RemoteObjectStore 접근 | **메인** | GC 맵, 메인 전용 |
| navigate 위임 | **메인** | `WebView`/`BrowsingContext` |

### 7.1 IO → 메인 위임 패턴 (Inspector.cpp:248 차용)

IO 스레드 `CDPConnection::readFrames` 가 텍스트 프레임을 얻으면:

```cpp
// IO 스레드
void CDPDispatcher::onMessageFromIO(std::string&& rawJson) {
    // rawJson 은 raw 바이트(UTF-8). GC 객체 미포함 → IO 보관 안전.
    auto* req = new CDPMessageReq{ this, std::move(rawJson) };  // plain heap
    m_webView->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
        nullptr, &CDPDispatcher::onMainTrampoline, req);
}

// 메인 스레드 (MessageLoopInterface.h:26 시그니처)
void CDPDispatcher::onMainTrampoline(size_t /*handle*/, void* data) {
    auto* req = static_cast<CDPMessageReq*>(data);
    req->dispatcher->dispatchOnMain(req->rawJson);
    delete req;
}
```

- `addIdlerWithNoGCRootingInOtherThread(nullptr, fn, data)`: Inspector.cpp:248-250 와 동일. 콜백 시그니처 `void(size_t, void*)` (MessageLoopInterface.h:26).
- `CDPMessageReq` 는 GC 비상속 plain struct(문자열만 보유) → IO 에서 생성/메인에서 delete 안전.
- **금지**: IO 스레드에서 `Node*`, `ValueRef*`, `String*`(GC) 보관/생성.

### 7.2 송신 (메인 → IO 소켓)

§2.6 결론대로 **write 는 메인 스레드에서 직접** `m_conn->sendText()`. 연결 생존은 원자 플래그 가드. (대안 A 송신 큐는 후속 최적화.)

---

## 8. 빌드 통합

### 8.1 config.cmake 플래그

`build/config.cmake` (라인 42 의 `STARFISH_ENABLE_INSPECTOR` 주석 블록 인근)에 항목 추가하고, 실제 정의는 `STARFISH_ENABLE_INSPECTOR` 가 `add_definitions(-D...)` 되는 곳과 동일 위치에 추가:
```cmake
# STARFISH_ENABLE_CDP : enable Chrome DevTools Protocol server (Target/Page/Runtime/DOM/Log)
...
IF (STARFISH_ENABLE_CDP)
    ADD_DEFINITIONS (-DSTARFISH_ENABLE_CDP)
ENDIF()
```
> `STARFISH_ENABLE_INSPECTOR` 의 정확한 `ADD_DEFINITIONS` 위치를 따라 동일 패턴 적용. (config.cmake 는 주석만 있고 실제 정의는 옵션 처리부에 있을 수 있으니 grep `STARFISH_ENABLE_INSPECTOR` 로 확인 후 인접 추가.)

### 8.2 GLOB 자동 포함

`build/starfish.cmake:51`:
```cmake
FILE (GLOB_RECURSE STARFISH_SRC ${STARFISH_ROOT}/src/*.cpp)
```
→ `src/core/cdp/**/*.cpp` 가 **자동 포함**. CMakeLists 수정 불필요(헤더는 GLOB 대상 아님이나 include 경로 `src/` 이미 존재).

### 8.3 링크 라이브러리

- POSIX 소켓(`socket/bind/listen/accept/recv/send/poll/shutdown`)은 glibc 표준 → **추가 링크 라이브러리 불필요**.
- SHA1/base64 자체 구현 → 외부 의존 0.
- pthread 는 기존 Thread/ThreadPool 이 이미 링크.

---

## 9. Puppeteer 호환 상태 머신

### 9.1 식별자 발급 규칙 (browser id.zig 차용)

- `frameId` = `targetId` = `"FID-%010d"` 형식(또는 `"TID-..."`). MVP 단일 타겟이므로 startup 시 1회 고정 발급, 예: `targetId="TID-0000000001"`, `frameId` 동일.
- `loaderId` = `"LID-0000000001"` (page.zig:115).
- `browserContextId` = `"BID-..."` 고정.
- `sessionId` = `"SID-..."` — `attachToTarget`/auto-attach 시 발급(증가 카운터).
- `executionContextId` = `1` (단일 컨텍스트).

### 9.2 enable 플래그

§1.5 `CDPSession` 의 bool 들. 각 도메인 `enable` 이 set, `disable` 이 clear. 이벤트는 해당 도메인 enable 일 때만 발사.

### 9.3 startup graceful 응답 (`"STARTUP"` sessionId, CDP.zig:330-339)

Puppeteer/Stagehand 는 실제 타겟 attach 전, `sessionId:"STARTUP"` 으로 일부 명령을 보낸다.
- `sessionId == "STARTUP"`:
  - `Page.getFrameTree` → **graceful 응답**(page.zig:96-106):
    ```json
    {"frameTree":{"frame":{"id":"TID-STARTUP","loaderId":"LID-STARTUP",
      "securityOrigin":"chrome://newtab/","url":"about:blank","secureContextType":"Secure"}}}
    ```
  - 그 외 모든 STARTUP 명령 → `sendResult({})` (빈 결과). (CDP.zig:338)
- 알 수 없는 비-STARTUP `sessionId` → `-32001 "Unknown sessionId"` (CDP.zig:305).

### 9.4 전형적 Puppeteer 연결 시퀀스 (MVP가 만족해야 할 흐름)

```
1. HTTP GET /json/version            → {"webSocketDebuggerUrl":"ws://host:port/"} (§2.3)
2. WS upgrade GET /                  → 101 Switching Protocols (Sec-WebSocket-Accept)
3. → Target.setDiscoverTargets {discover:true}     ⇐ {}  + (이벤트) Target.targetCreated
4. → Target.setAutoAttach {autoAttach:true,...}    ⇐ {}
   (auto-attach 면 createTarget/이미 존재 타겟에 대해) ⇐ (이벤트) Target.attachedToTarget {sessionId}
5. (이제 sessionId 부여된 명령들)
   → Page.enable        ⇐ {}
   → Runtime.enable     ⇐ {} + (이벤트) Runtime.executionContextCreated
   → Log.enable / DOM.enable  ⇐ {}
   → Page.getFrameTree  ⇐ {frameTree:{frame}}
6. → Runtime.evaluate {expression}   ⇐ {result:RemoteObject}   (evaluateString 경로)
7. → DOM.getDocument                 ⇐ {root:Node}
   → DOM.querySelector {nodeId,selector} ⇐ {nodeId}
8. (navigate 시) Page.navigate       ⇐ {frameId,loaderId}
   (이벤트) frameNavigated, lifecycleEvent(init/DOMContentLoaded/load),
            DOM.documentUpdated, Runtime.executionContextsCleared
            → NodeRegistry.reset(), RemoteObjectStore.reset()
```

### 9.5 이벤트 발사 시점 요약

| 트리거 | 발사 이벤트 (조건) |
|---|---|
| `Target.setDiscoverTargets(true)` | `Target.targetCreated` |
| attach (auto 또는 명시) | `Target.attachedToTarget` |
| `Runtime.enable` | `Runtime.executionContextCreated` |
| navigate 시작 | `Page.frameStartedLoading`(pageEnabled), `Page.frameNavigated` |
| DOMContentLoaded | `Page.domContentEventFired`, `Page.lifecycleEvent{DOMContentLoaded}` |
| load 완료 | `Page.loadEventFired`, `Page.lifecycleEvent{load}`, `Page.frameStoppedLoading` |
| navigate(문서 교체) | `Runtime.executionContextsCleared`, `DOM.documentUpdated` + 레지스트리 reset |
| console.* 호출 | `Runtime.consoleAPICalled`(runtimeEnabled), `Log.entryAdded`(logEnabled) |

---

## 10. 기존 코드 수정 요약 (최소·가드)

### 10.1 WebView.h / .cpp

```cpp
// WebView.h  (멤버 + 메서드)
#if defined(STARFISH_ENABLE_CDP)
public:
    void setupCDPServer(uint16_t port = 9222);
    CDPServer* cdpServer() const { return m_cdpServer; }
private:
    CDPServer* m_cdpServer = nullptr;   // GC 비상속 → 일반 포인터.
#endif
```
- `m_cdpServer` 는 GC 객체가 아니므로 GC 마킹 불필요. **단**, CDPDispatcher 가 보유한 `NodeRegistry`/`RemoteObjectStore`(둘 다 `gc` 상속)는 GC 루팅 필요.
  - 방안: NodeRegistry/RemoteObjectStore 를 `PersistentRefHolder`/GC root 로 등록하거나, WebView 의 GC 추적 멤버(GCVector 등)에 보관. 구현 시 starfish 의 GC root 등록 관용(예: `GC_add_roots` 또는 멤버를 GC 추적 컨테이너에 보관)을 따른다. `[추측]` — 정확한 root 등록 API 는 구현 시 `binding`/`Starfish.cpp` 의 기존 root 패턴 확인 필요.
- `destroy()` 경로에 `if (m_cdpServer) { m_cdpServer->stop(); delete m_cdpServer; }`.

### 10.2 Console.cpp (Log 분기 1곳)

각 `log/info/error/warn/debug` 의 `#if INSPECTOR` 블록 옆에:
```cpp
#if defined(STARFISH_ENABLE_CDP)
    if (m_webBase->isWebView()) {
        // CDPServer 경유로 Runtime.consoleAPICalled / Log.entryAdded 발사
        // (메인 스레드이므로 직접 호출. level 매핑: log->info, warn->warning, error->error, debug->verbose)
    }
#endif
```
> `m_webBase` 가 `WebView` 인지 확인(`isWebView()`, WebView.h:109) 후 `static_cast<WebView*>(m_webBase)->cdpServer()`. 단일 helper 함수로 5곳 공통화 권장.

### 10.3 inc/LWEWebView.h (선택)

```cpp
#if defined(STARFISH_ENABLE_CDP)
    void StartDevTools(uint16_t port = 9222);   // 내부적으로 WebView::setupCDPServer 호출
#endif
```

---

## 11. 미해결/구현 시 확인 필요 (명시적 추측)

1. **GC root 등록 정확 API**: `NodeRegistry`/`RemoteObjectStore`(gc 상속) 를 살려둘 root 등록 방식. `Starfish.cpp`/`binding` 의 기존 persistent root 패턴 확인 후 적용. (`[추측]`)
2. **송신 동시성**: write=메인, read=IO 분리로 안전하다고 가정. 다중 연결/동시 write 필요해지면 §2.6 (A) 송신 큐로 전환.
3. **WS 프래그멘테이션**: 단일 프레임 가정. DevTools 클라이언트 대형 메시지(>64KB) 분할 송신 시 멀티프레임 누적 필요할 수 있음.
4. **exceptionDetails 정밀도**: `evaluateString` 의 내부 에러 핸들링과 충돌 가능 → 정밀 스택이 필요하면 `Evaluator::execute` 직접 사용.
5. **objectId 기반 명령**(`getProperties`/`callFunctionOn`): MVP 는 evaluate 위주 시나리오 우선. 미구현 메서드는 `-32601`/`-32000` 으로 graceful 반환.
6. **`/json/list` 항목 채움**: Puppeteer launch 는 `/json/version` 만 사용 → `[]` 로도 가능. `connect()` 직접 호출 호환 위해 단일 항목 채움 권장.
```

---

## 12. MVP 이후 도메인 확장 (5 → 49 도메인)

MVP 5개 도메인 이후 서버는 `Schema.getDomains` 가 광고하는 **49개 도메인**으로
확장됐다. 권위 있는 도메인별 상태표는 `docs/CDP_DOMAINS.md`, 메서드 단위 표는
`docs/CDP.md`. 본 절은 설계 관점의 요약이다.

### 12.1 구현 관례

- **파일 vs 인라인**: 행위가 큰 도메인은 `domains/*Domain.cpp`(Network, Fetch,
  CSS, DOMSnapshot, Emulation, Storage, Accessibility, Animation, Tracing,
  Overlay, Memory, Performance, Input, Security 등). synthetic/stub 도메인은
  `CDPDispatcher.cpp::route()` 의 else-if 체인에 인라인.
- **정직성 원칙**: 엔진이 데이터를 줄 수 없으면 **거짓 데이터를 만들지 않는다**.
  값을 못 주면 빈 결과/`-32000` 에러로, 미구현 메서드는 `-32601` 로 반환.
- **Schema 동기화**: 라우팅되는 모든 도메인은 `Schema.getDomains` 의 `kDomains[]`
  에 등재돼야 한다(불일치는 버그 — DOMSnapshot 누락 사례를 수정함).

### 12.2 정직성 분류 (honesty taxonomy)

| 분류 | 의미 | 도메인 |
|------|------|--------|
| **real (엔진 연동)** | 실제 엔진 상태를 읽거나 변경 | `HeapProfiler.collectGarbage`→Boehm `GC_gcollect()`; **WebAuthn** 가상 인증기 인메모리 CRUD 레지스트리; **Debugger.getScriptSource**→`Runtime.compileScript` 소스 레지스트리 |
| **synthetic / ack stub** | 핸드셰이크·형태 호환용 빈 결과 또는 ack, 이벤트 없음 | CacheStorage, IndexedDB, ServiceWorker, WebAudio, Media, LayerTree, Preload, EventBreakpoints, BackgroundService, Autofill, FedCm, Database, DeviceAccess, Cast, Tethering (+ MVP 외 Profiler, PerformanceTimeline, Audits) |
| **error stub** | 리소스 레지스트리 부재를 정직하게 에러로 | Extensions(전부 `-32000`); FedCm 다이얼로그/Cast 싱크/DeviceAccess 프롬프트/Database/WebAudio `getRealtimeData`/LayerTree 스냅샷은 해당 id-op 만 `-32000` |

각 stub 은 그 도메인 기능이 엔진에서 **왜** 도달 불가인지를 코드 주석에 남긴다
(예: ServiceWorker 호스트가 CDP 빌드에서 `SERVICE_WORKER=0` 로 컴파일 제외,
IndexedDB 모듈이 `STARFISH_ENABLE_IDB` 가드로 off, WebAudio 모듈이
`STARFISH_ENABLE_WEBAUDIO` off 등).

### 12.3 도메인 전체 목록 (49)

- **MVP 코어(real)**: Target, Page, Runtime, DOM, Log.
- **MVP 외 real/부분**: Network, Fetch, Emulation, CSS, DOMStorage, Storage,
  Accessibility, Security, Performance, IO, Browser, Schema, Inspector,
  SystemInfo, Memory, DOMSnapshot, DOMDebugger, Animation, Tracing, Overlay,
  DeviceOrientation, Input, PerformanceTimeline, Audits, Profiler.
- **본 확장에서 추가(19)**: CacheStorage, IndexedDB, ServiceWorker, HeapProfiler,
  WebAuthn, WebAudio, Media, LayerTree, Preload, EventBreakpoints,
  BackgroundService, Autofill, FedCm, Database, DeviceAccess, Cast, Tethering,
  Extensions, Debugger.

### 12.4 Playwright 호환 — `Target.attachToBrowserTarget`

§9 의 Puppeteer 시퀀스에 더해 Playwright `chromium.connectOverCDP` 를 지원한다.
Playwright 는 page 세션을 얻기 전에 **브라우저 타깃을 먼저 attach** 한다
(`Target.attachToBrowserTarget`). 처리:

1. `TargetDomain` 이 `attachToBrowserTarget` → 연결 범위의 flat browser session
   id(`BSID-*`)를 발급해 `{sessionId}` 반환.
2. `CDPDispatcher` 는 그 browser session id 를 단 명령을 **initial context**
   (브라우저/연결 레벨 스코프)로 라우팅 → 이후 `Target.attachToTarget` 로 page
   세션을 붙이는 기존 경로가 그대로 동작.
3. `resetConnectionState` 에서 browser session 을 비워 재연결 시 재-attach.

Puppeteer 경로는 불변. 검증: `starfish-cdp-test` 스위트가 동일 테스트 파일을 두
클라이언트로 각각 통과(143 + 143).
