#include "StarFishConfig.h"
#include "Event.h"

#include <time.h>

#define BILLION 1000000000L
#define MILLION 1000000L

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

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    m_timeStamp = (BILLION * time.tv_sec + time.tv_nsec) / MILLION;
}

Event::Event(QualifiedName eventType, const EventInit& init)
    : ScriptWrappable(this)
    , m_isInitialized(true)
    , m_type(eventType)
    , m_bubbles(init.bubbles)
    , m_cancelable(init.cancelable)
{
    initScriptWrappable(this);

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    m_timeStamp = (BILLION * time.tv_sec + time.tv_nsec) / MILLION;
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
