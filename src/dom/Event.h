#ifndef __StarFishEvent__
#define __StarFishEvent__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

struct EventInit {
    EventInit();
    EventInit(bool bubbles, bool cancelable);

    bool bubbles;
    bool cancelable;
};

class Event : public ScriptWrappable {
protected:
    Event(ScriptBindingInstance* instance);

public:
    enum PhaseType {
        NONE                = 0,
        CAPTURING_PHASE     = 1,
        AT_TARGET           = 2,
        BUBBLING_PHASE      = 3
    };

    const String* type() const { return m_type != nullptr ? m_type : String::emptyString; }
    void setType(String* type) { m_type = type; }

    EventTarget* target() const { return m_target; }
    void setTarget(EventTarget* target) { m_target = target; }

    EventTarget* currentTarget() const { return m_currentTarget; }
    void setCurrentTarget(EventTarget* currentTarget) { m_currentTarget = currentTarget; }

    unsigned short eventPhase() const { return m_eventPhase; }
    void setEventPhase(unsigned short phase) { m_eventPhase = phase; }

    void stopPropagation() { m_propagationStopped = true; }
    void stopImmediatePropagation() { m_propagationStopped = true; m_immediatePropagationStopped = true; }

    bool bubbles() const { return m_bubbles; }
    bool cancelable() const { return m_cancelable; }

    void preventDefault() {
        if (m_cancelable)
            m_defaultPrevented = true; // canceled flag
    }
    bool defaultPrevented() const { return m_defaultPrevented; }
    void setDefaultPrevented(bool defaultPrevented) { m_defaultPrevented = defaultPrevented; }

private:
    String* m_type;
    EventTarget* m_target;
    EventTarget* m_currentTarget;

    unsigned short m_eventPhase { 0 };

    bool m_propagationStopped { false }; // stop propagation flag
    bool m_immediatePropagationStopped { false }; // stop immediate propagation flag

    bool m_bubbles { false };
    bool m_cancelable { false };

    bool m_defaultPrevented { false }; // canceled flag
    bool m_isInitialized { false }; // initialized flag
    bool m_dispatched { false }; // dispatch flag

    // TODO: The `timeStamp` attribute must return the value it was initialized to.
    //       When an event is created the attribute must be initialized to the number of milliseconds that have passed since 00:00:00 UTC on 1 January 1970, ignoring leap seconds.
    //DOMTimeStamp m_timeStamp;
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
