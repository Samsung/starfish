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

// https://heycam.github.io/webidl/#common-DOMTimeStamp
typedef unsigned long long DOMTimeStamp;

class Event : public ScriptWrappable {
protected:

public:
    enum PhaseType {
        NONE                = 0,
        CAPTURING_PHASE     = 1,
        AT_TARGET           = 2,
        BUBBLING_PHASE      = 3
    };

    Event();
    Event(String* eventType, const EventInit& init = EventInit(false, false));

    virtual ~Event() { }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    const String* type() const { return m_type; }
    EventTarget* target() const { return m_target; }
    void setTarget(EventTarget* target) { m_target = target; }

    EventTarget* currentTarget() const { return m_currentTarget; }
    void setCurrentTarget(EventTarget* currentTarget) { m_currentTarget = currentTarget; }

    unsigned short eventPhase() const { return m_eventPhase; }
    void setEventPhase(unsigned short phase) { m_eventPhase = phase; }

    bool stopPropagation() { return m_propagationStopped || m_immediatePropagationStopped; }
    void setStopPropagation() { m_propagationStopped = true; }
    bool stopImmediatePropagation() { return m_immediatePropagationStopped; }
    void setStopImmediatePropagation() { m_propagationStopped = true; m_immediatePropagationStopped = true; }

    bool bubbles() const { return m_bubbles; }
    bool cancelable() const { return m_cancelable; }

    void preventDefault()
    {
        if (m_cancelable)
            m_defaultPrevented = true; // canceled flag
    }
    bool defaultPrevented() const { return m_defaultPrevented; }
    void setDefaultPrevented(bool defaultPrevented) { m_defaultPrevented = defaultPrevented; }

    DOMTimeStamp timeStamp() const { return m_timeStamp; }

    bool isInitialized() const { return m_isInitialized; }
    void setIsInitialized(bool isInitialized) { m_isInitialized = isInitialized; }
    bool isDispatched() const { return m_isDispatched; }
    void setIsDispatched(bool isDispatched) { m_isDispatched = isDispatched; }

    /* Other methods (not in Event interface) */
    virtual bool isProgressEvent() const { return false; }

    ProgressEvent* asProgressEvent()
    {
        STARFISH_ASSERT(isProgressEvent());
        return (ProgressEvent*)this;
    }

private:
    bool m_isInitialized { false }; // initialized flag

    String* m_type { String::emptyString };
    EventTarget* m_target;
    EventTarget* m_currentTarget;

    unsigned short m_eventPhase { 0 };

    bool m_propagationStopped { false }; // stop propagation flag
    bool m_immediatePropagationStopped { false }; // stop immediate propagation flag

    bool m_bubbles { false };
    bool m_cancelable { false };
    bool m_defaultPrevented { false }; // canceled flag

    DOMTimeStamp m_timeStamp;

    bool m_isDispatched { false }; // dispatch flag
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

struct ProgressEventInit : public EventInit {
    ProgressEventInit();
    ProgressEventInit(bool bubbles, bool cancelable, bool lengthComputable, unsigned long long loaded, unsigned long long total);

    bool lengthComputable;
    unsigned long long loaded;
    unsigned long long total;
};

class ProgressEvent : public Event {
public:
    ProgressEvent(String* eventType, const ProgressEventInit& init = ProgressEventInit(false, false, false, 0, 0));

    bool lengthComputable() const { return m_lengthComputable; }
    unsigned long long loaded() const { return m_loaded; }
    unsigned long long total() const { return m_total; }

    /* Other methods (not in ProgressEvent interface) */
    virtual bool isProgressEvent() const { return true; }

private:
    bool m_lengthComputable { false };
    unsigned long long m_loaded { 0 };
    unsigned long long m_total { 0 };
};

}

#endif
