/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

    cairo_line_cap_t cavansLineCapToCairoLineCap(const CanvasLineCap& cap)
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
}
}
#endif
