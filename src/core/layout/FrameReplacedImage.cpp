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
