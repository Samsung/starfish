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

#ifndef __StarFishHTMLMetaElement__
#define __StarFishHTMLMetaElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLMetaElement : public HTMLElement {
public:
    HTMLMetaElement(Document* document)
        : HTMLElement(document)
        , m_name(String::emptyString)
        , m_content(String::emptyString)
        , m_httpEquiv(String::emptyString)
#ifdef STARFISH_TIZEN
        , m_tizenWidgetTransparentBackground(false)
#endif
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLMetaElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

    /* Other methods (not in DOM API) */

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didNodeInsertedToDocumentTree() override;
    virtual void didNodeRemovedFromDocumentTree() override;
    void checkPlatformFlags();

protected:
    String* m_name;
    String* m_content;
    String* m_httpEquiv;

#ifdef STARFISH_TIZEN
    bool m_tizenWidgetTransparentBackground;
#endif
};
}

#endif
