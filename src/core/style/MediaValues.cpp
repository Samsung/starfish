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
#include "core/layout/Frame.h"
#include "core/page/Screen.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"
#include "core/style/MediaValues.h"
#include "core/style/UnitHelper.h"

namespace StarFish {

// TODO: We are considering continuous media only.
// If we support paged media in the future, we should also consider this.
// TODO: We should check again whether the existing code is available or not for
// the computation functions below.

float defaultFontSize(Frame* frame)
{
    return frame->document()
               ->window()
               ->starFish()
               ->defaultFontSizeMultiplier() *
           DEFAULT_FONT_SIZE;
}

bool computeLengthImpl(double value, UnitType type, float defaultFontSize,
                       double viewportWidth, double viewportHeight,
                       double& result)
{
    switch (type) {
    case UnitType::Ems:
    case UnitType::Rems:
        result = value * defaultFontSize;
        return true;
    case UnitType::Pixels:
    case UnitType::UserUnits:
        result = value;
        return true;
    case UnitType::Exs:
    case UnitType::Chs:
        result = (value * defaultFontSize) / 2.0;
        return true;
    case UnitType::ViewportWidth:
        result = (value * viewportWidth) / 100.0;
        return true;
    case UnitType::ViewportHeight:
        result = (value * viewportHeight) / 100.0;
        return true;
    case UnitType::ViewportMin:
        result = (value * std::min(viewportWidth, viewportHeight)) / 100.0;
        return true;
    case UnitType::ViewportMax:
        result = (value * std::max(viewportWidth, viewportHeight)) / 100.0;
        return true;
    case UnitType::Centimeters:
        result = value * unitPxPerCm;
        return true;
    case UnitType::Millimeters:
        result = value * unitPxPerMm;
        return true;
    case UnitType::Inches:
        result = value * unitPxPerIn;
        return true;
    case UnitType::Points:
        result = value * unitPxPerPt;
        return true;
    case UnitType::Picas:
        result = value * unitPxPerPc;
        return true;
    default:
        return false;
    }
}

bool MediaValues::computeLength(double value, UnitType type, double& result)
{
    return computeLength(value, type, defaultFontSize(m_frame), viewportWidth(),
                         viewportHeight(), result);
}

bool MediaValues::computeLength(double value, UnitType type,
                                float defaultFontSize, double viewportWidth,
                                double viewportHeight, double& result)
{
    double res;
    if (!computeLengthImpl(value, type, defaultFontSize, viewportWidth,
                           viewportHeight, res)) {
        return false;
    }
    result = clampTo<double>(res);
    return true;
}

int32_t MediaValues::viewportWidth() const
{
    return m_frame->document()->window()->innerWidth();
}

int32_t MediaValues::viewportHeight() const
{
    return m_frame->document()->window()->innerHeight();
}

int32_t MediaValues::screenWidth() const
{
    // TODO: The current screen area is same as the viewport area.
    // The screen area for the device area should be considered later.
    return m_frame->document()->window()->screen()->width();
}

int32_t MediaValues::screenHeight() const
{
    // TODO: The current screen area is same as the viewport area.
    // The screen area for the device area should be considered later.
    return m_frame->document()->window()->screen()->height();
}

float MediaValues::devicePixelRatio() const
{
    return m_frame->document()->window()->screen()->devicePixelRatio();
}

int32_t MediaValues::colorBitsPerComponent() const
{
    // The colorDepth and pixelDepth attributes should return the number of bits
    // allocated to colors for a pixel in the output device, excluding the alpha
    // channel.
    return m_frame->document()->window()->screen()->pixelDepth() / 3;
}

bool MediaValues::isMonochrome() const
{
    return m_frame->document()->window()->screen()->isMonochrome();
}

bool MediaValues::hasScriptEngineInstance() const
{
    return m_frame->document()->webView()->scriptEngineInstance();
}

} /* namespace StarFish */
