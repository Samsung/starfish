#include "StarFishConfig.h"
#include "Event.h"

namespace StarFish {

EventInit::EventInit()
    : bubbles(false)
    , cancelable(false)
{
}

EventInit::EventInit(bool b, bool c)
    : bubbles(b)
    , cancelable(c)
{
}

Event::Event()
    : ScriptWrappable(this)
    , m_isInitialized(true)
{
    initScriptWrappable(this);
}

Event::Event(QualifiedName eventType, const EventInit& init)
    : ScriptWrappable(this)
    , m_isInitialized(true)
    , m_type(eventType)
    , m_bubbles(init.bubbles)
    , m_cancelable(init.cancelable)
{
    initScriptWrappable(this);
}

UIEvent::UIEvent(ScriptBindingInstance* instance)
    : Event()
{
    initScriptWrappable(this, instance);
}

MouseEvent::MouseEvent(ScriptBindingInstance* instance)
    : UIEvent(instance)
{
    initScriptWrappable(this, instance);
}

ProgressEvent::ProgressEvent(ScriptBindingInstance* instance, uint32_t loaded, uint32_t total)
    : Event()
{
    initScriptWrappable(this, instance, loaded, total);
}

}
