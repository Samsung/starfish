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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "Internal.h"
#include "binding/ScriptBindingInstance.h"
#include "core/page/WebBase.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/WorkerSettings.h"
#include "core/modules/worker/WorkerManager.h"
#include "core/modules/serviceworker/util/ParallelTask.h"
#include "core/modules/serviceworker/FetchCacheStream.h"
#include "core/fetch/Fetch.h"
#include "core/dom/ExecutionContext.h"
#include "core/fetch/Response.h"

using namespace Escargot;

namespace Starfish {

// TODO: Use ParallelTask. Enabling GC allocation on multiple threads.
class CacheTask : public IdleTask {
public:
    CacheTask(Internal* i, Promise* p)
        : IdleTask(i->globalScope())
        , m_internal(i)
        , m_promise(p)
    {
        m_resultValue = ValueRef::createUndefined();
        m_taskId = ++CacheTask::s_taskId;
    }

    void end() override
    {
        TRACE_SCOPE(INTERNAL, taskId(), "result", m_result,
                    m_resultValue->isUndefined());

        if (m_result) {
            m_promise->fulfill(m_resultValue);
        } else {
            m_promise->reject(m_resultValue);
        }
    }

    Promise* promise()
    {
        return m_promise;
    }

    int taskId()
    {
        return m_taskId;
    }

protected:
    bool m_result{ false };
    ValueRef* m_resultValue{ nullptr };
    Internal* m_internal;
    Promise* m_promise;
    static int s_taskId;

private:
    int m_taskId{ 0 };
};

int CacheTask::s_taskId = 0;

class ObjectWrap {
public:
    template <class T>
    static inline T* Unwrap(ObjectRef* object)
    {
        STARFISH_ASSERT(object != nullptr);
        STARFISH_ASSERT(object->extraData() != nullptr);
        return reinterpret_cast<T*>(object->extraData());
    }

    static ObjectRef* Wrap(ContextRef* context, void* data)
    {
        const auto& r = Evaluator::execute(
            context, [](ExecutionStateRef* state) -> ValueRef* {
                return ObjectRef::create(state);
            });
        STARFISH_ASSERT(r.isSuccessful());
        ObjectRef* object = r.result->asObject();
        object->setExtraData(data);
        return object;
    }
};

Internal::Internal(GlobalScope* globalScope,
                   ScriptBindingInstance* scriptBindingInstance)
    : ScriptWrappable(this)
    , m_globalScope(globalScope)
    , m_scriptBindingInstance(scriptBindingInstance)
{
    auto starfish =
        fetchWebBase(scriptBindingInstance->scriptContext())->starfish();

    m_fetchCacheStream = new FetchCacheStream(
        starfish->workerManager()->workerSettings()->dataDirectoryPath());
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
            String* url = m_url->urlString();
            size_t hash = url->hashValue();
            TRACE(INTERNAL, taskId(), CSTR(url), hash, CSTR(m_cacheName));
            STARFISH_ASSERT(m_url != nullptr);
            STARFISH_ASSERT(m_cacheName != nullptr);
            STARFISH_ASSERT(m_context != nullptr);

            auto fetchCacheStream = new FetchCacheStream(m_localStorageRootDir);
            m_result = fetchCacheStream->open(m_url->urlString()->hashValue(),
                                              CSTR(m_cacheName));
            if (m_result) {
                m_resultValue = ObjectWrap::Wrap(m_context, fetchCacheStream);
            }
        }
        ResourceURL* m_url{ nullptr };
        String* m_cacheName{ nullptr };
        ContextRef* m_context{ nullptr };
        std::string m_localStorageRootDir;
    };

    auto promise = new Promise(m_scriptBindingInstance);
    auto task = new CacheOpenTask(this, promise);
    auto context = m_scriptBindingInstance->scriptContext();
    auto starfish = fetchWebBase(context)->starfish();

    task->m_url = m_url;
    task->m_cacheName = cacheName;
    task->m_context = context;
    task->m_localStorageRootDir =
        starfish->workerManager()->workerSettings()->dataDirectoryPath();
    task->start();

    return promise;
}

Promise* Internal::put(ObjectRef* fetchCacheStreamWrap, Request* request,
                       Response* response)
{
    class CachePutTask : public CacheTask {
    public:
        CachePutTask(Internal* i, Promise* p)
            : CacheTask(i, p)
        {
        }

        void run() override
        {
            TRACE(INTERNAL, taskId(), CSTR(m_request->url()));
            STARFISH_ASSERT(m_request != nullptr);
            STARFISH_ASSERT(m_response != nullptr);
            STARFISH_ASSERT(m_fetchCacheStream != nullptr);
            m_result = m_fetchCacheStream->writeResponse(m_request, m_response);
        }
        Request* m_request{ nullptr };
        Response* m_response{ nullptr };
        FetchCacheStream* m_fetchCacheStream{ nullptr };
    };

    auto promise = new Promise(m_scriptBindingInstance);
    auto task = new CachePutTask(this, promise);
    task->m_request = request;
    task->m_response = response;
    task->m_fetchCacheStream =
        ObjectWrap::Unwrap<FetchCacheStream>(fetchCacheStreamWrap);
    task->start();

    return promise;
}

Promise* Internal::matchAll(ExecutionContext* executionContext,
                            ObjectRef* fetchCacheStreamWrap,
                            RequestInfo& requestInfo)
{
    /*
        TODO: Support search options (ignoreSearch/Method/Vary)
    */
    class CacheMatchAllTask : public CacheTask {
    public:
        CacheMatchAllTask(Internal* i, Promise* p)
            : CacheTask(i, p)
        {
        }

        void run() override
        {
            TRACE(INTERNAL, taskId(), CSTR(m_url),
                  m_fetchCacheStream->cacheDirPath());
            STARFISH_ASSERT(m_url != nullptr);
            STARFISH_ASSERT(m_context != nullptr);
            STARFISH_ASSERT(m_response != nullptr);
            STARFISH_ASSERT(m_fetchCacheStream != nullptr);

            ValueVectorRef* elements = ValueVectorRef::create();

            if (m_fetchCacheStream->readResponse(m_url, m_response)) {
                TRACE(INTERNAL, taskId(), "Found");
                elements->pushBack(m_response->scriptValue());
            } else {
                TRACE(INTERNAL, taskId(), "Not found");
            }

            const auto& r = Evaluator::execute(
                m_context,
                [](ExecutionStateRef* state,
                   ValueVectorRef* elements) -> ValueRef* {
                    return ArrayObjectRef::create(state, elements);
                },
                elements);

            STARFISH_ASSERT(r.isSuccessful());
            m_result = true;
            m_resultValue = r.result->asArrayObject();
        }
        String* m_url{ nullptr };
        ContextRef* m_context{ nullptr };
        Response* m_response{ nullptr };
        FetchCacheStream* m_fetchCacheStream{ nullptr };
    };

    auto promise = new Promise(m_scriptBindingInstance);
    auto task = new CacheMatchAllTask(this, promise);
    task->m_context = m_scriptBindingInstance->scriptContext();

    task->m_fetchCacheStream =
        ObjectWrap::Unwrap<FetchCacheStream>(fetchCacheStreamWrap);
    task->m_url = (new Request(executionContext, requestInfo))->url();
    task->m_response = new Response(executionContext);

    task->start();

    return promise;
}

Promise* Internal::cache_storage_keys()
{
    class CacheStroageKeysTask : public CacheTask {
    public:
        CacheStroageKeysTask(Internal* i, Promise* p)
            : CacheTask(i, p)
        {
        }

        void run() override
        {
            TRACE(INTERNAL, taskId());
            STARFISH_ASSERT(m_url != nullptr);
            STARFISH_ASSERT(m_context != nullptr);

            ValueVectorRef* elements = ValueVectorRef::create();

            m_internal->fetchCacheStream()->getKeys(
                m_url->urlString()->hashValue(), elements);

            // TODO: Check what happens if this works on another thread.
            const auto& r = Evaluator::execute(
                m_context,
                [](ExecutionStateRef* state,
                   ValueVectorRef* elements) -> ValueRef* {
                    return ArrayObjectRef::create(state, elements);
                },
                elements);

            STARFISH_ASSERT(r.isSuccessful());
            m_result = true;
            m_resultValue = r.result->asArrayObject();
        }

        ResourceURL* m_url{ nullptr };
        ContextRef* m_context{ nullptr };
    };

    auto promise = new Promise(m_scriptBindingInstance);
    auto task = new CacheStroageKeysTask(this, promise);
    task->m_url = m_url;
    task->m_context = m_scriptBindingInstance->scriptContext();
    task->start();

    return promise;
}

} // namespace Starfish

#endif
