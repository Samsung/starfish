/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLButtonElement__
#define __StarFishHTMLButtonElement__

#include "core/dom/HTMLFormElement.h"

namespace StarFish {

class HTMLButtonElement : public HTMLFormControl {
public:
    HTMLButtonElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLButtonElement() const override;

    // 4.4 Interface Node

    virtual String* localName() override;
    virtual QualifiedName name() override;

    // 4.10.6 Interface Button
    String* type() override;

    // Other methods
    bool handleDefaultEvent(Event* event) override;
};
}

#endif
