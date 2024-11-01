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

class HTMLSlotElement;
class StyleResolver;

class SlotAssignment : public gc {
public:
private:
};

class ShadowRoot : public DocumentFragment {
public:
    ShadowRoot(Document* document, ShadowRootMode mode, Element* host);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isShadowRoot() const override;

    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didNodeRemoved(Node* parent, Node* oldChild) override;

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
        if (m_slotAssignmentEnum == SlotAssignmentMode::Manual) {
            return String::createASCIIString("manual");
        } else {
            STARFISH_ASSERT(m_slotAssignmentEnum == SlotAssignmentMode::Named);
            return String::createASCIIString("named");
        }
    }

    SlotAssignmentMode slotAssignmentEnum() const
    {
        return m_slotAssignmentEnum;
    }

    void setSlotAssignment(SlotAssignmentMode slotAssignmentMode)
    {
        m_slotAssignmentEnum = slotAssignmentMode;
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

    StyleResolver& styleResolver()
    {
        return *m_styleResolver;
    }

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(slotchange);
#undef VIRTUAL
#undef OVERRIDE

    Optional<HTMLSlotElement*> assignedSlot(String* name);
    void assignSlot();
    void connectSlotWithSlottables();
    void updateSlotElements(bool shouldConnectSlotWithSlottables = true);

private:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        DocumentFragment::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(ShadowRoot, m_host));
        GC_set_bit(desc, GC_WORD_OFFSET(ShadowRoot, m_styleResolver));
        markHashTable(desc, GC_WORD_OFFSET(ShadowRoot, m_namedSlotElements));
    }

    ShadowRootMode m_mode : 8;
    bool m_delegatesFocus : 1;
    SlotAssignmentMode m_slotAssignmentEnum : 8;
    bool m_clonable : 1;
    bool m_serializable : 1;
    bool m_availableToElementInternals : 1;
    bool m_declarative : 1;
    Element* m_host;
    GCUnorderedMap<String*, HTMLSlotElement*> m_namedSlotElements;
    StyleResolver* m_styleResolver;
};
} // namespace Starfish

#endif
