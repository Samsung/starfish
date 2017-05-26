/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "core/style/MediaValues.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/layout/Frame.h"
#include "core/modules/window/Window.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

// TODO: We should check again whether the existing code is available or not for
//       the computation functions below.

// These conversions are defined in css-values
const double cssPixelsPerInch = 96;
const double cssPixelsPerCentimeter = cssPixelsPerInch / 2.54; // 2.54 cm/in
const double cssPixelsPerMillimeter = cssPixelsPerCentimeter / 10;
const double cssPixelsPerPoint = cssPixelsPerInch / 72;
const double cssPixelsPerPica = cssPixelsPerInch / 6;

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
        result = value * cssPixelsPerCentimeter;
        return true;
    case UnitType::Millimeters:
        result = value * cssPixelsPerMillimeter;
        return true;
    case UnitType::Inches:
        result = value * cssPixelsPerInch;
        return true;
    case UnitType::Points:
        result = value * cssPixelsPerPoint;
        return true;
    case UnitType::Picas:
        result = value * cssPixelsPerPica;
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

} /* namespace StarFish */
