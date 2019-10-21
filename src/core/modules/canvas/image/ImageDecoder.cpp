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
#include "ImageDecoder.h"

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
extern "C" {
#include <jpeglib.h>
}
#endif

#include <gif_lib.h>

#define GIF_DISPOSE_SHIFT 2
#define GIF_TRANSPARENT_MASK 0x01
#define GIF_DISPOSE_MASK 0x07

namespace Starfish {

static bool isPNGFormat(const std::vector<char>& inputBuffer)
{
    unsigned char* data = (unsigned char*)inputBuffer.data();
    if (inputBuffer.size() > 4 && data[0] == 137 && data[1] == 80 &&
        data[2] == 78 && data[3] == 71) {
        return true;
    }
    return false;
}

static bool isJPGFormat(const std::vector<char>& inputBuffer)
{
    unsigned char* data = (unsigned char*)inputBuffer.data();
    if (inputBuffer.size() > 3 && data[0] == 255 && data[1] == 216 &&
        data[2] == 255) {
        return true;
    } else if (inputBuffer.size() > 10 && data[6] == 69 && data[7] == 120 &&
               data[8] == 105 && data[9] == 102) {
        return true;
    } else {
        return false;
    }
}

static bool isGIFFormat(const std::vector<char>& inputBuffer)
{
    unsigned char* data = (unsigned char*)inputBuffer.data();
    if (inputBuffer.size() > 3 && data[0] == 71 && data[1] == 73 &&
        data[2] == 70) {
        return true;
    } else {
        return false;
    }
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

static ImageDecoder::DecodeResult decodePNG(
    const std::vector<char>& inputBuffer, bool needsDecoding)
{
    READ_DATA readData;
    png_byte colorType;
    png_byte bitDepth;
    png_bytep* rowPointers;

    ImageDecoder::DecodeResult result;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                             nullptr, nullptr);

    if (!png) {
        return result;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        return result;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        return result;
    }

    readData.mem = (unsigned char*)inputBuffer.data();
    readData.size = 0;
    png_set_read_fn(png, &readData, readPNGFromBufferedInput);

    png_read_info(png, info);

    result.m_width = png_get_image_width(png, info);
    result.m_height = png_get_image_height(png, info);
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

    png_uint_32 rowbytes = png_get_rowbytes(png, info);
    result.m_stride = rowbytes;
    if (needsDecoding) {
        rowPointers = (png_bytep*)malloc(sizeof(png_bytep) * result.m_height);
        result.m_buffer = (uint8_t*)malloc(rowbytes * result.m_height);
        STARFISH_RELEASE_ASSERT(result.m_buffer);
        for (png_uint_32 i = 0; i < (unsigned int)result.m_height; ++i) {
            rowPointers[i] = (png_bytep)result.m_buffer + i * rowbytes;
        }

        png_read_image(png, rowPointers);
        png_read_end(png, nullptr);
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

        uint8_t* data = (uint8_t*)result.m_buffer;
        for (png_uint_32 y = 0; y < result.m_height; ++y) {
            for (png_uint_32 x = 0; x < rowbytes; x += 4) {
                png_uint_32 idx = y * rowbytes + x;
                uint32_t* tmp = (uint32_t*)(&(data[idx]));
#ifdef PORT_PIXEL_ORDER_RGBA
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

    png_destroy_read_struct(&png, &info, nullptr);

    result.m_isSuccessful = true;
    return result;
}

#if !defined(OS_WINDOWS)
static void decodeJPG(jpeg_decompress_struct* dHandle,
                      ImageDecoder::DecodeResult& result,
                      const std::vector<char>& inputBuffer, bool needsDecoding)
{
    STARFISH_ASSERT(dHandle != nullptr);

    unsigned long dstSize = 0;
    jpeg_mem_src(dHandle, (unsigned char*)inputBuffer.data(),
                 inputBuffer.size());

    if (jpeg_read_header(dHandle, TRUE) != 1) {
        return;
    }

    if (jpeg_start_decompress(dHandle) != 1) {
        return;
    }

    result.m_width = dHandle->output_width;
    result.m_height = dHandle->output_height;
    result.m_stride = result.m_width * 4;

    if (!needsDecoding) {
        // decode first line for testing
        result.m_buffer = (uint8_t*)malloc(result.m_stride);
        STARFISH_RELEASE_ASSERT(result.m_buffer != nullptr);

        unsigned char* buffer_array[1];
        if (dHandle->output_scanline < dHandle->output_height) {
            buffer_array[0] = (unsigned char*)result.m_buffer +
                              (dHandle->output_scanline) * result.m_stride;
            jpeg_read_scanlines(dHandle, buffer_array, 1);
            result.m_isSuccessful = true;
        } else {
            result.m_isSuccessful = false;
        }

        free(result.m_buffer);
        result.m_buffer = nullptr;
        return;
    }

    dstSize = result.m_stride * result.m_height;
    result.m_buffer = (uint8_t*)malloc(dstSize);
    STARFISH_RELEASE_ASSERT(result.m_buffer != nullptr);

    unsigned char* buffer_array[1];
    if (dHandle->out_color_space == JCS_GRAYSCALE) {
        while (dHandle->output_scanline < dHandle->output_height) {
            buffer_array[0] = (unsigned char*)result.m_buffer +
                              (dHandle->output_scanline) * result.m_stride;
            jpeg_read_scanlines(dHandle, buffer_array, 1);
            {
                uint8_t* buf_raw = static_cast<uint8_t*>(buffer_array[0]);
                uint8_t* iter = buf_raw;
                iter += result.m_width;
                int g;
                for (int i = result.m_stride - 1; i >= 0; i -= 4) {
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
            buffer_array[0] = (unsigned char*)result.m_buffer +
                              (dHandle->output_scanline) * result.m_stride;
            jpeg_read_scanlines(dHandle, buffer_array, 1);
            {
                uint8_t* buf_raw = static_cast<uint8_t*>(buffer_array[0]);
                uint8_t* iter = buf_raw;
                iter += (result.m_stride * 3 / 4);
                int r, g, b;
                for (int i = result.m_stride - 1; i >= 0; i -= 4) {
                    b = *--iter;
                    g = *--iter;
                    r = *--iter;
#ifdef PORT_PIXEL_ORDER_RGBA
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
    result.m_isSuccessful = true;
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

static ImageDecoder::DecodeResult decodeJPG(
    const std::vector<char>& inputBuffer, bool needsDecoding)
{
    ImageDecoder::DecodeResult result;

    jpeg_decompress_struct dHandle;
    custom_error_mgr jerr;

    dHandle.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_handle;
    jerr.pub.emit_message = jpeg_message_handle;

    if (setjmp(jerr.setjmp_buffer)) {
        result.m_width = 0;
        result.m_height = 0;
        result.m_stride = 0;
        if (result.m_buffer) {
            free(result.m_buffer);
            result.m_buffer = NULL;
        }

        jpeg_destroy_decompress(&dHandle);
        result.m_isSuccessful = false;
        return result;
    }

    jpeg_create_decompress(&dHandle);

    decodeJPG(&dHandle, result, inputBuffer, needsDecoding);
    jpeg_destroy_decompress(&dHandle);
    return result;
}
#else
// https://stackoverflow.com/questions/45809347/how-to-decode-jpeg-using-win32
static ImageDecoder::DecodeResult decodeJPG(
    const std::vector<char>& inputBuffer, bool needsDecoding)
{
    ImageDecoder::DecodeResult result;
    // IWICImagingFactory is a structure containing the function pointers of
    // the WIC API
    static IWICImagingFactory* IWICFactory = nullptr;
    if (IWICFactory == NULL) {
        auto ret = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (ret == S_OK ||
            ret == S_FALSE) { // S_FALSE means COM already initialzed

        } else {
            return result;
        }

        if (CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                             CLSCTX_INPROC_SERVER,
                             IID_PPV_ARGS(&IWICFactory)) != S_OK) {
            return result;
        }
    }

    IWICStream* Stream = nullptr;
    if (IWICFactory->CreateStream(&Stream) != S_OK) {
        return result;
    }

    if (Stream->InitializeFromMemory((unsigned char*)inputBuffer.data(),
                                     inputBuffer.size()) != S_OK) {
        return result;
    }

    IWICBitmapDecoder* BitmapDecoder = nullptr;
    if (IWICFactory->CreateDecoderFromStream(Stream, NULL,
                                             WICDecodeMetadataCacheOnDemand,
                                             &BitmapDecoder) != S_OK) {
        return result;
    }

    IWICBitmapFrameDecode* FrameDecode = nullptr;
    // frames apply mostly to GIFs and other animated media. JPEGs just have
    // a single frame.
    if (BitmapDecoder->GetFrame(0, &FrameDecode) != S_OK) {
        return result;
    }

    IWICFormatConverter* FormatConverter = nullptr;
    if (IWICFactory->CreateFormatConverter(&FormatConverter) != S_OK) {
        return result;
    }

    // this function does not do any actual decoding
    if (FormatConverter->Initialize(FrameDecode,
#ifdef PORT_PIXEL_ORDER_RGBA
                                    GUID_WICPixelFormat32bppRGBA,
#else
                                    GUID_WICPixelFormat32bppBGRA,
#endif
                                    WICBitmapDitherTypeNone, nullptr, 0.0f,
                                    WICBitmapPaletteTypeCustom) != S_OK) {
        return result;
    }

    IWICBitmap* Bitmap = nullptr;
    if (IWICFactory->CreateBitmapFromSource(
            FormatConverter, WICBitmapCacheOnDemand, &Bitmap) != S_OK) {
        return result;
    }

    unsigned int Width = 0, Height = 0;
    if (Bitmap->GetSize(&Width, &Height) != S_OK) {
        return result;
    }
    m_width = Width;
    m_height = Height;
    m_stride = Width * 4;

    if (needsDecoding) {
        WICRect Rect = { 0, 0, (int)Width, (int)Height };
        IWICBitmapLock* Lock = nullptr;
        // this is the function that does the actual decoding. seems like
        // they
        // defer the decoding until it's actually needed
        if (Bitmap->Lock(&Rect, WICBitmapLockRead, &Lock) != S_OK) {
            return result;
        }

        unsigned int PixelDataSize = 0;
        unsigned char* PixelData = nullptr;
        if (Lock->GetDataPointer(&PixelDataSize, &PixelData) != S_OK) {
            return result;
        }

        result.m_buffer = (unsigned char*)malloc(Width * Height * 4);
        STARFISH_RELEASE_ASSERT(result.m_buffer != nullptr);
        memcpy(result.m_buffer, PixelData, PixelDataSize);
        Lock->Release();
    }

    Stream->Release();
    BitmapDecoder->Release();
    FrameDecode->Release();
    FormatConverter->Release();
    Bitmap->Release();

    result.m_isSuccessful = true;
    return result;
}

#endif

static int gifRead(GifFileType* gft, NULLABLE GifByteType* data, int size)
{
    STARFISH_ASSERT(gft != nullptr);

    ImageDecoder::GifReadData* readData =
        (ImageDecoder::GifReadData*)gft->UserData;

    if (size > 0) {
        STARFISH_ASSERT(data != nullptr);

        unsigned uSize = (unsigned)size;
        if (readData->pos + uSize > readData->size) {
            size -= readData->pos + uSize - readData->size;
            if (size < 0) {
                size = 0;
            }
        }
        memcpy(data, (GifByteType*)readData->mem + readData->pos, size);
        readData->pos += size;
    }
    return size;
}

static void releaseGIFResource(GifFileType* gifFile,
                               NULLABLE GifRowType* screenBuffer,
                               unsigned int size)
{
    STARFISH_ASSERT(gifFile != nullptr);

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

static ImageDecoder::DecodeResult decodeGIF(
    const std::vector<char>& inputBuffer, bool needsDecoding)
{
    ImageDecoder::DecodeResult result;

    int row = 0, col = 0;
    int width = 0, height = 0;
    int extCode = 0;
    int i = 0, j = 0;
    int errorCode = 0;
    unsigned int imageNum = 0;
    unsigned long size = 0;

    GifRecordType recordType = UNDEFINED_RECORD_TYPE;
    GifRowType* screenBuffer = nullptr;
    GifFileType* gifFile = nullptr;
    ColorMapObject* colorMap = nullptr;

    ImageDecoder::GifReadData readData;

    readData.mem = (void*)inputBuffer.data();
    readData.pos = 0;
    readData.size = inputBuffer.size();
#ifdef GIF_LIB_VERSION
    gifFile = DGifOpen(&readData, gifRead);
#else
    gifFile = DGifOpen(&readData, gifRead, &errorCode);
#endif
    if (!gifFile) {
        return result;
    }

    result.m_width = gifFile->SWidth;
    result.m_height = gifFile->SHeight;
    result.m_stride = result.m_width * 4;

    if (needsDecoding) {
        screenBuffer =
            (GifRowType*)malloc(result.m_height * sizeof(GifRowType));
        STARFISH_RELEASE_ASSERT(screenBuffer != nullptr);

        size = result.m_width * sizeof(GifPixelType);
        screenBuffer[0] = (GifRowType)calloc(1, size);

        for (i = 0; i < (int)(result.m_width); i++) {
            screenBuffer[0][i] = gifFile->SBackGroundColor;
        }

        for (i = 1; i < (int)(result.m_height); i++) {
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
                if (imageNum > 1) {
                    break;
                }
                if (gifFile->Image.Interlace) {
                    int interlacedOffset[] = { 0, 4, 2, 1 };
                    int interlacedJumps[] = { 8, 8, 4, 2 };
                    for (i = 0; i < 4; i++) {
                        for (j = row + interlacedOffset[i]; j < row + height;
                             j += interlacedJumps[i]) {
                            DGifGetLine(gifFile, &screenBuffer[j][col], width);
                        }
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
        } while (recordType != TERMINATE_RECORD_TYPE);

        if (imageNum > 1) {
            result.m_isAnimatedGIF = true;
        }

        colorMap = (gifFile->Image.ColorMap ? gifFile->Image.ColorMap
                                            : gifFile->SColorMap);

        if (colorMap == nullptr) {
            releaseGIFResource(gifFile, screenBuffer, result.m_height);
            return result;
        }

        // Convert GIF to RGBA
        GifRowType gifRow = nullptr;
        GifColorType* colorMapEntry = nullptr;
        GifByteType* buffer = nullptr;

        result.m_buffer =
            (uint8_t*)malloc(result.m_width * result.m_height * 4);
        STARFISH_RELEASE_ASSERT(result.m_buffer != nullptr);
        buffer = (GifByteType*)result.m_buffer;
        for (unsigned long h = 0; h < result.m_height; h++) {
            gifRow = screenBuffer[h];
            for (unsigned long w = 0; w < result.m_width; w++) {
                colorMapEntry = &colorMap->Colors[gifRow[w]];

                if (gifRow[w] == transparentIndex) {
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                } else {
#ifdef PORT_PIXEL_ORDER_RGBA
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
    }

    releaseGIFResource(gifFile, screenBuffer, result.m_height);

    result.m_isSuccessful = true;
    return result;
}

static ImageDecoder::DecodeResult decodeBuffer(
    const std::vector<char>& inputBuffer, bool full)
{
    if (isPNGFormat(inputBuffer)) {
        return decodePNG(inputBuffer, full);
    } else if (isJPGFormat(inputBuffer)) {
        return decodeJPG(inputBuffer, full);
    } else if (isGIFFormat(inputBuffer)) {
        return decodeGIF(inputBuffer, full);
    }
    return ImageDecoder::DecodeResult();
}

ImageDecoder::DecodeResult ImageDecoder::decodeJustImageSize()
{
    return decodeBuffer(m_inputBuffer, false);
}

ImageDecoder::DecodeResult ImageDecoder::decode()
{
    return decodeBuffer(m_inputBuffer, true);
}

bool ImageDecoder::isAnimatedGIF(const std::vector<char>& inputBuffer)
{
    bool result = false;
    int errorCode = 0;
    if (isGIFFormat(inputBuffer)) {
        int errorCode = 0;
        unsigned int imageNum = 0;
        int ext_code = 0;
        GifByteType* extension = nullptr;

        GifRecordType recordType = UNDEFINED_RECORD_TYPE;
        GifFileType* gifFile = nullptr;

        ImageDecoder::GifReadData readData;

        readData.mem = (void*)inputBuffer.data();
        readData.pos = 0;
        readData.size = inputBuffer.size();
#ifdef GIF_LIB_VERSION
        gifFile = DGifOpen(&readData, gifRead);
#else
        gifFile = DGifOpen(&readData, gifRead, &errorCode);
#endif
        if (!gifFile) {
            return false;
        }
        do {
            DGifGetRecordType(gifFile, &recordType);
            switch (recordType) {
            case IMAGE_DESC_RECORD_TYPE:
                DGifGetImageDesc(gifFile);
                imageNum++;
                break;
            case EXTENSION_RECORD_TYPE:
                if (DGifGetExtension(gifFile, &ext_code, &extension) ==
                    GIF_ERROR) {
                    break;
                }
                while (extension != NULL) {
                    if (DGifGetExtensionNext(gifFile, &extension) == GIF_OK) {
                        continue;
                    }
                }
                break;
            case TERMINATE_RECORD_TYPE:
                break;
            default:
                break;
            }
            if (imageNum > 1) {
                result = true;
                break;
            }
        } while (recordType != TERMINATE_RECORD_TYPE);
#ifdef GIF_LIB_VERSION
        DGifCloseFile(gifFile);
#elif GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
        DGifCloseFile(gifFile, &errorCode);
#else
        DGifCloseFile(gifFile);
#endif
    }
    return result;
}

bool ImageDecoder::prepareAnimatedGIF()
{
    GifFileType* gifFile = (GifFileType*)m_gifFile;
    GifRowType* gifBuffer = (GifRowType*)m_gifBuffer;

    if (gifFile == nullptr) {
        int errorCode = 0;
        int size = 0;

        m_gifReadData.mem = (void*)m_inputBuffer.data();
        m_gifReadData.pos = 0;
        m_gifReadData.size = m_inputBuffer.size();
#ifdef GIF_LIB_VERSION
        gifFile = DGifOpen(&m_gifReadData, gifRead);
#else
        gifFile = DGifOpen(&m_gifReadData, gifRead, &errorCode);
#endif
        if (!gifFile) {
            return false;
        }
        gifBuffer = (GifRowType*)malloc(gifFile->SHeight * sizeof(GifRowType));
        STARFISH_RELEASE_ASSERT(gifBuffer != nullptr);

        size = gifFile->SWidth * sizeof(GifPixelType);
        gifBuffer[0] = (GifRowType)calloc(1, size);

        for (int i = 0; i < (int)(gifFile->SWidth); i++) {
            gifBuffer[0][i] = gifFile->SBackGroundColor;
        }

        for (int i = 1; i < (int)(gifFile->SHeight); i++) {
            gifBuffer[i] = (GifRowType)calloc(1, size);
            memcpy(gifBuffer[i], gifBuffer[0], size);
        }

        m_decodedBuffer =
            (uint8_t*)malloc(gifFile->SWidth * gifFile->SHeight * 4);
        STARFISH_RELEASE_ASSERT(m_decodedBuffer != nullptr);

        m_gifFile = gifFile;
        m_gifBuffer = gifBuffer;
    }
    return true;
}

ImageDecoder::DecodeResult ImageDecoder::nextFrameOfAnimatedGIF()
{
    bool isNewFrame = false;
    size_t row = 0, col = 0;
    size_t width = 0, height = 0;
    int extCode = 0;
    int errorCode = 0;
    int transparentIndex = -1;
    ColorMapObject* colorMap = nullptr;
    GifRecordType recordType = UNDEFINED_RECORD_TYPE;

    prepareAnimatedGIF();

    GifFileType* gifFile = (GifFileType*)m_gifFile;
    GifRowType* gifBuffer = (GifRowType*)m_gifBuffer;
    DecodeResult result;
    result.m_width = gifFile->SWidth;
    result.m_height = gifFile->SHeight;
    result.m_stride = result.m_width * 4;
    result.m_buffer = m_decodedBuffer;

    colorMap = (gifFile->Image.ColorMap ? gifFile->Image.ColorMap
                                        : gifFile->SColorMap);

    do {
        DGifGetRecordType(gifFile, &recordType);
        switch (recordType) {
        case IMAGE_DESC_RECORD_TYPE:
            DGifGetImageDesc(gifFile);

            row = gifFile->Image.Top;
            col = gifFile->Image.Left;
            width = gifFile->Image.Width;
            height = gifFile->Image.Height;

            if (gifFile->Image.Interlace) {
                int interlacedOffset[] = { 0, 4, 2, 1 };
                int interlacedJumps[] = { 8, 8, 4, 2 };
                for (size_t i = 0; i < 4; i++) {
                    for (size_t j = row + interlacedOffset[i]; j < row + height;
                         j += interlacedJumps[i]) {
                        DGifGetLine(gifFile, &gifBuffer[j][col], width);
                    }
                }
            } else {
                for (size_t i = 0; i < height; i++) {
                    DGifGetLine(gifFile, &gifBuffer[row++][col], width);
                }
            }

            {
                // Convert GIF to RGBA
                GifRowType gifRow = nullptr;
                GifColorType* colorMapEntry = nullptr;
                GifByteType* buffer = nullptr;

                STARFISH_RELEASE_ASSERT(result.m_buffer != nullptr);

                row = gifFile->Image.Top;
                col = gifFile->Image.Left;
                width = gifFile->Image.Width;
                height = gifFile->Image.Height;

                for (unsigned long h = row; h < row + height; h++) {
                    gifRow = gifBuffer[h];
                    for (unsigned long w = col; w < col + width; w++) {
                        buffer = result.m_buffer + h * result.m_stride + w * 4;
                        colorMapEntry = &colorMap->Colors[gifRow[w]];
                        if (gifRow[w] == transparentIndex) {
                            buffer = buffer + 4;
                        } else {
#ifdef PORT_PIXEL_ORDER_RGBA
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
            }
            isNewFrame = true;

            break;
        case EXTENSION_RECORD_TYPE: {
            GifByteType* extension = nullptr;
            if (DGifGetExtension(gifFile, &extCode, &extension) == GIF_ERROR)
                return result;
            do {
                switch (extCode) {
                case COMMENT_EXT_FUNC_CODE: {
                    break;
                }
                case GRAPHICS_EXT_FUNC_CODE: {
                    const int flags = extension[1];
                    const int dispose =
                        (flags >> GIF_DISPOSE_SHIFT) & GIF_DISPOSE_MASK;
                    const int delay = extension[2] | (extension[3] << 8);
                    result.delay = delay;
                    if (extension[0] != 4) {
                        return result;
                    }
                    if (dispose == 3) {
                    } else {
                    }
                    transparentIndex =
                        (flags & GIF_TRANSPARENT_MASK) ? extension[4] : -1;
                    break;
                }
                case PLAINTEXT_EXT_FUNC_CODE: {
                    break;
                }
                case APPLICATION_EXT_FUNC_CODE: {
                    break;
                }
                default:
                    break;
                }
                DGifGetExtensionNext(gifFile, &extension);

            } while (extension != nullptr);
            break;
        }
        case TERMINATE_RECORD_TYPE: {
#ifdef GIF_LIB_VERSION
            DGifCloseFile(gifFile);
#elif GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
            DGifCloseFile(gifFile, &errorCode);
#else
            DGifCloseFile(gifFile);
#endif
            m_gifReadData.mem = (void*)m_inputBuffer.data();
            m_gifReadData.pos = 0;
            m_gifReadData.size = m_inputBuffer.size();
#ifdef GIF_LIB_VERSION
            gifFile = DGifOpen(&readData, gifRead);
#else
            gifFile = DGifOpen(&m_gifReadData, gifRead, &errorCode);
#endif
            result.m_isSuccessful = false;
            result.delay = 0;
            m_gifFile = gifFile;
        } break;
        default:
            break;
        }
        if (isNewFrame) {
            break;
        }
    } while (recordType != TERMINATE_RECORD_TYPE);

    if (colorMap == nullptr) {
        releaseGIFResource(gifFile, gifBuffer, result.m_height);
        return result;
    }
    result.m_isSuccessful = true;
    return result;
}

ImageDecoder::~ImageDecoder()
{
    if (m_gifFile) {
        GifFileType* gifFile = (GifFileType*)m_gifFile;
        GifRowType* gifBuffer = (GifRowType*)m_gifBuffer;
        releaseGIFResource(gifFile, gifBuffer, gifFile->SHeight);
        m_gifFile = nullptr;
        m_gifBuffer = nullptr;
    }
}

} // namespace Starfish

#endif

#if defined(PORT_IMAGEDECODER_BACKEND_MOCK)

namespace Starfish {

ImageDecoder::~ImageDecoder()
{
}

ImageDecoder::DecodeResult ImageDecoder::decodeJustImageSize()
{
    return ImageDecoder::DecodeResult();
}

ImageDecoder::DecodeResult ImageDecoder::decode()
{
    return ImageDecoder::DecodeResult();
}

bool ImageDecoder::prepareAnimatedGIF()
{
    return false;
}

bool ImageDecoder::isAnimatedGIF(const std::vector<char>& inputBuffer)
{
    return false;
}

ImageDecoder::DecodeResult ImageDecoder::nextFrameOfAnimatedGIF()
{
    return ImageDecoder::DecodeResult();
}
}
#endif
