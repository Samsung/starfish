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
class AudioHandler;
class AudioParam;
class AudioGraphReleaseQueue;

enum class ChannelCountMode { Max, ClampedMax, Explicit };

enum class ChannelInterpretation { Speakers, Discrete };

struct AudioNodeOptions {
    uint32_t channelCount() const
    {
        return m_channelCount;
    }
    void setChannelCount(uint32_t value)
    {
        m_channelCount = value;
        m_hasChannelCount = true;
    }
    bool hasChannelCount() const
    {
        return m_hasChannelCount;
    }
    String* channelCountMode() const
    {
        return m_channelCountMode.valueOr(String::emptyString);
    }
    void setChannelCountMode(String* value)
    {
        m_channelCountMode = value;
    }
    bool hasChannelCountMode() const
    {
        return m_channelCountMode.hasValue();
    }
    String* channelInterpretation() const
    {
        return m_channelInterpretation.valueOr(String::emptyString);
    }
    void setChannelInterpretation(String* value)
    {
        m_channelInterpretation = value;
    }
    bool hasChannelInterpretation() const
    {
        return m_channelInterpretation.hasValue();
    }

private:
    uint32_t m_channelCount{ 0 };
    bool m_hasChannelCount{ false };
    Optional<String*> m_channelCountMode;
    Optional<String*> m_channelInterpretation;
};

class AudioNode : public EventTarget {
public:
    AudioNode(ExecutionContext* executionContext, BaseAudioContext* context);
    // Runs only when a derived constructor throws; collection goes through
    // the GC finalizer registered by the constructor.
    ~AudioNode();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioNode)

    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    virtual AudioNode* connect(AudioNode* destinationNode, uint32_t output = 0,
                               uint32_t input = 0);
    void connect(AudioParam* destinationParam, uint32_t output = 0);
    void disconnect();
    void disconnect(uint32_t output);
    void disconnect(AudioNode* destinationNode);
    void disconnect(AudioNode* destinationNode, uint32_t output);
    void disconnect(AudioNode* destinationNode, uint32_t output,
                    uint32_t input);
    void disconnect(AudioParam* destinationParam);
    void disconnect(AudioParam* destinationParam, uint32_t output);

    virtual BaseAudioContext* context()
    {
        return m_context;
    }

    AudioHandler* handler() const
    {
        return m_handler;
    }

    DEFINE_GETTER(uint32_t, numberOfInputs)
    DEFINE_GETTER(uint32_t, numberOfOutputs)
    DEFINE_GETTER(uint32_t, channelCount)
    virtual void setChannelCount(uint32_t value);

    DEFINE_GETTER_SETTER(ChannelCountMode, channelCountMode, ChannelCountMode)
    String* channelCountModeStr();
    virtual void setChannelCountModeStr(String* channelCountMode);

    DEFINE_GETTER_SETTER(ChannelInterpretation, channelInterpretation,
                         ChannelInterpretation)
    String* channelInterpretationStr();
    virtual void setChannelInterpretationStr(String* channelInterpretation);

    virtual bool isAudioDestinationNode() const
    {
        return false;
    }

    AudioDestinationNode* asAudioDestinationNode()
    {
        STARFISH_ASSERT(isAudioDestinationNode());
        return (AudioDestinationNode*)this;
    }

protected:
    void applyOptions(const AudioNodeOptions& options);
    void configureInputs();
    ExecutionContext* m_executionContext{ nullptr };

    uint32_t m_numberOfInputs{ 0 };
    uint32_t m_numberOfOutputs{ 1 };
    uint32_t m_channelCount{ 2 };
    ChannelCountMode m_channelCountMode{ ChannelCountMode::Max };
    ChannelInterpretation m_channelInterpretation{
        ChannelInterpretation::Speakers
    };

    BaseAudioContext* m_context{ nullptr };
    AudioHandler* m_handler{ nullptr };
    struct Connection {
        Connection(AudioNode* node, uint32_t outputIndex, uint32_t inputIndex)
            : destination(node)
            , output(outputIndex)
            , input(inputIndex)
        {
        }
        AudioNode* destination;
        uint32_t output;
        uint32_t input;
    };
    GCVector<Connection> m_connections;
    struct ParamConnection {
        AudioParam* destination;
        uint32_t output;
    };
    GCVector<ParamConnection> m_paramConnections;

private:
    AudioNode(ExecutionContext* executionContext);
    void releaseHandler();

    // Reports collection of this wrapper so the graph can free m_handler.
    AudioGraphReleaseQueue* m_releaseQueue{ nullptr };
};
} // namespace Starfish
#endif
#endif
