/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFrameSVGLineBox__
#define __StarfishFrameSVGLineBox__

#include "core/layout/svg/FrameSVGBox.h"

namespace Starfish {

/*
void paintPathArcCommand(Canvas* canvas, double x1, double y1, double rx,
                         double ry, double xAxisRotation, bool isLargeArc,
                         bool isPositiveSweep, double x2, double y2);
*/

class FrameSVGLineBox final : public FrameSVGBox {
public:
    FrameSVGLineBox(Node* node)
        : FrameSVGBox(node)
        , m_x1(0)
        , m_y1(0)
        , m_x2(0)
        , m_y2(0)
    {
    }

    virtual const char* name() override
    {
        return "FrameSVGLineBox";
    }

    virtual void layoutSVG() override
    {
        FrameBox* cb = layoutParent()->asFrameBox();
        auto x1 = style()->x1();
        auto y1 = style()->y1();
        auto x2 = style()->x2();
        auto y2 = style()->y2();

        if (x1.isSpecified()) {
            m_x1 = x1.numberData();
        } else if (x1.isPercent()) {
            m_x1 = x1.percentValue(cb->width());
        }

        if (y1.isSpecified()) {
            m_y1 = y1.numberData();
        } else if (y1.isPercent()) {
            m_y1 = y1.percentValue(cb->height());
        }

        if (x2.isSpecified()) {
            m_x2 = x2.numberData();
        } else if (x2.isPercent()) {
            m_x2 = x2.percentValue(cb->width());
        }

        if (y2.isSpecified()) {
            m_y2 = y2.numberData();
        } else if (y2.isPercent()) {
            m_y2 = y2.percentValue(cb->height());
        }
    }

    virtual void paintSVG(PaintingContext& ctx) override
    {
        FrameBox* cb = layoutParent()->asFrameBox();
        ctx.m_canvas->beginPath();
        ctx.m_canvas->moveTo(m_x1, m_y1);
        ctx.m_canvas->lineTo(m_x2, m_y2);

        ctx.m_canvas->setColor(style()->fill().color());
        ctx.m_canvas->fillPreserve();

        ctx.m_canvas->setLineWidth(
            style()->strokeWidth().specifiedValue(cb->width(), this));
        ctx.m_canvas->setStrokeColor(style()->stroke().color());
        ctx.m_canvas->stroke();
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameSVGBox::fillGCDescriptor(desc);
    }

    float m_x1, m_y1;
    float m_x2, m_y2;
};
}

#endif
