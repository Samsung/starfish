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

#ifndef __StarFishHTMLFormControlsCollection__
#define __StarFishHTMLFormControlsCollection__

#include "binding/ScriptWrappable.h"
#include "core/dom/HTMLCollection.h"

namespace StarFish {

class Node;

class HTMLFormControlsCollection : public HTMLCollection {
public:
    HTMLFormControlsCollection(Node* root,
                               NodeListImpl::FilterFunctionType filterType);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLFormControlsCollection() const;

    // Note : inherits length and item()

    virtual Element* namedItem(String* name) override;
};
}
#endif
