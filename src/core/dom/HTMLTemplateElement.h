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

#ifndef __StarfishHTMLTemplateElement__
#define __StarfishHTMLTemplateElement__

#include "core/dom/HTMLElement.h"

namespace Starfish {

class HTMLTemplateElement : public HTMLElement {
public:
    HTMLTemplateElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTemplateElement() const override;

    DocumentFragment* content() const
    {
        return m_content;
    }

    String* shadowRootMode();
    void setShadowRootMode(String* s);

    virtual void finishParsing() override;

private:
    DocumentFragment* m_content;
};
} // namespace Starfish

#endif
