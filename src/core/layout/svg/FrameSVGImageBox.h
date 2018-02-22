/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameSVGImageBox__
#define __StarFishFrameSVGImageBox__

#include "core/layout/svg/FrameSVGBox.h"
#include "core/dom/svg/SVGImageElement.h"

namespace StarFish {

class FrameSVGImageBox : public FrameSVGBox {
public:
    FrameSVGImageBox(Node* node)
        : FrameSVGBox(node)
    {
    }

    virtual bool isFrameSVGImageBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameSVGImageBox";
    }

    virtual void paintSVG(PaintingContext& ctx)
    {
        SVGImageElement* e = node()->asSVGImageElement();
        if (e->imageData()) {
            if (e->preserveAspectRatioValue() == NativeImageData::None) {
                ctx.m_canvas->drawImage(e->imageData(),
                                        Unit::Rect(0, 0, width(), height()));
            } else {
                NativeImageData* id = e->imageData();
                auto v = e->preserveAspectRatioValue();
                LayoutUnit containerWidth = width();
                LayoutUnit containerHeight = height();

                if (containerWidth == LayoutUnit(0) ||
                    containerHeight == LayoutUnit(0)) {
                    return;
                }

                LayoutUnit imageDstWidth;
                LayoutUnit imageDstHeight;

                if (containerWidth / containerHeight >
                    (float)id->width() / (float)id->height()) {
                    imageDstWidth = containerHeight * (float)id->width() /
                                    (float)id->height();
                    imageDstHeight = containerHeight;
                } else {
                    imageDstWidth = containerWidth;
                    imageDstHeight = containerWidth * (float)id->height() /
                                     (float)id->width();
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

                ctx.m_canvas->drawImage(
                    id, Unit::Rect(x, y, imageDstWidth, imageDstHeight));
            }
        }
    }

protected:
};
}

#endif
