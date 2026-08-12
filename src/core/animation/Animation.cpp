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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/animation/Animation.h"

#include "core/animation/AnimationExecutor.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/Event.h"

namespace Starfish {

Animation::Animation()
    : EventTarget()
    , m_executionContext(nullptr)
    , m_target(nullptr)
    , m_animationName(nullptr)
    , m_isFinished(false)
{
}

Animation::Animation(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_target(nullptr)
    , m_animationName(nullptr)
    , m_isFinished(false)
{
}

void Animation::cancel()
{
    // https://drafts.csswg.org/web-animations-1/#canceling-an-animation-section
    // An animation that already finished its active period still holds its
    // filled values, so it has to be dropped here as well.
    if (!m_target || !m_animationName) {
        return;
    }

    AnimationExecutor* executor = m_target->document()->animationExecutor();
    if (!executor->cancelWebAnimation(m_animationName, m_target)) {
        return;
    }

    m_target->setNeedsStyleRecalcForAnimation();
    m_isFinished = false;
    fireEvent(m_target->starfish()->staticStrings()->m_cancel.localName());
}

void Animation::notifyFinished()
{
    if (m_isFinished) {
        return;
    }
    m_isFinished = true;
    fireEvent(m_target->starfish()->staticStrings()->m_finish.localName());
}

void Animation::fireEvent(String* eventType)
{
    EventInit init(false, false);
    dispatchEventIdleTimeByUA(new Event(m_executionContext, eventType, init));
}
} // namespace Starfish
