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

#include "StarfishConfig.h"
#include <png.h>
#include "ImageUtils.h"
namespace Starfish {

static void PngWriteCallback(png_structp png_ptr, png_bytep data,
                             png_size_t length)
{
    std::vector<uint8_t>* p = (std::vector<uint8_t>*)png_get_io_ptr(png_ptr);
    p->insert(p->end(), data, data + length);
}

std::vector<uint8_t> ImageUtils::encodePNG(const uint8_t* src, size_t w,
                                           size_t h, size_t stride)
{
    STARFISH_ASSERT(src != nullptr);

    std::vector<uint8_t> result;
    result.clear();
    png_structp p =
        png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(p);
    setjmp(png_jmpbuf(p));
    png_set_IHDR(p, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    // png_set_compression_level(p, 1);
    std::vector<uint8_t*> rows(h);
    for (size_t y = 0; y < h; ++y) {
        rows[y] = (uint8_t*)src + y * w * 4;
    }
    png_set_rows(p, info_ptr, &rows[0]);
    png_set_write_fn(p, &result, PngWriteCallback, NULL);
#if defined(PORT_PIXEL_ORDER_RGBA)
    png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
#else
    png_write_png(p, info_ptr, PNG_TRANSFORM_BGR, NULL);
#endif

    png_destroy_write_struct(&p, NULL);

    return result;
}
} // namespace Starfish
