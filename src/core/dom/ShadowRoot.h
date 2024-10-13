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

#ifndef __StarfishShadowRoot__
#define __StarfishShadowRoot__

#include "core/dom/DocumentFragment.h"
#include "core/dom/ShadowRootInit.h"
#include "core/layout/Frame.h"

namespace Starfish {

class ShadowRoot : public DocumentFragment {
public:
    ShadowRoot(Document* document, ShadowRootMode mode, Element* host)
        : DocumentFragment(document)
        , m_mode(mode)
        , m_delegatesFocus(false)
        , m_slotAssignment(SlotAssignmentMode::Named)
        , m_clonable(false)
        , m_serializable(false)
        , m_availableToElementInternals(false)
        , m_declarative(false)
        , m_host(host)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isShadowRoot() const override;

    String* mode() const;
    ShadowRootMode modeEnum() const
    {
        return m_mode;
    }

    bool isClosed() const
    {
        return m_mode == ShadowRootMode::Closed;
    }
    bool isOpened() const
    {
        return m_mode == ShadowRootMode::Open;
    }

    bool delegatesFocus() const
    {
        return m_delegatesFocus;
    }

    void setDelegatesFocus(bool delegatesFocus)
    {
        m_delegatesFocus = delegatesFocus;
    }

    bool availableToElementInternals() const
    {
        return m_availableToElementInternals;
    }

    void setAvailableToElementInternals(bool availableToElementInternals)
    {
        m_availableToElementInternals = availableToElementInternals;
    }

    bool declarative() const
    {
        return m_declarative;
    }

    void setDeclarative(bool declarative)
    {
        m_declarative = declarative;
    }

    String* slotAssignment() const
    {
        if (m_slotAssignment == SlotAssignmentMode::Manual) {
            return String::createASCIIString("manual");
        } else {
            STARFISH_ASSERT(m_slotAssignment == SlotAssignmentMode::Named);
            return String::createASCIIString("named");
        }
    }

    SlotAssignmentMode slotAssignmentEnum() const
    {
        return m_slotAssignment;
    }

    void setSlotAssignment(SlotAssignmentMode slotAssignmentMode)
    {
        m_slotAssignment = slotAssignmentMode;
    }

    bool clonable() const
    {
        return m_clonable;
    }

    void setClonable(bool clonable)
    {
        m_clonable = clonable;
    }

    bool serializable() const
    {
        return m_serializable;
    }

    void setSerializable(bool serializable)
    {
        m_serializable = serializable;
    }

    Element* host() const
    {
        return m_host;
    }

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(slotchange);
#undef VIRTUAL
#undef OVERRIDE

private:
    ShadowRootMode m_mode : 8;
    bool m_delegatesFocus : 1;
    SlotAssignmentMode m_slotAssignment : 8;
    bool m_clonable : 1;
    bool m_serializable : 1;
    bool m_availableToElementInternals : 1;
    bool m_declarative : 1;
    Element* m_host;
};
} // namespace Starfish

#endif
