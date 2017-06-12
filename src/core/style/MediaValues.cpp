/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "core/style/MediaValues.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/layout/Frame.h"
#include "core/modules/window/Window.h"
#include "core/style/ComputedStyle.h"
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
    return m_frame->document()
        ->window()
        ->starFish()
        ->screenInfo()
        .rect.size()
        .width();
}

int32_t MediaValues::screenHeight() const
{
    // TODO: The current screen area is same as the viewport area.
    // The screen area for the device area should be considered later.
    return m_frame->document()
        ->window()
        ->starFish()
        ->screenInfo()
        .rect.size()
        .height();
}

float MediaValues::devicePixelRatio() const
{
    return m_frame->document()->window()->starFish()->devicePixelRatio();
}

int32_t MediaValues::colorBitsPerComponent() const
{
    return m_frame->document()
        ->window()
        ->starFish()
        ->screenInfo()
        .depthPerComponent;
}

bool MediaValues::isMonochrome() const
{
    return m_frame->document()->window()->starFish()->screenInfo().isMonochrome;
}

} /* namespace StarFish */
