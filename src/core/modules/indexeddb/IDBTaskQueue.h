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

#include "core/modules/indexeddb/IDBRequest.h"

namespace Starfish {

class Thread;
class WebBase;
class ExecutionContext;
class IDBConnectionData;
class IDBRequest;

struct IDBTaskQueueItemData : public gc {
    IDBRequest* idbRequest{ nullptr };
    IDBRequestErrorType error{ IDBRequestErrorType::None };
};

struct IDBTaskQueueItem {
    using IDBTaskQueueMainWork = void (*)(IDBConnectionData* connectionData,
                                          IDBTaskQueueItemData* data);
    using IDBTaskQueueAfterWork = void (*)(IDBTaskQueueItemData* data);

    IDBTaskQueueItem(IDBTaskQueueMainWork work, IDBTaskQueueAfterWork afterWork,
                     IDBTaskQueueItemData* data);

    IDBTaskQueueMainWork mainWork{ nullptr };
    IDBTaskQueueAfterWork afterWork{ nullptr };
    IDBTaskQueueItemData* workData{ nullptr };
};

class IDBTaskQueue {
public:
    IDBTaskQueue();
    ~IDBTaskQueue();

    void run();
    void stop();

    void addTask(std::unique_ptr<IDBTaskQueueItem> item);

private:
    static void worker(void* data);

    std::thread m_thread;
    std::deque<std::unique_ptr<IDBTaskQueueItem>> m_tasks;
    std::atomic_bool m_isStopped;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};
} // namespace Starfish

#endif
#endif
