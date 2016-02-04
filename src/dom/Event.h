#ifndef __StarFishEvent__
#define __StarFishEvent__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Event : public ScriptWrappable {
protected:
    Event(ScriptBindingInstance* instance);
public:

};

class UIEvent : public Event {
protected:
    UIEvent(ScriptBindingInstance* instance);
public:
};

class MouseEvent : public UIEvent {
public:
    MouseEvent(ScriptBindingInstance* instance);
};

class ProgressEvent : public Event {
public:
    ProgressEvent(ScriptBindingInstance* instance,uint32_t loaded,uint32_t total);
};

}

#endif
