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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLScriptElement.h"
#include "core/dom/Text.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/fetch/RequestData.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/extra/MimeType.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/util/Cryptographic.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/parser/PreloadScanner.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/loader/ElementResourceClient.h"
#include "binding/ScriptWrappable.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptEngineInstance.h"

#include "rapidjson/document.h"

namespace Starfish {

#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
extern uint64_t g_profilingBaseTime;
#endif

static void buildScriptResourceRequest(HTMLScriptElement* element,
                                       ResourceURL* rurl, bool async,
                                       bool defer, bool module, bool fromParser,
                                       bool shouldResumeParsing,
                                       bool forceSync);

void* HTMLScriptElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLScriptElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLScriptElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLScriptElement, m_nonce));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLScriptElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

static bool isJavaScriptType(const char* type, size_t len)
{
    if (len == 0) {
        return true;
    }
#define ALLOW_TYPE(ctype)                                   \
    else if (len >= (sizeof(ctype) - 1) &&                  \
             memcmp(ctype, type, (sizeof(ctype) - 1)) == 0) \
    {                                                       \
        return true;                                        \
    }
    ALLOW_TYPE("text/javascript")
    ALLOW_TYPE("application/javascript")
    ALLOW_TYPE("application/x-javascript")
    ALLOW_TYPE("application/octet-stream")
    ALLOW_TYPE("application/ecmascript")
    ALLOW_TYPE("text/ecmascript")
    ALLOW_TYPE("text/plain")
    ALLOW_TYPE("text/html")
    return false;
}

class ScriptProfileLogger {
public:
    ScriptProfileLogger()
#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
        : m_timer("HTMLScriptElement script execution")
#endif
    {
#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
        STARFISH_LOG_INFO("[SCRIPT_PROFILING] Start JS Execution at %dms",
                          (int)(timestamp() - g_profilingBaseTime));
#endif
    }

    ~ScriptProfileLogger()
    {
#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
        STARFISH_LOG_INFO("[SCRIPT_PROFILING] End JS Execution at %dms",
                          (int)(timestamp() - g_profilingBaseTime));
#endif
    }

#if defined(STARFISH_ENABLE_SCRIPT_PROFILING)
    ProfilerTimer m_timer;
#endif
};

static ResourceURL* resolveModuleSrcFromImportMap(Document* document,
                                                  ResourceURL* src)
{
    auto resolvedURL = document->resolveModuleSrcFromImportMap(src->string());
    if (resolvedURL) {
        return resolvedURL.value();
    }
    return src;
}

class DeferredScriptDownloadClient : public ResourceClient {
public:
    static void requestDynamicImportedScriptOrItsSubScript(
        ExecutionContext* executionContext, ResourceURL* targetURL,
        Optional<Promise*> promise)
    {
        auto& moduleScripts = executionContext->document()->moduleScripts();
        moduleScripts.push_back(
            new Document::ScriptModuleData(nullptr, targetURL, nullptr, false));
        if (promise) {
            moduleScripts.back()->promiseForDynamicLoadedModule.push_back(
                promise.value());
        }

        Document* document = executionContext->document();
        TextResource* res = document->resourceLoader().fetchText(targetURL);
        res->addResourceClient(new DeferredScriptDownloadClient(res, document));

        RequestData* reqData = new RequestData();
        reqData->m_url = targetURL;
        reqData->m_referrer = new ReferrerURL(document->documentURI());
        reqData->m_destination = RequestDestination::Script;
        reqData->m_syncLevel = RequestSyncLevel::NeverSync;
        reqData->m_mode = RequestMode::NoCORS;
        res->request(reqData, true);
    }

    DeferredScriptDownloadClient(HTMLScriptElement* script, Resource* res,
                                 bool fromParser, bool module)
        : ResourceClient(res)
        , m_isModule(module)
        , m_isLoaded(false)
        , m_successToLoad(false)
        , m_fromParser(fromParser)
        , m_responseMIMEType(String::emptyString)
        , m_document(script->document())
        , m_element(script)
    {
        m_document->m_deferredScriptElements.push_back(
            std::make_pair(script, this));
    }

    DeferredScriptDownloadClient(Resource* res, Document* document)
        : ResourceClient(res)
        , m_isModule(true)
        , m_isLoaded(false)
        , m_successToLoad(false)
        , m_fromParser(false)
        , m_responseMIMEType(String::emptyString)
        , m_document(document)
        , m_element(nullptr)
    {
        m_document->m_pendingDynamicLoadedModules.push_back(
            std::make_pair(res->url(), this));
    }

    void removeThisClientFromDocumentList()
    {
        size_t pos = 0;
        if (m_element) {
            while (true) {
                if (m_document->m_deferredScriptElements[pos].second == this) {
                    break;
                }
                pos++;
            }

            m_document->m_deferredScriptElements.erase(pos);
        } else {
            while (true) {
                if (m_document->m_pendingDynamicLoadedModules[pos].second ==
                    this) {
                    break;
                }
                pos++;
            }

            m_document->m_pendingDynamicLoadedModules.erase(pos);
        }
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_isLoaded = true;
        didScriptLoaded();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        m_successToLoad = true;
        m_isLoaded = true;

        auto mimeType = MimeType::parseFromString(
            m_resource->resourceRequest()->responseMimeType());
        m_responseMIMEType = mimeType.stringWithoutParameter();

        if (m_element) {
            auto& deferredScriptElements = m_document->m_deferredScriptElements;
            while (deferredScriptElements.size() &&
                   deferredScriptElements.begin()->second->m_isLoaded) {
                auto client = deferredScriptElements.begin()->second;
                auto s = client->m_responseMIMEType->toASCIILower()
                             ->toUTF8NonGCString();
                bool isModule = client->m_isModule;
                if (isJavaScriptType(s.data(), s.length())) {
                    String* text = client->m_resource->asTextResource()->text();
                    if (isModule) {
                        Optional<ScriptModule> module = initModule(
                            m_document->window()->scriptBindingInstance(), text,
                            client->resource()->url()->urlString());
                        auto& moduleScripts = m_document->moduleScripts();
                        if (module) {
                            bool fromParser = false;
                            for (auto* ms : moduleScripts) {
                                if (ms->url && *ms->url.value() ==
                                                   *client->resource()->url()) {
                                    STARFISH_ASSERT(!ms->module.hasValue());
                                    ms->module = module;
                                    fromParser = ms->fromParser;
                                    break;
                                }
                            }

                            auto requests = moduleRequests(module.value());
                            for (size_t i = 0; i < requests.size(); i++) {
                                String* src = requests[i];
                                ResourceURL* rurl = new ResourceURL(
                                    src,
                                    client->resource()->url()->urlString());
                                buildScriptResourceRequest(
                                    m_element.value(), rurl, false, false, true,
                                    fromParser, false, false);
                            }
                        } else {
                            // parsing error
                            for (auto* ms : moduleScripts) {
                                if (ms->url && *ms->url.value() ==
                                                   *client->resource()->url()) {
                                    ms->hasLoadingError = true;
                                    break;
                                }
                            }
                        }
                    } else {
                        STARFISH_ASSERT(client->m_element.hasValue());
                        Document::CurrentScriptManager manager(
                            client->m_element->document(),
                            client->m_element.value());
                        ScriptProfileLogger logger;
                        evaluateString(
                            client->m_element->window()
                                ->scriptBindingInstance(),
                            text,
                            ResourceClient::resource()->url()->urlString());
                    }
                }
                deferredScriptElements.erase(deferredScriptElements.begin());
            }
        } else {
            // dynamic loaded script
            auto& pendingDynamicLoadedModules =
                m_document->m_pendingDynamicLoadedModules;
            while (pendingDynamicLoadedModules.size() &&
                   pendingDynamicLoadedModules.begin()->second->m_isLoaded) {
                auto client = pendingDynamicLoadedModules.begin()->second;
                auto s = client->m_responseMIMEType->toASCIILower()
                             ->toUTF8NonGCString();
                if (isJavaScriptType(s.data(), s.length())) {
                    String* text = client->m_resource->asTextResource()->text();
                    Optional<ScriptModule> module = initModule(
                        m_document->window()->scriptBindingInstance(), text,
                        client->resource()->url()->urlString());
                    auto& moduleScripts = m_document->moduleScripts();
                    if (module) {
                        for (auto* ms : moduleScripts) {
                            if (ms->url && *ms->url.value() ==
                                               *client->resource()->url()) {
                                STARFISH_ASSERT(!ms->module.hasValue());
                                ms->module = module;
                                break;
                            }
                        }

                        auto requests = moduleRequests(module.value());
                        for (size_t i = 0; i < requests.size(); i++) {
                            String* src = requests[i];
                            ResourceURL* rurl = new ResourceURL(
                                src, client->resource()->url()->urlString());
                            ResourceURL* targetURL =
                                resolveModuleSrcFromImportMap(m_document, rurl);

                            bool hasURL = false;
                            for (auto* ms : moduleScripts) {
                                if (ms->url && *ms->url.value() == *targetURL) {
                                    // we already have the module.
                                    hasURL = true;
                                    break;
                                }
                            }
                            if (!hasURL) {
                                requestDynamicImportedScriptOrItsSubScript(
                                    m_document->executionContext(), targetURL,
                                    nullptr);
                            }
                        }
                    } else {
                        // parsing error
                        for (auto* ms : moduleScripts) {
                            if (ms->url && *ms->url.value() ==
                                               *client->resource()->url()) {
                                ms->hasLoadingError = true;
                                break;
                            }
                        }
                    }
                }
                pendingDynamicLoadedModules.erase(
                    pendingDynamicLoadedModules.begin());
            }
        }
        didScriptLoaded();
    }

    void didScriptLoaded()
    {
        if (m_element) {
            m_element->m_didScriptExecuted = true;
        }
        if (!m_successToLoad) {
            removeThisClientFromDocumentList();

            auto& moduleScripts = m_document->moduleScripts();
            for (auto* ms : moduleScripts) {
                if (ms->url && *ms->url.value() == *resource()->url()) {
                    ms->hasLoadingError = true;

                    // dynamic loaded script
                    if (!m_element) {
                        for (auto* promise :
                             ms->promiseForDynamicLoadedModule) {
                            notifyDynamicLoadedModuleError(
                                m_document->scriptBindingInstance(), promise);
                        }
                        ms->promiseForDynamicLoadedModule.clear();
                    }

                    break;
                }
            }
        }

        if (m_document->documentBuilder() == nullptr) {
            size_t fromParserCount = 0;
            for (const auto& e : m_document->m_deferredScriptElements) {
                if (e.second->m_fromParser) {
                    fromParserCount++;
                }
            }
            if (fromParserCount == 0) {
                m_document->notifyDomContentLoaded();
            }
        }
    }

    bool m_isModule;
    bool m_isLoaded;
    bool m_successToLoad;
    bool m_fromParser;
    String* m_responseMIMEType;
    Document* m_document;
    Optional<HTMLScriptElement*> m_element;
};

class ScriptDownloadClient : public ResourceClient {
public:
    ScriptDownloadClient(HTMLScriptElement* script, Resource* res,
                         bool shouldResumeParsing)
        : ResourceClient(res)
        , m_element(script)
        , m_shouldResumeParsing(shouldResumeParsing)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        didScriptLoaded();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        auto mimeType =
            MimeType::parseFromString(m_resource->responseMimeType());
        auto s = mimeType.stringWithoutParameter()
                     ->toASCIILower()
                     ->toUTF8NonGCString();
        if (isJavaScriptType(s.data(), s.length())) {
            String* text = m_resource->asTextResource()->text();
            Document::CurrentScriptManager currentScriptManager(
                m_element->document(), m_element);
            ScriptProfileLogger logger;
            evaluateString(m_element->window()->scriptBindingInstance(), text,
                           ResourceClient::resource()->url()->urlString());
        }
        didScriptLoaded();
    }

    void didScriptLoaded()
    {
        m_element->m_didScriptExecuted = true;
        if (m_shouldResumeParsing) {
            m_element->document()->resumeDocumentParsing();
        }
    }

protected:
    HTMLScriptElement* m_element;
    bool m_shouldResumeParsing;
};

static bool checkSrcSecurity(HTMLScriptElement* element, ResourceURL* rurl)
{
    if (!element->document()->contentSecurityPolicy()->allowNonceOrSource(
            CSPDirectives::ScriptSrc, element->nonce(), rurl)) {
        String* eventType =
            element->starfish()->staticStrings()->m_error.localName();
        Event* e = new Event(element->executionContext(), eventType,
                             EventInit(false, false));
        element->dispatchEventIdleTimeByUA(e);
        return false;
    }

    return true;
}

static void buildScriptResourceRequest(HTMLScriptElement* element,
                                       ResourceURL* rurl, bool async,
                                       bool defer, bool module, bool fromParser,
                                       bool shouldResumeParsing, bool forceSync)
{
    ResourceURL* targetURL = rurl;
    if (module) {
        auto& moduleScripts = element->document()->moduleScripts();
        for (auto* ms : moduleScripts) {
            if (ms->url && *ms->url.value() == *rurl) {
                // we already have the module.
                return;
            }
        }

        targetURL = resolveModuleSrcFromImportMap(element->document(), rurl);
        moduleScripts.push_back(new Document::ScriptModuleData(
            nullptr, targetURL, element, fromParser));
    }

    String* charset = element
                          ->getAttributeOrEmpty(
                              element->starfish()->staticStrings()->m_charset)
                          ->trim();
    TextResource* res =
        element->document()->resourceLoader().fetchText(targetURL, charset);
    if (module || (!async && defer)) {
        res->addResourceClient(
            new DeferredScriptDownloadClient(element, res, fromParser, module));
    } else {
        res->addResourceClient(
            new ScriptDownloadClient(element, res, shouldResumeParsing));
    }

    // load, error event of module is dispatched by another place
    if (!module) {
        res->addResourceClient(new ElementResourceClient(element, res, true));
    }

    RequestData* reqData = new RequestData();
    reqData->m_url = targetURL;
    reqData->m_referrer = new ReferrerURL(element->document()->documentURI());
    reqData->m_destination = RequestDestination::Script;
    reqData->m_syncLevel =
        forceSync ? RequestSyncLevel::AlwaysSync : RequestSyncLevel::NeverSync;

    // https://html.spec.whatwg.org/multipage/urls-and-fetching.html#cors-settings-attributes
    auto crossOrigin = element->getAttribute(
        element->starfish()->staticStrings()->m_crossorigin);
    if (crossOrigin.hasValue()) {
        reqData->m_mode = RequestMode::CORS;
        reqData->m_credentials =
            crossOrigin.getValue()->equalsIgnoreCase("use-credentials")
                ? RequestCredentials::Include
                : RequestCredentials::SameOrigin;
    } else {
        reqData->m_mode = RequestMode::NoCORS;
    }

    res->request(reqData, true);
}

bool HTMLScriptElement::executeScript(bool forceSync, bool inParser)
{
    bool result = executeScriptImpl(forceSync, inParser);
    return result;
}

template <typename Encoding>
struct JSONStringReadonlyStream {
    typedef typename Encoding::Ch Ch;

    JSONStringReadonlyStream(const Ch* src, size_t length)
        : src_(src)
        , head_(src)
        , tail_(src + length)
    {
    }

    Ch Peek() const
    {
        if (UNLIKELY(tail_ <= src_)) {
            return 0;
        }
        return *src_;
    }
    Ch Take()
    {
        if (UNLIKELY(tail_ <= src_)) {
            return 0;
        }
        return *src_++;
    }
    size_t Tell() const
    {
        return static_cast<size_t>(src_ - head_);
    }
    Ch* PutBegin()
    {
        RAPIDJSON_ASSERT(false);
        return 0;
    }
    void Put(Ch)
    {
        RAPIDJSON_ASSERT(false);
    }
    void Flush()
    {
        RAPIDJSON_ASSERT(false);
    }
    size_t PutEnd(Ch*)
    {
        RAPIDJSON_ASSERT(false);
        return 0;
    }

    const Ch* src_;  //!< Current read position.
    const Ch* head_; //!< Original head of the string.
    const Ch* tail_;
};

static bool processImportMap(Document* document, String* txt)
{
    auto scriptBindingInstance = document->scriptBindingInstance();
    rapidjson::Document jsonDocument;
    txt->peekUTF8Buffer(
        [](const char* ptr, size_t len, void* data) -> size_t {
            rapidjson::Document* document =
                reinterpret_cast<rapidjson::Document*>(data);
            JSONStringReadonlyStream<rapidjson::UTF8<char>> stringStream(ptr,
                                                                         len);
            document->ParseStream(stringStream);
            return 0;
        },
        &jsonDocument);

    if (jsonDocument.HasParseError()) {
        return false;
    }

    auto iter = jsonDocument.MemberBegin();
    while (iter != jsonDocument.MemberEnd()) {
        std::string str(iter->name.GetString(), iter->name.GetStringLength());
        if (str == "imports") {
            auto& obj = iter->value;
            if (!obj.IsObject()) {
                return false;
            }

            auto subIter = obj.MemberBegin();
            while (subIter != obj.MemberEnd()) {
                if (subIter->value.IsString()) {
                    String* s =
                        String::fromUTF8(subIter->name.GetString(),
                                         subIter->name.GetStringLength());
                    if (s->startsWith("./")) {
                        s = s->substring(2, s->length() - 2);
                    }

                    document->importMap().push_back(new Document::ImportMapData(
                        String::fromUTF8(subIter->name.GetString(),
                                         subIter->name.GetStringLength()),
                        new ResourceURL(
                            String::fromUTF8(subIter->value.GetString(),
                                             subIter->value.GetStringLength()),
                            document->baseURL()->baseURI())));
                }
                subIter++;
            }

            break;
        } else if (str == "imports" || str == "integrity") {
            STARFISH_UNIMPLEMENTED(
                "HTMLScriptElement importmap (scopes, integrity)");
        }
        iter++;
    }

    return true;
}

bool HTMLScriptElement::executeScriptImpl(bool forceSync, bool inParser)
{
    if (m_isParserInserted) {
        return false;
    }

    scriptBindingInstance()->engineInstance()->drainMicroTaskQueue();

    if (!m_isAlreadyStarted &&
        isInDocumentScopeAndDocumentParticipateInRendering()) {
        if (!isValidScriptType()) {
            return false;
        }

        if (!isEventForSupported()) {
            return false;
        }

        if (blockForNoModule()) {
            return false;
        }

        Optional<String*> srcStr =
            getAttribute(starfish()->staticStrings()->m_src);
        if (!srcStr.hasValue()) {
            if (!firstChild()) {
                return false;
            }
            String* script = text();

            if (script->length() > 0 &&
                !document()->contentSecurityPolicy()->allowInline(
                    CSPDirectives::ScriptSrc, script, nonce())) {
                String* eventType =
                    starfish()->staticStrings()->m_error.localName();
                Event* e = new Event(executionContext(), eventType,
                                     EventInit(false, false));
                dispatchEventIdleTimeByUA(e);
                return false;
            }

            m_isAlreadyStarted = true;
            if (isModule()) {
                Optional<ScriptModule> module =
                    initModule(window()->scriptBindingInstance(), script);
                if (module) {
                    document()->moduleScripts().push_back(
                        new Document::ScriptModuleData(module.value(), nullptr,
                                                       this, inParser));

                    auto requests = moduleRequests(module.value());
                    for (size_t i = 0; i < requests.size(); i++) {
                        String* src = requests[i];
                        ResourceURL* rurl = new ResourceURL(
                            src, document()->baseURL()->baseURI());
                        if (!checkSrcSecurity(this, rurl)) {
                            continue;
                        }

                        buildScriptResourceRequest(this, rurl, false, false,
                                                   true, inParser, false,
                                                   false);
                    }
                }
            } else if (isImportMap()) {
                if (!processImportMap(document(), script)) {
                    String* eventType =
                        starfish()->staticStrings()->m_error.localName();
                    Event* e = new Event(executionContext(), eventType,
                                         EventInit(false, false));
                    dispatchEventIdleTimeByUA(e);
                    return false;
                }
            } else {
                Document::CurrentScriptManager currentScriptManager(document(),
                                                                    this);
                ScriptProfileLogger logger;
                evaluateString(window()->scriptBindingInstance(), script);
                scriptBindingInstance()
                    ->engineInstance()
                    ->drainMicroTaskQueue();
                m_didScriptExecuted = true;
            }
            return false;
        } else {
            String* url = srcStr.getValue();
            m_isAlreadyStarted = true;

            if (!url->length()) {
                return false;
            }

            ResourceURL* rurl =
                new ResourceURL(url, document()->baseURL()->baseURI());
            if (!checkSrcSecurity(this, rurl)) {
                return false;
            }

            if (isImportMap()) {
                return false;
            }

            if (document()->preloadScanner() && !async() && !defer() &&
                !isModule() && inParser) {
                auto ps = document()->preloadScanner();
                for (size_t i = 0; i < ps->preloadedJS().size(); i++) {
                    Resource* res = ps->preloadedJS()[i];
                    if (*res->url() == *rurl) {
                        setShouldResumeParsing(!forceSync);
                        if (res->isReceiving()) {
                            res->addResourceClient(new ScriptDownloadClient(
                                this, res, shouldResumeParsing()));
                            res->addResourceClient(
                                new ElementResourceClient(this, res, true));
                            return true;
                        } else if (res->isFinished()) {
                            webView()->messageLoop()->addIdler(
                                window(),
                                [](size_t id, void* res, void* self) {
                                    HTMLScriptElement* scriptElement =
                                        (HTMLScriptElement*)self;
                                    ScriptDownloadClient download(
                                        scriptElement, (Resource*)res,
                                        scriptElement->shouldResumeParsing());
                                    download.didLoadFinished();
                                    ElementResourceClient onload(
                                        (HTMLScriptElement*)self,
                                        (Resource*)res, true);
                                    onload.didLoadFinished();
                                },
                                res, this);
                            return true;
                        }
                        break;
                    }
                }
            }

            bool treatAsDefer = defer() || isModule();
            if (async() || !treatAsDefer) {
                setShouldResumeParsing(inParser && !forceSync && !async());
            }
            buildScriptResourceRequest(this, rurl, async(), defer(), isModule(),
                                       inParser, shouldResumeParsing(),
                                       forceSync);

            if (async() || treatAsDefer) {
                return false;
            } else {
                return true;
            }
        }
    }
    return false;
}

void HTMLScriptElement::didAttributeChanged(QualifiedName name,
                                            Optional<String*> old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starfish()->staticStrings()->m_src) {
        executeScript();
    } else if (name == starfish()->staticStrings()->m_crossorigin) {
        if (attributeRemoved) {
            removeAttribute(value);
        } else if (attributeCreated || !old->equalsIgnoreCase(value)) {
            if (value->equalsIgnoreCase("use-credentials")) {
                setAttribute(starfish()->staticStrings()->m_crossorigin, value);
            } else {
                setAttribute(starfish()->staticStrings()->m_crossorigin,
                             String::fromUTF8("anonymous"));
            }
        }
    } else if (name == starfish()->staticStrings()->m_nonce) {
        if (attributeRemoved) {
            removeAttribute(value);
            m_nonce = nullptr;
        } else if (attributeCreated || !old->equals(value)) {
            m_nonce = value;
        }
    }
}

void HTMLScriptElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    executeScript();
}

void HTMLScriptElement::didCharacterDataModified(String* before, String* after)
{
    HTMLElement::didCharacterDataModified(before, after);
    executeScript();
}

void HTMLScriptElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    executeScript();
}

String* HTMLScriptElement::src()
{
    String* url = getAttributeOrEmpty(starfish()->staticStrings()->m_src);
    if (url->equals(String::emptyString)) {
        return String::emptyString;
    }

    return ResourceURL::mergeDocumentURIWithURIString(
        document()->baseURL()->baseURI(), url);
}

void HTMLScriptElement::setSrc(String* src)
{
    setAttribute(starfish()->staticStrings()->m_src, src);
}

String* HTMLScriptElement::nonce() const
{
    if (m_nonce) {
        return m_nonce.value();
    }
    return getAttributeOrEmpty(starfish()->staticStrings()->m_nonce);
}

void HTMLScriptElement::setNonce(String* str)
{
    // In order to mitigate nonce exfiltration via content attributes,
    // we hide the nonce from the element’s content attribute and move it into
    // an internal slot.
    // https://w3c.github.io/webappsec-csp/#nonce-exfiltration-content-attributes
    m_nonce = str;
}

String* HTMLScriptElement::type()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_type);
}

void HTMLScriptElement::setType(String* type)
{
    setAttribute(starfish()->staticStrings()->m_type, type);
}

String* HTMLScriptElement::charset()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_charset);
}

void HTMLScriptElement::setCharset(String* charset)
{
    setAttribute(starfish()->staticStrings()->m_charset, charset);
}

Optional<String*> HTMLScriptElement::crossOrigin()
{
    return getAttribute(starfish()->staticStrings()->m_crossorigin);
}

void HTMLScriptElement::setCrossOrigin(Optional<String*> crossOrigin)
{
    if (crossOrigin.hasValue()) {
        setAttribute(starfish()->staticStrings()->m_crossorigin,
                     crossOrigin.getValue());
    } else {
        removeAttribute(starfish()->staticStrings()->m_crossorigin);
    }
}

String* HTMLScriptElement::text()
{
    String* str = String::emptyString;
    for (Node* child = firstChild(); child != nullptr;
         child = child->nextSibling()) {
        if (child->nodeType() == TEXT_NODE) {
            STARFISH_ASSERT(child->textContent().hasValue());
            str = str->concat(child->textContent().getValue());
        }
    }
    return str;
}

void HTMLScriptElement::setText(String* s)
{
    if (firstChild() && firstChild()->isText()) {
        firstChild()->asText()->setData(s);
    } else {
        setTextContent(s);
    }
}

bool HTMLScriptElement::async()
{
    return hasAttribute(starfish()->staticStrings()->m_async) != SIZE_MAX;
}

void HTMLScriptElement::setAsync(bool b)
{
    if (b) {
        setAttribute(starfish()->staticStrings()->m_async, String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_async);
    }
}

bool HTMLScriptElement::defer()
{
    return hasAttribute(starfish()->staticStrings()->m_defer) != SIZE_MAX;
}

void HTMLScriptElement::setDefer(bool b)
{
    if (b) {
        setAttribute(starfish()->staticStrings()->m_defer, String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_defer);
    }
}

bool HTMLScriptElement::isModule()
{
    return type()->equalsIgnoreCase("module");
}

bool HTMLScriptElement::isImportMap()
{
    return type()->equalsIgnoreCase("importmap");
}

Node* HTMLScriptElement::clone()
{
    HTMLScriptElement* n = HTMLElement::clone()->asHTMLScriptElement();
    n->m_isAlreadyStarted = m_isAlreadyStarted;
    n->m_didScriptExecuted = m_didScriptExecuted;
    return n;
}

bool HTMLScriptElement::isValidScriptType()
{
    if (isValidClassicScriptType()) {
        return true;
    }

    if (isModule()) {
        return true;
    }

    if (isImportMap()) {
        return true;
    }

    return false;
}

bool HTMLScriptElement::isValidClassicScriptType()
{
    Optional<String*> typeStr =
        getAttribute(starfish()->staticStrings()->m_type);
    if (typeStr.hasValue()) {
        auto utf8Data = typeStr.getValue()->toASCIILower()->toUTF8NonGCString();

        auto mime = MimeType::parseFromString(typeStr.getValue());
        if (!mime.isValid() || mime.hasParameter()) {
            return false;
        }

        if (!isJavaScriptType(utf8Data.data(), utf8Data.length())) {
            return false;
        }
    }
    return true;
}

bool HTMLScriptElement::isEventForSupported()
{
    String* event = getAttributeOrEmpty(starfish()->staticStrings()->m_event);
    String* htmlFor = getAttributeOrEmpty(starfish()->staticStrings()->m_for);

    if (!isValidClassicScriptType() || event->isEmpty() || htmlFor->isEmpty()) {
        return true;
    }

    event = event->trim();
    if (!event->equalsIgnoreCase("onload") &&
        !event->equalsIgnoreCase("onload()")) {
        return false;
    }

    htmlFor = htmlFor->trim();
    if (!htmlFor->equalsIgnoreCase("window")) {
        return false;
    }
    return true;
}

bool HTMLScriptElement::blockForNoModule()
{
    return isValidClassicScriptType() &&
           hasAttribute(starfish()->staticStrings()->m_nomodule) != SIZE_MAX;
}

void HTMLScriptElement::requestDynamicImportedModule(
    ExecutionContext* executionContext, ResourceURL* targetURL,
    Promise* promise)
{
    auto& moduleScripts = executionContext->document()->moduleScripts();
    for (auto* ms : moduleScripts) {
        if (ms->url && *ms->url.value() == *targetURL) {
            // we already have the module.
            ms->promiseForDynamicLoadedModule.push_back(promise);
            return;
        }
    }

    DeferredScriptDownloadClient::requestDynamicImportedScriptOrItsSubScript(
        executionContext, targetURL, promise);
}
} // namespace Starfish
