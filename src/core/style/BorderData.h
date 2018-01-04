/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishBorderData__
#define __StarFishBorderData__

#include "core/style/BorderValue.h"
#include "core/style/BorderImage.h"

namespace StarFish {

class ComputedStyle;

class BorderData : public gc {
public:
    BorderData()
    {
    }

    bool hasBorderStyle()
    {
        return m_top.hasBorderStyle() || m_right.hasBorderStyle() ||
               m_bottom.hasBorderStyle() || m_left.hasBorderStyle();
    }

    bool hasBorderImageData()
    {
        return m_image.imageData() != NULL;
    }

    BorderValue& top()
    {
        return m_top;
    }

    BorderValue& right()
    {
        return m_right;
    }

    BorderValue& bottom()
    {
        return m_bottom;
    }

    BorderValue& left()
    {
        return m_left;
    }

    BorderImage& image()
    {
        return m_image;
    }

    void makeZeroWidth()
    {
        m_left.setWidth(Length(Length::Fixed, 0));
        m_right.setWidth(Length(Length::Fixed, 0));
        m_top.setWidth(Length(Length::Fixed, 0));
        m_bottom.setWidth(Length(Length::Fixed, 0));
    }

    void checkComputed(Length curFontSize, Length rootFontSize, Font* font,
                       LayoutSize windowSize, ComputedStyle* cs)
    {
        if (m_left.hasBorderStyle()) {
            m_left.checkComputed(curFontSize, rootFontSize, font, windowSize,
                                 cs);
        } else {
            m_left.setWidth(Length(Length::Fixed, 0));
        }

        if (m_right.hasBorderStyle()) {
            m_right.checkComputed(curFontSize, rootFontSize, font, windowSize,
                                  cs);
        } else {
            m_right.setWidth(Length(Length::Fixed, 0));
        }

        if (m_top.hasBorderStyle()) {
            m_top.checkComputed(curFontSize, rootFontSize, font, windowSize,
                                cs);
        } else {
            m_top.setWidth(Length(Length::Fixed, 0));
        }

        if (m_bottom.hasBorderStyle()) {
            m_bottom.checkComputed(curFontSize, rootFontSize, font, windowSize,
                                   cs);
        } else {
            m_bottom.setWidth(Length(Length::Fixed, 0));
        }

        m_image.checkComputed(curFontSize, rootFontSize, font, windowSize, cs);
    }

    bool operator==(const BorderData& o)
    {
        return this->m_top == o.m_top && this->m_right == o.m_right &&
               this->m_bottom == o.m_bottom && this->m_left == o.m_left &&
               this->m_image == o.m_image;
    }

    bool operator!=(const BorderData& o)
    {
        return !operator==(o);
    }

private:
    BorderValue m_top;
    BorderValue m_right;
    BorderValue m_bottom;
    BorderValue m_left;

    BorderImage m_image;
};

} /* namespace StarFish */

#endif /* __StarFishBorderData__ */
