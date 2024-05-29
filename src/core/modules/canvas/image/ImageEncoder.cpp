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
#if defined(OS_WINDOWS)
#include <Wincodec.h>
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Windowscodecs.lib")
#else
#include <jpeglib.h>
#endif
#include "ImageEncoder.h"
namespace Starfish {

static void PngWriteCallback(png_structp png_ptr, png_bytep data,
                             png_size_t length)
{
    std::vector<uint8_t>* p = (std::vector<uint8_t>*)png_get_io_ptr(png_ptr);
    p->insert(p->end(), data, data + length);
}

std::vector<uint8_t> ImageEncoder::encodePNG(const uint8_t* src, size_t w,
                                             size_t h,
                                             ImageColorSpace colorSpace)
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
    if (colorSpace == ImageColorSpace::RGBA ||
        colorSpace == ImageColorSpace::BGRA) {
        for (size_t y = 0; y < h; ++y) {
            rows[y] = (uint8_t*)src + y * w * 4;
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    png_set_rows(p, info_ptr, &rows[0]);
    png_set_write_fn(p, &result, PngWriteCallback, NULL);
    if (colorSpace == ImageColorSpace::RGBA) {
        png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    } else {
        png_write_png(p, info_ptr, PNG_TRANSFORM_BGR, NULL);
    }
    png_destroy_write_struct(&p, NULL);

    return result;
}

std::vector<uint8_t> ImageEncoder::encodeJPEG(const uint8_t* src, size_t w,
                                              size_t h,
                                              ImageColorSpace colorSpace)
{
    STARFISH_ASSERT(src != nullptr);

    std::vector<uint8_t> result;
    result.clear();

#if !defined(OS_WINDOWS)

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    unsigned char* jpegBuffer = nullptr;
    unsigned long jpegSize = 0;

    jpeg_create_compress(&cinfo);
    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.err = jpeg_std_error(&jerr);

    if (colorSpace == ImageColorSpace::RGBA) {
        cinfo.in_color_space = JCS_EXT_RGBA;
        cinfo.input_components = 4;
    } else {
        cinfo.in_color_space = JCS_EXT_BGRA;
        cinfo.input_components = 4;
    }

    jpeg_mem_dest(&cinfo, &jpegBuffer, &jpegSize);
    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, true);

    for (size_t i = 0; i < cinfo.image_height; i++) {
        JSAMPROW scanline =
            static_cast<JSAMPROW>(const_cast<uint8_t*>(src) + i * w * 4);
        jpeg_write_scanlines(&cinfo, &scanline, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    result.insert(result.end(), jpegBuffer, jpegBuffer + jpegSize);
    free(jpegBuffer);
#else

    STARFISH_UNSUPPORTED("encodeJPEG on Windows");

#endif
    return result;
}
} // namespace Starfish
