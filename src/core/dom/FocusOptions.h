/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFocusOptions__
#define __StarfishFocusOptions__

namespace Starfish {

struct FocusOptions {
    FocusOptions()
    {
    }

    void setPreventScroll(bool v)
    {
        m_preventScroll = v;
    }

    bool preventScroll() const
    {
        return m_preventScroll;
    }

    void setFocusVisible(bool v)
    {
        m_focusVisible = v;
    }

    bool focusVisible() const
    {
        return m_focusVisible;
    }

    bool m_preventScroll = false;
    bool m_focusVisible = true;
};

} // namespace Starfish

#endif
