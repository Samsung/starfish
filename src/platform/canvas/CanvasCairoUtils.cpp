/*
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION embedded systems
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#if defined(PORT_CANVAS_BACKEND_CAIRO)

#include <cairo.h>
#include "core/dom/canvas/CanvasLineCap.h"
#include "core/dom/canvas/CanvasLineJoin.h"
#include "platform/canvas/CanvasCairoUtils.h"

namespace Starfish {
namespace CanvasCairoUtils {
    CanvasLineCap cairoLineCapToCavansLineCap(const cairo_line_cap_t& cap)
    {
        switch (cap) {
        case CAIRO_LINE_CAP_BUTT:
            return CanvasLineCap::Butt;
            break;
        case CAIRO_LINE_CAP_ROUND:
            return CanvasLineCap::Round;
            break;
        case CAIRO_LINE_CAP_SQUARE:
            return CanvasLineCap::Square;
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
            return CanvasLineCap::Butt;
            break;
        }
    }

    cairo_line_cap_t canvasLineCapToCairoLineCap(const CanvasLineCap& cap)
    {
        switch (cap) {
        case CanvasLineCap::Butt:
            return CAIRO_LINE_CAP_BUTT;
            break;
        case CanvasLineCap::Round:
            return CAIRO_LINE_CAP_ROUND;
            break;
        case CanvasLineCap::Square:
            return CAIRO_LINE_CAP_SQUARE;
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
            return CAIRO_LINE_CAP_BUTT;
            break;
        }
    }

    CanvasLineJoin cairoLineJoinToCanvasLineJoin(const cairo_line_join_t& join)
    {
        switch (join) {
        case CAIRO_LINE_JOIN_MITER:
            return CanvasLineJoin::Miter;
            break;
        case CAIRO_LINE_JOIN_ROUND:
            return CanvasLineJoin::Round;
        case CAIRO_LINE_JOIN_BEVEL:
            return CanvasLineJoin::Bevel;
        default:
            STARFISH_ASSERT_NOT_REACHED();
            return CanvasLineJoin::Miter;
            break;
        }
    }

    cairo_line_join_t canvasLineJoinToCairoLineJoin(const CanvasLineJoin& join)
    {
        switch (join) {
        case CanvasLineJoin::Miter:
            return CAIRO_LINE_JOIN_MITER;
            break;
        case CanvasLineJoin::Round:
            return CAIRO_LINE_JOIN_ROUND;
            break;
        case CanvasLineJoin::Bevel:
            return CAIRO_LINE_JOIN_BEVEL;
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
            return CAIRO_LINE_JOIN_MITER;
            break;
        }
    }

    // Source from webkit project:
    // Source/WebCore/platform/graphics/cairo/CairoUtilities.cpp
    cairo_operator_t blendModeToCairoOperator(BlendMode bm)
    {
        cairo_operator_t newOperator;
        switch (bm) {
        case BlendMode::Multiply:
            newOperator = CAIRO_OPERATOR_MULTIPLY;
            break;
        case BlendMode::Screen:
            newOperator = CAIRO_OPERATOR_SCREEN;
            break;
        case BlendMode::Overlay:
            newOperator = CAIRO_OPERATOR_OVERLAY;
            break;
        case BlendMode::Darken:
            newOperator = CAIRO_OPERATOR_DARKEN;
            break;
        case BlendMode::Lighten:
            newOperator = CAIRO_OPERATOR_LIGHTEN;
            break;
        case BlendMode::ColorDodge:
            newOperator = CAIRO_OPERATOR_COLOR_DODGE;
            break;
        case BlendMode::ColorBurn:
            newOperator = CAIRO_OPERATOR_COLOR_BURN;
            break;
        case BlendMode::HardLight:
            newOperator = CAIRO_OPERATOR_HARD_LIGHT;
            break;
        case BlendMode::SoftLight:
            newOperator = CAIRO_OPERATOR_SOFT_LIGHT;
            break;
        case BlendMode::Difference:
            newOperator = CAIRO_OPERATOR_DIFFERENCE;
            break;
        case BlendMode::Exclusion:
            newOperator = CAIRO_OPERATOR_EXCLUSION;
            break;
        case BlendMode::Hue:
            newOperator = CAIRO_OPERATOR_HSL_HUE;
            break;
        case BlendMode::Saturation:
            newOperator = CAIRO_OPERATOR_HSL_SATURATION;
            break;
        case BlendMode::Color:
            newOperator = CAIRO_OPERATOR_HSL_COLOR;
            break;
        case BlendMode::Luminosity:
            newOperator = CAIRO_OPERATOR_HSL_LUMINOSITY;
            break;
        default:
            newOperator = CAIRO_OPERATOR_OVER;
        }
        return newOperator;
    }
} // namespace CanvasCairoUtils
} // namespace Starfish
#endif
