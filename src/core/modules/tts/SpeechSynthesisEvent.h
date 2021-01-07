/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifdef STARFISH_ENABLE_TTS
#ifndef __StarfishSpeechSynthesisEvent__
#define __StarfishSpeechSynthesisEvent__

#include "core/dom/Event.h"

namespace Starfish {

struct SpeechSynthesisEventInit : public EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    SpeechSynthesisEventInit()
        : EventInit()
        , m_utterance(nullptr)
        , m_charIndex(0)
        , m_elapsedTime(0.0)
        , m_name(String::emptyString)

    {
    }
    SpeechSynthesisEventInit(SpeechSynthesisUtterance* u, uint64_t index,
                             float time, String* name)
        : EventInit()
        , m_utterance(u)
        , m_charIndex(index)
        , m_elapsedTime(time)
        , m_name(name)

    {
    }

    SpeechSynthesisUtterance* utterance() const
    {
        return m_utterance;
    }
    uint64_t charIndex() const
    {
        return m_charIndex;
    }
    float elapsedTime() const
    {
        return m_elapsedTime;
    }
    String* name() const
    {
        return m_name;
    }

    void setUtterance(SpeechSynthesisUtterance* u)
    {
        m_utterance = u;
    }
    void setCharIndex(uint64_t index)
    {
        m_charIndex = index;
    }
    void setElapsedTime(float time)
    {
        m_elapsedTime = time;
    }
    void setName(String* name)
    {
        m_name = name;
    }

private:
    SpeechSynthesisUtterance* m_utterance;
    uint64_t m_charIndex;
    float m_elapsedTime;
    String* m_name;
};

class SpeechSynthesisEvent : public Event {
public:
    SpeechSynthesisEvent(ExecutionContext* executionContext, String* eventType,
                         const SpeechSynthesisEventInit& init)
        : Event(executionContext, eventType, init)
        , m_utterance(init.utterance())
        , m_charIndex(init.charIndex())
        , m_elapsedTime(init.elapsedTime())
        , m_name(init.name())
    {
    }

    SpeechSynthesisUtterance* utterance()
    {
        return m_utterance;
    }
    uint64_t charIndex()
    {
        return m_charIndex;
    }
    float elapsedTime()
    {
        return m_elapsedTime;
    }
    String* name()
    {
        return m_name;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSpeechSynthesisEvent() const override;

private:
    SpeechSynthesisUtterance* m_utterance;
    uint64_t m_charIndex;
    float m_elapsedTime;
    String* m_name;
};
} // namespace Starfish
#endif
#endif // STARFISH_ENABLE_TTS
