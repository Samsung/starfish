/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

class ExecutionContext {
public:
    ExecutionContext(GlobalScope* globalScope, ScriptBindingInstance* instance,
                     ResourceURL* uri);

    virtual bool isDocument() const
    {
        return false;
    }
    virtual bool isWorkerGlobalScope() const
    {
        return false;
    }

    Document* asDocument();

    GlobalScope* globalScope() const
    {
        return m_globalScope;
    }

    ScriptBindingInstance* ownerScriptBindingInstance() const
    {
        return m_scriptBindingInstance;
    }

    uint64_t createdTick()
    {
        return m_createdTick;
    }

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
    virtual ResourceURL* baseURL() const = 0;
    virtual String* referrer() = 0;

private:
    GlobalScope* const m_globalScope;

protected:
    ScriptBindingInstance* const m_scriptBindingInstance;
    uint64_t m_createdTick;
    ResourceURL* m_documentURI;

#ifdef STARFISH_ENABLE_SERVICE_WORKER
public:
    ServiceWorker* activeServiceWorker() const;
    void setActiveServiceWorker(ServiceWorker* serviceWorker);

private:
    ServiceWorker* m_activeServiceWorker{ nullptr };
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
};
}

#endif
