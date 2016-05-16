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
    void (*m_fn)(void*);
    void* m_data;
    Ecore_Idler* m_idler;
    MessageLoop* m_ml;
};

void* MessageLoop::addIdler(void (*fn)(void*), void* data)
{

    IdlerData* id = new(NoGC) IdlerData;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;
    id->m_idler =  ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->scriptBindingInstance());
        id->m_fn(id->m_data);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);

    return id;
}

void MessageLoop::removeIdler(void* handle)
{
    IdlerData* id = (IdlerData*)handle;
    ecore_idler_del(id->m_idler);
    GC_FREE(id);
}

}
