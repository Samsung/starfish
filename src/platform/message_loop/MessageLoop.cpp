#include "StarFishConfig.h"
#include "MessageLoop.h"
#include "dom/binding/ScriptBindingInstance.h"

#include <Elementary.h>

namespace StarFish {

void MessageLoop::run()
{
#ifndef STARFISH_TIZEN_WEARABLE_APP
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
    bool m_shouldExecute;
};

size_t MessageLoop::addIdler(void (*fn)(size_t, void*), void* data)
{
    IdlerData* id = new(NoGC) IdlerData;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_idler =  ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->scriptBindingInstance());
        id->m_fn((size_t)id, id->m_data);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(void (*fn)(size_t, void*, void*), void* data, void* data1)
{
    IdlerData* id = new(NoGC) IdlerData;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_ml = this;
    id->m_idler =  ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->scriptBindingInstance());
        ((void (*)(size_t, void*, void*))id->m_fn)((size_t)id, id->m_data, id->m_data1);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);

    return (size_t)id;
}

size_t MessageLoop::addIdler(void (*fn)(size_t, void*, void*, void*), void* data, void* data1, void* data2)
{
    IdlerData* id = new(NoGC) IdlerData;
    id->m_fn = (void (*)(size_t, void*))fn;
    id->m_data = data;
    id->m_data1 = data1;
    id->m_data2 = data2;
    id->m_ml = this;
    id->m_idler =  ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->scriptBindingInstance());
        ((void (*)(size_t, void*, void*, void*))id->m_fn)((size_t)id, id->m_data, id->m_data1, id->m_data2);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);

    return (size_t)id;
}

size_t MessageLoop::addIdlerWithNoGCRootingInOtherThread(void (*fn)(size_t, void*), void* data)
{
    IdlerData* id = new IdlerData;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_shouldExecute = true;
    ecore_main_loop_thread_safe_call_async([](void* data) -> void {
        ecore_idler_add([](void* data) -> Eina_Bool {
            IdlerData* id = (IdlerData*)data;
            if (id->m_shouldExecute) {
                ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->scriptBindingInstance());
                id->m_fn((size_t)id, id->m_data);
            }
            delete id;
            return ECORE_CALLBACK_CANCEL;
        }, data);
    }, id);
    return (size_t)id;
}

void MessageLoop::removeIdler(size_t handle)
{
    IdlerData* id = (IdlerData*)handle;
    ecore_idler_del(id->m_idler);
    GC_FREE(id);
}

void MessageLoop::removeIdlerWithNoGCRooting(size_t handle)
{
    IdlerData* id = (IdlerData*)handle;
    id->m_shouldExecute = false;
}

}
