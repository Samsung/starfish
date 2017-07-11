/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "core/modules/canvas/image/ImageData.h"
#include "platform/file/FileIO.h"

#include <png.h>
#include <turbojpeg.h>
#include <gif_lib.h>

namespace StarFish {

class ImageDataMISC : public ImageData {
public:
    ImageDataMISC(String* localImageSrc)
    {
        FILE* fp = fopen(localImageSrc->utf8Data(), "rb");
        decodeImage(fp, localImageSrc, nullptr, 0);
        fclose(fp);
    }

    ImageDataMISC(const char* buf, size_t len)
    {
        if (buf && len != 0)
            decodeImage(nullptr, nullptr, buf, len);
    }

    virtual size_t bufferSize()
    {
        if (m_image) {
            return m_width * m_height * 4;
        } else {
            return 0;
        }
    }

    void reigsterFinalizer()
    {
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           // STARFISH_LOG_INFO("ImageDataEFL::~ImageDataEFL\n");
                                       },
                                       m_image, NULL, NULL);
    }

    virtual void* unwrap()
    {
        return m_image;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

private:
    enum ImageFormat { PNG, JPG, GIF, ERROR };

    static bool isPNGFormat(const unsigned char* data)
    {
        if (data[0] == 137 && data[1] == 80 && data[2] == 78 && data[3] == 71) {
            return true;
        }
        return false;
    }

    static bool isJPGFormat(const unsigned char* data)
    {
        if (data[0] == 255 && data[1] == 216 && data[2] == 255 &&
            data[3] == 224) {
            return true;
        } else if (data[6] == 69 && data[7] == 120 && data[8] == 105 &&
                   data[9] == 102) {
            return true;
        } else {
            return false;
        }
    }

    static bool isGIFFormat(const unsigned char* data)
    {
        if (data[0] == 71 && data[1] == 73 && data[2] == 70) {
            return true;
        } else {
            return false;
        }
    }

    static ImageFormat parseImageFormatFromBuffer(const char* buf)
    {
        ImageFormat imageFormat = ImageFormat::ERROR;

        if (isPNGFormat((unsigned char*)buf)) {
            imageFormat = ImageFormat::PNG;
        } else if (isJPGFormat((unsigned char*)buf)) {
            imageFormat = ImageFormat::JPG;
        } else if (isGIFFormat((unsigned char*)buf)) {
            imageFormat = ImageFormat::GIF;
        } else {
            // TODO ERROR
        }

        return imageFormat;
    }

    static ImageFormat parseImageFormatFromFile(FILE* fp)
    {
        ImageFormat imageFormat = ImageFormat::ERROR;

        if (!fp) {
            return imageFormat;
        }

        unsigned char* buf = new unsigned char[11];
        fgets((char*)buf, 11, fp);

        if (isPNGFormat(buf)) {
            imageFormat = ImageFormat::PNG;
        } else if (isJPGFormat(buf)) {
            imageFormat = ImageFormat::JPG;
        } else if (isGIFFormat((unsigned char*)buf)) {
            imageFormat = ImageFormat::GIF;
        } else {
            // TODO ERROR
        }

        rewind(fp);
        delete buf;
        return imageFormat;
    }

    typedef struct {
        const unsigned char* mem;
        unsigned long int size;
    } READ_DATA;

    static void readPNGFromBufferedInput(png_structp png, png_bytep data,
                                         png_size_t size)
    {
        READ_DATA* readData = (READ_DATA*)png_get_io_ptr(png);

        if (readData->mem && size > 0) {
            memcpy(data, readData->mem + readData->size, size);
            readData->size += size;
        }
    }

    void readPNGFileOrBufferedInput(FILE* fp, const char* bufferedInput,
                                    size_t len)
    {
        READ_DATA readData;
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

        if (!fp) {
            readData.mem = (unsigned char*)bufferedInput;
            readData.size = 0;
            png_set_read_fn(png, &readData, readPNGFromBufferedInput);
        } else {
            if (fp) {
                png_init_io(png, fp);
            }
        }

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

        if (colorType == PNG_COLOR_TYPE_RGB ||
            colorType == PNG_COLOR_TYPE_GRAY ||
            colorType == PNG_COLOR_TYPE_PALETTE) {
            png_set_filler(png, 0xff, PNG_FILLER_AFTER);
        }

        if (colorType == PNG_COLOR_TYPE_GRAY ||
            colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png);
        }
        png_set_bgr(png);
        png_read_update_info(png, info);

        rowPointers = (png_bytep*)malloc(sizeof(png_bytep) * m_height);

        png_uint_32 rowbytes = png_get_rowbytes(png, info);

        if ((m_image = (unsigned char*)malloc(rowbytes * m_height)) ==
            nullptr) {
            png_destroy_read_struct(&png, &info, nullptr);
            return;
        }

        for (png_uint_32 i = 0; i < (unsigned int)m_height; ++i) {
            rowPointers[i] = (png_bytep)m_image + i * rowbytes;
        }

        png_read_image(png, rowPointers);
        png_read_end(png, nullptr);
        png_destroy_read_struct(&png, &info, nullptr);
        free(rowPointers);
    }

    void readJPGFile(FILE* fp)
    {
        tjhandle dHandle = nullptr;
        unsigned char* srcBuf = nullptr;
        int jpegSize = 0;
        int TD_BU = 0;
        size_t readSize = 0;

        fseek(fp, 0, SEEK_END);
        jpegSize = ftell(fp);
        rewind(fp);

        if ((dHandle = tjInitDecompress()) == nullptr) {
            fclose(fp);
            STARFISH_LOG_ERROR("%s %d\n : dHandle is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        srcBuf = (unsigned char*)malloc(sizeof(unsigned char) * jpegSize);
        if (srcBuf == nullptr) {
            fclose(fp);
            tjDestroy(dHandle);
            STARFISH_LOG_ERROR("%s %d\n : srcBuf is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        readSize = fread(srcBuf, 1, jpegSize, fp);
        if (readSize <= 0) {
            fclose(fp);
            tjDestroy(dHandle);
            tjFree(srcBuf);
            STARFISH_LOG_ERROR("%s %d\n : readSize fail", __FUNCTION__,
                               __LINE__);
            return;
        }

        int hdrw = 0;
        int hdrh = 0;
        int hdrsubsamp = -1;
        int scaledWidth = 0;
        int scaledHeight = 0;
        unsigned long dstSize = 0;
        int n = 0;

        tjscalingfactor sf1 = { 1, 1 };
        tjscalingfactor* sf = tjGetScalingFactors(&n);

        tjDecompressHeader2(dHandle, srcBuf, jpegSize, &hdrw, &hdrh,
                            &hdrsubsamp);

        if (!sf || !n) {
            STARFISH_LOG_ERROR("%s %d\n : scaledfactor is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        scaledWidth = TJSCALED(hdrw, sf1);
        scaledHeight = TJSCALED(hdrh, sf1);
        dstSize = scaledWidth * scaledHeight * tjPixelSize[TJPF_BGRA];

        m_image = (unsigned char*)malloc(dstSize);

        tjDecompress2(dHandle, srcBuf, jpegSize, (unsigned char*)m_image,
                      scaledWidth, 0, scaledHeight, TJPF_BGRA, TD_BU);

        m_width = scaledWidth;
        m_height = scaledHeight;

        if (dHandle) {
            tjDestroy(dHandle);
        }
        if (srcBuf) {
            tjFree(srcBuf);
        }
    }

    void readJPGBufferedInput(const char* buf, size_t len)
    {
        tjhandle dHandle = nullptr;
        int TD_BU = 0;

        if ((dHandle = tjInitDecompress()) == nullptr) {
            STARFISH_LOG_ERROR("%s %d\n : dHandle is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        int hdrw = 0;
        int hdrh = 0;
        int hdrsubsamp = -1;
        int scaledWidth = 0;
        int scaledHeight = 0;
        unsigned long dstSize = 0;
        int n = 0;

        tjscalingfactor sf1 = { 1, 1 };
        tjscalingfactor* sf = tjGetScalingFactors(&n);

        tjDecompressHeader2(dHandle, (unsigned char*)buf, len, &hdrw, &hdrh,
                            &hdrsubsamp);

        if (!sf || !n) {
            STARFISH_LOG_ERROR("%s %d\n : scaledfactor is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        scaledWidth = TJSCALED(hdrw, sf1);
        scaledHeight = TJSCALED(hdrh, sf1);
        dstSize = scaledWidth * scaledHeight * tjPixelSize[TJPF_BGRA];

        m_image = (unsigned char*)malloc(dstSize);

        tjDecompress2(dHandle, (unsigned char*)buf, len,
                      (unsigned char*)m_image, scaledWidth, 0, scaledHeight,
                      TJPF_BGRA, TD_BU);

        m_width = scaledWidth;
        m_height = scaledHeight;

        if (dHandle) {
            tjDestroy(dHandle);
        }
    }

    void readGIFFile(String* localImageSrc)
    {
        int row = 0, col = 0;
        int width = 0, height = 0;
        int extCode = 0;
        int i = 0, j = 0;
        unsigned int imageNum = 0;
        unsigned long size = 0;

        GifRecordType recordType;
        GifRowType* screenBuffer = nullptr;
        GifFileType* GifFile = nullptr;
        ColorMapObject* ColorMap = nullptr;

        if (localImageSrc) {
            GifFile = DGifOpenFileName(localImageSrc->utf8Data());
        }

        m_width = GifFile->SWidth;
        m_height = GifFile->SHeight;

        screenBuffer = (GifRowType*)malloc(m_height * sizeof(GifRowType));

        size = m_width * sizeof(GifPixelType);
        screenBuffer[0] = (GifRowType)calloc(1, size);

        for (i = 0; i < (int)(m_width); i++) {
            screenBuffer[0][i] = GifFile->SBackGroundColor;
        }

        for (i = 1; i < (int)(m_height); i++) {
            screenBuffer[i] = (GifRowType)calloc(1, size);
            memcpy(screenBuffer[i], screenBuffer[0], size);
        }

        do {
            DGifGetRecordType(GifFile, &recordType);
            switch (recordType) {
            case IMAGE_DESC_RECORD_TYPE:
                DGifGetImageDesc(GifFile);

                row = GifFile->Image.Top;
                col = GifFile->Image.Left;
                width = GifFile->Image.Width;
                height = GifFile->Image.Height;

                imageNum++;

                if (GifFile->Image.Interlace) {
                    int interlacedOffset[] = { 0, 4, 2, 1 };
                    int interlacedJumps[] = { 8, 8, 4, 2 };
                    for (i = 0; i < 4; i++)
                        for (j = row + interlacedOffset[i]; j < row + height;
                             j += interlacedJumps[i]) {
                            DGifGetLine(GifFile, &screenBuffer[j][col], width);
                        }
                } else {
                    for (i = 0; i < height; i++) {
                        DGifGetLine(GifFile, &screenBuffer[row++][col], width);
                    }
                }
                break;
            case EXTENSION_RECORD_TYPE: {
                GifByteType* extension = nullptr;
                DGifGetExtension(GifFile, &extCode, &extension);
                while (extension != nullptr) {
                    DGifGetExtensionNext(GifFile, &extension);
                }
            } break;
            case TERMINATE_RECORD_TYPE:
                break;
            default:
                break;
            }
            if (imageNum > 0) {
                break;
            }
        } while (recordType != TERMINATE_RECORD_TYPE);

        ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap
                                            : GifFile->SColorMap);

        // Convert GIF to RGBA
        GifRowType gifRow;
        GifColorType* colorMapEntry = nullptr;
        GifByteType* buffer = nullptr;

        m_image = (void*)malloc(m_width * m_height * 4);
        buffer = (GifByteType*)m_image;
        for (unsigned long h = 0; h < m_height; h++) {
            gifRow = screenBuffer[h];
            for (unsigned long w = 0; w < m_width; w++) {
                colorMapEntry = &ColorMap->Colors[gifRow[w]];
                *buffer++ = colorMapEntry->Blue;
                *buffer++ = colorMapEntry->Green;
                *buffer++ = colorMapEntry->Red;
                *buffer++ = 255;
            }
        }

        if (screenBuffer) {
            if (screenBuffer[0]) {
                free(screenBuffer[0]);
            }
            for (i = 1; i < (int)(m_height); i++) {
                if (screenBuffer[i]) {
                    free(screenBuffer[i]);
                }
            }
            free(screenBuffer);
        }

        DGifCloseFile(GifFile);
    }

    void decodeImage(FILE* fp, String* localImageSrc, const char* buf,
                     size_t len)
    {
        ImageFormat imageFormat;
        if (fp) {
            imageFormat = parseImageFormatFromFile(fp);
        } else {
            imageFormat = parseImageFormatFromBuffer(buf);
        }
        switch (imageFormat) {
        case ImageFormat::PNG:
            readPNGFileOrBufferedInput(fp, buf, len);
            break;
        case ImageFormat::JPG:
            if (fp) {
                readJPGFile(fp);
            } else {
                readJPGBufferedInput(buf, len);
            }
            break;
        case ImageFormat::GIF:
            if (localImageSrc) {
                readGIFFile(localImageSrc);
            } else {
                // TODO
            }
            break;
        default:
            // TODO ERROR
            break;
        }
    }

protected:
    void* m_image;
    size_t m_width;
    size_t m_height;
};

ImageData* ImageData::create(String* localImageSrc)
{
    ImageData* imageData = new ImageDataMISC(localImageSrc);
    if (imageData->unwrap() == NULL) {
        return NULL;
    }
    return imageData;
}

ImageData* ImageData::create(const char* buf, size_t len)
{
    ImageData* imageData = new ImageDataMISC(buf, len);
    if (imageData->unwrap() == NULL) {
        return NULL;
    }
    return imageData;
}
}

#endif
