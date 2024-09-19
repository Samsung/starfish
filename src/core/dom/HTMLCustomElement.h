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

#ifndef __StarfishHTMLCustomElement__
#define __StarfishHTMLCustomElement__

#include "core/dom/HTMLElement.h"

namespace Starfish {

class HTMLUnknownElement;
struct CustomElementRegistryData;

class HTMLCustomElement : public HTMLElement {
public:
    HTMLCustomElement(Document* document, const QualifiedName& qname,
                      CustomElementRegistryData* customElementRegistryData)
        : HTMLElement(document, qname)
        , m_customElementRegistryData(customElementRegistryData)
    {
    }

    virtual bool isHTMLCustomElement() const override
    {
        return true;
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
    virtual void didAttributeChanged(QualifiedName name, Nullable<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didNodeInsertedToDocumentTree() override;
    virtual void didNodeRemovedFromDocumentTree() override;
    virtual void didNodeAdopted(Document* oldDocument) override;

    CustomElementRegistryData* customElementRegistryData() const
    {
        return m_customElementRegistryData;
    }

private:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        HTMLElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLCustomElement,
                                        m_customElementRegistryData));
    }

    CustomElementRegistryData* m_customElementRegistryData;
};
} // namespace Starfish

#endif
