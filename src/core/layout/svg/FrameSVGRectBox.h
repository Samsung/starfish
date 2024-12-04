/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishFrameSVGRectBox__
#define __StarfishFrameSVGRectBox__

#include "core/layout/svg/FrameSVGBox.h"

namespace Starfish {

void paintPathArcCommand(Path* path, double x1, double y1, double rx, double ry,
                         double xAxisRotation, bool isLargeArc,
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

    virtual void layoutSVG(SVGLayoutContext& ctx) override
    {
        auto styleRX = style()->rx();
        auto styleRY = style()->ry();

        if (styleRX.isSpecified() && styleRY.isSpecified()) {
            m_rx = styleRX.specifiedValue(ctx.viewport.width(), this);
            m_ry = styleRY.specifiedValue(ctx.viewport.height(), this);
        } else if (styleRX.isSpecified() && !styleRY.isSpecified()) {
            m_rx = m_ry = styleRX.specifiedValue(ctx.viewport.width(), this);
        } else if (!styleRX.isSpecified() && styleRY.isSpecified()) {
            m_rx = m_ry = styleRY.specifiedValue(ctx.viewport.height(), this);
        }
    }

    virtual Optional<Path*> path() override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameSVGBox::fillGCDescriptor(desc);
    }

    float m_rx, m_ry;
};
} // namespace Starfish

#endif
