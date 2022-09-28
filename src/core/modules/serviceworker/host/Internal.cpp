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

#include "StarfishConfig.h"
#include "Internal.h"
#include "core/modules/serviceworker/util/ParallelTask.h"
#include "core/modules/serviceworker/util/Trace.h"
#include "core/modules/serviceworker/FetchCacheStream.h"
#include "core/fetch/Fetch.h"

namespace Starfish {

// TODO: Use ParallelTask. Enabling GC allocation on multiple threads.
class CacheTask : public IdleTask {
public:
    CacheTask(Internal* i, Promise* p)
        : m_internal(i)
        , m_promise(p)
    {
    }

    void end() override
    {
        TRACE_SCOPE(INTERNAL, "result", m_result);
        if (m_result) {
            m_promise->fulfill(scriptUndefined());
        } else {
            m_promise->reject(scriptUndefined());
        }
    }

    Promise* promise()
    {
        return m_promise;
    }

protected:
    bool m_result{ false };
    Internal* m_internal;
    Promise* m_promise;
};

Internal::Internal(ScriptBindingInstance* scriptBindingInstance)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(scriptBindingInstance)
{
    m_fetchCacheStream = new FetchCacheStream();
}

Promise* Internal::open(String* cacheName)
{
    class CacheOpenTask : public CacheTask {
    public:
        CacheOpenTask(Internal* i, Promise* p)
            : CacheTask(i, p)
        {
        }

        void run() override
        {
            STARFISH_ASSERT(m_url != nullptr);
            STARFISH_ASSERT(m_cacheName != nullptr);
            m_result = m_internal->fetchCacheStream()->open(
                m_url->urlString()->hashValue(), CSTR(m_cacheName));
        }
        ResourceURL* m_url{ nullptr };
        String* m_cacheName{ nullptr };
    };

    TRACE_SCOPE(INTERNAL, CSTR(cacheName));

    auto promise = new Promise(m_scriptBindingInstance);
    auto task = new CacheOpenTask(this, promise);
    task->m_url = m_url;
    task->m_cacheName = cacheName;
    task->start();

    return promise;
}

Promise* Internal::put(Request* request, Response* response)
{
    class CachePutTask : public CacheTask {
    public:
        CachePutTask(Internal* i, Promise* p)
            : CacheTask(i, p)
        {
        }

        void run() override
        {
            STARFISH_ASSERT(m_request != nullptr);
            STARFISH_ASSERT(m_response != nullptr);
            m_result = m_internal->fetchCacheStream()->writeResponse(
                m_request, m_response);
        }
        Request* m_request{ nullptr };
        Response* m_response{ nullptr };
    };

    TRACE_SCOPE(INTERNAL, CSTR(request->url()));

    auto promise = new Promise(m_scriptBindingInstance);
    auto task = new CachePutTask(this, promise);
    task->m_request = request;
    task->m_response = response;
    task->start();

    return promise;
}

} // namespace Starfish

#endif
