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
#include <cstring>
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
        BrowsingContext* child =
            m_dispatcher->page()->browsingContextForExecutionContextId(ctxId);
        if (child) {
            return child;
        }
    }
    return wv->mainBrowsingContext();
}

void RuntimeDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();
    WebView* wv = m_dispatcher->webView();

    if (method == "enable") {
        s->runtimeEnabled = true;
        cmd.sendResultEmpty();

        // Emit Runtime.executionContextCreated.
        std::string origin;
        BrowsingContext* bc = wv->mainBrowsingContext();
        if (bc && bc->document() && bc->document()->origin()) {
            origin = bc->document()->origin()->toUTF8NonGCString();
        }

        rapidjson::Document doc;
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value params(rapidjson::kObjectType);
        rapidjson::Value context(rapidjson::kObjectType);
        context.AddMember("id", (int)s->executionContextId, alloc);
        context.AddMember(
            "origin", rapidjson::Value(origin.c_str(), origin.size(), alloc),
            alloc);
        context.AddMember("name", "", alloc);
        context.AddMember("uniqueId", "1", alloc);
        rapidjson::Value auxData(rapidjson::kObjectType);
        auxData.AddMember("isDefault", true, alloc);
        auxData.AddMember("type", "default", alloc);
        auxData.AddMember(
            "frameId",
            rapidjson::Value(s->frameId.c_str(), s->frameId.size(), alloc),
            alloc);
        context.AddMember("auxData", auxData, alloc);
        params.AddMember("context", context, alloc);
        CDPCommand evt(m_dispatcher, Optional<int64_t>(), cmd.sessionId(),
                       nullptr);
        evt.sendEvent("Runtime.executionContextCreated", params, doc);

        // Announce any already-present child iframe frames + their contexts.
        // (When connecting to an already-loaded page there is no navigate, so
        // finishNavigation's discovery never ran.)
        m_dispatcher->page()->discoverChildFrames(cmd.sessionId());
        return;
    }

    if (method == "disable") {
        s->runtimeEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "runIfWaitingForDebugger") {
        cmd.sendResultEmpty();
        return;
    }

    if (method == "evaluate") {
        if (!cmd.params() || !cmd.params()->HasMember("expression") ||
            !(*cmd.params())["expression"].IsString()) {
            cmd.sendError(-32602, "'expression' is required");
            return;
        }
        const char* expr = (*cmd.params())["expression"].GetString();
        bool returnByValue = false;
        if (cmd.params()->HasMember("returnByValue") &&
            (*cmd.params())["returnByValue"].IsBool()) {
            returnByValue = (*cmd.params())["returnByValue"].GetBool();
        }
        bool awaitPromise = false;
        if (cmd.params()->HasMember("awaitPromise") &&
            (*cmd.params())["awaitPromise"].IsBool()) {
            awaitPromise = (*cmd.params())["awaitPromise"].GetBool();
        }
        std::string source(expr);
        BrowsingContext* bc = targetBrowsingContext(wv, cmd, "contextId");
        evaluateSource(wv, bc, source, returnByValue, awaitPromise, cmd);
        // Page-script run synchronously here may have removed an iframe (e.g.
        // document.querySelector('iframe').remove()); reconcile the frame tree
        // so Page.frameDetached fires for any frame that just disappeared.
        m_dispatcher->page()->sweepDetachedFrames(cmd.sessionId());

        // page.setContent rewrite (bare-expression form).
        maybeNotifyDocumentRewrite(wv, source, cmd.sessionId());
        return;
    }

    if (method == "callFunctionOn") {
        std::string fnSource;
        if (cmd.params() && cmd.params()->HasMember("functionDeclaration") &&
            (*cmd.params())["functionDeclaration"].IsString()) {
            fnSource = (*cmd.params())["functionDeclaration"].GetString();
        }
        BrowsingContext* bc =
            targetBrowsingContext(wv, cmd, "executionContextId");
        callFunctionOn(wv, bc, cmd);
        m_dispatcher->page()->sweepDetachedFrames(cmd.sessionId());
        // page.setContent runs document.open/write/close as a function here.
        maybeNotifyDocumentRewrite(wv, fnSource, cmd.sessionId());
        return;
    }

    if (method == "compileScript") {
        BrowsingContext* bc =
            targetBrowsingContext(wv, cmd, "executionContextId");
        compileScript(wv, bc, cmd);
        return;
    }

    if (method == "runScript") {
        BrowsingContext* bc =
            targetBrowsingContext(wv, cmd, "executionContextId");
        runScript(wv, bc, cmd);
        return;
    }

    if (method == "getProperties") {
        // Object handles are global to the store; properties enumerate in the
        // main context (objectId already carries the live ObjectRef).
        getProperties(wv, wv->mainBrowsingContext(), cmd);
        return;
    }

    if (method == "getHeapUsage") {
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
        return;
    }

    if (method == "globalLexicalScopeNames") {
        globalLexicalScopeNames(wv, cmd);
        return;
    }

    if (method == "queryObjects") {
        // Escargot exposes no heap-instance iteration API, so the set of live
        // objects with a given prototype cannot be enumerated. Report not
        // implemented rather than silently returning an empty/incorrect set.
        cmd.sendError(-32000,
                      "Runtime.queryObjects is not supported by this engine");
        return;
    }

    if (method == "releaseObject") {
        if (cmd.params() && cmd.params()->HasMember("objectId") &&
            (*cmd.params())["objectId"].IsString()) {
            std::string oid = (*cmd.params())["objectId"].GetString();
            if (oid.rfind("OBJ-", 0) == 0) {
                int id = atoi(oid.c_str() + 4);
                m_dispatcher->remoteObjectStore()->release(id);
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "addBinding") {
        if (!cmd.params() || !cmd.params()->HasMember("name") ||
            !(*cmd.params())["name"].IsString()) {
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
        return;
    }

    if (method == "removeBinding") {
        if (cmd.params() && cmd.params()->HasMember("name") &&
            (*cmd.params())["name"].IsString()) {
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
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

// Heap descriptor stored on the injected window[name] function via
// setExtraData, so the native callback knows which binding name fired. Plain
// heap (the FunctionObjectRef holds it for the lifetime of the document;
// leaking one tiny record per (binding x document) is acceptable for the CDP
// MVP).
struct BindingExtra {
    std::string name;
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
    wv->cdpServer()->dispatcher()->emitBindingCalled(wv, extra->name, payload);
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

            BindingExtra* extra = new BindingExtra();
            extra->name = *d->name;

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

// If `value` is a Promise and awaitPromise is requested, drive the engine until
// it settles, then return the settled value (and set isRejected). Pending
// promises in Starfish may be gated on microtasks (drained via
// executePendingJob) and on observer callbacks driven by the rendering tick
// (e.g. IntersectionObserver, used by Puppeteer's isIntersectingViewport).
// Since the CDP command runs synchronously on the main thread while the message
// loop is blocked, we pump those steps here in a bounded loop. Non-promise
// values and pending-on-real-IO promises are returned as-is.
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
    // Still pending (e.g. waiting on a real timer/network): return the promise
    // object itself, matching prior behavior.
    return value;
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
    if (!cmd.params() || !cmd.params()->HasMember("functionDeclaration") ||
        !(*cmd.params())["functionDeclaration"].IsString()) {
        cmd.sendError(-32602, "'functionDeclaration' is required");
        return;
    }

    if (!bc || !bc->scriptBindingInstance() ||
        !bc->scriptBindingInstance()->isScriptingEnabledIgnoringCDP()) {
        cmd.sendError(-32000, "No scripting context");
        return;
    }
    ScriptBindingInstance* sbi = bc->scriptBindingInstance();
    ContextRef* ctx = sbi->scriptContext();
    RemoteObjectStore* store = m_dispatcher->remoteObjectStore();

    std::string fn = (*cmd.params())["functionDeclaration"].GetString();
    bool returnByValue = false;
    if (cmd.params()->HasMember("returnByValue") &&
        (*cmd.params())["returnByValue"].IsBool()) {
        returnByValue = (*cmd.params())["returnByValue"].GetBool();
    }
    bool awaitPromise = false;
    if (cmd.params()->HasMember("awaitPromise") &&
        (*cmd.params())["awaitPromise"].IsBool()) {
        awaitPromise = (*cmd.params())["awaitPromise"].GetBool();
    }

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
        rapidjson::Value ex(rapidjson::kObjectType);
        ex.AddMember("exceptionId", 1, alloc);
        ex.AddMember("text", rapidjson::Value(text.c_str(), text.size(), alloc),
                     alloc);
        ex.AddMember("lineNumber", 0, alloc);
        ex.AddMember("columnNumber", 0, alloc);
        rapidjson::Value exObj(rapidjson::kObjectType);
        serializeRemoteObject(sbi, store, errorValue, false, exObj, alloc);
        ex.AddMember("exception", exObj, alloc);
        result.AddMember("exceptionDetails", ex, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    bool settledRejected = false;
    ValueRef* settled = settlePromiseIfNeeded(wv, ctx, res.result, awaitPromise,
                                              settledRejected);

    if (settledRejected) {
        rapidjson::Value remote(rapidjson::kObjectType);
        serializeRemoteObject(sbi, store, settled, false, remote, alloc);
        result.AddMember("result", remote, alloc);
        std::string text =
            settled->toStringWithoutException(ctx)->toStdUTF8String();
        rapidjson::Value ex(rapidjson::kObjectType);
        ex.AddMember("exceptionId", 1, alloc);
        ex.AddMember("text", rapidjson::Value(text.c_str(), text.size(), alloc),
                     alloc);
        ex.AddMember("lineNumber", 0, alloc);
        ex.AddMember("columnNumber", 0, alloc);
        rapidjson::Value exObj(rapidjson::kObjectType);
        serializeRemoteObject(sbi, store, settled, false, exObj, alloc);
        ex.AddMember("exception", exObj, alloc);
        result.AddMember("exceptionDetails", ex, alloc);
        cmd.sendResult(result, doc);
        return;
    }

    rapidjson::Value remote(rapidjson::kObjectType);
    serializeRemoteObject(sbi, store, settled, returnByValue, remote, alloc);
    result.AddMember("result", remote, alloc);
    cmd.sendResult(result, doc);
}

void RuntimeDomain::compileScript(WebView* wv, BrowsingContext* bc,
                                  CDPCommand& cmd)
{
    if (!cmd.params() || !cmd.params()->HasMember("expression") ||
        !(*cmd.params())["expression"].IsString()) {
        cmd.sendError(-32602, "'expression' is required");
        return;
    }
    std::string expr = (*cmd.params())["expression"].GetString();
    bool persist = false;
    if (cmd.params()->HasMember("persistScript") &&
        (*cmd.params())["persistScript"].IsBool()) {
        persist = (*cmd.params())["persistScript"].GetBool();
    }
    std::string sourceURL;
    if (cmd.params()->HasMember("sourceURL") &&
        (*cmd.params())["sourceURL"].IsString()) {
        sourceURL = (*cmd.params())["sourceURL"].GetString();
    }

    if (!bc || !bc->scriptBindingInstance() ||
        !bc->scriptBindingInstance()->isScriptingEnabledIgnoringCDP()) {
        cmd.sendError(-32000, "No scripting context");
        return;
    }
    ContextRef* ctx = bc->scriptBindingInstance()->scriptContext();

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
        rapidjson::Value ex(rapidjson::kObjectType);
        ex.AddMember("exceptionId", 1, alloc);
        ex.AddMember("text", rapidjson::Value(msg.c_str(), msg.size(), alloc),
                     alloc);
        ex.AddMember("lineNumber", 0, alloc);
        ex.AddMember("columnNumber", 0, alloc);
        result.AddMember("exceptionDetails", ex, alloc);
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
        result.AddMember(
            "scriptId",
            rapidjson::Value(scriptId.c_str(), scriptId.size(), alloc), alloc);
    }
    cmd.sendResult(result, doc);
}

void RuntimeDomain::runScript(WebView* wv, BrowsingContext* bc, CDPCommand& cmd)
{
    if (!cmd.params() || !cmd.params()->HasMember("scriptId") ||
        !(*cmd.params())["scriptId"].IsString()) {
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

    bool returnByValue = false;
    if (cmd.params()->HasMember("returnByValue") &&
        (*cmd.params())["returnByValue"].IsBool()) {
        returnByValue = (*cmd.params())["returnByValue"].GetBool();
    }
    bool awaitPromise = false;
    if (cmd.params()->HasMember("awaitPromise") &&
        (*cmd.params())["awaitPromise"].IsBool()) {
        awaitPromise = (*cmd.params())["awaitPromise"].GetBool();
    }

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
    if (!cmd.params() || !cmd.params()->HasMember("objectId") ||
        !(*cmd.params())["objectId"].IsString()) {
        cmd.sendError(-32602, "'objectId' is required");
        return;
    }

    if (!bc || !bc->scriptBindingInstance() ||
        !bc->scriptBindingInstance()->isScriptingEnabledIgnoringCDP()) {
        cmd.sendError(-32000, "No scripting context");
        return;
    }
    ScriptBindingInstance* sbi = bc->scriptBindingInstance();
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
        prop.AddMember(
            "name", rapidjson::Value(names[i].c_str(), names[i].size(), alloc),
            alloc);
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
    if (!bc || !bc->scriptBindingInstance() ||
        !bc->scriptBindingInstance()->isScriptingEnabledIgnoringCDP()) {
        cmd.sendError(-32000, "No scripting context");
        return;
    }
    ContextRef* ctx = bc->scriptBindingInstance()->scriptContext();

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
            rapidjson::Value ex(rapidjson::kObjectType);
            ex.AddMember("exceptionId", 1, alloc);
            ex.AddMember("text",
                         rapidjson::Value(msg.c_str(), msg.size(), alloc),
                         alloc);
            ex.AddMember("lineNumber", 0, alloc);
            ex.AddMember("columnNumber", 0, alloc);
            result.AddMember("exceptionDetails", ex, alloc);
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
            rapidjson::Value ex(rapidjson::kObjectType);
            ex.AddMember("exceptionId", 1, alloc);
            ex.AddMember("text",
                         rapidjson::Value(text.c_str(), text.size(), alloc),
                         alloc);
            ex.AddMember("lineNumber", lineNumber, alloc);
            ex.AddMember("columnNumber", columnNumber, alloc);
            rapidjson::Value exObj(rapidjson::kObjectType);
            serializeRemoteObject(sbi, m_dispatcher->remoteObjectStore(),
                                  errorValue, false, exObj, alloc);
            ex.AddMember("exception", exObj, alloc);
            result.AddMember("exceptionDetails", ex, alloc);
            cmd.sendResult(result, doc);
            return;
        }

        bool settledRejected = false;
        ValueRef* settled = settlePromiseIfNeeded(
            wv, ctx, sbresult.result, awaitPromise, settledRejected);

        if (settledRejected) {
            rapidjson::Value remote(rapidjson::kObjectType);
            serializeRemoteObject(sbi, m_dispatcher->remoteObjectStore(),
                                  settled, false, remote, alloc);
            result.AddMember("result", remote, alloc);
            std::string text =
                settled->toStringWithoutException(ctx)->toStdUTF8String();
            rapidjson::Value ex(rapidjson::kObjectType);
            ex.AddMember("exceptionId", 1, alloc);
            ex.AddMember("text",
                         rapidjson::Value(text.c_str(), text.size(), alloc),
                         alloc);
            ex.AddMember("lineNumber", 0, alloc);
            ex.AddMember("columnNumber", 0, alloc);
            rapidjson::Value exObj(rapidjson::kObjectType);
            serializeRemoteObject(sbi, m_dispatcher->remoteObjectStore(),
                                  settled, false, exObj, alloc);
            ex.AddMember("exception", exObj, alloc);
            result.AddMember("exceptionDetails", ex, alloc);
            cmd.sendResult(result, doc);
            return;
        }

        rapidjson::Value remote(rapidjson::kObjectType);
        serializeRemoteObject(sbi, m_dispatcher->remoteObjectStore(), settled,
                              returnByValue, remote, alloc);
        result.AddMember("result", remote, alloc);
        cmd.sendResult(result, doc);
        return;
    }
}

} // namespace Starfish

#endif
