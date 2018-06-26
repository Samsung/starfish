/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"

#if defined(PORT_IMAGEDECODER_BACKEND_MISC)

#if defined(PORT_CANVAS_BACKEND_CAIRO) || defined(PORT_CANVAS_BACKEND_SKIA)
#define NEEDS_PREMULTIPLIED_ALPHA
#endif

#define PNG_SKIP_SETJMP_CHECK

#include "core/modules/canvas/image/NativeImageData.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
#endif

#include <png.h>
#if defined(OS_WINDOWS)
#include <Wincodec.h>
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Windowscodecs.lib")
#else
#include <jpeglib.h>
#endif
#include <gif_lib.h>

namespace StarFish {

class NativeImageDataMISC : public NativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_GENERIC_MALLOC(sizeof(NativeImageDataMISC),
                                 NativeImageData::nativeImageDataGCKind());
    }

    NativeImageDataMISC(String* localImageSrc)
    {
        auto utf8Data = localImageSrc->toUTF8NonGCString();
        FILE* fp = fopen(utf8Data.data(), "rb");
        decodeImage(fp, localImageSrc, nullptr, 0);
        fclose(fp);
        initInternalSurface();
    }

    NativeImageDataMISC(const char* buf, size_t len)
    {
        m_image = nullptr;
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        m_imageSurface = nullptr;
#endif
        m_width = 0;
        m_height = 0;
        m_stride = 0;
        m_hasTransparentPixel = false;

        if (buf && len != 0) {
            decodeImage(nullptr, nullptr, buf, len);
            initInternalSurface();
        }
    }

    NativeImageDataMISC(size_t w, size_t h)
    {
        m_image = (unsigned char*)malloc(w * h * 4);
        m_width = w;
        m_height = h;
        m_stride = w * 4;
        m_hasTransparentPixel = true;
        initInternalSurface();
    }

    virtual ~NativeImageDataMISC()
    {
        disposeNativeImageData();
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

    virtual void disposeNativeImageData()
    {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        if (m_imageSurface) {
            cairo_surface_destroy(m_imageSurface);
        }
#endif
        free(m_image);

        NativeImageData::disposeNativeImageData();
    }

    void initInternalSurface()
    {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        if (m_width && m_height) {
            m_imageSurface = cairo_image_surface_create_for_data(
                (unsigned char*)m_image, CAIRO_FORMAT_ARGB32, m_width, m_height,
                m_stride);
        }
#endif
    }

    virtual void* internalSurface()
    {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        return m_imageSurface;
#endif
#if defined(PORT_CANVAS_BACKEND_SKIA)
        return nullptr;
#endif
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
    enum ImageFormat { PNG, JPG, GIF, FORMAT_ERROR };

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
        ImageFormat imageFormat = ImageFormat::FORMAT_ERROR;

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
        ImageFormat imageFormat = ImageFormat::FORMAT_ERROR;

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
            free(rowPointers);
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
        for (png_uint_32 y = 0; y < m_height; ++y) {
            for (png_uint_32 x = 0; x < rowbytes; x += 4) {
                png_uint_32 idx = y * rowbytes + x;
                uint32_t* tmp = (uint32_t*)(&(data[idx]));
                if (data[idx + 3] != 255) {
                    m_hasTransparentPixel = true;
                }

#ifdef STARFISH_ANDROID
                *tmp = ARGB_TO_PREMULTIPLY_ALPHA(data[idx + 2], data[idx + 1],
                                                 data[idx], data[idx + 3]);
#else
                *tmp = ARGB_TO_PREMULTIPLY_ALPHA(data[idx], data[idx + 1],
                                                 data[idx + 2], data[idx + 3]);
#endif
            }
        }

#undef ARGB_TO_PREMULTIPLY_ALPHA
#endif
        free(rowPointers);
    }

#if !defined(OS_WINDOWS)
    void decodeJPG(jpeg_decompress_struct* dHandle, unsigned char* buf,
                   const int size)
    {
        unsigned long dstSize = 0;
        jpeg_mem_src(dHandle, buf, size);

        if (jpeg_read_header(dHandle, TRUE) != 1) {
            return;
        }

        if (jpeg_start_decompress(dHandle) != 1) {
            return;
        }
        m_width = dHandle->output_width;
        m_height = dHandle->output_height;
        m_stride = m_width * 4;
        dstSize = m_stride * m_height;
        m_image = malloc(dstSize);

        unsigned char* buffer_array[1];
        if (dHandle->out_color_space == JCS_GRAYSCALE) {
            while (dHandle->output_scanline < dHandle->output_height) {
                buffer_array[0] = (unsigned char*)m_image +
                                  (dHandle->output_scanline) * m_stride;
                jpeg_read_scanlines(dHandle, buffer_array, 1);
                {
                    uint8_t* buf_raw = static_cast<uint8_t*>(buffer_array[0]);
                    uint8_t* iter = buf_raw;
                    iter += m_width;
                    int g;
                    for (int i = m_stride - 1; i >= 0; i -= 4) {
                        g = *--iter;
                        buf_raw[i] = 255;
                        buf_raw[i - 1] = g;
                        buf_raw[i - 2] = g;
                        buf_raw[i - 3] = g;
                    }
                }
            }
        } else {
            while (dHandle->output_scanline < dHandle->output_height) {
                buffer_array[0] = (unsigned char*)m_image +
                                  (dHandle->output_scanline) * m_stride;
                jpeg_read_scanlines(dHandle, buffer_array, 1);
                {
                    uint8_t* buf_raw = static_cast<uint8_t*>(buffer_array[0]);
                    uint8_t* iter = buf_raw;
                    iter += (m_stride * 3 / 4);
                    int r, g, b;
                    for (int i = m_stride - 1; i >= 0; i -= 4) {
                        b = *--iter;
                        g = *--iter;
                        r = *--iter;
#ifdef STARFISH_ANDROID
                        buf_raw[i] = 255;
                        buf_raw[i - 1] = b;
                        buf_raw[i - 2] = g;
                        buf_raw[i - 3] = r;
#else
                        buf_raw[i] = 255;
                        buf_raw[i - 1] = r;
                        buf_raw[i - 2] = g;
                        buf_raw[i - 3] = b;
#endif
                    }
                }
            }
        }

        jpeg_finish_decompress(dHandle);
    }

    struct custom_error_mgr {
        jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    typedef struct custom_error_mgr* custom_error_ptr;

    static void jpeg_error_handle(j_common_ptr cinfo)
    {
        custom_error_ptr c_err = (custom_error_ptr)cinfo->err;
        longjmp(c_err->setjmp_buffer, 1);
    }

    static void jpeg_message_handle(j_common_ptr cinfo, int msg_level)
    {
        custom_error_ptr c_err = (custom_error_ptr)cinfo->err;
        if (msg_level < 0) {
            longjmp(c_err->setjmp_buffer, 1);
        }
    }

    void readJPGFile(FILE* fp)
    {
        jpeg_decompress_struct dHandle;
        custom_error_mgr jerr;

        dHandle.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = jpeg_error_handle;
        jerr.pub.emit_message = jpeg_message_handle;
        if (setjmp(jerr.setjmp_buffer)) {
            m_width = 0;
            m_height = 0;
            m_stride = 0;
            if (m_image) {
                free(m_image);
                m_image = NULL;
            }
            jpeg_destroy_decompress(&dHandle);
            return;
        }

        unsigned char* srcBuf = nullptr;
        int jpegSize = 0;
        size_t readSize = 0;

        fseek(fp, 0, SEEK_END);
        jpegSize = ftell(fp);
        rewind(fp);
        jpeg_create_decompress(&dHandle);

        srcBuf = (unsigned char*)malloc(sizeof(unsigned char) * jpegSize);
        if (srcBuf == nullptr) {
            jpeg_destroy_decompress(&dHandle);
            STARFISH_LOG_ERROR("%s %d\n : srcBuf is NULL", __FUNCTION__,
                               __LINE__);
            return;
        }

        readSize = fread(srcBuf, 1, jpegSize, fp);
        if (readSize <= 0) {
            jpeg_destroy_decompress(&dHandle);
            free(srcBuf);
            STARFISH_LOG_ERROR("%s %d\n : readSize fail", __FUNCTION__,
                               __LINE__);
            return;
        }
        decodeJPG(&dHandle, srcBuf, jpegSize);
        jpeg_destroy_decompress(&dHandle);
        free(srcBuf);
    }

    void readJPGBufferedInput(const char* buf, size_t len)
    {
        jpeg_decompress_struct dHandle;
        custom_error_mgr jerr;

        dHandle.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = jpeg_error_handle;
        jerr.pub.emit_message = jpeg_message_handle;

        if (setjmp(jerr.setjmp_buffer)) {
            m_width = 0;
            m_height = 0;
            m_stride = 0;
            if (m_image) {
                free(m_image);
                m_image = NULL;
            }

            jpeg_destroy_decompress(&dHandle);
            return;
        }

        jpeg_create_decompress(&dHandle);

        decodeJPG(&dHandle, (unsigned char*)buf, len);
        jpeg_destroy_decompress(&dHandle);
    }
#else
    // https://stackoverflow.com/questions/45809347/how-to-decode-jpeg-using-win32
    bool Win32DecodeJpeg(void* ImageData, unsigned int ImageDataSize)
    {
        // IWICImagingFactory is a structure containing the function pointers of
        // the WIC API
        static IWICImagingFactory* IWICFactory;
        if (IWICFactory == NULL) {
            auto ret = CoInitializeEx(NULL, COINIT_MULTITHREADED);
            if (ret == S_OK ||
                ret == S_FALSE) { // S_FALSE means COM already initialzed

            } else {
                return false;
            }

            if (CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                 CLSCTX_INPROC_SERVER,
                                 IID_PPV_ARGS(&IWICFactory)) != S_OK) {
                return false;
            }
        }

        IWICStream* Stream;
        if (IWICFactory->CreateStream(&Stream) != S_OK) {
            return false;
        }

        if (Stream->InitializeFromMemory((unsigned char*)ImageData,
                                         ImageDataSize) != S_OK) {
            return false;
        }

        IWICBitmapDecoder* BitmapDecoder;
        if (IWICFactory->CreateDecoderFromStream(Stream, NULL,
                                                 WICDecodeMetadataCacheOnDemand,
                                                 &BitmapDecoder) != S_OK) {
            return false;
        }

        IWICBitmapFrameDecode* FrameDecode;
        // frames apply mostly to GIFs and other animated media. JPEGs just have
        // a single frame.
        if (BitmapDecoder->GetFrame(0, &FrameDecode) != S_OK) {
            return false;
        }

        IWICFormatConverter* FormatConverter;
        if (IWICFactory->CreateFormatConverter(&FormatConverter) != S_OK) {
            return false;
        }

        // this function does not do any actual decoding
        if (FormatConverter->Initialize(FrameDecode,
                                        GUID_WICPixelFormat32bppBGRA,
                                        WICBitmapDitherTypeNone, nullptr, 0.0f,
                                        WICBitmapPaletteTypeCustom) != S_OK) {
            return false;
        }

        IWICBitmap* Bitmap;
        if (IWICFactory->CreateBitmapFromSource(
                FormatConverter, WICBitmapCacheOnDemand, &Bitmap) != S_OK) {
            return false;
        }

        unsigned int Width, Height;
        if (Bitmap->GetSize(&Width, &Height) != S_OK) {
            return false;
        }
        WICRect Rect = { 0, 0, (int)Width, (int)Height };

        IWICBitmapLock* Lock;
        // this is the function that does the actual decoding. seems like they
        // defer the decoding until it's actually needed
        if (Bitmap->Lock(&Rect, WICBitmapLockRead, &Lock) != S_OK) {
            return false;
        }

        unsigned int PixelDataSize = 0;
        unsigned char* PixelData;
        if (Lock->GetDataPointer(&PixelDataSize, &PixelData) != S_OK) {
            return false;
        }

        m_image = (unsigned char*)malloc(Width * Height * 4);
        m_width = Width;
        m_height = Height;
        m_stride = Width * 4;
        m_hasTransparentPixel = true;

        memcpy(m_image, PixelData, PixelDataSize);

        Stream->Release();
        BitmapDecoder->Release();
        FrameDecode->Release();
        FormatConverter->Release();
        Bitmap->Release();
        Lock->Release();

        return true;
    }
    void decodeJPG(unsigned char* buf, const int size)
    {
        Win32DecodeJpeg(buf, size);
    }
    void readJPGFile(FILE* fp)
    {
        fseek(fp, 0, SEEK_END);
        auto jpegSize = ftell(fp);
        rewind(fp);

        unsigned char* srcBuf =
            (unsigned char*)malloc(sizeof(unsigned char) * jpegSize);
        auto readSize = fread(srcBuf, 1, jpegSize, fp);
        if (readSize <= 0) {
            STARFISH_LOG_ERROR("%s %d\n : readSize fail", __FUNCTION__,
                               __LINE__);
            return;
        }

        decodeJPG(srcBuf, jpegSize);
    }

    void readJPGBufferedInput(const char* buf, size_t len)
    {
        decodeJPG((unsigned char*)buf, len);
    }
#endif
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
#elif GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
        int errorCode = 0;
        DGifCloseFile(gifFile, &errorCode);
#else
        DGifCloseFile(gifFile);
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
        if (screenBuffer == NULL) {
            STARFISH_LOG_ERROR("Gif Open Error: malloc failed\n");
            return;
        }

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

        int transparentIndex = -1;
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
                    if (extension[0] == 4) {
                        const int flags = extension[1];
                        if ((flags & 0x01)) {
                            transparentIndex = extension[4];
                        }
                    }
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
        GifRowType gifRow;
        GifColorType* colorMapEntry = nullptr;
        GifByteType* buffer = nullptr;

        m_image = (void*)malloc(m_width * m_height * 4);
        buffer = (GifByteType*)m_image;
        for (unsigned long h = 0; h < m_height; h++) {
            gifRow = screenBuffer[h];
            for (unsigned long w = 0; w < m_width; w++) {
                colorMapEntry = &colorMap->Colors[gifRow[w]];

                if (gifRow[w] == transparentIndex) {
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    m_hasTransparentPixel = true;
                } else {
#ifdef STARFISH_ANDROID
                    *buffer++ = colorMapEntry->Red;
                    *buffer++ = colorMapEntry->Green;
                    *buffer++ = colorMapEntry->Blue;
                    *buffer++ = 255;
#else
                    *buffer++ = colorMapEntry->Blue;
                    *buffer++ = colorMapEntry->Green;
                    *buffer++ = colorMapEntry->Red;
                    *buffer++ = 255;
#endif
                }
            }
        }

        releaseGIFResource(gifFile, screenBuffer, m_height);
    }

    void decodeImage(FILE* fp, String* localImageSrc, const char* buf,
                     size_t len)
    {
        m_hasTransparentPixel = false;
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

    virtual bool hasTransparentPixel()
    {
        return m_hasTransparentPixel;
    }

#ifdef STARFISH_ENABLE_TEST
    virtual void dumpImage(const char* path)
    {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        if (m_imageSurface) {
            cairo_surface_write_to_png(m_imageSurface, path);
        }
#endif
    }
#endif

protected:
    bool m_hasTransparentPixel;
    void* m_image;
    size_t m_width;
    size_t m_stride;
    size_t m_height;
#if defined(PORT_CANVAS_BACKEND_CAIRO)
    cairo_surface_t* m_imageSurface;
#endif
};

NativeImageData* NativeImageData::create(String* localImageSrc)
{
    NativeImageData* imageData = new NativeImageDataMISC(localImageSrc);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

NativeImageData* NativeImageData::create(const char* buf, size_t len)
{
    NativeImageData* imageData = new NativeImageDataMISC(buf, len);
    if (imageData->data() == NULL) {
        return NULL;
    }
    return imageData;
}

NativeImageData* NativeImageData::create(size_t width, size_t height)
{
    return new NativeImageDataMISC(width, height);
}
} // namespace StarFish

#endif
