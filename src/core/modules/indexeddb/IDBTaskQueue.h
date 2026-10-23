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

#ifndef __StarfishIDBTaskQueue__
#define __StarfishIDBTaskQueue__

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include "core/modules/indexeddb/IDBRequest.h"

namespace Starfish {

class ExecutionContext;
class GlobalScope;
class MessageLoop;
class IDBConnectionData;
class IDBRequest;

// The queue's thread collects its own heap and knows nothing of the
// requester's, so an item carries plain data only: the IDB thread reads and
// writes the plain fields of workData, while the request is touched on the
// requester's thread alone, by afterWork, and stays rooted in its context
// until then.
struct IDBTaskQueueItemData {
    IDBRequest* idbRequest{ nullptr };
    IDBRequestErrorType error{ IDBRequestErrorType::None };
};

struct IDBTaskQueueItem {
    using IDBTaskQueueMainWork = void (*)(IDBConnectionData* connectionData,
                                          IDBTaskQueueItemData* data);
    using IDBTaskQueueAfterWork = void (*)(IDBTaskQueueItemData* data);
    // Runs instead of afterWork when the request's context went away first;
    // frees workData without touching the request.
    using IDBTaskQueueDropWork = void (*)(IDBTaskQueueItemData* data);

    IDBTaskQueueItem(IDBTaskQueueMainWork work, IDBTaskQueueAfterWork afterWork,
                     IDBTaskQueueDropWork dropWork, IDBTaskQueueItemData* data);

    IDBTaskQueueMainWork mainWork{ nullptr };
    IDBTaskQueueAfterWork afterWork{ nullptr };
    IDBTaskQueueDropWork dropWork{ nullptr };
    IDBTaskQueueItemData* workData{ nullptr };

    // Where the result goes back to; captured on the requester's thread by
    // addTask() so the IDB thread never reads the request.
    ExecutionContext* context{ nullptr };
    MessageLoop* loop{ nullptr };
    GlobalScope* globalScope{ nullptr };
};

class IDBTaskQueue {
public:
    IDBTaskQueue();
    ~IDBTaskQueue();

    void run();
    void stop();

    void addTask(std::unique_ptr<IDBTaskQueueItem> item);

    // Drops the queued items of a context that is going away and waits for
    // the one being worked on, so nothing reaches the context afterwards.
    void cancelTasks(ExecutionContext* context);

private:
    static void worker(void* data);
    static void postResult(std::unique_ptr<IDBTaskQueueItem> item);

    std::thread m_thread;
    std::deque<std::unique_ptr<IDBTaskQueueItem>> m_tasks;
    std::atomic_bool m_isStopped;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    // The context of the item being worked on and whether that context was
    // cancelled meanwhile; both under m_mutex.
    ExecutionContext* m_runningContext{ nullptr };
    bool m_runningCancelled{ false };
    std::condition_variable m_runningDone;
};
} // namespace Starfish

#endif
#endif
