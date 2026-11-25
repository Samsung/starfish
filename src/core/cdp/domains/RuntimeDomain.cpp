/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "RuntimeDomain.h"
#include "PageDomain.h"
#include "TargetDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../RemoteObject.h"
#include "../CDPServer.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/dom/Document.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptEngineInstance.h"
#include "binding/ScriptWrappable.h"
#include "EscargotPublic.h"
#include <gc.h>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

namespace Starfish {

using namespace Escargot;

BrowsingContext* RuntimeDomain::targetBrowsingContext(WebView* wv,
                                                      CDPCommand& cmd,
                                                      const char* key)
{
    if (cmd.params() && cmd.params()->HasMember(key) &&
        (*cmd.params())[key].IsInt()) {
        uint32_t ctxId = (uint32_t)(*cmd.params())[key].GetInt();
        Optional<BrowsingContext*> child =
            m_dispatcher->page()->browsingContextForExecutionContextId(ctxId);
        if (child) {
            return child.value();
        }
    }
    return wv->mainBrowsingContext();
}

void RuntimeDomain::handleEnable(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    s->runtimeEnabled = true;
    cmd.sendResultEmpty();

    m_dispatcher->page()->emitExecutionContextCreated(cmd.sessionId());

    // Announce any already-present child iframe frames + their contexts.
    // (When connecting to an already-loaded page there is no navigate, so
    // finishNavigation's discovery never ran.)
    m_dispatcher->page()->discoverChildFrames(cmd.sessionId());
}

void RuntimeDomain::handleDisable(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    s->runtimeEnabled = false;
    cmd.sendResultEmpty();
}

void RuntimeDomain::handleRunIfWaitingForDebugger(CDPCommand& cmd,
                                                  const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    if (s->waitingForDebugger) {
        s->waitingForDebugger = false;
        if (!s->pendingNavigationUrl.empty()) {
            std::string url;
            url.swap(s->pendingNavigationUrl);
            bool started = s->pendingNavigationStarted;
            s->pendingNavigationStarted = false;
            if (started) {
                // Resume reveals the existing blank document before the
                // queued navigation replaces it.
                m_dispatcher->target()->notifyTargetInfoChanged(
                    m_dispatcher->currentContext());
            }
            CDPCommand navigation(m_dispatcher, Optional<int64_t>(),
                                  cmd.sessionId(), nullptr);
            m_dispatcher->page()->navigate(navigation, url, started);
        }
    }
    cmd.sendResultEmpty();
}

void RuntimeDomain::handleEvaluate(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();

    if (!hasStringParam(cmd, "expression")) {
        cmd.sendError(-32602, "'expression' is required");
        return;
    }
    const rapidjson::Value& expression = (*cmd.params())["expression"];
    bool returnByValue = paramBool(cmd, "returnByValue", false);
    bool awaitPromise = paramBool(cmd, "awaitPromise", false);
    std::string source(expression.GetString(), expression.GetStringLength());
    BrowsingContext* bc = targetBrowsingContext(wv, cmd, "contextId");
    evaluateSource(wv, bc, source, returnByValue, awaitPromise, cmd);
    // Page-script run synchronously here may have removed an iframe (e.g.
    // document.querySelector('iframe').remove()); reconcile the frame tree
    // so Page.frameDetached fires for any frame that just disappeared.
    m_dispatcher->page()->sweepDetachedFrames(cmd.sessionId());

    // page.setContent rewrite (bare-expression form).
    maybeNotifyDocumentRewrite(wv, source, cmd.sessionId());
}

void RuntimeDomain::handleCallFunctionOn(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();

    std::string fnSource;
    fnSource = paramString(cmd, "functionDeclaration", fnSource);
    BrowsingContext* bc = targetBrowsingContext(wv, cmd, "executionContextId");
    callFunctionOn(wv, bc, cmd);
    m_dispatcher->page()->sweepDetachedFrames(cmd.sessionId());
    // page.setContent runs document.open/write/close as a function here.
    maybeNotifyDocumentRewrite(wv, fnSource, cmd.sessionId());
}

void RuntimeDomain::handleCompileScript(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();

    BrowsingContext* bc = targetBrowsingContext(wv, cmd, "executionContextId");
    compileScript(wv, bc, cmd);
}

void RuntimeDomain::handleRunScript(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();

    BrowsingContext* bc = targetBrowsingContext(wv, cmd, "executionContextId");
    runScript(wv, bc, cmd);
}

void RuntimeDomain::handleGetProperties(CDPCommand& cmd, const std::string&)
{
    WebView* wv = m_dispatcher->webView();

    // Object handles are global to the store; properties enumerate in the
    // main context (objectId already carries the live ObjectRef).
    getProperties(wv, wv->mainBrowsingContext(), cmd);
}

void RuntimeDomain::handleGetHeapUsage(CDPCommand& cmd, const std::string&)
{
    // Same GC source as Performance.getMetrics: total committed heap and
    // used = total - free.
    size_t total = GC_get_heap_size();
    size_t free = GC_get_free_bytes();
    size_t used = total > free ? total - free : 0;

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("usedSize", (double)used, alloc);
    result.AddMember("totalSize", (double)total, alloc);
    cmd.sendResult(result, doc);
}

void RuntimeDomain::handleGlobalLexicalScopeNames(CDPCommand& cmd,
                                                  const std::string&)
{
    WebView* wv = m_dispatcher->webView();

    globalLexicalScopeNames(wv, cmd);
}

void RuntimeDomain::handleQueryObjects(CDPCommand& cmd, const std::string&)
{
    // Escargot exposes no heap-instance iteration API, so the set of live
    // objects with a given prototype cannot be enumerated. Report not
    // implemented rather than silently returning an empty/incorrect set.
    cmd.sendError(-32000,
                  "Runtime.queryObjects is not supported by this engine");
}

void RuntimeDomain::handleReleaseObject(CDPCommand& cmd, const std::string&)
{
    if (hasStringParam(cmd, "objectId")) {
        std::string oid = (*cmd.params())["objectId"].GetString();
        if (oid.rfind("OBJ-", 0) == 0) {
            int id = atoi(oid.c_str() + 4);
            m_dispatcher->remoteObjectStore()->release(id);
        }
    }
    cmd.sendResultEmpty();
}

void RuntimeDomain::handleAddBinding(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    if (!hasStringParam(cmd, "name")) {
        cmd.sendError(-32602, "'name' is required");
        return;
    }
    std::string name = (*cmd.params())["name"].GetString();
    // Record for re-injection on navigate; avoid duplicates.
    bool known = false;
    for (const std::string& b : s->bindings) {
        if (b == name) {
            known = true;
            break;
        }
    }
    if (!known) {
        s->bindings.push_back(name);
    }
    injectBinding(wv, name);
    cmd.sendResultEmpty();
}

void RuntimeDomain::handleRemoveBinding(CDPCommand& cmd, const std::string&)
{
    CDPSession* s = m_dispatcher->session();

    if (hasStringParam(cmd, "name")) {
        std::string name = (*cmd.params())["name"].GetString();
        auto& v = s->bindings;
        for (auto it = v.begin(); it != v.end(); ++it) {
            if (*it == name) {
                v.erase(it);
                break;
            }
        }
    }
    cmd.sendResultEmpty();
}

void RuntimeDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    static const CDPMethodHandler<RuntimeDomain> handlers[] = {
        { "enable", &RuntimeDomain::handleEnable },
        { "disable", &RuntimeDomain::handleDisable },
        { "runIfWaitingForDebugger",
          &RuntimeDomain::handleRunIfWaitingForDebugger },
        { "evaluate", &RuntimeDomain::handleEvaluate },
        { "callFunctionOn", &RuntimeDomain::handleCallFunctionOn },
        { "compileScript", &RuntimeDomain::handleCompileScript },
        { "runScript", &RuntimeDomain::handleRunScript },
        { "getProperties", &RuntimeDomain::handleGetProperties },
        { "getHeapUsage", &RuntimeDomain::handleGetHeapUsage },
        { "globalLexicalScopeNames",
          &RuntimeDomain::handleGlobalLexicalScopeNames },
        { "queryObjects", &RuntimeDomain::handleQueryObjects },
        { "releaseObject", &RuntimeDomain::handleReleaseObject },
        { "addBinding", &RuntimeDomain::handleAddBinding },
        { "removeBinding", &RuntimeDomain::handleRemoveBinding },
    };
    if (CDPHandlerFn<RuntimeDomain> handler =
            findCDPMethodHandler(handlers, method)) {
        (this->*handler)(cmd, method);
        return;
    }
    cmd.sendError(-32601, "'method' wasn't found");
}

// Descriptor stored on the injected window[name] function via setExtraData,
// so the native callback knows which binding name fired. Page script can pass
// that function to any binding, which reads extraData as a ScriptWrappable,
// so it must be one.
class BindingExtra : public ScriptWrappable {
public:
    explicit BindingExtra(String* name)
        : ScriptWrappable(this)
        , m_name(name)
    {
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override
    {
    }
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return nullptr;
    }

    String* m_name;
};

// Native callback for window[name](payload). Resolves the originating WebView
// from the executing global object, then emits Runtime.bindingCalled through
// that WebView's CDP dispatcher. Returns undefined to page JS (puppeteer's
// helper script tracks the result via a separate callback, not the return
// value).
static ValueRef* bindingNativeCallback(ExecutionStateRef* state,
                                       ValueRef* thisValue, size_t argc,
                                       ValueRef** argv, bool isNewExpression)
{
    ScriptObject callee = toCalleeObject(state);
    if (!callee) {
        return ValueRef::createUndefined();
    }
    BindingExtra* extra = (BindingExtra*)callee->extraData();
    if (!extra) {
        return ValueRef::createUndefined();
    }

    std::string payload;
    if (argc > 0) {
        StringRef* str = argv[0]->toStringWithoutException(state->context());
        payload = str->toStdUTF8String();
    }

    Window* window = (Window*)state->context()->globalObject()->extraData();
    if (!window) {
        return ValueRef::createUndefined();
    }
    WebView* wv = window->webView();
    if (!wv || !wv->cdpServer() || !wv->cdpServer()->dispatcher()) {
        return ValueRef::createUndefined();
    }
    wv->cdpServer()->dispatcher()->emitBindingCalled(
        wv, extra->m_name->toUTF8NonGCString(), payload);
    return ValueRef::createUndefined();
}

void RuntimeDomain::injectBinding(WebView* wv, const std::string& name)
{
    BrowsingContext* bc = wv->mainBrowsingContext();
    if (!bc || !bc->scriptBindingInstance() ||
        !bc->scriptBindingInstance()->isScriptingEnabledIgnoringCDP()) {
        return;
    }
    ContextRef* ctx = bc->scriptBindingInstance()->scriptContext();

    struct InjectData {
        const std::string* name;
    } data{ &name };

    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, InjectData* d) -> ValueRef* {
            ContextRef* context = state->context();
            GlobalObjectRef* global = context->globalObject();

            BindingExtra* extra = new BindingExtra(
                String::fromUTF8(d->name->data(), d->name->size()));

            StringRef* fnName =
                StringRef::createFromUTF8(d->name->data(), d->name->size());
            FunctionObjectRef* fn = FunctionObjectRef::create(
                state, FunctionObjectRef::NativeFunctionInfo(
                           AtomicStringRef::create(context, d->name->data(),
                                                   d->name->size()),
                           bindingNativeCallback, 1, true, false));
            fn->setExtraData(extra);

            global->defineDataProperty(state, ValueRef::create(fnName),
                                       ValueRef::create(fn), true, false, true);
            return ValueRef::createUndefined();
        },
        &data);
}

// Parse "OBJ-<n>" -> n, or -1 if not a valid handle id string.
static int parseObjectId(const char* oid)
{
    if (!oid) {
        return -1;
    }
    if (strncmp(oid, "OBJ-", 4) != 0) {
        return -1;
    }
    return atoi(oid + 4);
}

// Evaluate a self-contained source expression and return its ValueRef, or set
// *errOut to the thrown value. Used to resolve value-typed call arguments
// (their JSON re-serialized as a JS literal) into live ValueRefs.
static ValueRef* evalExpr(ContextRef* ctx, const std::string& expr,
                          OptionalRef<ValueRef>& errOut)
{
    StringRef* source = StringRef::createFromUTF8(expr.data(), expr.size());
    StringRef* fileName = StringRef::createFromUTF8("<cdp-arg>", 9);
    ScriptParserRef::InitializeScriptResult parsed =
        ctx->scriptParser()->initializeScript(source, fileName, false);
    if (!parsed.isSuccessful()) {
        errOut = ValueRef::createUndefined();
        return ValueRef::createUndefined();
    }
    Evaluator::EvaluatorResult r = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ScriptRef* script) -> ValueRef* {
            return script->execute(state);
        },
        parsed.script.value());
    if (r.error.hasValue()) {
        errOut = r.error;
        return ValueRef::createUndefined();
    }
    return r.result;
}

// If `value` is a Promise and awaitPromise is requested, first drain
// microtasks and observer callbacks that can settle synchronously. A caller
// that still sees a pending Promise defers the CDP response so the normal
// message loop can run timers and network completion.
static ValueRef* settlePromiseIfNeeded(WebView* wv, ContextRef* ctx,
                                       ValueRef* value, bool awaitPromise,
                                       bool& isRejected)
{
    isRejected = false;
    if (!awaitPromise || !value->isObject() ||
        !value->asObject()->isPromiseObject()) {
        return value;
    }

    PromiseObjectRef* promise = value->asObject()->asPromiseObject();

    auto drainJobs = [&]() {
        while (ctx->vmInstance()->hasPendingJob()) {
            ctx->vmInstance()->executePendingJob();
        }
    };

    // Bounded settle loop: drain microtasks, then advance one
    // rendering/observer tick (which fires IntersectionObserver/ResizeObserver
    // callbacks), repeat.
    const int kMaxTicks = 200;
    for (int i = 0; i < kMaxTicks; i++) {
        drainJobs();
        if (promise->state() != PromiseObjectRef::Pending) {
            break;
        }
        wv->layoutIfNeeded();
        wv->updateObservation();
    }
    drainJobs();

    if (promise->state() == PromiseObjectRef::Rejected) {
        isRejected = true;
        return promise->promiseResult();
    }
    if (promise->state() == PromiseObjectRef::FulFilled) {
        return promise->promiseResult();
    }
    // A timer or network operation still owns the Promise. The caller will
    // defer the response and poll from the normal message loop.
    return value;
}

// The script binding of bc when it can run script, or nullptr after replying
// that there is no scripting context.
static ScriptBindingInstance* scriptingInstance(BrowsingContext* bc,
                                                CDPCommand& cmd)
{
    if (!bc || !bc->scriptBindingInstance() ||
        !bc->scriptBindingInstance()->isScriptingEnabledIgnoringCDP()) {
        cmd.sendError(-32000, "No scripting context");
        return nullptr;
    }
    return bc->scriptBindingInstance();
}

// Adds exceptionDetails with the message and position. A thrown value also
// becomes its exception RemoteObject.
static void addExceptionDetails(rapidjson::Value& result,
                                const std::string& text, int lineNumber,
                                int columnNumber, ScriptBindingInstance* sbi,
                                RemoteObjectStore* store, ValueRef* thrown,
                                rapidjson::Document::AllocatorType& alloc)
{
    rapidjson::Value ex(rapidjson::kObjectType);
    ex.AddMember("exceptionId", 1, alloc);
    addStringMember(ex, "text", text, alloc);
    ex.AddMember("lineNumber", lineNumber, alloc);
    ex.AddMember("columnNumber", columnNumber, alloc);
    if (thrown) {
        rapidjson::Value exObj(rapidjson::kObjectType);
        serializeRemoteObject(sbi, store, thrown, false, exObj, alloc);
        ex.AddMember("exception", exObj, alloc);
    }
    result.AddMember("exceptionDetails", ex, alloc);
}

// Fill `result` for a settled value. A rejected value also gets
// exceptionDetails, and returnByValue is ignored for it.
// The synchronous paths and the deferred Promise path all use this.
static void buildSettledResult(ScriptBindingInstance* sbi,
                               RemoteObjectStore* store, ValueRef* settled,
                               bool rejected, bool returnByValue,
                               rapidjson::Value& result,
                               rapidjson::Document::AllocatorType& alloc)
{
    rapidjson::Value remote(rapidjson::kObjectType);
    if (!rejected) {
        serializeRemoteObject(sbi, store, settled, returnByValue, remote,
                              alloc);
        result.AddMember("result", remote, alloc);
        return;
    }
    serializeRemoteObject(sbi, store, settled, false, remote, alloc);
    result.AddMember("result", remote, alloc);
    std::string text = settled->toStringWithoutException(sbi->scriptContext())
                           ->toStdUTF8String();
    addExceptionDetails(result, text, 0, 0, sbi, store, settled, alloc);
}

// Chrome sends this error when the page navigates or closes while a
// command waits on a Promise.
static const char* const kTargetNavigatedMessage =
    "Inspected target navigated or closed";

// One Runtime.evaluate / callFunctionOn waiting on a pending Promise.
// It lives on the malloc heap, so its GC pointers are rooted while it is
// registered. RuntimeDomain owns it and frees it on every exit path.
struct RuntimeDomain::PendingEvaluation {
    uint64_t id;
    WebView* webView;
    ScriptBindingInstance* binding;
    PromiseObjectRef* promise;
    bool returnByValue;
    bool hasCommandId;
    int64_t commandId;
    std::string sessionId;
};

void RuntimeDomain::freePending(PendingEvaluation* p)
{
    GC_remove_roots(p, reinterpret_cast<char*>(p) + sizeof(*p));
    p->~PendingEvaluation();
    std::free(p);
}

RuntimeDomain::PendingEvaluation* RuntimeDomain::takePending(uint64_t id)
{
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        if ((*it)->id == id) {
            PendingEvaluation* p = *it;
            m_pending.erase(it);
            return p;
        }
    }
    return nullptr;
}

void RuntimeDomain::settlePending(uint64_t id)
{
    PendingEvaluation* p = takePending(id);
    if (!p) {
        return;
    }
    TargetContext* target = m_dispatcher->contextForWebView(p->webView);
    if (target) {
        CDPDispatcher::ScopedTarget scopedTarget(m_dispatcher, target);
        Optional<int64_t> commandId;
        if (p->hasCommandId) {
            commandId = p->commandId;
        }
        CDPCommand command(m_dispatcher, commandId, p->sessionId, nullptr);
        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);
        buildSettledResult(p->binding, m_dispatcher->remoteObjectStore(),
                           p->promise->promiseResult(),
                           p->promise->state() == PromiseObjectRef::Rejected,
                           p->returnByValue, result, alloc);
        command.sendResult(result, doc);
    }
    freePending(p);
}

void RuntimeDomain::cancelPendingEvaluations(WebView* wv, bool notify)
{
    cancelMatching(wv, nullptr, notify);
}

void RuntimeDomain::cancelPendingEvaluations(ScriptBindingInstance* sbi)
{
    cancelMatching(nullptr, sbi, true);
}

void RuntimeDomain::cancelMatching(WebView* wv, ScriptBindingInstance* sbi,
                                   bool notify)
{
    std::vector<PendingEvaluation*> taken;
    for (auto it = m_pending.begin(); it != m_pending.end();) {
        if ((wv && (*it)->webView == wv) || (sbi && (*it)->binding == sbi)) {
            taken.push_back(*it);
            it = m_pending.erase(it);
        } else {
            ++it;
        }
    }
    for (PendingEvaluation* p : taken) {
        TargetContext* target = m_dispatcher->contextForWebView(p->webView);
        if (notify && target && p->hasCommandId) {
            CDPDispatcher::ScopedTarget scopedTarget(m_dispatcher, target);
            CDPCommand command(m_dispatcher, Optional<int64_t>(p->commandId),
                               p->sessionId, nullptr);
            command.sendError(-32000, kTargetNavigatedMessage);
        }
        freePending(p);
    }
}

void RuntimeDomain::cancelAllPendingEvaluations()
{
    std::vector<PendingEvaluation*> taken;
    taken.swap(m_pending);
    for (PendingEvaluation* p : taken) {
        freePending(p);
    }
}

// Native fulfill/reject handler. The function's extraData is the pending id,
// not a pointer, so a late call after cancellation finds nothing and does
// nothing.
static ValueRef* pendingPromiseCallback(ExecutionStateRef* state,
                                        ValueRef* thisValue, size_t argc,
                                        ValueRef** argv, bool isNewExpression)
{
    ScriptObject callee = toCalleeObject(state);
    if (!callee) {
        return ValueRef::createUndefined();
    }
    uint64_t id =
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(callee->extraData()));
    Window* window =
        static_cast<Window*>(state->context()->globalObject()->extraData());
    if (!window) {
        return ValueRef::createUndefined();
    }
    WebView* wv = window->webView();
    if (!wv || !wv->cdpServer() || !wv->cdpServer()->dispatcher()) {
        return ValueRef::createUndefined();
    }
    wv->cdpServer()->dispatcher()->runtime()->settlePending(id);
    return ValueRef::createUndefined();
}

bool RuntimeDomain::deferPromise(WebView* webView, ScriptBindingInstance* sbi,
                                 ValueRef* value, bool returnByValue,
                                 bool awaitPromise, CDPCommand& command)
{
    if (!awaitPromise || !value->isObject() ||
        !value->asObject()->isPromiseObject()) {
        return false;
    }
    PromiseObjectRef* promise = value->asObject()->asPromiseObject();
    if (promise->state() != PromiseObjectRef::Pending) {
        return false;
    }

    void* storage = std::malloc(sizeof(PendingEvaluation));
    STARFISH_RELEASE_ASSERT(storage);
    PendingEvaluation* p = new (storage) PendingEvaluation();
    p->id = ++m_nextPendingId;
    p->webView = webView;
    p->binding = sbi;
    p->promise = promise;
    p->returnByValue = returnByValue;
    p->hasCommandId = command.hasId();
    p->commandId = command.hasId() ? command.id() : 0;
    p->sessionId = command.sessionId();
    GC_add_roots(p, reinterpret_cast<char*>(p) + sizeof(*p));
    m_pending.push_back(p);

    struct ThenData {
        PromiseObjectRef* promise;
        uint64_t id;
    } data{ promise, p->id };
    Evaluator::EvaluatorResult r = Evaluator::execute(
        sbi->scriptContext(),
        [](ExecutionStateRef* state, ThenData* d) -> ValueRef* {
            FunctionObjectRef* fn = FunctionObjectRef::create(
                state, FunctionObjectRef::NativeFunctionInfo(
                           AtomicStringRef::emptyAtomicString(),
                           pendingPromiseCallback, 1, true, false));
            fn->setExtraData(
                reinterpret_cast<void*>(static_cast<uintptr_t>(d->id)));
            d->promise->then(state, fn, fn);
            return ValueRef::createUndefined();
        },
        &data);
    if (!r.isSuccessful()) {
        // The handlers were not attached; nothing can settle this record.
        freePending(takePending(p->id));
        return false;
    }
    return true;
}

// Data passed into the Evaluator closure that performs the actual function
// invocation with a live receiver and live argument values.
struct CallData {
    ValueRef* fn;
    ValueRef* thisVal;
    ValueRef** argv;
    size_t argc;
};

void RuntimeDomain::callFunctionOn(WebView* wv, BrowsingContext* bc,
                                   CDPCommand& cmd)
{
    if (!hasStringParam(cmd, "functionDeclaration")) {
        cmd.sendError(-32602, "'functionDeclaration' is required");
        return;
    }

    ScriptBindingInstance* sbi = scriptingInstance(bc, cmd);
    if (!sbi) {
        return;
    }
    ContextRef* ctx = sbi->scriptContext();
    RemoteObjectStore* store = m_dispatcher->remoteObjectStore();

    std::string fn = (*cmd.params())["functionDeclaration"].GetString();
    bool returnByValue = paramBool(cmd, "returnByValue", false);
    bool awaitPromise = paramBool(cmd, "awaitPromise", false);

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

    // Resolve the receiver: objectId -> stored ObjectRef, else globalThis.
    ValueRef* thisVal = ctx->globalObject();
    if (cmd.params()->HasMember("objectId") &&
        (*cmd.params())["objectId"].IsString()) {
        int id = parseObjectId((*cmd.params())["objectId"].GetString());
        ObjectRef* obj = id >= 0 ? store->lookup(id) : nullptr;
        if (!obj) {
            cmd.sendError(-32000, "Could not find object with given id");
            return;
        }
        thisVal = obj;
    }

    // Compile the function declaration to a live function value.
    OptionalRef<ValueRef> evalErr;
    ValueRef* fnVal = evalExpr(ctx, "(" + fn + ")", evalErr);
    if (evalErr.hasValue() || !fnVal->isCallable()) {
        cmd.sendError(-32000, "functionDeclaration is not callable");
        return;
    }

    // Resolve arguments: value-typed (JSON literal) or objectId (live handle).
    std::vector<ValueRef*> argv;
    if (cmd.params()->HasMember("arguments") &&
        (*cmd.params())["arguments"].IsArray()) {
        const rapidjson::Value& args = (*cmd.params())["arguments"];
        for (rapidjson::SizeType i = 0; i < args.Size(); i++) {
            const rapidjson::Value& a = args[i];
            if (a.HasMember("objectId") && a["objectId"].IsString()) {
                int id = parseObjectId(a["objectId"].GetString());
                ObjectRef* obj = id >= 0 ? store->lookup(id) : nullptr;
                argv.push_back(obj ? (ValueRef*)obj
                                   : ValueRef::createUndefined());
            } else if (a.HasMember("value")) {
                rapidjson::StringBuffer buf;
                rapidjson::Writer<rapidjson::StringBuffer> w(buf);
                a["value"].Accept(w);
                OptionalRef<ValueRef> argErr;
                ValueRef* v = evalExpr(
                    ctx,
                    "(" + std::string(buf.GetString(), buf.GetSize()) + ")",
                    argErr);
                argv.push_back(argErr.hasValue() ? ValueRef::createUndefined()
                                                 : v);
            } else if (a.HasMember("unserializableValue") &&
                       a["unserializableValue"].IsString()) {
                OptionalRef<ValueRef> argErr;
                ValueRef* v = evalExpr(
                    ctx,
                    "(" + std::string(a["unserializableValue"].GetString()) +
                        ")",
                    argErr);
                argv.push_back(argErr.hasValue() ? ValueRef::createUndefined()
                                                 : v);
            } else {
                argv.push_back(ValueRef::createUndefined());
            }
        }
    }

    CallData cd;
    cd.fn = fnVal;
    cd.thisVal = thisVal;
    cd.argv = argv.empty() ? nullptr : argv.data();
    cd.argc = argv.size();

    // Run inside a macrotask so promises resolved synchronously by the callee
    // may legally enqueue their .then reactions (matches evaluateString()); the
    // manager drains the microtask queue when this function returns. Without it
    // the engine aborts on the first JS-job enqueue (markJSJobEnqueued). This
    // is what makes puppeteer page.exposeFunction's result-return path work.
    MicroTaskExecutionManager microTaskManager(sbi->engineInstance());

    Evaluator::EvaluatorResult res = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, CallData* d) -> ValueRef* {
            return d->fn->call(state, d->thisVal, d->argc, d->argv);
        },
        &cd);

    rapidjson::Value result(rapidjson::kObjectType);
    if (res.error.hasValue()) {
        ValueRef* errorValue = res.error.value();
        rapidjson::Value remote(rapidjson::kObjectType);
        serializeRemoteObject(sbi, store, errorValue, false, remote, alloc);
        result.AddMember("result", remote, alloc);
        std::string text =
            errorValue->toStringWithoutException(ctx)->toStdUTF8String();
        addExceptionDetails(result, text, 0, 0, sbi, store, errorValue, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    bool settledRejected = false;
    ValueRef* settled = settlePromiseIfNeeded(wv, ctx, res.result, awaitPromise,
                                              settledRejected);
    if (deferPromise(wv, sbi, res.result, returnByValue, awaitPromise, cmd)) {
        return;
    }

    buildSettledResult(sbi, store, settled, settledRejected, returnByValue,
                       result, alloc);
    cmd.sendResult(result, doc);
}

void RuntimeDomain::compileScript(WebView* wv, BrowsingContext* bc,
                                  CDPCommand& cmd)
{
    if (!hasStringParam(cmd, "expression")) {
        cmd.sendError(-32602, "'expression' is required");
        return;
    }
    std::string expr = (*cmd.params())["expression"].GetString();
    bool persist = paramBool(cmd, "persistScript", false);
    std::string sourceURL;
    if (cmd.params()->HasMember("sourceURL") &&
        (*cmd.params())["sourceURL"].IsString()) {
        sourceURL = (*cmd.params())["sourceURL"].GetString();
    }

    ScriptBindingInstance* sbi = scriptingInstance(bc, cmd);
    if (!sbi) {
        return;
    }
    ContextRef* ctx = sbi->scriptContext();

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);

    // Compile (parse) the expression to detect syntax errors up front.
    StringRef* source = StringRef::createFromUTF8(expr.data(), expr.size());
    std::string fn =
        sourceURL.empty() ? std::string("<cdp-compile>") : sourceURL;
    StringRef* fileName = StringRef::createFromUTF8(fn.data(), fn.size());
    ScriptParserRef::InitializeScriptResult parsed =
        ctx->scriptParser()->initializeScript(source, fileName, false);

    if (!parsed.isSuccessful()) {
        std::string msg = parsed.parseErrorMessage
                              ? parsed.parseErrorMessage->toStdUTF8String()
                              : "SyntaxError";
        addExceptionDetails(result, msg, 0, 0, nullptr, nullptr, nullptr,
                            alloc);
        cmd.sendResult(result, doc);
        return;
    }

    // Compiled OK. Persist the source (not the GC-managed ScriptRef; see
    // CDPSession::compiledScripts) and hand back a scriptId only when asked.
    if (persist) {
        CDPSession* s = m_dispatcher->session();
        std::string scriptId =
            "script-" + std::to_string(++s->compiledScriptCounter);
        s->compiledScripts[scriptId] = expr;
        addStringMember(result, "scriptId", scriptId, alloc);
    }
    cmd.sendResult(result, doc);
}

void RuntimeDomain::runScript(WebView* wv, BrowsingContext* bc, CDPCommand& cmd)
{
    if (!hasStringParam(cmd, "scriptId")) {
        cmd.sendError(-32602, "'scriptId' is required");
        return;
    }
    std::string scriptId = (*cmd.params())["scriptId"].GetString();
    CDPSession* s = m_dispatcher->session();
    auto it = s->compiledScripts.find(scriptId);
    if (it == s->compiledScripts.end()) {
        cmd.sendError(-32000, "No script with given id");
        return;
    }

    bool returnByValue = paramBool(cmd, "returnByValue", false);
    bool awaitPromise = paramBool(cmd, "awaitPromise", false);

    // Evaluate the persisted source, serializing the result exactly like
    // Runtime.evaluate.
    evaluateSource(wv, bc, it->second, returnByValue, awaitPromise, cmd);
}

// Data passed into the Evaluator closure that enumerates own properties.
struct PropEnumData {
    ObjectRef* obj;
    bool ownOnly;
    // Collected (name, value) pairs plus attribute flags.
    std::vector<std::string>* names;
    std::vector<ValueRef*>* values;
    std::vector<bool>* enumerable;
    std::vector<bool>* configurable;
    std::vector<bool>* writable;
};

void RuntimeDomain::maybeNotifyDocumentRewrite(WebView* wv,
                                               const std::string& source,
                                               const std::string& sessionId)
{
    // Trigger: the evaluated source performed an explicit document.open() (the
    // signature puppeteer's setFrameContent always emits) AND the resulting
    // main document is a script-created one. Guarding on
    // openFunctionExplicitCalled() avoids firing for unrelated evaluates that
    // merely mention "document.open".
    if (source.find("document.open") == std::string::npos) {
        return;
    }
    BrowsingContext* mbc = wv ? wv->mainBrowsingContext() : nullptr;
    if (mbc && mbc->document() &&
        mbc->document()->openFunctionExplicitCalled()) {
        m_dispatcher->page()->notifyDocumentRewritten(sessionId);
    }
}

void RuntimeDomain::getProperties(WebView* wv, BrowsingContext* bc,
                                  CDPCommand& cmd)
{
    if (!hasStringParam(cmd, "objectId")) {
        cmd.sendError(-32602, "'objectId' is required");
        return;
    }

    ScriptBindingInstance* sbi = scriptingInstance(bc, cmd);
    if (!sbi) {
        return;
    }
    ContextRef* ctx = sbi->scriptContext();
    RemoteObjectStore* store = m_dispatcher->remoteObjectStore();

    int id = parseObjectId((*cmd.params())["objectId"].GetString());
    ObjectRef* obj = id >= 0 ? store->lookup(id) : nullptr;
    if (!obj) {
        cmd.sendError(-32000, "Could not find object with given id");
        return;
    }

    std::vector<std::string> names;
    std::vector<ValueRef*> values;
    std::vector<bool> enumerable, configurable, writable;

    PropEnumData ped;
    ped.obj = obj;
    ped.names = &names;
    ped.values = &values;
    ped.enumerable = &enumerable;
    ped.configurable = &configurable;
    ped.writable = &writable;

    Evaluator::EvaluatorResult res = Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, PropEnumData* d) -> ValueRef* {
            d->obj->enumerateObjectOwnProperties(
                state,
                [d](ExecutionStateRef* st, ValueRef* name, bool w, bool e,
                    bool c) -> bool {
                    d->names->push_back(
                        name->toStringWithoutException(st->context())
                            ->toStdUTF8String());
                    d->values->push_back(d->obj->get(st, name));
                    d->writable->push_back(w);
                    d->enumerable->push_back(e);
                    d->configurable->push_back(c);
                    return true;
                },
                true);
            return ValueRef::createUndefined();
        },
        &ped);

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);

    if (res.error.hasValue()) {
        cmd.sendError(-32000, "Failed to enumerate properties");
        return;
    }

    rapidjson::Value arr(rapidjson::kArrayType);
    for (size_t i = 0; i < names.size(); i++) {
        rapidjson::Value prop(rapidjson::kObjectType);
        addStringMember(prop, "name", names[i], alloc);
        rapidjson::Value rv(rapidjson::kObjectType);
        serializeRemoteObject(sbi, store, values[i], false, rv, alloc);
        prop.AddMember("value", rv, alloc);
        prop.AddMember("writable", (bool)writable[i], alloc);
        prop.AddMember("enumerable", (bool)enumerable[i], alloc);
        prop.AddMember("configurable", (bool)configurable[i], alloc);
        prop.AddMember("isOwn", true, alloc);
        arr.PushBack(prop, alloc);
    }
    result.AddMember("result", arr, alloc);
    cmd.sendResult(result, doc);
}

// Data passed into the Evaluator closure that collects global lexical names.
struct LexicalNamesData {
    GlobalObjectRef* global;
    std::vector<std::string>* names;
};

void RuntimeDomain::globalLexicalScopeNames(WebView* wv, CDPCommand& cmd)
{
    BrowsingContext* bc = targetBrowsingContext(wv, cmd, "executionContextId");
    ScriptBindingInstance* sbi = scriptingInstance(bc, cmd);
    if (!sbi) {
        return;
    }
    ContextRef* ctx = sbi->scriptContext();

    // Approximation: Escargot's public API exposes no accessor for the global
    // lexical environment record, so we enumerate the global object's own
    // property names instead. This covers var/function global declarations
    // (which become globalThis own properties) but MISSES let/const/class
    // bindings, which live in the global lexical scope and are not own
    // properties of globalThis. Returning the var/function set keeps CDP
    // clients (e.g. puppeteer) from throwing on an absent reply.
    std::vector<std::string> names;
    LexicalNamesData lnd{ ctx->globalObject(), &names };
    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, LexicalNamesData* d) -> ValueRef* {
            d->global->enumerateObjectOwnProperties(
                state,
                [d](ExecutionStateRef* st, ValueRef* name, bool w, bool e,
                    bool c) -> bool {
                    d->names->push_back(
                        name->toStringWithoutException(st->context())
                            ->toStdUTF8String());
                    return true;
                },
                true);
            return ValueRef::createUndefined();
        },
        &lnd);

    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Value arr(rapidjson::kArrayType);
    for (const std::string& n : names) {
        arr.PushBack(rapidjson::Value(n.c_str(), n.size(), alloc), alloc);
    }
    result.AddMember("names", arr, alloc);
    cmd.sendResult(result, doc);
}

void RuntimeDomain::evaluateSource(WebView* wv, BrowsingContext* bc,
                                   const std::string& exprStr,
                                   bool returnByValue, bool awaitPromise,
                                   CDPCommand& cmd)
{
    {
        if (!bc || !bc->scriptBindingInstance()) {
            cmd.sendError(-32000, "No scripting context");
            return;
        }
        ScriptBindingInstance* sbi = bc->scriptBindingInstance();
        if (!sbi->isScriptingEnabledIgnoringCDP()) {
            cmd.sendError(-32000, "Scripting disabled");
            return;
        }
        ContextRef* ctx = sbi->scriptContext();

        // Run inside a macrotask (see callFunctionOn): lets synchronously
        // resolved promises enqueue their reactions and drains the microtask
        // queue on return, matching evaluateString().
        MicroTaskExecutionManager microTaskManager(sbi->engineInstance());

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value result(rapidjson::kObjectType);

        // Replicate evaluateString()'s execution path (ScriptWrappable.cpp:
        // 1471) directly so that exceptions are detected precisely via
        // EvaluatorResult::error and WITHOUT the side effect of dispatching a
        // window "error" event (which evaluateString does on runtime errors).
        StringRef* source =
            StringRef::createFromUTF8(exprStr.data(), exprStr.size());
        StringRef* fileName = StringRef::createFromUTF8("<cdp>", 5);
        ScriptParserRef::InitializeScriptResult parsed =
            ctx->scriptParser()->initializeScript(source, fileName, false);

        if (!parsed.isSuccessful()) {
            // Parse error: report exceptionDetails with the parse message.
            std::string msg = parsed.parseErrorMessage
                                  ? parsed.parseErrorMessage->toStdUTF8String()
                                  : "SyntaxError";
            rapidjson::Value remote(rapidjson::kObjectType);
            remote.SetObject();
            remote.AddMember("type", "undefined", alloc);
            result.AddMember("result", remote, alloc);
            addExceptionDetails(result, msg, 0, 0, nullptr, nullptr, nullptr,
                                alloc);
            cmd.sendResult(result, doc);
            return;
        }

        Evaluator::EvaluatorResult sbresult = Evaluator::execute(
            ctx,
            [](ExecutionStateRef* state, ScriptRef* script) -> ValueRef* {
                return script->execute(state);
            },
            parsed.script.value());

        if (sbresult.error.hasValue()) {
            // Runtime exception: serialize the thrown value as the exception
            // RemoteObject and populate exceptionDetails (text + best-effort
            // line/column from the deepest stack frame). No window error event.
            ValueRef* errorValue = sbresult.error.value();
            rapidjson::Value remote(rapidjson::kObjectType);
            serializeRemoteObject(sbi, m_dispatcher->remoteObjectStore(),
                                  errorValue, returnByValue, remote, alloc);
            result.AddMember("result", remote, alloc);

            std::string text =
                errorValue->toStringWithoutException(ctx)->toStdUTF8String();
            int lineNumber = 0;
            int columnNumber = 0;
            if (sbresult.stackTrace.size() > 0) {
                size_t last = sbresult.stackTrace.size() - 1;
                lineNumber = (int)sbresult.stackTrace[last].loc.line;
                columnNumber = (int)sbresult.stackTrace[last].loc.column;
            }
            addExceptionDetails(result, text, lineNumber, columnNumber, sbi,
                                m_dispatcher->remoteObjectStore(), errorValue,
                                alloc);
            cmd.sendResult(result, doc);
            return;
        }

        bool settledRejected = false;
        ValueRef* settled = settlePromiseIfNeeded(
            wv, ctx, sbresult.result, awaitPromise, settledRejected);
        if (deferPromise(wv, sbi, sbresult.result, returnByValue, awaitPromise,
                         cmd)) {
            return;
        }

        buildSettledResult(sbi, m_dispatcher->remoteObjectStore(), settled,
                           settledRejected, returnByValue, result, alloc);
        cmd.sendResult(result, doc);
        return;
    }
}

} // namespace Starfish

#endif
