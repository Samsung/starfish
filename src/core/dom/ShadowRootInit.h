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

#ifndef __StarfishShadowRootInit__
#define __StarfishShadowRootInit__

namespace Starfish {

enum class ShadowRootMode { Open, Closed };
enum class SlotAssignmentMode { Manual, Named };

struct ShadowRootInit {
    ShadowRootInit()
    {
    }

    void setMode(String* mode)
    {
        if (mode->equals("open")) {
            m_mode = ShadowRootMode::Open;
        } else {
            STARFISH_ASSERT(mode->equals("closed"));
            m_mode = ShadowRootMode::Closed;
        }
    }

    String* mode() const
    {
        if (m_mode == ShadowRootMode::Open) {
            return String::createASCIIString("open");
        } else {
            return String::createASCIIString("closed");
        }
    }

    void setSlotAssignment(String* slotAssignment)
    {
        if (slotAssignment->equals("manual")) {
            m_slotAssignment = SlotAssignmentMode::Manual;
        } else {
            STARFISH_ASSERT(slotAssignment->equals("named"));
            m_slotAssignment = SlotAssignmentMode::Named;
        }
    }

    String* slotAssignment() const
    {
        if (m_slotAssignment == SlotAssignmentMode::Manual) {
            return String::createASCIIString("manual");
        } else {
            return String::createASCIIString("named");
        }
    }

    void setDelegatesFocus(bool delegatesFocus)
    {
        m_delegatesFocus = delegatesFocus;
    }

    bool delegatesFocus() const
    {
        return m_delegatesFocus;
    }

    void setClonable(bool clonable)
    {
        m_clonable = clonable;
    }

    bool clonable() const
    {
        return m_clonable;
    }

    void setSerializable(bool serializable)
    {
        m_serializable = serializable;
    }

    bool serializable() const
    {
        return m_serializable;
    }

    ShadowRootMode m_mode = ShadowRootMode::Open;
    bool m_delegatesFocus = false;
    SlotAssignmentMode m_slotAssignment = SlotAssignmentMode::Named;
    bool m_clonable = false;
    bool m_serializable = false;
};

} // namespace Starfish

#endif
