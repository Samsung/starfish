/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishEventModifierData__
#define __StarFishEventModifierData__

namespace StarFish {

class EventModifierData {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    EventModifierData()
        : m_ctrlKey(false)
        , m_shiftKey(false)
        , m_altKey(false)
        , m_metaKey(false)
    {
    }

    bool ctrlKey() const
    {
        return m_ctrlKey;
    }

    void setCtrlKey(bool ctrlKey)
    {
        m_ctrlKey = ctrlKey;
    }

    bool shiftKey() const
    {
        return m_shiftKey;
    }

    void setShiftKey(bool shiftKey)
    {
        m_shiftKey = shiftKey;
    }

    bool altKey() const
    {
        return m_altKey;
    }

    void setAltKey(bool altKey)
    {
        m_altKey = altKey;
    }

    bool metaKey() const
    {
        return m_metaKey;
    }

    void setMetaKey(bool metaKey)
    {
        m_metaKey = metaKey;
    }

private:
    bool m_ctrlKey;
    bool m_shiftKey;
    bool m_altKey;
    bool m_metaKey;

    // TODO Implement
    // bool m_modifierAltGraph;
    // bool m_modifierCapsLock;
    // bool m_modifierFn;
    // bool m_modifierFnLock;
    // bool m_modifierHyper;
    // bool m_modifierNumLock;
    // bool m_modifierScrollLock;
    // bool m_modifierSuper;
    // bool m_modifierSymbol;
    // bool m_modifierSymbolLock;
};

} // namespace StarFish
#endif
