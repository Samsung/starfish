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

#ifndef __StarFishFrameSVGRectBox__
#define __StarFishFrameSVGRectBox__

#include "core/layout/svg/FrameSVGBox.h"

namespace StarFish {

void paintPathArcCommand(Canvas* canvas, double x1, double y1, double rx,
                         double ry, double xAxisRotation, bool isLargeArc,
                         bool isPositiveSweep, double x2, double y2);

class FrameSVGRectBox final : public FrameSVGBox {
public:
    FrameSVGRectBox(Node* node)
        : FrameSVGBox(node)
        , m_rx(0)
        , m_ry(0)
    {
    }

    virtual const char* name() override
    {
        return "FrameSVGRectBox";
    }

    virtual void layoutSVG() override
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

    virtual void paintSVG(PaintingContext& ctx) override
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

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameSVGBox::fillGCDescriptor(desc);
    }

    float m_rx, m_ry;
};
}

#endif
