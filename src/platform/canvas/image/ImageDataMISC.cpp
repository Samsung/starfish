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

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#define NEEDS_PREMULTIPLIED_ALPHA
#endif

#define PNG_SKIP_SETJMP_CHECK

#include "core/modules/canvas/image/ImageData.h"

#include <cairo.h>
#include <png.h>
#include <turbojpeg.h>
#include <gif_lib.h>

namespace StarFish {

class ImageDataMISC : public ImageData {
public:
    ImageDataMISC(String* localImageSrc)
    {
        auto utf8Data = localImageSrc->toUTF8NonGCString();
        FILE* fp = fopen(utf8Data.data(), "rb");
        decodeImage(fp, localImageSrc, nullptr, 0);
        fclose(fp);
        registerFinalizer();
    }

    ImageDataMISC(const char* buf, size_t len)
    {
        if (buf && len != 0) {
            decodeImage(nullptr, nullptr, buf, len);
            registerFinalizer();
        }
    }

    ImageDataMISC(size_t w, size_t h)
    {
        m_image = (unsigned char*)malloc(w * h * 4);
        m_width = w;
        m_height = h;
        m_stride = w * 4;
        registerFinalizer();
    }

    virtual uint8_t* data()
    {
        return (uint8_t*)m_image;
    }

    virtual void clear()
    {
        void* address = m_image;
        size_t end = bufferSize();
        memset(address, 0x00, end);
    }

    virtual size_t bufferSize()
    {
        if (m_image) {
            return m_stride * m_height;
        } else {
            return 0;
        }
    }

    void registerFinalizer()
    {
        if (m_width && m_height) {
            m_imageSurface = cairo_image_surface_create_for_data(
                (unsigned char*)m_image, CAIRO_FORMAT_ARGB32, m_width, m_height,
                m_stride);
        }
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                ImageDataMISC* self = (ImageDataMISC*)obj;
                if (self->m_imageSurface) {
                    cairo_surface_destroy(self->m_imageSurface);
                }
                free(self->m_image);
            },
            NULL, NULL, NULL);
    }

    virtual void* internalSurface()
    {
        return m_imageSurface;
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t stride()
    {
        return m_stride;
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
        } else if (isGIFFormat(buf)) {
            imageFormat = ImageFormat::GIF;
        } else {
            // TODO ERROR
        }

        rewind(fp);
        delete[] buf;
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
            STARFISH_LOG_ERROR("%s %d\n : png_structp is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            STARFISH_LOG_ERROR("%s %d\n : png_infop is NULL", __FUNCTION__,
                               __LINE__);
            png_destroy_read_struct(&png, nullptr, nullptr);
            return;
        }

        if (setjmp(png_jmpbuf(png))) {
            STARFISH_LOG_ERROR("%s %d\n : internal libpng error", __FUNCTION__,
                               __LINE__);
            png_destroy_read_struct(&png, &info, nullptr);
            return;
        }

        if (!fp) {
            readData.mem = (unsigned char*)bufferedInput;
            readData.size = 0;
            png_set_read_fn(png, &readData, readPNGFromBufferedInput);
        } else {
            png_init_io(png, fp);
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

        m_stride = rowbytes;
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
#ifdef NEEDS_PREMULTIPLIED_ALPHA
#define ARGB_TO_PREMULTIPLY_ALPHA(sr, sg, sb, sa)                              \
    (unsigned)(((unsigned)((unsigned char)(sr) * ((unsigned char)(sa) + 1)) >> \
                8) |                                                           \
               ((unsigned)((unsigned char)(sg) * ((unsigned char)(sa) + 1) >>  \
                           8)                                                  \
                << 8) |                                                        \
               ((unsigned)((unsigned char)(sb) * ((unsigned char)(sa) + 1) >>  \
                           8)                                                  \
                << 16) |                                                       \
               ((unsigned)(unsigned char)(sa) << 24))

        uint8_t* data = (uint8_t*)m_image;
        for (png_uint_32 y = 0; y < m_height; y++) {
            for (png_uint_32 x = 0; x < rowbytes; x += 4) {
                uint32_t* tmp = (uint32_t*)(&(data[y * rowbytes + x]));
                *tmp = ARGB_TO_PREMULTIPLY_ALPHA(
                    data[y * rowbytes + x], data[y * rowbytes + x + 1],
                    data[y * rowbytes + x + 2], data[y * rowbytes + x + 3]);
            }
        }

#undef ARGB_TO_PREMULTIPLY_ALPHA
#endif
        free(rowPointers);
    }

    void decodeJPG(tjhandle dHandle, unsigned char* buf, const int size)
    {
        int hdrw = 0;
        int hdrh = 0;
        int hdrsubsamp = -1;
        int scaledWidth = 0;
        int scaledHeight = 0;
        unsigned long dstSize = 0;
        int n = 0;

        tjscalingfactor sf1 = { 1, 1 };
        tjscalingfactor* sf = tjGetScalingFactors(&n);

        tjDecompressHeader2(dHandle, buf, size, &hdrw, &hdrh, &hdrsubsamp);

        if (!sf || !n) {
            STARFISH_LOG_ERROR("%s %d\n : scaledfactor is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        scaledWidth = TJSCALED(hdrw, sf1);
        scaledHeight = TJSCALED(hdrh, sf1);
        dstSize = scaledWidth * scaledHeight * tjPixelSize[TJPF_BGRA];

        m_image = (unsigned char*)malloc(dstSize);

        tjDecompress2(dHandle, buf, size, (unsigned char*)m_image, scaledWidth,
                      0, scaledHeight, TJPF_BGRA, 0);

        m_width = scaledWidth;
        m_height = scaledHeight;
        m_stride = m_width * 4;
    }

    void readJPGFile(FILE* fp)
    {
        tjhandle dHandle = nullptr;
        unsigned char* srcBuf = nullptr;
        int jpegSize = 0;
        size_t readSize = 0;

        fseek(fp, 0, SEEK_END);
        jpegSize = ftell(fp);
        rewind(fp);

        if ((dHandle = tjInitDecompress()) == nullptr) {
            STARFISH_LOG_ERROR("%s %d\n : dHandle is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        srcBuf = (unsigned char*)malloc(sizeof(unsigned char) * jpegSize);
        if (srcBuf == nullptr) {
            tjDestroy(dHandle);
            STARFISH_LOG_ERROR("%s %d\n : srcBuf is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        readSize = fread(srcBuf, 1, jpegSize, fp);
        if (readSize <= 0) {
            tjDestroy(dHandle);
            tjFree(srcBuf);
            STARFISH_LOG_ERROR("%s %d\n : readSize fail", __FUNCTION__,
                               __LINE__);
            return;
        }

        decodeJPG(dHandle, srcBuf, jpegSize);
        tjDestroy(dHandle);
        tjFree(srcBuf);
    }

    void readJPGBufferedInput(const char* buf, size_t len)
    {
        tjhandle dHandle = nullptr;

        if ((dHandle = tjInitDecompress()) == nullptr) {
            STARFISH_LOG_ERROR("%s %d\n : dHandle is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        decodeJPG(dHandle, (unsigned char*)buf, len);
        tjDestroy(dHandle);
    }

    typedef struct {
        unsigned long long size;
        void* mem;
    } GIF_READ_DATA;

    static int gifRead(GifFileType* gft, GifByteType* data, int size)
    {
        GIF_READ_DATA* readData = (GIF_READ_DATA*)gft->UserData;

        if (readData->mem && size > 0) {
            memcpy(data, (GifByteType*)readData->mem + readData->size, size);
            readData->size += size;
        }
        return size;
    }

    static void releaseGIFResource(GifFileType* gifFile,
                                   GifRowType* screenBuffer, unsigned int size)
    {
        if (screenBuffer) {
            for (unsigned int i = 0; i < size; i++) {
                if (screenBuffer[i]) {
                    free(screenBuffer[i]);
                }
            }
            free(screenBuffer);
        }
#ifdef GIF_LIB_VERSION
        DGifCloseFile(gifFile);
#else
        int errorCode = 0;
        DGifCloseFile(gifFile, &errorCode);
#endif
    }

    static int getGifTransparentIndex(GifFileType* gif)
    {
#ifdef GIF_LIB_VERSION
        return 0;
#else
        GraphicsControlBlock first_gcb;
        memset(&first_gcb, 0, sizeof(first_gcb));
        DGifSavedExtensionToGCB(gif, 0, &first_gcb);
        return first_gcb.TransparentColor;
#endif
    }

    void readGIFFileOrBufferedInput(String* localImageSrc,
                                    const char* bufferedInput)
    {
        int row = 0, col = 0;
        int width = 0, height = 0;
        int extCode = 0;
        int i = 0, j = 0;
        int errorCode = 0;
        unsigned int imageNum = 0;
        unsigned long size = 0;

        GifRecordType recordType;
        GifRowType* screenBuffer = nullptr;
        GifFileType* gifFile = nullptr;
        ColorMapObject* colorMap = nullptr;

        GIF_READ_DATA readData;

        if (localImageSrc) {
            auto utf8Data = localImageSrc->toUTF8NonGCString();
#ifdef GIF_LIB_VERSION
            gifFile = DGifOpenFileName(utf8Data.data());
#else
            gifFile = DGifOpenFileName(utf8Data.data(), &errorCode);
#endif
            if (!gifFile) {
                STARFISH_LOG_ERROR("Gif Open File Error, %d\n", errorCode);
                return;
            }
        } else {
            readData.mem = (void*)bufferedInput;
            readData.size = 0;
#ifdef GIF_LIB_VERSION
            gifFile = DGifOpen(&readData, gifRead);
#else
            gifFile = DGifOpen(&readData, gifRead, &errorCode);
#endif
            if (!gifFile) {
                STARFISH_LOG_ERROR("Gif Open Error, %d\n", errorCode);
                return;
            }
        }

        m_width = gifFile->SWidth;
        m_height = gifFile->SHeight;

        screenBuffer = (GifRowType*)malloc(m_height * sizeof(GifRowType));

        size = m_width * sizeof(GifPixelType);
        m_stride = m_width * 4;
        screenBuffer[0] = (GifRowType)calloc(1, size);

        for (i = 0; i < (int)(m_width); i++) {
            screenBuffer[0][i] = gifFile->SBackGroundColor;
        }

        for (i = 1; i < (int)(m_height); i++) {
            screenBuffer[i] = (GifRowType)calloc(1, size);
            memcpy(screenBuffer[i], screenBuffer[0], size);
        }

        do {
            DGifGetRecordType(gifFile, &recordType);
            switch (recordType) {
            case IMAGE_DESC_RECORD_TYPE:
                DGifGetImageDesc(gifFile);

                row = gifFile->Image.Top;
                col = gifFile->Image.Left;
                width = gifFile->Image.Width;
                height = gifFile->Image.Height;

                imageNum++;

                if (gifFile->Image.Interlace) {
                    int interlacedOffset[] = { 0, 4, 2, 1 };
                    int interlacedJumps[] = { 8, 8, 4, 2 };
                    for (i = 0; i < 4; i++)
                        for (j = row + interlacedOffset[i]; j < row + height;
                             j += interlacedJumps[i]) {
                            DGifGetLine(gifFile, &screenBuffer[j][col], width);
                        }
                } else {
                    for (i = 0; i < height; i++) {
                        DGifGetLine(gifFile, &screenBuffer[row++][col], width);
                    }
                }
                break;
            case EXTENSION_RECORD_TYPE: {
                GifByteType* extension = nullptr;
                DGifGetExtension(gifFile, &extCode, &extension);
                while (extension != nullptr) {
                    DGifGetExtensionNext(gifFile, &extension);
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

        colorMap = (gifFile->Image.ColorMap ? gifFile->Image.ColorMap
                                            : gifFile->SColorMap);

        if (colorMap == nullptr) {
            STARFISH_LOG_ERROR("Gif Image does not have a colormap\n");
            releaseGIFResource(gifFile, screenBuffer, m_height);
            return;
        }

        // Convert GIF to RGBA
        int ti = getGifTransparentIndex(gifFile);
        GifRowType gifRow;
        GifColorType* colorMapEntry = nullptr;
        GifByteType* buffer = nullptr;

        m_image = (void*)malloc(m_width * m_height * 4);
        buffer = (GifByteType*)m_image;
        for (unsigned long h = 0; h < m_height; h++) {
            gifRow = screenBuffer[h];
            for (unsigned long w = 0; w < m_width; w++) {
                colorMapEntry = &colorMap->Colors[gifRow[w]];
#ifdef GIF_LIB_VERSION
                *buffer++ = colorMapEntry->Blue;
                *buffer++ = colorMapEntry->Green;
                *buffer++ = colorMapEntry->Red;
                *buffer++ = 255;
#else
                if (ti == NO_TRANSPARENT_COLOR) {
                    *buffer++ = colorMapEntry->Blue;
                    *buffer++ = colorMapEntry->Green;
                    *buffer++ = colorMapEntry->Red;
                    *buffer++ = 255;
                } else {
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                }
#endif
            }
        }

        releaseGIFResource(gifFile, screenBuffer, m_height);
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
                readGIFFileOrBufferedInput(localImageSrc, nullptr);
            } else {
                readGIFFileOrBufferedInput(nullptr, buf);
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
    size_t m_stride;
    size_t m_height;
    cairo_surface_t* m_imageSurface;
};

ImageData* ImageData::create(String* localImageSrc)
{
    ImageData* imageData = new ImageDataMISC(localImageSrc);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

ImageData* ImageData::create(const char* buf, size_t len)
{
    ImageData* imageData = new ImageDataMISC(buf, len);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

ImageData* ImageData::create(size_t width, size_t height)
{
    return new ImageDataMISC(width, height);
}
}

#endif
