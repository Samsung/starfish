/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "core/modules/webaudio/GainNode.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

GainNode::GainNode(ExecutionContext* executionContext,
                   BaseAudioContext* context)
    : GainNode(executionContext, context, GainOptions())
{
}

GainNode::GainNode(ExecutionContext* executionContext,
                   BaseAudioContext* context, GainOptions options)
    : AudioNode(executionContext, context)
    , m_gain(new AudioParam(executionContext, options.gain()))
{
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
}

ScriptBindingInstance* GainNode::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

} // namespace Starfish

#endif
