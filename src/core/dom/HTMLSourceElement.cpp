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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/HTMLSourceElement.h"

namespace StarFish {

QualifiedName HTMLSourceElement::name()
{
    return starFish()->staticStrings()->m_sourceTagName;
}

String* HTMLSourceElement::src()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_src);
}

String* HTMLSourceElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLSourceElement::setSrc(String* src)
{
    setAttribute(starFish()->staticStrings()->m_src, src);
}

void HTMLSourceElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}
}

#endif
