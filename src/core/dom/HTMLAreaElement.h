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

#ifndef __StarFishHTMLAreaElement__
#define __StarFishHTMLAreaElement__

#include "core/dom/HTMLHyperlinkContainer.h"

namespace StarFish {

class HTMLAreaElement : public HTMLHyperlinkContainer {
public:
    HTMLAreaElement(Document* document)
        : HTMLHyperlinkContainer(document)
        , m_relList(nullptr)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLAreaElement() const;

    DOMTokenList* relList();

    /* 4.4 Interface Node */
    virtual QualifiedName name();

private:
    DOMTokenList* m_relList;
};
}
#endif
