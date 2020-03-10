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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/HTMLElement.h"

namespace Starfish {

String* DocumentFragment::nodeName()
{
    return starfish()->staticStrings()->m_documentFragmentLocalName.string();
}

Node* DocumentFragment::clone()
{
    DocumentFragment* newNode = new DocumentFragment(document());
    return newNode;
}

Element* DocumentFragment::getElementById(String* id)
{
    if (id->length() == 0) {
        return nullptr;
    }

    AtomicString aid = AtomicString::createAtomicString(starfish(), id);
    return (Element*)Traverse::findDescendant(this, [&](Node* child) {
        if (child->isElement() && child->asElement()->atomicId() == aid) {
            return true;
        } else {
            return false;
        }
    });
}
}
