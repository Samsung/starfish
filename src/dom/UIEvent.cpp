/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "dom/UIEvent.h"

namespace StarFish {

UIEventInit::UIEventInit()
    : UIEventInit(false)
{
}

UIEventInit::UIEventInit(bool bubbles)
    : UIEventInit(bubbles, false)
{
}

UIEventInit::UIEventInit(bool bubbles, bool cancelable)
    : EventInit(bubbles, cancelable)
    , m_view(nullptr)
    , m_detail(false)
{
}

Window* UIEventInit::view() const
{
    return m_view;
}

void UIEventInit::setView(Window* view)
{
    m_view = view;
}

int32_t UIEventInit::detail() const
{
    return m_detail;
}

void UIEventInit::setDetail(int32_t detail)
{
    m_detail = detail;
}

EventModifierInit::EventModifierInit()
    : EventModifierInit(false)
{
}

EventModifierInit::EventModifierInit(bool bubbles)
    : EventModifierInit(bubbles, false)
{
}

EventModifierInit::EventModifierInit(bool bubbles, bool cancelable)
    : UIEventInit(bubbles, cancelable)
    , m_ctrlKey(false)
    , m_shiftKey(false)
    , m_altKey(false)
    , m_metaKey(false)
    , m_modifierAltGraph(false)
    , m_modifierCapsLock(false)
    , m_modifierFn(false)
    , m_modifierFnLock(false)
    , m_modifierHyper(false)
    , m_modifierNumLock(false)
    , m_modifierScrollLock(false)
    , m_modifierSuper(false)
    , m_modifierSymbol(false)
    , m_modifierSymbolLock(false)
{
}

bool EventModifierInit::ctrlKey() const
{
    return m_ctrlKey;
}

void EventModifierInit::setCtrlKey(bool ctrlKey)
{
    m_ctrlKey = ctrlKey;
}

bool EventModifierInit::shiftKey() const
{
    return m_shiftKey;
}

void EventModifierInit::setShiftKey(bool shiftKey)
{
    m_shiftKey = shiftKey;
}

bool EventModifierInit::altKey() const
{
    return m_altKey;
}

void EventModifierInit::setAltKey(bool altKey)
{
    m_altKey = altKey;
}

bool EventModifierInit::metaKey() const
{
    return m_metaKey;
}

void EventModifierInit::setMetaKey(bool metaKey)
{
    m_metaKey = metaKey;
}

bool EventModifierInit::modifierAltGraph() const
{
    return m_modifierAltGraph;
}

void EventModifierInit::setModifierAltGraph(bool modifierAltGraph)
{
    m_modifierAltGraph = modifierAltGraph;
}

bool EventModifierInit::modifierCapsLock() const
{
    return m_modifierCapsLock;
}

void EventModifierInit::setModifierCapsLock(bool modifierCapsLock)
{
    m_modifierCapsLock = modifierCapsLock;
}

bool EventModifierInit::modifierFn() const
{
    return m_modifierFn;
}

void EventModifierInit::setModifierFn(bool modifierFn)
{
    m_modifierFn = modifierFn;
}

bool EventModifierInit::modifierFnLock() const
{
    return m_modifierFnLock;
}

void EventModifierInit::setModifierFnLock(bool modifierFnLock)
{
    m_modifierFnLock = modifierFnLock;
}

bool EventModifierInit::modifierHyper() const
{
    return m_modifierHyper;
}

void EventModifierInit::setModifierHyper(bool modifierHyper)
{
    m_modifierHyper = modifierHyper;
}

bool EventModifierInit::modifierNumLock() const
{
    return m_modifierNumLock;
}

void EventModifierInit::setModifierNumLock(bool modifierNumLock)
{
    m_modifierNumLock = modifierNumLock;
}

bool EventModifierInit::modifierScrollLock() const
{
    return m_modifierScrollLock;
}

void EventModifierInit::setModifierScrollLock(bool modifierScrollLock)
{
    m_modifierScrollLock = modifierScrollLock;
}

bool EventModifierInit::modifierSuper() const
{
    return m_modifierSuper;
}

void EventModifierInit::setModifierSuper(bool modifierSuper)
{
    m_modifierSuper = modifierSuper;
}

bool EventModifierInit::modifierSymbol() const
{
    return m_modifierSymbol;
}

void EventModifierInit::setModifierSymbol(bool modifierSymbol)
{
    m_modifierSymbol = modifierSymbol;
}

bool EventModifierInit::modifierSymbolLock() const
{
    return m_modifierSymbolLock;
}

void EventModifierInit::setModifierSymbolLock(bool modifierSymbolLock)
{
    m_modifierSymbolLock = modifierSymbolLock;
}
}
