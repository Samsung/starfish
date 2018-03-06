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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

QualifiedName HTMLBodyElement::name()
{
    return starFish()->staticStrings()->m_bodyTagName;
}

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, load);

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, message);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, messageerror);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, unload);

void HTMLBodyElement::didComputedStyleChanged(ComputedStyle* oldStyle,
                                              ComputedStyle* newStyle)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
    if (!newStyle->backgroundColor().isTransparent() ||
        !newStyle->backgroundImage()->equals(String::emptyString)) {
        document()->browsingContext()->m_hasBodyElementBackground = true;
    } else {
        document()->browsingContext()->m_hasBodyElementBackground = false;
    }
}

void HTMLBodyElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();
    if (name == ss->m_onload) {
        window()->setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onunload) {
        window()->setAttributeEventListener(ss->m_unload, value, this);
    }
}
}
