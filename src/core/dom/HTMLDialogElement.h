/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishHTMLDialogElement__
#define __StarfishHTMLDialogElement__

#include "core/dom/HTMLElement.h"

namespace Starfish {

class HTMLDialogElement : public HTMLElement {
public:
    HTMLDialogElement(Document* document, const QualifiedName& qname)
        : HTMLElement(document, qname)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLDialogElement() const override;

    bool open();
    void setOpen(bool open);
    DEFINE_GETTER_SETTER(String*, returnValue, ReturnValue);

    void show();
    void showModal();
    void close(String* returnValue = String::emptyString);

    bool isInShowModal()
    {
        return m_isInShowModal;
    }

    void didAttributeChanged(QualifiedName name, Nullable<String*> old,
                             String* value, bool attributeCreated,
                             bool attributeRemoved) override;

private:
    String* m_returnValue{ String::emptyString };
    bool m_isInShowModal{ false };
};
} // namespace Starfish
#endif
