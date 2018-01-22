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
#include "core/dom/Node.h"
#include "core/dom/HTMLImageElement.h"
#include "core/layout/FrameReplacedImage.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace StarFish {
void FrameReplacedImage::paintReplaced(Canvas* canvas)
{
    FrameReplaced::paintReplaced(canvas);
    NativeImageData* id = node()->asHTMLImageElement()->imageData();
    if (id) {
        if (id->preserveAspectRatioValue() == NativeImageData::None) {
            Unit::Rect frameRect = Unit::Rect(
                borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                width() - borderWidth() - paddingWidth(),
                height() - borderHeight() - paddingHeight());

            if (style()->hasObjectSizing()) {
                canvas->save();
                canvas->clip(frameRect);
                LayoutRect imgRect =
                    computeObjectFit(id->width(), id->height());
                canvas->drawImage(id, Unit::Rect(imgRect.x(), imgRect.y(),
                                                 imgRect.width(),
                                                 imgRect.height()));
                canvas->restore();
            } else {
                canvas->drawImage(id, frameRect);
            }
        } else {
            canvas->translate(borderLeft() + paddingLeft(),
                              borderTop() + paddingTop());

            auto v = id->preserveAspectRatioValue();
            LayoutUnit containerWidth =
                width() - borderWidth() - paddingWidth();
            LayoutUnit containerHeight =
                height() - borderHeight() - paddingHeight();

            if (containerWidth == LayoutUnit(0) ||
                containerHeight == LayoutUnit(0)) {
                return;
            }

            LayoutUnit imageDstWidth;
            LayoutUnit imageDstHeight;

            if (containerWidth / containerHeight >
                (float)id->width() / (float)id->height()) {
                imageDstWidth =
                    containerHeight * (float)id->width() / (float)id->height();
                imageDstHeight = containerHeight;
            } else {
                imageDstWidth = containerWidth;
                imageDstHeight =
                    containerWidth * (float)id->height() / (float)id->width();
            }

            LayoutUnit remainX = containerWidth - imageDstWidth;
            LayoutUnit remainY = containerHeight - imageDstHeight;

            LayoutUnit x, y;

            if (v == NativeImageData::xMinYMin) {
            } else if (v == NativeImageData::xMidYMin) {
                x = remainX / 2;
            } else if (v == NativeImageData::xMaxYMin) {
                x = remainX;
            } else if (v == NativeImageData::xMinYMid) {
                y = remainY / 2;
            } else if (v == NativeImageData::xMidYMid) {
                x = remainX / 2;
                y = remainY / 2;
            } else if (v == NativeImageData::xMaxYMid) {
                x = remainX;
                y = remainY / 2;
            } else if (v == NativeImageData::xMinYMax) {
                y = remainY;
            } else if (v == NativeImageData::xMidYMax) {
                x = remainX / 2;
                y = remainY;
            } else if (v == NativeImageData::xMaxYMax) {
                x = remainX;
                y = remainY;
            }

            canvas->drawImage(id,
                              Unit::Rect(x, y, imageDstWidth, imageDstHeight));
        }
    }
}

IntrinsicSize FrameReplacedImage::intrinsicSize()
{
    IntrinsicSize result;
    NativeImageData* id = node()->asHTMLImageElement()->imageData();
    if (id) {
        result.m_hasAspectRatio = true;
        result.m_isContentExists = true;
        result.m_intrinsicContentSize = LayoutSize(id->width(), id->height());
    } else {
        result.m_hasAspectRatio = false;
        result.m_isContentExists = false;
        result.m_intrinsicContentSize = LayoutSize(15, 15);
    }
    return result;
}
}
