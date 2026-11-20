/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/HTMLTitleElement.h"
#if defined(STARFISH_ENABLE_CDP)
#include "core/cdp/domains/TargetDomain.h"
#endif

namespace Starfish {

String* HTMLTitleElement::text()
{
    Optional<String*> value = textContent();
    if (value.hasValue()) {
        return value.getValue();
    }
    return String::emptyString;
}

void HTMLTitleElement::setText(String* title)
{
    setTextContent(title);
}

#if defined(STARFISH_ENABLE_CDP)
void HTMLTitleElement::didCharacterDataModified(String* before, String* after)
{
    HTMLElement::didCharacterDataModified(before, after);
    if (isInDocumentScope()) {
        notifyTargetInfoChanged(document());
    }
}

void HTMLTitleElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    if (isInDocumentScope()) {
        notifyTargetInfoChanged(document());
    }
}

void HTMLTitleElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    HTMLElement::didNodeRemoved(parent, oldChild);
    if (isInDocumentScope()) {
        notifyTargetInfoChanged(document());
    }
}

void HTMLTitleElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    notifyTargetInfoChanged(document());
}

void HTMLTitleElement::didNodeRemovedFromDocumentTree()
{
    HTMLElement::didNodeRemovedFromDocumentTree();
    notifyTargetInfoChanged(document());
}
#endif
} // namespace Starfish
