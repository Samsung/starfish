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

#include "core/modules/canvas/image/SVGNativeImageData.h"
#include "core/modules/canvas/image/ImageDecoder.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
#endif

#define MinimumDelay 3

namespace Starfish {

class SVGNativeImageDataMISC : public SVGNativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_GENERIC_MALLOC(sizeof(SVGNativeImageDataMISC),
                                 NativeImageData::nativeImageDataGCKind());
    }

    SVGNativeImageDataMISC(const std::vector<char>& compressedImageData,
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

    SVGNativeImageDataMISC(const std::vector<char>& compressedImageData,
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

    SVGNativeImageDataMISC(const std::vector<char>& compressedImageData,
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

    SVGNativeImageDataMISC(size_t w, size_t h)
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

    virtual ~SVGNativeImageDataMISC()
    {
        disposeNativeImageData();
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
} // namespace Starfish
