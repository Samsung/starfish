/*
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

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#ifndef __StarfishCanvasCairoUtils__
#define __StarfishCanvasCairoUtils__

namespace Starfish {
enum class CanvasLineCap;
enum class CanvasLineJoin;

namespace CanvasCairoUtils {
    CanvasLineCap cairoLineCapToCavansLineCap(const cairo_line_cap_t& cap);
    cairo_line_cap_t cavansLineCapToCairoLineCap(const CanvasLineCap& cap);
    CanvasLineJoin cairoLineJoinToCanvasLineJoin(const cairo_line_join_t& join);
    cairo_line_join_t canvasLineJoinToCairoLineJoin(const CanvasLineJoin& join);
}
}

#endif
#endif
