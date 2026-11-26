/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishHTMLAllCollection__
#define __StarfishHTMLAllCollection__

#include "binding/ScriptWrappable.h"
#include "binding/generated/HTMLCollectionOrElementUnion.h"

namespace Starfish {

class HTMLAllCollection : public ScriptWrappable {
public:
    explicit HTMLAllCollection(Document* document);
    void init(ScriptBindingInstance*, void*) override;
    void postInit(ScriptBindingInstance*) override;
    bool isHTMLAllCollection() const override;
    ScriptBindingInstance* scriptBindingInstance() override;

    size_t length() const;
    Optional<Element*> defaultIndexedGetter(uint32_t index);
    Optional<HTMLCollectionOrElement> namedItem(String* name);
    Optional<HTMLCollectionOrElement> item(String* nameOrIndex);
    Optional<HTMLCollectionOrElement> item()
    {
        return {};
    }

private:
    Document* m_document;
    HTMLCollection* m_elements;
};
} // namespace Starfish

#endif
