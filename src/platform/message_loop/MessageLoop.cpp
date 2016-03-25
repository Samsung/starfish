#include "StarFishConfig.h"
#include "MessageLoop.h"

#include <Elementary.h>

namespace StarFish {

void MessageLoop::run()
{
#ifndef STARFISH_TIZEN_WEARABLE
    elm_run();
#endif
}

void MessageLoop::addIdler(void (*fn)(void*), void* data)
{
    struct IdlerData {
        void (*m_fn)(void*);
        void* m_data;
    };

    IdlerData* id = new(NoGC) IdlerData;
    id->m_fn = fn;
    id->m_data = data;
    ecore_idler_add([](void* data) -> Eina_Bool {
        IdlerData* id = (IdlerData*)data;

        id->m_fn(id->m_data);

        GC_FREE(id);
        return ECORE_CALLBACK_CANCEL;
    }, id);
}

}
