/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/Node.h"
#include "core/layout/Frame.h"
#include "core/style/Length.h"
#include "core/style/CalcData.h"
#include "core/page/Window.h"

namespace Starfish {

float Angle::specifiedValue() const
{
    STARFISH_ASSERT(isSpecified());

    if (isFixed()) {
        return fixed();
    } else if (isCalc()) {
        return calcData()->angleValue().toDegreeValue();
    }

    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}
} // namespace Starfish
