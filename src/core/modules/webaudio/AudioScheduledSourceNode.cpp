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

#include "core/modules/webaudio/AudioScheduledSourceNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/Event.h"

namespace Starfish {
AudioScheduledSourceNode::AudioScheduledSourceNode(
    ExecutionContext* executionContext, BaseAudioContext* context)
    : AudioNode(executionContext, context)
{
}

ScriptBindingInstance* AudioScheduledSourceNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-audioscheduledsourcenode-start
void AudioScheduledSourceNode::start(double when)
{
    if (m_hasStartCalled) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "Source has already started");
    }
    if (when < 0) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Start time must be non-negative");
    }
    m_startTime = when;
    m_hasStartCalled = true;
}

// https://webaudio.github.io/web-audio-api/#dom-audioscheduledsourcenode-stop
void AudioScheduledSourceNode::stop(double when)
{
    if (!m_hasStartCalled) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    if (when < 0) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Stop time must be non-negative");
    }
    m_stopTime = when;
    m_hasStopCalled = true;
}

DEFINE_EVENT_LISTENER(AudioScheduledSourceNode, ended);

void AudioScheduledSourceNode::dispatchEnded()
{
    Event* event =
        new Event(executionContext(), staticStrings()->m_ended.localName());
    dispatchEventByUA(event);
}

} // namespace Starfish

#endif
