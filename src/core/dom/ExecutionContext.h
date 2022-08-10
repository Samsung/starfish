/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishExecutionContext__
#define __StarfishExecutionContext__

namespace Starfish {

class GlobalScope;
class ScriptBindingInstance;
class WebBase;
class ServiceWorker;
class ResourceURL;
class String;
class Document;
class WebOrigin;
class ResourceRequest;
class ContentSecurityPolicy;
class Event;
class WebSocket;

class ExecutionContext : public gc {
public:
    ExecutionContext(GlobalScope* globalScope, ScriptBindingInstance* instance,
                     ResourceURL* uri, String* charSet,
                     void* documentOrWorkerGlobalScope, bool hasDocument);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    bool hasDocument() const
    {
        return m_hasDocument;
    }
    bool hasWorkerGlobalScope() const
    {
        return !m_hasDocument;
    }

    Document* document();

    GlobalScope* globalScope() const
    {
        return m_globalScope;
    }

    ScriptBindingInstance* scriptBindingInstance() const
    {
        return m_scriptBindingInstance;
    }

    uint64_t createdTick()
    {
        return m_createdTick;
    }

    Starfish* starfish() const;
    WebBase* webBase() const;

    ResourceURL* documentURI() const
    {
        return m_documentURI;
    }
    void setDocumentURI(ResourceURL* newURL)
    {
        m_documentURI = newURL;
    }
    String* urlString();

    String* referrer();
    DEFINE_SETTER(ResourceURL*, referrer, Referrer)

    ResourceURL* baseURL() const;
    DEFINE_SETTER(ResourceURL*, baseURL, BaseURL)

    DEFINE_GETTER_SETTER(String*, characterSet, CharacterSet)
    DEFINE_GETTER_SETTER(WebOrigin*, webOrigin, WebOrigin)

    void addActiveResourceRequests(ResourceRequest* request);
    void removeActiveResourceRequests(ResourceRequest* request);
    void disposeActiveResourceRequests();
#ifdef STARFISH_ENABLE_WEBSOCKET
    void addActiveWebSockets(WebSocket* webSocket);
    void removeActiveWebSockets(WebSocket* webSocket);
    void disposeActiveWebSockets();
#endif
    void addPointerInRootSet(void* ptr);
    void removePointerFromRootSet(void* ptr);
    void clearPointerRootMap();
#ifndef NDEBUG
    size_t countPointersInRootSet(void* ptr);
#endif

    DEFINE_GETTER_SETTER(ContentSecurityPolicy*, contentSecurityPolicy,
                         ContentSecurityPolicy)

    void initContentSecurityPolicy(ContentSecurityPolicy* inheritedPolicy);

    void dispatchEventIdleTimeByUA(Event* event);

private:
    GlobalScope* const m_globalScope;
    void* m_documentOrWorkerGlobalScope;
    bool m_hasDocument;
    ScriptBindingInstance* const m_scriptBindingInstance;
    uint64_t m_createdTick;
    ResourceURL* m_documentURI;
    ResourceURL* m_referrer;
    ResourceURL* m_baseURL;
    GCVector<ResourceRequest*> m_activeResourceRequests;
#ifdef STARFISH_ENABLE_WEBSOCKET
    GCVector<WebSocket*> m_activeWebSockets;
#endif
    GCUnorderedMap<void*, size_t> m_rootMap;
    String* m_characterSet;
    WebOrigin* m_webOrigin;
    ContentSecurityPolicy* m_contentSecurityPolicy;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_globalScope));
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext,
                                        m_documentOrWorkerGlobalScope));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(ExecutionContext, m_scriptBindingInstance));
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_documentURI));
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_referrer));
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_baseURL));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(ExecutionContext, m_activeResourceRequests));
#ifdef STARFISH_ENABLE_WEBSOCKET
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_activeWebSockets));
#endif
        // we should mark every word of m_rootMap
        // because, we don't know that
        // where pointer of std:pair<Key,Value>* is located in GCUnodrderedMap
        // it depends on how std::unordered_map is implementated
        for (size_t i = 0; i < sizeof(m_rootMap); i += sizeof(size_t)) {
            GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_rootMap) +
                                 (i / sizeof(size_t)));
        }
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_characterSet));
        GC_set_bit(desc, GC_WORD_OFFSET(ExecutionContext, m_webOrigin));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(ExecutionContext, m_contentSecurityPolicy));
    }

#ifdef STARFISH_ENABLE_SERVICE_WORKER
public:
    NULLABLE ServiceWorker* activeServiceWorker() const;
    void setActiveServiceWorker(ServiceWorker* serviceWorker);

private:
    // TODO: We assume there is only one service worker per execution context.
    // We should use a map if multiple sw needs to be supported.
    // e.g) GCUnorderedMap<std::string, ServiceWorker*> m_mapServiceWorker;
    // map<scriptURL, ServiceWorker*>
    ServiceWorker* m_activeServiceWorker{ nullptr };
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
};
} // namespace Starfish

#endif
