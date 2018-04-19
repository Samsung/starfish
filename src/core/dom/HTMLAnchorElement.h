/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLAnchorElement__
#define __StarFishHTMLAnchorElement__

#include "core/dom/HTMLHyperlinkContainer.h"

namespace StarFish {

class HTMLAnchorElement : public HTMLHyperlinkContainer {
public:
    HTMLAnchorElement(Document* document)
        : HTMLHyperlinkContainer(document)
        , m_relList(nullptr)
    {
        m_tabIndexWasSetExplicitly = true;
        m_tabIndex = 0;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLAnchorElement() const;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* val, bool attributeCreated,
                                     bool attributeRemoved) override;

    DOMTokenList* relList();

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    /* Other methods (not in DOM API) */
    virtual bool handleDefaultEvent(Event* event) override;

    bool supportsFocus() const override;

private:
    DOMTokenList* m_relList;
};
}

#endif
