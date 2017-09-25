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
#include "core/modules/canvas/font/Font.h"
#include "core/style/Length.h"
#include "core/style/CalcData.h"

namespace StarFish {
void Length::changeToFixedIfNeeded(Length curFontSize, Length rootFontSize,
                                   Font* font)
{
    if (!isComputed()) {
        if (m_type == Em) {
            float fSize = curFontSize.isFixed() ? curFontSize.fixed() : 0.0f;
            m_data = fSize * m_data.m_numberData;
            m_type = Fixed;
        } else if (m_type == Ex) {
            float fSize = curFontSize.isFixed() ? curFontSize.fixed() : 0.0f;
            m_data =
                fSize * m_data.m_numberData * font->metrics().m_xheightRate;
            m_type = Fixed;
        } else if (m_type == Rem) {
            float fSize = rootFontSize.isFixed() ? rootFontSize.fixed() : 0.0f;
            m_data = fSize * m_data.m_numberData;
            m_type = Fixed;
        } else if (m_type == Calc) {
            GCVector<CalcTerm*>& data = m_data.m_calcData->terms();
            auto iter = data.begin();

            while (iter != data.end()) {
                GCVector<CalcValue>& data2 = (*iter)->values();
                auto iter2 = data2.begin();
                while (iter2 != data2.end()) {
                    CalcValue& v = *iter2;
                    if (v.type().isLength()) {
                        Length l = v.lengthValue().toLength();
                        if (!l.isComputed()) {
                            l.changeToFixedIfNeeded(curFontSize, rootFontSize,
                                                    font);
                            v.setValue(CSSLength(CSSLength::PX, l.fixed()));
                        }
                    }
                    iter2++;
                }
                iter++;
            }
        }
        // InheritableNumber does not change its value
    }
}

bool Length::isCalcAndLengthOfType() const
{
    return m_type == Calc && m_data.m_calcData->type().isLength();
}

float Length::specifiedValue(LayoutUnit parentLength, LayoutUnit viewportWidth,
                             LayoutUnit viewportHeight) const
{
    STARFISH_ASSERT(isSpecified());
    if (isFixed()) {
        return fixed();
    } else if (isViewportPercent()) {
        return viewportPercentValue(viewportWidth, viewportHeight);
    } else if (isPercent()) {
        return percentValue(parentLength);
    } else {
        return m_data.m_calcData->specifiedValue(parentLength, viewportWidth,
                                                 viewportHeight);
    }
}

bool Length::operator==(const Length& src) const
{
    if (m_type != src.m_type) {
        return false;
    }

    if (m_type == Calc) {
        return m_data.m_calcData->toString()->equals(
            src.m_data.m_calcData->toString());
    } else {
        return m_data.m_numberData == src.m_data.m_numberData;
    }
}

String* Length::dumpString()
{
    if (isCalc()) {
        return m_data.m_calcData->toString();
    }

    char temp[100];
    if (isFixed()) {
        snprintf(temp, sizeof(temp), "%.1f", fixed());
    } else if (isPercent()) {
        snprintf(temp, sizeof(temp), "%.1f%%", percent());
    } else if (isViewportPercent()) {
        if (m_type == Vw) {
            snprintf(temp, sizeof(temp), "%.1fvw", viewportPercent());
        } else if (m_type == Vh) {
            snprintf(temp, sizeof(temp), "%.1fvh", viewportPercent());
        } else if (m_type == Vmin) {
            snprintf(temp, sizeof(temp), "%.1fvmin", viewportPercent());
        } else if (m_type == Vmax) {
            snprintf(temp, sizeof(temp), "%.1fvmax", viewportPercent());
        }
    } else if (isAuto()) {
        snprintf(temp, sizeof(temp), "auto");
    } else if (isInheritableNumber()) {
        snprintf(temp, sizeof(temp), "%.1f(num)", number());
    }
    return String::fromUTF8(temp);
}

void LengthSize::checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font)
{
    m_width.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_height.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
}

void LengthPosition::checkComputed(Length curFontSize, Length rootFontSize,
                                   Font* font)
{
    m_x.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_y.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
}

void LengthBox::checkComputed(Length curFontSize, Length rootFontSize,
                              Font* font)
{
    m_left.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_right.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_top.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_bottom.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
}
}
