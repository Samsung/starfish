/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "platform/window/Window.h"
#include "animation/Animation.h"
#include "dom/Node.h"
#include "dom/Document.h"

namespace StarFish {

// This function returns current time
// * return unit: millisecond
static size_t getCurrentMillisecond()
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
}

AnimationTask::AnimationTask(Node* target,
                             CSSStyleValuePair::KeyKind targetProperty,
                             AnimatedValue from, AnimatedValue to,
                             float durationS, float delayS,
                             CubicBeizer* cubicBezier)
{
    m_isExpired = false;
    m_targetElement = target;
    m_durationMs = durationS * 1000;
    m_delayMs = delayS * 1000;
    m_startTimeMs = getCurrentMillisecond() + m_delayMs;
    m_fromValue = from;
    m_toValue = to;
    m_property = targetProperty;
    m_cubicBezier = cubicBezier;
}

// This function update last execution time.
// * This function is called before calling step() function.
void AnimationTask::update()
{
    m_lastModifiedTimeMs = getCurrentMillisecond();
}

// This function returns false if
// * Current AnimationTask is expired, OR
// * Minimum period(THRESHOLD_TICK,10ms) has not passed yet
// * This function is called when we get tick from ecore_animator.
bool AnimationTask::canExecute()
{
    if (isExpired()) {
        return false;
    }
    size_t currentTime = getCurrentMillisecond();
    if ((currentTime >= m_startTimeMs)) {
        if (m_lastModifiedTimeMs == 0 ||
            ((currentTime - m_lastModifiedTimeMs) > THRESHOLD_TICK)) {
            update();
            return true;
        }
    }
    return false;
}

// This function calculte progress value to get intermediate value of animation.
// * returnVal range : 0-1
float AnimationTask::progress()
{
    if (m_durationMs == 0 || isExpired()) {
        return 100.0;
    }
    float result =
        (m_lastModifiedTimeMs - m_startTimeMs) / ((float)m_durationMs);
    if (result >= 1) {
        result = 1;
        m_isExpired = true;
    }
    result = m_cubicBezier->getValue(result);
    return result;
}

// This function change computed style of target node.
// * After this function, NeedsPainting flag will be set.
void ColorAnimationTask::execute()
{
    Color from = m_fromValue.getColor();
    Color to = m_toValue.getColor();

    Node* current = node();
    ComputedStyle* style = current->style();
    float tmp_progress = progress();

    // It is arbitrary logic which I made for testing
    unsigned char r = from.r() * (1 - tmp_progress) + to.r() * tmp_progress;
    unsigned char g = from.g() * (1 - tmp_progress) + to.g() * tmp_progress;
    unsigned char b = from.b() * (1 - tmp_progress) + to.b() * tmp_progress;
    unsigned char a = from.a();

    // TODO : More types should be supported
    if (m_property == CSSStyleValuePair::KeyKind::BackgroundColor) {
        style->setBackgroundColor(Color(r, g, b, a));
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    current->setNeedsPainting();
}

// This function change computed style of target node.
// * After this function, NeedsLayout flag will be set.
void LengthAnimationTask::execute()
{
    Length from = m_fromValue.getLength();
    Length to = m_toValue.getLength();

    Node* current = node();
    ComputedStyle* style = current->style();
    float tmp_progress = progress();
    float newLength;
    if (from.fixed() < to.fixed()) {
        newLength = from.fixed() + (to.fixed() - from.fixed()) * tmp_progress;
    } else {
        newLength = from.fixed() - (from.fixed() - to.fixed()) * tmp_progress;
    }

    // TODO : More types should be supported
    if (m_property == CSSStyleValuePair::KeyKind::Width) {
        style->setWidth(Length(Length::Fixed, newLength));
    } else if (m_property == CSSStyleValuePair::KeyKind::Height) {
        style->setHeight(Length(Length::Fixed, newLength));
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    current->setNeedsLayout();
}

// [NOTICE]
// registerAnimation will be replaced 'createAnimation'
// Creation of AnimationTask will happen in Animation Executor.
void AnimationExecutor::registerAnimation(AnimationTask* newTask)
{
    startIfNeeds();
    cancelPreviousAnimation(newTask->node(), newTask->propertyType());
    m_animationList.push_back(newTask);
}

// This function cancel previous animation
// which is related taget node and its property.
// * This function is called when we need new animation.
void AnimationExecutor::cancelPreviousAnimation(
    Node* target, CSSStyleValuePair::KeyKind cssType)
{
    // TODO : Need optimization
    m_animationList.erase(
        std::remove_if(m_animationList.begin(), m_animationList.end(),
                       [&target, cssType](AnimationTask* current) {
                           if (current->node() == target &&
                               current->propertyType() == cssType) {
                               return true;
                           }
                           return false;
                       }),
        m_animationList.end());
}

// This function clear All animation which is related target node.
void AnimationExecutor::cancelAnimation(Node* target)
{
    m_animationList.erase(std::remove_if(m_animationList.begin(),
                                         m_animationList.end(),
                                         [&target](AnimationTask* current) {
                                             if (current->node() == target) {
                                                 return true;
                                             }
                                             return false;
                                         }),
                          m_animationList.end());
}

// [NOTICE]
// * Currently, Its behavior hardly coupled with ecore_animator
// * I believe it's best choice at this moment.
// * We do not have a sophisticated solution that surpasses this
void AnimationExecutor::startIfNeeds()
{
    if (m_isAlive && m_platformAnimator) {
        return;
    }
    m_isAlive = true;
    m_platformAnimator = ecore_animator_add(
        [](void* user_data) -> Eina_Bool {
            AnimationExecutor* executor = (AnimationExecutor*)user_data;
            if (executor->isAlive()) {
                executor->step();
                return ECORE_CALLBACK_RENEW;
            }
            return ECORE_CALLBACK_CANCEL;
        },
        this);
}

void AnimationExecutor::stop()
{
    if (!m_isAlive) {
        return;
    }
    m_isAlive = false;
    if (m_platformAnimator) {
        ecore_animator_del(m_platformAnimator);
        m_platformAnimator = nullptr;
    }
}

void AnimationExecutor::stopIfNeeds()
{
    if (m_animationList.size() == 0) {
        stop();
    }
}

// [NOTICE]
// * Basically, Do execute every task at one tick.
// * In reality, there will not be many works to be done(means number of
// parallel animations)
// * And Calculating and Changing computed style is pretty light work.
// * If we spent a lot of time during other stuff, Ecore_main_loop will adjust
// next execution.
void AnimationExecutor::step()
{
    STARFISH_ASSERT(m_isAlive);
    for (auto it = m_animationList.begin(); it != m_animationList.end();) {
        if ((*it)->isExpired()) {
            it = m_animationList.erase(it);
        } else {
            if ((*it)->canExecute()) {
                (*it)->execute();
            }
            it++;
        }
    }
    stopIfNeeds();
}
}
