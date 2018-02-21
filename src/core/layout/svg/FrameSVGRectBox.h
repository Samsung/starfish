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

#ifndef __StarFishFrameSVGRectBox__
#define __StarFishFrameSVGRectBox__

#include "core/layout/svg/FrameSVGBox.h"

namespace StarFish {

void paintPathArcCommand(Canvas* canvas, double x1, double y1, double rx,
                         double ry, double xAxisRotation, bool isLargeArc,
                         bool isPositiveSweep, double x2, double y2);

class FrameSVGRectBox : public FrameSVGBox {
public:
    FrameSVGRectBox(Node* node)
        : FrameSVGBox(node)
        , m_rx(0)
        , m_ry(0)
    {
    }

    virtual bool isFrameSVGRectBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameSVGRectBox";
    }

    virtual void layoutSVG()
    {
        auto styleRX = style()->rx();
        auto styleRY = style()->ry();

        FrameBox* cb = layoutParent()->asFrameBox();

        if (styleRX.isSpecified() && styleRY.isSpecified()) {
            m_rx = styleRX.specifiedValue(cb->width(), this);
            m_ry = styleRY.specifiedValue(cb->height(), this);
        } else if (styleRX.isSpecified() && !styleRY.isSpecified()) {
            m_rx = m_ry = styleRX.specifiedValue(cb->width(), this);
        } else if (!styleRX.isSpecified() && styleRY.isSpecified()) {
            m_rx = m_ry = styleRY.specifiedValue(cb->height(), this);
        }
    }

    virtual void paintSVG(PaintingContext& ctx)
    {
        float rx = m_rx, ry = m_ry;

        if (rx > width() / 2) {
            rx = width() / 2;
        }

        if (ry > height() / 2) {
            ry = height() / 2;
        }

        ctx.m_canvas->beginPath();

        if (rx == 0 && ry == 0) {
            ctx.m_canvas->moveTo(0, 0);
            ctx.m_canvas->lineTo((float)width(), 0);
            ctx.m_canvas->lineTo((float)width(), (float)height());
            ctx.m_canvas->lineTo(0, (float)height());
            ctx.m_canvas->lineTo(0, 0);
        } else {
            // perform an absolute moveto operation to location (x+rx,y),
            ctx.m_canvas->moveTo(rx, 0);
            // perform an absolute horizontal lineto operation to location
            // (x+width-rx,y)
            ctx.m_canvas->lineTo(width() - rx, 0);
            // perform an absolute elliptical arc operation to coordinate
            // (x+width,y+ry)
            paintPathArcCommand(ctx.m_canvas, width() - rx, 0, rx, ry, 0, false,
                                true, width(), ry);
            // perform a absolute vertical lineto to location
            // (x+width,y+height-ry)
            ctx.m_canvas->lineTo(width(), height() - ry);
            // perform an absolute elliptical arc operation to coordinate
            // (x+width-rx,y+height)
            paintPathArcCommand(ctx.m_canvas, width(), height() - ry, rx, ry, 0,
                                false, true, width() - rx, height());
            // perform an absolute horizontal lineto to location (x+rx,y+height)
            ctx.m_canvas->lineTo(rx, height());
            // perform an absolute elliptical arc operation to coordinate
            // (x,y+height-ry)
            paintPathArcCommand(ctx.m_canvas, rx, height(), rx, ry, 0, false,
                                true, 0, height() - ry);
            // perform an absolute absolute vertical lineto to location (x,y+ry)
            ctx.m_canvas->lineTo(0, ry);
            // perform an absolute elliptical arc operation to coordinate
            // (x+rx,y)
            paintPathArcCommand(ctx.m_canvas, 0, ry, rx, ry, 0, false, true, rx,
                                0);
        }

        ctx.m_canvas->setColor(style()->fill().color());
        ctx.m_canvas->fillPreserve();

        ctx.m_canvas->setColor(style()->stroke().color());
        ctx.m_canvas->stroke();
    }

protected:
    float m_rx, m_ry;
};
}

#endif
