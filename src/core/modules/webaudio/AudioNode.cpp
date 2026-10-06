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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/webaudio/AudioNode.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/modules/webaudio/AudioContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"

namespace Starfish {

AudioNode::AudioNode(ExecutionContext* executionContext)
    : AudioNode(executionContext, nullptr)
{
}

AudioNode::AudioNode(ExecutionContext* executionContext,
                     BaseAudioContext* context)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_context(context)
{
    if (!context) {
        return;
    }
    // https://webaudio.github.io/web-audio-api/#lifetime-AudioNode
    // The native handler outlives this wrapper while it can still produce
    // sound; the graph decides when to free it once the wrapper is gone.
    m_releaseQueue = context->graph()->releaseQueue();
    m_releaseQueue->retain();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void*) {
            static_cast<AudioNode*>(obj)->releaseHandler();
        },
        NULL, NULL, NULL);
}

AudioNode::~AudioNode()
{
    // The object is being destroyed by C++, so cancel the GC finalizer
    // before its allocation is reclaimed.
    GC_REGISTER_FINALIZER_NO_ORDER(this, NULL, NULL, NULL, NULL);
    releaseHandler();
}

void AudioNode::releaseHandler()
{
    if (!m_releaseQueue) {
        return;
    }
    if (m_handler) {
        m_releaseQueue->postHandler(m_handler);
    }
    m_releaseQueue->release();
    m_releaseQueue = nullptr;
}

ScriptBindingInstance* AudioNode::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-audionode-connect
AudioNode* AudioNode::connect(AudioNode* destinationNode, uint32_t output,
                              uint32_t input)
{
    if (destinationNode->context() != context()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }
    if (output >= m_numberOfOutputs ||
        input >= destinationNode->numberOfInputs()) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "AudioNode connection index out of range");
    }
    for (const Connection& connection : m_connections) {
        if (connection.destination == destinationNode &&
            connection.output == output && connection.input == input) {
            return destinationNode;
        }
    }
    m_connections.push_back(Connection(destinationNode, output, input));
    context()->graph()->queueConnection(
        &m_handler->output(output), &destinationNode->handler()->input(input),
        true);
    return destinationNode;
}

// https://webaudio.github.io/web-audio-api/#dom-audionode-connect-destinationparam-output
void AudioNode::connect(AudioParam* destinationParam, uint32_t output)
{
    if (destinationParam->context() != context()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "AudioParam belongs to another context");
    }
    if (output >= m_numberOfOutputs) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "AudioNode output index out of range");
    }
    for (const ParamConnection& connection : m_paramConnections) {
        if (connection.destination == destinationParam &&
            connection.output == output) {
            return;
        }
    }
    m_paramConnections.push_back({ destinationParam, output });
    context()->graph()->queueConnection(&m_handler->output(output),
                                        destinationParam->timeline(), true);
}

void AudioNode::disconnect()
{
    for (const Connection& connection : m_connections) {
        context()->graph()->queueConnection(
            &m_handler->output(connection.output),
            &connection.destination->handler()->input(connection.input), false);
    }
    m_connections.clear();
    for (const ParamConnection& connection : m_paramConnections) {
        context()->graph()->queueConnection(
            &m_handler->output(connection.output),
            connection.destination->timeline(), false);
    }
    m_paramConnections.clear();
}

void AudioNode::disconnect(uint32_t output)
{
    if (output >= m_numberOfOutputs) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "AudioNode output index out of range");
    }
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        if (it->output == output) {
            context()->graph()->queueConnection(
                &m_handler->output(it->output),
                &it->destination->handler()->input(it->input), false);
            it = m_connections.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_paramConnections.begin();
         it != m_paramConnections.end();) {
        if (it->output == output) {
            context()->graph()->queueConnection(
                &m_handler->output(output), it->destination->timeline(), false);
            it = m_paramConnections.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioNode::disconnect(AudioNode* destinationNode)
{
    bool found = false;
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        if (it->destination == destinationNode) {
            context()->graph()->queueConnection(
                &m_handler->output(it->output),
                &destinationNode->handler()->input(it->input), false);
            it = m_connections.erase(it);
            found = true;
        } else {
            ++it;
        }
    }
    if (!found) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "No connection to AudioNode");
    }
}

void AudioNode::disconnect(AudioNode* destinationNode, uint32_t output)
{
    if (output >= m_numberOfOutputs) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "AudioNode output index out of range");
    }
    bool found = false;
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        if (it->destination == destinationNode && it->output == output) {
            context()->graph()->queueConnection(
                &m_handler->output(it->output),
                &destinationNode->handler()->input(it->input), false);
            it = m_connections.erase(it);
            found = true;
        } else {
            ++it;
        }
    }
    if (!found) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "No connection to AudioNode output");
    }
}

void AudioNode::disconnect(AudioNode* destinationNode, uint32_t output,
                           uint32_t input)
{
    if (output >= m_numberOfOutputs ||
        input >= destinationNode->numberOfInputs()) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "AudioNode connection index out of range");
    }
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        if (it->destination == destinationNode && it->output == output &&
            it->input == input) {
            context()->graph()->queueConnection(
                &m_handler->output(output),
                &destinationNode->handler()->input(input), false);
            m_connections.erase(it);
            return;
        }
    }
    throw new DOMException(executionContext(), DOMException::INVALID_ACCESS_ERR,
                           "No connection to AudioNode input");
}

void AudioNode::disconnect(AudioParam* destinationParam)
{
    bool found = false;
    for (auto it = m_paramConnections.begin();
         it != m_paramConnections.end();) {
        if (it->destination == destinationParam) {
            context()->graph()->queueConnection(&m_handler->output(it->output),
                                                destinationParam->timeline(),
                                                false);
            it = m_paramConnections.erase(it);
            found = true;
        } else {
            ++it;
        }
    }
    if (!found) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "No connection to AudioParam");
    }
}

void AudioNode::disconnect(AudioParam* destinationParam, uint32_t output)
{
    if (output >= m_numberOfOutputs) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "AudioNode output index out of range");
    }
    for (auto it = m_paramConnections.begin(); it != m_paramConnections.end();
         ++it) {
        if (it->destination == destinationParam && it->output == output) {
            context()->graph()->queueConnection(&m_handler->output(output),
                                                destinationParam->timeline(),
                                                false);
            m_paramConnections.erase(it);
            return;
        }
    }
    throw new DOMException(executionContext(), DOMException::INVALID_ACCESS_ERR,
                           "No connection to AudioParam output");
}

void AudioNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value < 1 || value > 32) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unsupported AudioNode channel count");
    }
    m_channelCount = value;
    configureInputs();
}

void AudioNode::applyOptions(const AudioNodeOptions& options)
{
    AudioGraphLock graphLock(context()->graph());
    if (options.hasChannelCount()) {
        setChannelCount(options.channelCount());
    }
    if (options.hasChannelCountMode()) {
        setChannelCountModeStr(options.channelCountMode());
    }
    if (options.hasChannelInterpretation()) {
        setChannelInterpretationStr(options.channelInterpretation());
    }
}

void AudioNode::configureInputs()
{
    AudioGraphLock graphLock(context()->graph());
    AudioInputChannelMode mode = AudioInputChannelMode::Max;
    if (m_channelCountMode == ChannelCountMode::ClampedMax) {
        mode = AudioInputChannelMode::ClampedMax;
    } else if (m_channelCountMode == ChannelCountMode::Explicit) {
        mode = AudioInputChannelMode::Explicit;
    }
    for (uint32_t index = 0; index < m_numberOfInputs; index++) {
        m_handler->input(index).configure(m_channelCount, mode,
                                          m_channelInterpretation ==
                                              ChannelInterpretation::Speakers);
    }
}

String* AudioNode::channelCountModeStr()
{
    AudioGraphLock graphLock(context()->graph());
    switch (m_channelCountMode) {
    case ChannelCountMode::Max:
        return String::createASCIIString("max");
    case ChannelCountMode::ClampedMax:
        return String::createASCIIString("clamped-max");
    case ChannelCountMode::Explicit:
        return String::createASCIIString("explicit");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return String::emptyString;
}
void AudioNode::setChannelCountModeStr(String* channelCountMode)
{
    AudioGraphLock graphLock(context()->graph());
    if (channelCountMode->equals("max")) {
        m_channelCountMode = ChannelCountMode::Max;
    } else if (channelCountMode->equals("clamped-max")) {
        m_channelCountMode = ChannelCountMode::ClampedMax;
    } else if (channelCountMode->equals("explicit")) {
        m_channelCountMode = ChannelCountMode::Explicit;
    }
    configureInputs();
}

String* AudioNode::channelInterpretationStr()
{
    AudioGraphLock graphLock(context()->graph());
    switch (m_channelInterpretation) {
    case ChannelInterpretation::Speakers:
        return String::createASCIIString("speakers");
    case ChannelInterpretation::Discrete:
        return String::createASCIIString("discrete");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return String::emptyString;
}

void AudioNode::setChannelInterpretationStr(String* channelInterpretation)
{
    AudioGraphLock graphLock(context()->graph());
    if (channelInterpretation->equals("speakers")) {
        m_channelInterpretation = ChannelInterpretation::Speakers;
    } else if (channelInterpretation->equals("discrete")) {
        m_channelInterpretation = ChannelInterpretation::Discrete;
    }
    configureInputs();
}
} // namespace Starfish

#endif
