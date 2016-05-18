#include "StarFishConfig.h"
#include "ElementResourceClient.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/window/Window.h"
#include "dom/Element.h"
#include "dom/Document.h"

namespace StarFish {

void ElementResourceClient::didLoadFinished()
{
    ResourceClient::didLoadFinished();
    auto fn = [](size_t handle, void* data)
    {
        Element* element = (Element*)data;
        String* eventType = element->document()->window()->starFish()->staticStrings()->m_load.localName();
        Event* e = new Event(eventType, EventInit(false, false));
        element->EventTarget::dispatchEvent(element, e);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->document()->window()->starFish()->messageLoop()->addIdler(fn, m_element);
    }
}

void ElementResourceClient::didLoadFailed()
{
    ResourceClient::didLoadFailed();
    auto fn = [](size_t handle, void* data)
    {
        Element* element = (Element*)data;
        String* eventType = element->document()->window()->starFish()->staticStrings()->m_error.localName();
        Event* e = new Event(eventType, EventInit(false, false));
        element->EventTarget::dispatchEvent(element, e);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->document()->window()->starFish()->messageLoop()->addIdler(fn, m_element);
    }
}

}
