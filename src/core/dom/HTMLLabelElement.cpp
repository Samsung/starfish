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

#include "StarFishConfig.h"
#include "StarFish.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/Traverse.h"
#include "core/dom/HTMLLabelElement.h"

namespace StarFish {

QualifiedName HTMLLabelElement::name()
{
    return starFish()->staticStrings()->m_labelTagName;
}

HTMLFormElement* HTMLLabelElement::form()
{
    HTMLElement* element = control();
    if (!element) {
        return nullptr;
    }

    return element->asHTMLFormControl()->form();
}

HTMLElement* HTMLLabelElement::control()
{
    String* id = getAttributeOrEmpty(starFish()->staticStrings()->m_for);
    if (id->equals(String::emptyString)) {
        return (HTMLElement*)Traverse::findDescendant(this, [&](Node* child) {
            if (child->isLabelable()) {
                return true;
            } else {
                return false;
            }
        });
    }

    if (Element* control = document()->getElementById(id)) {
        if (control->isLabelable()) {
            return (HTMLElement*)control;
        }
    }

    return nullptr;
}
}
