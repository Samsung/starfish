/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"

#if defined(PORT_IMAGEDECODER_BACKEND_MISC)

#include <png.h>
#include "ImageDecoder.h"

namespace StarFish {

ImageDecoder::ImageDecoder(const char* filename)
    : m_fp(nullptr)
    , m_width(0)
    , m_height(0)
    , m_imageData(nullptr)
{
    m_fp = fopen(filename, "rb");
}

ImageDecoder::~ImageDecoder()
{
    if (m_fp) {
        fclose(m_fp);
    }
}

void* ImageDecoder::buffer()
{
    switch (parseImageFormat()) {
    case ImageFormat::PNG:
        readPNGFile();
        break;
    case ImageFormat::JPG:
        // TODO
        break;
    case ImageFormat::GIF:
        // TODO
        break;
    default:
        // TODO ERROR
        break;
    }
    return (void*)m_imageData;
}

int ImageDecoder::width()
{
    return m_width;
}

int ImageDecoder::height()
{
    return m_height;
}

ImageDecoder::ImageFormat ImageDecoder::parseImageFormat()
{
    ImageFormat imageFormat = ImageFormat::ERROR;

    if (!m_fp) {
        return imageFormat;
    }

    unsigned char* buf = new unsigned char[5];
    fgets((char*)buf, 5, m_fp);

    if (buf[0] == 137 && buf[1] == 80 && buf[2] == 78 && buf[3] == 71) {
        imageFormat = ImageFormat::PNG;
    } else if (buf[0] == 255 && buf[1] == 216 && buf[2] == 255 &&
               buf[3] == 224) {
        imageFormat = ImageFormat::JPG;
    } else {
        // TODO ERROR
    }

    rewind(m_fp);
    delete buf;
    return imageFormat;
}

void ImageDecoder::readPNGFile()
{
    png_byte colorType;
    png_byte bitDepth;
    png_bytep* rowPointers;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                             nullptr, nullptr);

    if (!png) {
        abort();
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        abort();
    }

    if (setjmp(png_jmpbuf(png))) {
        abort();
    }

    png_init_io(png, m_fp);
    png_read_info(png, info);

    m_width = png_get_image_width(png, info);
    m_height = png_get_image_height(png, info);
    colorType = png_get_color_type(png, info);
    bitDepth = png_get_bit_depth(png, info);

    if (bitDepth == 16) {
        png_set_strip_16(png);
    }

    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xff, PNG_FILLER_AFTER);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    png_set_bgr(png);
    png_read_update_info(png, info);

    rowPointers = (png_bytep*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(
        sizeof(png_bytep) * m_height);

    png_uint_32 rowbytes = png_get_rowbytes(png, info);

    if ((m_imageData = (unsigned char*)GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(
             rowbytes * m_height)) == nullptr) {
        png_destroy_read_struct(&png, &info, nullptr);
        return;
    }

    for (png_uint_32 i = 0; i < (unsigned int)m_height; ++i) {
        rowPointers[i] = (png_bytep)m_imageData + i * rowbytes;
    }

    png_read_image(png, rowPointers);
    png_read_end(png, nullptr);
    png_destroy_read_struct(&png, &info, nullptr);
}
}
#endif
