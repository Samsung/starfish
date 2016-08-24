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

#include "StarFishConfig.h"
#include "MessageLoop.h"
#include "dom/binding/ScriptBindingInstance.h"
#include "platform/threading/Thread.h"
#include "platform/threading/Locker.h"
#include "platform/window/Window.h"

#include <Elementary.h>

namespace StarFish {

void MessageLoop::run()
{
#ifndef STARFISH_TIZEN_WEARABLE_LIB
    elm_run();
#endif
}

struct IdlerData {
    void (*m_fn)(size_t, void*);
    void* m_data;
    void* m_data1;
    void* m_data2;
    Ecore_Idler* m_idler;
    MessageLoop* m_ml;
    volatile bool m_shouldExecute;
    bool m_isMainThreadData;
};

size_t MessageLoop::addIdler(void (*fn)(size_t, void*), void* data)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new(NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_idler = ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->window()->scriptBindingInstance());
        id->m_fn((size_t)id, id->m_data);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(void (*fn)(size_t, void*, void*), void* data, void* data1)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new(NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_idler =  ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->window()->scriptBindingInstance());
        ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data, id->m_data1);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(void (*fn)(size_t, void*, void*, void*), void* data, void* data1, void* data2)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new(NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_data2 = data2;
    id->m_ml = this;
    id->m_idler =  ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->window()->scriptBindingInstance());
        ((void (*)(size_t, void*, void*, void*))id->m_fn)((size_t)id, id->m_data, id->m_data1, id->m_data2);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);

    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(void (*fn)(size_t, void*), void* data)
{
    IdlerData* id = new IdlerData;
    id->m_isMainThreadData = false;
    id->m_shouldExecute = true;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;

    {
        Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
        m_idlersFromOtherThread.insert((size_t)id);
    }

    ecore_main_loop_thread_safe_call_async([](void* data) -> void {
        ecore_idler_add([](void* data) -> Eina_Bool {
            IdlerData* id = (IdlerData*)data;
            {
                Locker<Mutex> l(*id->m_ml->m_idlersFromOtherThreadMutex);
                id->m_ml->m_idlersFromOtherThread.erase(id->m_ml->m_idlersFromOtherThread.find((size_t)id));
            }
            if (id->m_shouldExecute) {
                ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->window()->scriptBindingInstance());
                id->m_fn((size_t)id, id->m_data);
            }
            delete id;
            return ECORE_CALLBACK_CANCEL;
        }, data);
    }, id);
    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoScriptInstanceEntering(void (*fn)(size_t handle, void*, void*), void* data, void* data1)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = new(NoGC) IdlerData;
    m_idlers.insert((size_t)id);
    id->m_isMainThreadData = true;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_idler =  ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        id->m_ml->m_idlers.erase(id->m_ml->m_idlers.find((size_t)id));
        ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data, id->m_data1);
        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);
    return (size_t)id;
}

void MessageLoop::removeIdler(size_t handle)
{
    STARFISH_ASSERT(isMainThread());
    IdlerData* id = (IdlerData*)handle;
    m_idlers.erase(m_idlers.find(handle));
    ecore_idler_del(id->m_idler);
    GC_FREE(id);
}

void MessageLoop::removeIdlerWithNoGCRooting(size_t handle)
{
    IdlerData* id = (IdlerData*)handle;
    id->m_shouldExecute = false;
}

void MessageLoop::clearPendingIdlers()
{
    auto iter = m_idlers.begin();
    while (iter != m_idlers.end()) {
        IdlerData* id = (IdlerData*)*iter;
        ecore_idler_del(id->m_idler);
        GC_FREE(id);
    }
    m_idlers.clear();

    Locker<Mutex> l(*m_idlersFromOtherThreadMutex);
    auto iter2 = m_idlersFromOtherThread.begin();
    while (iter2 != m_idlersFromOtherThread.end()) {
        IdlerData* id = (IdlerData*)*iter2;
        id->m_shouldExecute = false;
    }
}

}
