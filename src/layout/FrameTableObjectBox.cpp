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

#include "StarFishConfig.h"
#include "FrameTableObjectBox.h"

namespace StarFish {

FrameTableObjectBox::FrameTableObjectBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

// Draws the border around the area defined by "rect"
void FrameTableObjectBox::paintBorders(Canvas* canvas, LayoutRect& rect)
{
    canvas->save();

    if (style()->hasBorderImageData()) {
        // Draw image borders at the four corners as shown below.
        //   ______________
        //  |_|          |_|
        //  |              |
        //  |              |
        //  |_            _|
        //  |_|__________|_|
        //

        double bWidth = style()->surround()->border.top().width().specifiedValue(height());
        double bImgWidth = style()->surround()->border.image().widths().top().specifiedValue(bWidth);
        double bImgSlice = style()->surround()->border.image().slices().top().specifiedValue(height());

        size_t imgWidth = style()->surround()->border.image().imageData()->width();
        size_t imgHeight = style()->surround()->border.image().imageData()->height();

        size_t lSlice = style()->surround()->border.image().slices().left().specifiedValue(width());
        size_t tSlice = style()->surround()->border.image().slices().top().specifiedValue(height());
        size_t rSlice = style()->surround()->border.image().slices().right().specifiedValue(width());
        size_t bSlice = style()->surround()->border.image().slices().bottom().specifiedValue(height());

        ImageData* imgData = style()->surround()->border.image().imageData();

        if (bImgSlice > imgWidth || bImgSlice > imgHeight) {
            bImgSlice = std::min(imgWidth, imgHeight);
        }

        double value = std::min((float)width() / (bImgWidth*2), (float)height() / (bImgWidth*2));
        if (value < 1) {
            bImgWidth *= value;
        }

        double scale = bImgWidth / bImgSlice;
        bool isFill = false;

        if ((lSlice + rSlice > imgWidth) || (tSlice + bSlice > imgHeight)) {
            float drawRect = std::min((float)width(), (float)height()) / 2.0;

            if (drawRect > bImgWidth) {
                drawRect = bImgWidth;
            }

            // left-top
            canvas->drawBorderImage(imgData,
                Rect(rect.x(), rect.y(), drawRect, drawRect), lSlice, tSlice, 0, 0, scale, isFill);
            // right-top
            canvas->drawBorderImage(imgData,
                Rect((float)rect.width() - drawRect, rect.y(), drawRect, drawRect), 0, tSlice, rSlice, 0, scale, isFill);
            // right-bottom
            canvas->drawBorderImage(imgData,
                Rect((float)rect.width() - drawRect, (float)(rect.y() + rect.height()) - drawRect, drawRect, drawRect), 0, 0, rSlice, bSlice, scale, isFill);
            // left-bottom
            canvas->drawBorderImage(imgData,
                Rect(rect.x(), (float)(rect.y() + rect.height()) - drawRect, drawRect, drawRect), lSlice, 0, 0, bSlice, scale, isFill);
        } else {
            isFill = style()->surround()->border.image().sliceFill();
            canvas->drawBorderImage(imgData,
                Rect(rect.x(), rect.y(), rect.width(), rect.height()), lSlice, tSlice, rSlice, bSlice, scale, isFill);
        }
    } else if (style()->hasBorderStyle()) {
        // Draw trapezium-like borders around FrameTableSections
        // The area for FrameTableSections is obtained from m_tableRect,
        // which has been already calculated in layoutHeight();
        //    _______________
        //   |\_____________/|
        //   ||             ||
        //   ||             ||
        //   ||             ||
        //   ||_____________||
        //   |/_____________\|
        //

        // top
        canvas->setColor(style()->borderTopColor());
        canvas->drawRect(
            LayoutLocation(rect.x(), rect.y()),
            LayoutLocation(rect.x() + rect.width(), rect.y()),
            LayoutLocation(rect.x() + rect.width() - borderRight(), rect.y() + borderTop()),
            LayoutLocation(rect.x() + borderLeft(), rect.y() + borderTop())
        );

        // right
        canvas->setColor(style()->borderRightColor());
        canvas->drawRect(
            LayoutLocation(rect.x() + rect.width() - borderRight(), rect.y() + borderTop()),
            LayoutLocation(rect.x() + rect.width(), rect.y()),
            LayoutLocation(rect.x() + rect.width(), rect.y() + rect.height()),
            LayoutLocation(rect.x() + rect.width() - borderRight(), rect.y() + rect.height() - borderBottom())
        );

        // bottom
        canvas->setColor(style()->borderBottomColor());
        canvas->drawRect(
            LayoutLocation(rect.x() + borderLeft(), rect.y() + rect.height() - borderBottom()),
            LayoutLocation(rect.x() + rect.width() - borderRight(), rect.y() + rect.height() - borderBottom()),
            LayoutLocation(rect.x() + rect.width(), rect.y() + rect.height()),
            LayoutLocation(rect.x(), rect.y() + rect.height())
        );

        // left
        canvas->setColor(style()->borderLeftColor());
        canvas->drawRect(
            LayoutLocation(rect.x(), rect.y()),
            LayoutLocation(rect.x() + borderLeft(), rect.y() + borderTop()),
            LayoutLocation(rect.x() + borderLeft(), rect.y() + rect.height() - borderBottom()),
            LayoutLocation(rect.x(), rect.y() + rect.height())
        );
    }

    canvas->restore();
}

}
