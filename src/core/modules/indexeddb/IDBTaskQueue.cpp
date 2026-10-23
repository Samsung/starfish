/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_IDB)

#include "StarfishConfig.h"
#include <EscargotPublic.h>
#include "core/page/WebBase.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/indexeddb/IDBTransaction.h"
#include "core/modules/indexeddb/IDBRequest.h"
#include "core/modules/indexeddb/IDBConnection.h"
#include "core/modules/indexeddb/IDBTaskQueue.h"

namespace Starfish {

IDBTaskQueueItem::IDBTaskQueueItem(IDBTaskQueueMainWork work,
                                   IDBTaskQueueAfterWork afterWork,
                                   IDBTaskQueueDropWork dropWork,
                                   IDBTaskQueueItemData* data)
    : mainWork(work)
    , afterWork(afterWork)
    , dropWork(dropWork)
    , workData(data)
{
}

IDBTaskQueue::IDBTaskQueue()
    : m_isStopped(false)
{
    STARFISH_ASSERT(isMainThread());
}

IDBTaskQueue::~IDBTaskQueue()
{
    stop();

    for (auto& item : m_tasks) {
        item->dropWork(item->workData);
    }
    m_tasks.clear();
    m_tasks.shrink_to_fit();
}

void IDBTaskQueue::run()
{
    m_thread = std::thread(IDBTaskQueue::worker, this);
}

void IDBTaskQueue::worker(void* data)
{
    // https://w3c.github.io/IndexedDB/#asynchronously-execute-a-request

    STARFISH_RELEASE_ASSERT(Escargot::Globals::supportsThreading());
    Escargot::Globals::initializeThread();

    IDBTaskQueue* self = static_cast<IDBTaskQueue*>(data);

    IDBConnectionData connectionData;

    while (self->m_isStopped == false) {
        std::unique_ptr<IDBTaskQueueItem> item;
        {
            std::unique_lock<std::mutex> lock(self->m_mutex);
            if (self->m_tasks.empty()) {
                self->m_cond.wait(lock);
                continue;
            }

            item = std::move(self->m_tasks.front());
            // TODO: 5-1: Wait until request is the first item in transaction’s
            // request list that is not processed.
            self->m_tasks.pop_front();
            self->m_runningContext = item->context;
            self->m_runningCancelled = false;
        }

        item->mainWork(&connectionData, item->workData);

        {
            // The result is posted under the lock so a cancel cannot slip in
            // between the check and the post.
            std::unique_lock<std::mutex> lock(self->m_mutex);
            self->m_runningContext = nullptr;
            if (self->m_runningCancelled) {
                item->dropWork(item->workData);
            } else {
                postResult(std::move(item));
            }
        }
        self->m_runningDone.notify_all();
    }

    Escargot::Globals::finalizeThread();
}

void IDBTaskQueue::postResult(std::unique_ptr<IDBTaskQueueItem> item)
{
    struct Param {
        std::unique_ptr<IDBTaskQueueItem> item;
    };
    Param* p = new Param({ std::move(item) });

    p->item->loop->addIdlerWithNoGCRootingInOtherThread(
        p->item->globalScope,
        [](size_t, void* data) {
            auto* p = static_cast<Param*>(data);
            IDBRequest* request = p->item->workData->idbRequest;

            request->setProcessed(true);
            if (request->transaction()) {
                request->transaction()->removeRequest(request);
            }
            request->setDone(true);

            p->item->afterWork(p->item->workData);
            p->item->context->removePointerFromRootSet(request);
            delete p;
        },
        [](void* data) {
            auto* p = static_cast<Param*>(data);
            p->item->dropWork(p->item->workData);
            delete p;
        },
        p);
}

void IDBTaskQueue::stop()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_isStopped) {
            return;
        }

        m_isStopped = true;

        lock.unlock();
        m_cond.notify_one();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void IDBTaskQueue::addTask(std::unique_ptr<IDBTaskQueueItem> item)
{
    IDBRequest* request = item->workData->idbRequest;
    ExecutionContext* context = request->executionContext();
    STARFISH_ASSERT(context->isContextThread());

    item->context = context;
    item->loop = context->webBase()->messageLoop();
    item->globalScope = context->globalScope();

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isStopped) {
        item->dropWork(item->workData);
        return;
    }

    // Only this queue refers to the in-flight request's data holder, which
    // the collector does not see; the request stays rooted until its result
    // is delivered.
    context->addPointerInRootSet(request);
    m_tasks.push_back(std::move(item));

    lock.unlock();
    m_cond.notify_one();
}

void IDBTaskQueue::cancelTasks(ExecutionContext* context)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto iter = m_tasks.begin();
    while (iter != m_tasks.end()) {
        if ((*iter)->context == context) {
            (*iter)->dropWork((*iter)->workData);
            iter = m_tasks.erase(iter);
        } else {
            iter++;
        }
    }

    if (m_runningContext == context) {
        m_runningCancelled = true;
        m_runningDone.wait(lock, [&] { return m_runningContext != context; });
    }
}

} // namespace Starfish

#endif
