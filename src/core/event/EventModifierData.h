/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
