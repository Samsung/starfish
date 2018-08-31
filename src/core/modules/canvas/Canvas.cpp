/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "Canvas.h"
#include "StarFish.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

#if !defined(PORT_COMPOSITOR_BACKEND_GL)
static size_t g_totalCanvasSurfaceSimpleSize;
class CanvasSurfaceSimple : public CanvasSurface {
public:
    CanvasSurfaceSimple(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_window = wnd;
        m_width = w;
        m_height = h;
        m_imageWidth = m_bufferWidth = m_width = -1;
        m_imageHeight = m_bufferHeight = m_height = -1;
        m_pixelRatio = 1;
        m_buffer = nullptr;

        attachNativeBuffer(w, h);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceSimple* s =
                                               (CanvasSurfaceSimple*)obj;
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void detachNativeBuffer()
    {
        if (m_buffer) {
            g_totalCanvasSurfaceSimpleSize -=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            free(m_buffer);
            m_buffer = nullptr;
            m_width = 0;
            m_height = 0;
            m_imageWidth = m_bufferWidth = m_width = 0;
            m_imageHeight = m_bufferHeight = m_height = 0;
            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceSimpleSize / 1024.f / 1024.f);
        }
    }

    bool attachNativeBuffer(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            detachNativeBuffer();
            m_width = w;
            m_height = h;

            float windowDevicePixelRatio =
                m_window->starFish()->screenInfo().devicePixelRatio;

            m_pixelRatio = 1;

            while ((m_width / m_pixelRatio * windowDevicePixelRatio > 20000) ||
                   (m_height / m_pixelRatio * windowDevicePixelRatio > 20000)) {
                m_pixelRatio++;
            }

            m_imageWidth =
                std::max((size_t)1, (size_t)(m_width / m_pixelRatio *
                                             windowDevicePixelRatio));
            m_imageHeight =
                std::max((size_t)1, (size_t)(m_height / m_pixelRatio *
                                             windowDevicePixelRatio));

            m_bufferWidth = std::max(
                (size_t)1, (size_t)(w / m_pixelRatio * windowDevicePixelRatio));
            m_bufferHeight = std::max(
                (size_t)1, (size_t)(h / m_pixelRatio * windowDevicePixelRatio));

            m_bufferStride = m_bufferWidth * 4;

            m_buffer = (unsigned char*)malloc(m_bufferWidth * m_bufferHeight *
                                              sizeof(uint32_t));
            g_totalCanvasSurfaceSimpleSize +=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceSimpleSize / 1024.f / 1024.f);
            return true;
        }
        return false;
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT(w <= m_bufferWidth * m_pixelRatio);
        STARFISH_RELEASE_ASSERT(h <= m_bufferHeight * m_pixelRatio);

        m_width = w;
        m_height = h;

        m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
        m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

        STARFISH_RELEASE_ASSERT(m_imageWidth <= m_bufferWidth);
        STARFISH_RELEASE_ASSERT(m_imageHeight <= m_bufferHeight);
    }

    virtual uint8_t* mapBuffer()
    {
        return m_buffer;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual size_t bufferWidth()
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight()
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return m_pixelRatio;
    }

    virtual size_t bufferStride()
    {
        return m_bufferStride;
    }

    virtual void clear()
    {
        size_t end = m_bufferStride * m_bufferHeight;
        memset(m_buffer, 0x00, end);
    }

protected:
    PlatformWindow* m_window;
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_pixelRatio;
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceSimple(wnd, w, h);
}
#endif

class CanvasSurfaceCanvasTarget : public CanvasSurface {
public:
    CanvasSurfaceCanvasTarget(uint8_t* buffer, size_t w, size_t h,
                              size_t stride)
    {
        m_width = w;
        m_height = h;
        m_bufferStride = stride;
        m_imageWidth = m_bufferWidth = m_width = w;
        m_imageHeight = m_bufferHeight = m_height = h;
        m_pixelRatio = 1;
        m_buffer = buffer;
    }

    virtual void detachNativeBuffer()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    bool attachNativeBuffer(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return false;
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual uint8_t* mapBuffer()
    {
        return m_buffer;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual size_t bufferWidth()
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight()
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return m_pixelRatio;
    }

    virtual size_t bufferStride()
    {
        return m_bufferStride;
    }

    virtual void clear()
    {
        size_t end = m_bufferStride * m_bufferHeight;
        memset(m_buffer, 0x00, end);
    }

protected:
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_pixelRatio;
};

CanvasSurface* CanvasSurface::createCanvasTarget(uint8_t* buffer, size_t w,
                                                 size_t h, size_t stride)
{
    return new CanvasSurfaceCanvasTarget(buffer, w, h, stride);
}
}
