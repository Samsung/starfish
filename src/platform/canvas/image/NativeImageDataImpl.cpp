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

#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/modules/canvas/Canvas.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
#endif

namespace Starfish {

class NativeImageDataImpl : public NativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_GENERIC_MALLOC(
            sizeof(NativeImageDataImpl),
            BufferedNativeImageData::nativeImageDataGCKind());
    }

    NativeImageDataImpl(size_t w, size_t h)
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

    virtual ~NativeImageDataImpl()
    {
        disposeNativeImageData();
    }

    virtual void pruneInternalDataIfPossible() override
    {
        if (m_image) {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
            cairo_surface_destroy(m_imageSurface);
            m_imageSurface = nullptr;
#endif
            free(m_image);
            m_image = nullptr;
        }
    }

    virtual uint8_t* data() override
    {
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

private:
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
#if defined(PORT_CANVAS_BACKEND_CAIRO)
    cairo_surface_t* m_imageSurface;
#endif
};

NativeImageData* BufferedNativeImageData::create(size_t width, size_t height)
{
    return new NativeImageDataImpl(width, height);
}
} // namespace Starfish
