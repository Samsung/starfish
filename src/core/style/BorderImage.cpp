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
#include "core/style/BorderImage.h"
#include "core/style/CalcData.h"
#include "platform/loader/ImageResource.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

void BorderImageLength::checkComputed(Length curFontSize, Length rootFontSize,
                                      Font* font, LayoutSize windowSize,
                                      ComputedStyle* cs)
{
    if (!isLength()) {
        return;
    }
    if (!m_length.isCalc()) {
        m_length.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                       windowSize.width(), windowSize.height(),
                                       cs);
        return;
    }

    auto* calc = m_length.calcData();
    if (calc->calcValueType().isNumber()) {
        *this = BorderImageLength(static_cast<double>(calc->numberValue()));
    } else {
        // Compute the absolute length terms even when a percentage keeps the
        // expression unresolved until the used value (css-values-4).
        m_length = calc->computedLength(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        cs, true);
    }
}

NativeImageData* BorderImageData::imageData()
{
    if (!isNull() && data()->m_imageResource) {
        return data()->m_imageResource->imageData();
    }
    return NULL;
}
} // namespace Starfish
