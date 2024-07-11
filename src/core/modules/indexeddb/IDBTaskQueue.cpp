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
                                   IDBTaskQueueItemData* data)
    : mainWork(work)
    , afterWork(afterWork)
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
        }

        item->mainWork(&connectionData, item->workData);

        STARFISH_ASSERT(item->workData->idbRequest);
        item->workData->idbRequest->setProcessed(true);

        ExecutionContext* executionContext =
            item->workData->idbRequest->executionContext();
        GlobalScope* globalScope = executionContext->globalScope();

        struct Param {
            std::unique_ptr<IDBTaskQueueItem> item;
        };
        Param* p = new Param({ std::move(item) });

        executionContext->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                globalScope,
                [](size_t, void* data) {
                    auto* p = static_cast<Param*>(data);
                    IDBRequest* request = p->item->workData->idbRequest;

                    if (request->transaction()) {
                        request->transaction()->removeRequest(request);
                    }

                    p->item->workData->idbRequest->setDone(true);

                    p->item->afterWork(p->item->workData);

                    delete p;
                },
                p);
    }

    Escargot::Globals::finalizeThread();
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

    m_thread.join();
}

void IDBTaskQueue::addTask(std::unique_ptr<IDBTaskQueueItem> item)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isStopped) {
        return;
    }

    m_tasks.push_back(std::move(item));

    lock.unlock();
    m_cond.notify_one();
}

} // namespace Starfish

#endif
