/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_WEBWORKER_HOST)
#pragma once

#include "binding/ScriptWrappable.h"
#include "core/dom/EventTarget.h"
#include "core/fetch/Request.h"

namespace Escargot {

class ObjectRef;

} // namespace Escargot

namespace Starfish {

class DOMException;
class FetchCacheStream;
class Promise;
class Request;
class ResourceURL;
class Response;
class String;
class ExecutionContext;
class GlobalScope;

class Internal : public ScriptWrappable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }
    bool isInternal() const;

    Internal(GlobalScope* globaslScope,
             ScriptBindingInstance* scriptBindingInstance);

    // Cache interfaces
    FetchCacheStream* fetchCacheStream()
    {
        return m_fetchCacheStream;
    }

    Promise* open(String* cacheName); // binding interface
    Promise* put(Escargot::ObjectRef* fetchCacheStreamWrap, Request* request,
                 Response* response); // binding interface
    Promise* cache_storage_keys();    // binding interface
    Promise* matchAll(ExecutionContext* executionContext,
                      Escargot::ObjectRef* fetchCacheStreamWrap,
                      RequestInfo& request); // binding interface

    void setUrl(ResourceURL* url)
    {
        m_url = url;
    }

    GlobalScope* globalScope()
    {
        return m_globalScope;
    }

private:
    Internal();
    ResourceURL* m_url{ nullptr };
    FetchCacheStream* m_fetchCacheStream{ nullptr };
    GlobalScope* m_globalScope{ nullptr };
    ScriptBindingInstance* m_scriptBindingInstance{ nullptr };
};

} // namespace Starfish

#endif
