/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/dom/CSS.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSParser.h"
#include "core/style/StyleRule.h"

namespace StarFish {
bool CSS::supports(Document* document, String* property, String* value)
{
    StringBuilder builder;
    builder.appendString(property);
    builder.appendChar(':');
    builder.appendString(value);
    auto condition = builder.finalize();

    return supports(document, condition);
}

bool CSS::supports(Document* document, String* conditionText)
{
    CSSParser parser(document);

    return parser.parseSupportCondition(conditionText);
}
}
