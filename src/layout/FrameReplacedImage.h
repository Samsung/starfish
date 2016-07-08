/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameReplacedImage__
#define __StarFishFrameReplacedImage__

#include "layout/FrameReplaced.h"
#include "platform/canvas/image/ImageData.h"

namespace StarFish {

class FrameReplacedImage : public FrameReplaced {
public:
    FrameReplacedImage(Node* node, String* src)
        : FrameReplaced(node, nullptr)
    {
    }

    virtual bool isFrameReplacedImage()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedImage";
    }

    virtual void paintReplaced(Canvas* canvas)
    {
        if (node()->asElement()->asHTMLElement()->asHTMLImageElement()->imageData())
            canvas->drawImage(node()->asElement()->asHTMLElement()->asHTMLImageElement()->imageData(), Rect(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
            width() - borderWidth() - paddingWidth(), height() - borderHeight() - paddingHeight()));
    }

    virtual IntrinsicSizeUsedInLayout intrinsicSize()
    {
        IntrinsicSizeUsedInLayout result;
        HTMLImageElement* img = node()->asElement()->asHTMLElement()->asHTMLImageElement();
        ImageData* id = img->imageData();
        if (id) {
            result.m_intrinsicContentSize = LayoutSize(id->width(), id->height());
            String* widthString = img->width();
            String* heightString = img->height();
            bool widthIsEmpty = widthString->equals(String::emptyString);
            bool heightIsEmpty = heightString->equals(String::emptyString);
            if (widthIsEmpty && heightIsEmpty) {
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement = std::make_pair(Length(), Length());
            } else if (widthIsEmpty) {
                float h = String::parseFloat(heightString);
                bool heightIsPercent = heightString->lastIndexOf('%') == heightString->length() - 1;
                Length height = heightIsPercent? Length(Length::Percent, (float)h / 100) : Length(Length::Fixed, h);
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement = std::make_pair(Length(), height);
            } else if (heightIsEmpty) {
                float w = String::parseFloat(widthString);
                bool widthIsPercent = widthString->lastIndexOf('%') == widthString->length() - 1;
                Length width = widthIsPercent? Length(Length::Percent, (float)w / 100) : Length(Length::Fixed, w);
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement = std::make_pair(width, Length());
            } else {
                float w = String::parseFloat(widthString);
                float h = String::parseFloat(heightString);
                bool heightIsPercent = heightString->lastIndexOf('%') == heightString->length() - 1;
                bool widthIsPercent = widthString->lastIndexOf('%') == widthString->length() - 1;
                Length width = widthIsPercent? Length(Length::Percent, (float)w / 100) : Length(Length::Fixed, w);
                Length height = heightIsPercent? Length(Length::Percent, (float)h / 100) : Length(Length::Fixed, h);
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement = std::make_pair(width, height);
            }
        } else {
            result.m_intrinsicSizeIsSpecifiedByAttributeOfElement = std::make_pair(Length(Length::Fixed, 0), Length(Length::Fixed, 0));
        }

        if (result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.first.isSpecified()) {
            if (!result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.first.isPositveOfZero()) {
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.first = Length();
            }
        }

        if (result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.second.isSpecified()) {
            if (!result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.second.isPositveOfZero()) {
                result.m_intrinsicSizeIsSpecifiedByAttributeOfElement.second = Length();
            }
        }


        return result;
    }

    ImageData* imageData()
    {
        return nullptr;
    }
protected:
};
}

#endif
