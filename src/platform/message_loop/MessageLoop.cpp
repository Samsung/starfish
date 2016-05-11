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

void MessageLoop::addIdler(void (*fn)(void*), void* data)
{
    struct IdlerData {
        void (*m_fn)(void*);
        void* m_data;
        MessageLoop* m_ml;
    };

    IdlerData* id = new(NoGC) IdlerData;
    id->m_fn = fn;
    id->m_data = data;
    id->m_ml = this;

    ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;
        ScriptBindingInstanceEnterer enter(id->m_ml->m_starFish->scriptBindingInstance());
        id->m_fn(id->m_data);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);
}

}
