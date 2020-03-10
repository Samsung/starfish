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

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioNode__
#define __StarfishAudioNode__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {
class ExecutionContext;
class BaseAudioContext;

enum class ChannelCountMode { Max, ClampedMax, Explicit };

enum class ChannelInterpretation { Speakers, Discrete };

class AudioNode : public EventTarget {
public:
    AudioNode(ExecutionContext* executionContext, BaseAudioContext* context);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioNode)

    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    virtual AudioNode* connect(AudioNode* destinationNode, uint32_t output = 0,
                               uint32_t input = 0);

    virtual BaseAudioContext* context()
    {
        return m_context;
    }

    DEFINE_GETTER(uint32_t, numberOfInputs)
    DEFINE_GETTER(uint32_t, numberOfOutputs)
    DEFINE_GETTER_SETTER(uint32_t, channelCount, ChannelCount)

    DEFINE_GETTER_SETTER(ChannelCountMode, channelCountMode, ChannelCountMode)
    String* channelCountModeStr();
    void setChannelCountModeStr(String* channelCountMode);

    DEFINE_GETTER_SETTER(ChannelInterpretation, channelInterpretation,
                         ChannelInterpretation)
    String* channelInterpretationStr();
    void setChannelInterpretationStr(String* channelInterpretation);

    virtual bool isAudioDestinationNode()
    {
        return false;
    }

    AudioDestinationNode* asAudioDestinationNode()
    {
        STARFISH_ASSERT(isAudioDestinationNode());
        return (AudioDestinationNode*)this;
    }

protected:
    ExecutionContext* m_executionContext{ nullptr };

    uint32_t m_numberOfInputs{ 0 };
    uint32_t m_numberOfOutputs{ 1 };
    uint32_t m_channelCount{ 2 };
    ChannelCountMode m_channelCountMode{ ChannelCountMode::Max };
    ChannelInterpretation m_channelInterpretation{
        ChannelInterpretation::Speakers
    };

    BaseAudioContext* m_context{ nullptr };
    AudioNode* m_destinationNode{ nullptr };

private:
    AudioNode(ExecutionContext* executionContext);
};
}
#endif
#endif
