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
#include "core/modules/canvas/Canvas.h"
namespace Starfish {

static void PngWriteCallback(png_structp png_ptr, png_bytep data,
                             png_size_t length)
{
    std::vector<uint8_t>* p = (std::vector<uint8_t>*)png_get_io_ptr(png_ptr);
    p->insert(p->end(), data, data + length);
}

static bool encodePNGOnce(const uint8_t* src, size_t w, size_t h, size_t stride,
                          ImageEncoder::ImageColorSpace colorSpace,
                          bool isPremultiplied, std::vector<uint8_t>& result)
{
    result.clear();
    // PNG holds straight alpha, so a premultiplied row is un-multiplied into
    // this copy first. It is made before setjmp(): a longjmp back must not
    // skip the destructor of anything made after it.
    std::vector<uint8_t> row(isPremultiplied ? w * 4 : 0);
    png_structp p =
        png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!p) {
        return false;
    }
    png_infop info_ptr = png_create_info_struct(p);
    if (!info_ptr) {
        png_destroy_write_struct(&p, NULL);
        return false;
    }
    if (setjmp(png_jmpbuf(p))) {
        // libpng longjmp'd here on an internal error; the write struct is
        // left in an unusable state and must not be reused.
        png_destroy_write_struct(&p, &info_ptr);
        result.clear();
        return false;
    }
    png_set_IHDR(p, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    // png_set_compression_level(p, 1);
    png_set_write_fn(p, &result, PngWriteCallback, NULL);
    png_write_info(p, info_ptr);
    if (colorSpace == ImageEncoder::ImageColorSpace::BGRA) {
        png_set_bgr(p);
    }
    for (size_t y = 0; y < h; ++y) {
        const uint8_t* line = src + y * stride;
        if (isPremultiplied) {
            memcpy(row.data(), line, w * 4);
            convertImageBufferAsUnmultipliedAlphaIfNeeds(row.data(), w, w * 4,
                                                         1);
            line = row.data();
        }
        png_write_row(p, line);
    }
    png_write_end(p, info_ptr);
    png_destroy_write_struct(&p, &info_ptr);

    return true;
}

std::vector<uint8_t> ImageEncoder::encodePNG(const uint8_t* src, size_t w,
                                             size_t h, size_t stride,
                                             ImageColorSpace colorSpace,
                                             bool isPremultiplied)
{
    STARFISH_ASSERT(src != nullptr);

#if defined(STARFISH_ENABLE_TEST)
    const int maxAttempts = 3;
#else
    const int maxAttempts = 1;
#endif

    std::vector<uint8_t> result;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        if (encodePNGOnce(src, w, h, stride, colorSpace, isPremultiplied,
                          result)) {
            break;
        }
    }

    return result;
}

std::vector<uint8_t> ImageEncoder::encodePNG(const uint8_t* src, size_t w,
                                             size_t h,
                                             ImageColorSpace colorSpace)
{
    return encodePNG(src, w, h, w * 4, colorSpace);
}

std::vector<uint8_t> ImageEncoder::encodeJPEG(const uint8_t* src, size_t w,
                                              size_t h, size_t stride,
                                              ImageColorSpace colorSpace,
                                              bool isPremultiplied)
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

    // JPEG has no alpha: straight colors are composited onto black (HTML,
    // "A serialization of the bitmap as a file"); premultiplied ones already
    // are.
    std::vector<uint8_t> row(isPremultiplied ? 0 : w * 4);
    for (size_t i = 0; i < cinfo.image_height; i++) {
        const uint8_t* line = src + i * stride;
        if (!isPremultiplied) {
            memcpy(row.data(), line, w * 4);
            convertImageBufferAsPremultipliedAlphaIfNeeds(row.data(), w, w * 4,
                                                          1);
            line = row.data();
        }
        JSAMPROW scanline = const_cast<uint8_t*>(line);
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

std::vector<uint8_t> ImageEncoder::encodeJPEG(const uint8_t* src, size_t w,
                                              size_t h,
                                              ImageColorSpace colorSpace)
{
    return encodeJPEG(src, w, h, w * 4, colorSpace);
}
} // namespace Starfish
