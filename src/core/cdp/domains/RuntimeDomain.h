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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPRuntimeDomain__)
#define __StarfishCDPRuntimeDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;
class WebView;
class BrowsingContext;

class RuntimeDomain {
public:
    RuntimeDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

    // Inject a window[name] native function into the main world that, when
    // called from page JS, emits Runtime.bindingCalled with the first argument
    // (coerced to string) as payload. Used by Runtime.addBinding and re-invoked
    // after navigation (Page.navigate) to re-inject all registered bindings.
    void injectBinding(WebView* wv, const std::string& name);

private:
    // Resolve the optional "contextId" / "executionContextId" param to the
    // BrowsingContext it targets. A child iframe contextId resolves to that
    // iframe's BrowsingContext (its own Escargot context); anything else (no
    // id, main world, or isolated world) resolves to the main BrowsingContext.
    BrowsingContext* targetBrowsingContext(WebView* wv, CDPCommand& cmd,
                                           const char* key);

    // Parse + execute a source expression in `bc`'s script context and reply
    // with a Runtime RemoteObject result (or exceptionDetails). Shared by
    // Runtime.evaluate and Runtime.callFunctionOn.
    void evaluateSource(WebView* wv, BrowsingContext* bc,
                        const std::string& exprStr, bool returnByValue,
                        bool awaitPromise, CDPCommand& cmd);

    // Runtime.callFunctionOn: invoke a stringified function with a live
    // receiver (objectId -> handle, else globalThis) and value/objectId args.
    void callFunctionOn(WebView* wv, BrowsingContext* bc, CDPCommand& cmd);

    // Runtime.compileScript: parse `expression` in `bc`'s context to detect
    // syntax errors. On success, if persistScript is set, store the source as
    // "script-N" (see CDPSession::compiledScripts) and reply with scriptId; on
    // a parse error reply with exceptionDetails.
    void compileScript(WebView* wv, BrowsingContext* bc, CDPCommand& cmd);

    // Runtime.runScript: look up a persisted scriptId and evaluate its stored
    // source via evaluateSource (same result serialization as evaluate).
    void runScript(WebView* wv, BrowsingContext* bc, CDPCommand& cmd);

    // Runtime.getProperties: enumerate own properties of a handled object.
    void getProperties(WebView* wv, BrowsingContext* bc, CDPCommand& cmd);

    // Runtime.globalLexicalScopeNames: approximated by enumerating globalThis
    // own property names (covers global var/function; misses let/const/class,
    // which Escargot's public API does not expose).
    void globalLexicalScopeNames(WebView* wv, CDPCommand& cmd);

    // puppeteer page.setContent() rewrites the document by evaluating
    // document.open()/write()/close() (as a function via callFunctionOn, or as
    // a bare expression via evaluate). The engine applies it synchronously but
    // it is not a navigation, so on its own puppeteer's LifecycleWatcher would
    // hang waiting for a fresh loader + "load". When `source` performs an
    // explicit document.open() and the resulting main document is
    // script-created, drive the post-rewrite Page lifecycle
    // (PageDomain::notifyDocumentRewritten) so setContent resolves and later
    // page.$ / evaluate run against the new document. No-op otherwise.
    void maybeNotifyDocumentRewrite(WebView* wv, const std::string& source,
                                    const std::string& sessionId);

    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
