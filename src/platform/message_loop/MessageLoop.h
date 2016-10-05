/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishMessageLoop__
#define __StarFishMessageLoop__

#include "platform/threading/Mutex.h"

namespace StarFish {

class MessageLoop : public gc {
    friend class StarFish;
    friend class Window;
public:
    MessageLoop(StarFish* sf)
        : m_renderingIsDelayed(false)
        , m_starFish(sf)
        , m_idlersFromOtherThreadMutex(new Mutex())
    {
    }

    size_t addIdler(void (*fn)(size_t handle, void*), void* data);
    size_t addIdler(void (*fn)(size_t handle, void*, void*), void* data, void* data1);
    size_t addIdler(void (*fn)(size_t handle, void*, void*, void*), void* data, void* data1, void* data2);
    size_t addIdlerWithNoGCRootingInOtherThread(void (*fn)(size_t handle, void*), void* data);
    size_t addIdlerWithNoScriptInstanceEntering(void (*fn)(size_t handle, void*, void*), void* data, void* data1);

    void removeIdler(size_t handle);
    void removeIdlerWithNoGCRooting(size_t handle);

    bool hasPendingIdler()
    {
        return m_idlers.size();
    }

    void clearPendingIdlers();
protected:
    bool m_renderingIsDelayed;
    StarFish* m_starFish;
    std::unordered_set<size_t> m_idlers;
    Mutex* m_idlersFromOtherThreadMutex;
    std::unordered_set<size_t> m_idlersFromOtherThread;
    void run();
};

}

#endif
