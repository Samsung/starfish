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
#include "platform/loader/ImageResource.h"
#include "core/style/StyleBackgroundData.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/style/GradientData.h"

namespace StarFish {

NativeImageData* BackgroundLayer::imageData() const
{
    if (m_imageResource) {
        return m_imageResource->imageData();
    }
    return nullptr;
}

void BackgroundLayer::checkComputed(Length curFontSize, Length rootFontSize,
                                    Font* font, LayoutSize windowSize,
                                    ComputedStyle* cs)
{
    if (m_image && m_image->type().isURL()) {
        if (m_size.hasLengthValue()) {
            if (m_size.m_lengthValue) {
                m_size.m_lengthValue->checkComputed(curFontSize, rootFontSize,
                                                    font, windowSize, cs);
            }
        }

        m_positionX.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                          windowSize.width(),
                                          windowSize.height(), cs);
        m_positionY.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                          windowSize.width(),
                                          windowSize.height(), cs);
    } else if (m_image && m_image->type().isGradient()) {
        m_image->gradientValue()->checkComputed(curFontSize, rootFontSize, font,
                                                windowSize, cs);
    }
}
}
