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

Event::Event(ScriptBindingInstance* instance)
    : ScriptWrappable(this)
{
    initScriptWrappable(this, instance);
}

Event::Event(ScriptBindingInstance* instance, QualifiedName eventType, bool canBubble, bool cancelable)
    : ScriptWrappable(this)
    , m_type(eventType)
    , m_bubbles(canBubble)
    , m_cancelable(cancelable)
{
    initScriptWrappable(this, instance);
}

UIEvent::UIEvent(ScriptBindingInstance* instance)
    : Event(instance)
{
    initScriptWrappable(this, instance);
}

MouseEvent::MouseEvent(ScriptBindingInstance* instance)
    : UIEvent(instance)
{
    initScriptWrappable(this, instance);
}

ProgressEvent::ProgressEvent(ScriptBindingInstance* instance,uint32_t loaded,uint32_t total)
    : Event(instance)
{
    initScriptWrappable(this, instance,loaded,total);
}

}
