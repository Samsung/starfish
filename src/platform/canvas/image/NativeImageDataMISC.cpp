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

#include "StarfishConfig.h"

#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/image/ImageDecoder.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
#endif

#define MinimumDelay 3

namespace Starfish {

class NativeImageDataMISC : public NativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_GENERIC_MALLOC(sizeof(NativeImageDataMISC),
                                 NativeImageData::nativeImageDataGCKind());
    }

    NativeImageDataMISC(const std::vector<char>& compressedImageData,
                        std::string&& imageURL)
        : m_image(nullptr)
        , m_width(0)
        , m_stride(0)
        , m_height(0)
        , m_imageURL(imageURL)
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        , m_imageSurface(nullptr)
#endif
        , m_isAnimatedGIF(false)
        , m_delay(0)
        , m_imageDecoder(nullptr)
    {
        if (compressedImageData.data() && compressedImageData.size() != 0) {
            ImageDecoder id(compressedImageData);
            auto idResult = id.decodeJustImageSize();
            if (idResult.m_isSuccessful && idResult.m_width &&
                idResult.m_height) {
                m_width = idResult.m_width;
                m_height = idResult.m_height;
                m_stride = idResult.m_stride;
                m_inputBuffer.insert(m_inputBuffer.end(),
                                     compressedImageData.begin(),
                                     compressedImageData.end());
            }
        }
    }

    NativeImageDataMISC(const std::vector<char>& compressedImageData,
                        std::string&& imageURL, uint8_t* decodedImageBuffer,
                        size_t width, size_t height, size_t stride)
        : m_image(decodedImageBuffer)
        , m_width(width)
        , m_stride(stride)
        , m_height(height)
        , m_imageURL(imageURL)
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        , m_imageSurface(nullptr)
#endif
        , m_isAnimatedGIF(false)
        , m_delay(0)
        , m_imageDecoder(nullptr)
    {
        STARFISH_ASSERT(decodedImageBuffer != nullptr);
        STARFISH_ASSERT(width != 0);
        STARFISH_ASSERT(height != 0);
        STARFISH_ASSERT(compressedImageData.size() != 0);

        m_inputBuffer.insert(m_inputBuffer.end(), compressedImageData.begin(),
                             compressedImageData.end());
    }

    NativeImageDataMISC(const std::vector<char>& compressedImageData,
                        std::string&& imageURL, size_t width, size_t height,
                        size_t stride)
        : m_image(nullptr)
        , m_width(width)
        , m_stride(stride)
        , m_height(height)
        , m_imageURL(imageURL)
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        , m_imageSurface(nullptr)
#endif
        , m_isAnimatedGIF(true)
        , m_delay(0)
        , m_imageDecoder(nullptr)
    {
        STARFISH_ASSERT(width != 0);
        STARFISH_ASSERT(height != 0);
        STARFISH_ASSERT(compressedImageData.size() != 0);

        m_inputBuffer.insert(m_inputBuffer.end(), compressedImageData.begin(),
                             compressedImageData.end());

        m_imageDecoder = new ImageDecoder(m_inputBuffer);
    }

    NativeImageDataMISC(size_t w, size_t h)
    {
        m_image = (unsigned char*)malloc(w * h * 4);
        STARFISH_RELEASE_ASSERT(m_image);
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        m_imageSurface = nullptr;
#endif
        m_width = w;
        m_height = h;
        m_stride = w * 4;
        initInternalSurface();
    }

    virtual ~NativeImageDataMISC()
    {
        disposeNativeImageData();
    }

    virtual const std::string& compressedImageURL() override
    {
        STARFISH_ASSERT(hasCompressedData());
        return m_imageURL;
    }

    virtual void pruneInternalDataIfPossible() override
    {
        if (m_image && m_inputBuffer.size()) {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
            cairo_surface_destroy(m_imageSurface);
            m_imageSurface = nullptr;
#endif
            free(m_image);
            m_image = nullptr;
        }
    }

    virtual void prepareNextFrame() override
    {
        if (m_imageDecoder && hasAnimatedGIF()) {
            STARFISH_ASSERT(m_imageDecoder != nullptr);
            auto idResult = m_imageDecoder->nextFrameOfAnimatedGIF();
            m_delay = idResult.delay;
            if (m_delay <= MinimumDelay) {
                m_delay = MinimumDelay;
            }
            m_image = idResult.m_buffer;
        }
    }

    virtual uint8_t* data() override
    {
        if (!hasAnimatedGIF() && !m_image && m_inputBuffer.size()) {
            ImageDecoder id(m_inputBuffer);
            auto idResult = id.decode();
            if (idResult.m_isSuccessful) {
                m_image = idResult.m_buffer;
            } else {
                // fallback
                m_image = malloc(m_stride * m_height);
                STARFISH_RELEASE_ASSERT(m_image);
                memset(m_image, 0x00, m_stride * m_height);
            }
            initInternalSurface();

            if (bufferSize() < m_inputBuffer.size()) {
                std::vector<char>().swap(m_inputBuffer);
            }
        }
        return (uint8_t*)m_image;
    }

    virtual void clear() override
    {
        void* address = m_image;
        if (address) {
            size_t end = bufferSize();
            memset(address, 0x00, end);
        }
    }

    virtual size_t bufferSize() override
    {
        return m_stride * m_height;
    }

    virtual void disposeNativeImageData() override
    {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        if (m_imageSurface) {
            cairo_surface_destroy(m_imageSurface);
        }
#endif
        if (m_imageDecoder) {
            delete m_imageDecoder;
            m_imageDecoder = nullptr;
        }
        free(m_image);
        std::vector<char>().swap(m_inputBuffer);
        std::string().swap(m_imageURL);
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

    virtual void* unwrap() override
    {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        return m_imageSurface;
#elif defined(PORT_CANVAS_BACKEND_SKIA)
        return nullptr;
#elif defined(PORT_CANVAS_BACKEND_MOCK)
        return nullptr;
#else
        return nullptr;
#endif
    }

    virtual size_t width() override
    {
        return m_width;
    }

    virtual size_t stride() override
    {
        return m_stride;
    }

    virtual size_t height() override
    {
        return m_height;
    }

    virtual bool hasAnimatedGIF() override
    {
        return m_isAnimatedGIF;
    }

    virtual size_t delay() override
    {
        return m_delay;
    }

private:
    virtual bool hasCompressedData() override
    {
        return m_inputBuffer.size();
    }

    virtual bool isDecompressed() override
    {
        if (hasCompressedData()) {
            return m_image != nullptr;
        } else {
            return true;
        }
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
    void* m_image;
    size_t m_width;
    size_t m_stride;
    size_t m_height;
    std::vector<char> m_inputBuffer;
    std::string m_imageURL;
#if defined(PORT_CANVAS_BACKEND_CAIRO)
    cairo_surface_t* m_imageSurface;
#endif
    bool m_isAnimatedGIF{ false };
    size_t m_delay{ 0 };
    ImageDecoder* m_imageDecoder{ nullptr };
};

NativeImageData* NativeImageData::create(
    const std::vector<char>& compressedImageData, std::string&& imageURL)
{
    NativeImageData* imageData =
        new NativeImageDataMISC(compressedImageData, std::move(imageURL));
    if (imageData->width() == 0 || imageData->height() == 0) {
        return NULL;
    }
    return imageData;
}

NativeImageData* NativeImageData::create(
    const std::vector<char>& compressedImageData, std::string&& imageURL,
    uint8_t* decodedImageBuffer, size_t width, size_t height, size_t stride)
{
    STARFISH_ASSERT(decodedImageBuffer != nullptr);
    STARFISH_ASSERT(width != 0);
    STARFISH_ASSERT(height != 0);
    STARFISH_ASSERT(compressedImageData.size() != 0);

    NativeImageData* imageData =
        new NativeImageDataMISC(compressedImageData, std::move(imageURL),
                                decodedImageBuffer, width, height, stride);
    return imageData;
}

NativeImageData* NativeImageData::create(
    const std::vector<char>& compressedImageData, std::string&& imageURL,
    size_t width, size_t height, size_t stride)
{
    STARFISH_ASSERT(width != 0);
    STARFISH_ASSERT(height != 0);
    STARFISH_ASSERT(compressedImageData.size() != 0);

    NativeImageData* imageData = new NativeImageDataMISC(
        compressedImageData, std::move(imageURL), width, height, stride);
    return imageData;
}

NativeImageData* NativeImageData::create(size_t width, size_t height)
{
    return new NativeImageDataMISC(width, height);
}
} // namespace Starfish
