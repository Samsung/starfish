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
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebBase.h"

namespace Starfish {

Animation::Animation()
    : EventTarget()
    , m_executionContext(nullptr)
    , m_target(nullptr)
    , m_animationName(nullptr)
    , m_isFinished(false)
    , m_isCanceled(false)
{
}

Animation::Animation(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_target(nullptr)
    , m_animationName(nullptr)
    , m_isFinished(false)
    , m_isCanceled(false)
{
}

void Animation::cancel()
{
    // https://drafts.csswg.org/web-animations-1/#canceling-an-animation-section
    // An animation that already finished its active period still holds its
    // filled values, so it has to be dropped here as well.
    if (!m_target || !m_animationName || m_isCanceled) {
        return;
    }

    AnimationExecutor* executor = m_target->document()->animationExecutor();
    if (executor->cancelWebAnimation(m_animationName, m_target)) {
        m_target->setNeedsStyleRecalcForAnimation();
    }

    // A finished animation is not idle, so the cancel event fires even when
    // its tasks already left the executor (fill:none past the active period).
    notifyCanceled();
}

void Animation::notifyFinished()
{
    if (m_isFinished || m_isCanceled) {
        return;
    }
    m_isFinished = true;
    fireEvent(m_target->starfish()->staticStrings()->m_finish.localName());
}

void Animation::notifyCanceled()
{
    if (m_isCanceled) {
        return;
    }
    m_isCanceled = true;
    m_isFinished = false;
    fireEvent(m_target->starfish()->staticStrings()->m_cancel.localName());
}

void Animation::notifyRemoved()
{
    // A replaced animation stops applying its value but stays finished; only
    // the remove event reports the transition.
    fireEvent(m_target->starfish()->staticStrings()->m_remove.localName());
}

void Animation::fireEvent(String* eventType)
{
    EventInit init(false, false);
    Event* event = new Event(m_executionContext, eventType, init);
    // The event is delivered at idle time, so the animation may be canceled
    // between queueing and dispatch; a canceled animation must not report a
    // finish anymore.
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t handle, void* data0, void* data1) {
            Animation* animation = reinterpret_cast<Animation*>(data0);
            Event* event = reinterpret_cast<Event*>(data1);
            if (animation->m_isCanceled &&
                event->type()->equals(animation->m_target->starfish()
                                          ->staticStrings()
                                          ->m_finish.localName())) {
                return;
            }
            animation->dispatchEventByUA(animation, event);
        },
        this, event);
}
} // namespace Starfish
