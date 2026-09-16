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

#include "core/modules/canvas/image/AnimatedGIFNativeImageData.h"
#include "core/modules/canvas/image/ImageDecoder.h"
#include "core/modules/canvas/Canvas.h"

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#include <cairo.h>
#endif

#include <chrono>

#define MinimumDelay 3

// How many frames a single prepareNextFrame() call decodes at most while
// catching up. GIF frames are deltas, so reaching a later frame always costs a
// decode per intervening frame; past this many frames the catch-up itself is
// what keeps the animation late, so the rest of the backlog is dropped.
#define MaxCatchUpFrames 32

namespace Starfish {

class AnimatedGIFNativeImageDataImpl : public AnimatedGIFNativeImageData {
public:
    void* operator new(size_t size)
    {
        return GC_GENERIC_MALLOC(
            sizeof(AnimatedGIFNativeImageDataImpl),
            BufferedNativeImageData::nativeImageDataGCKind());
    }

    AnimatedGIFNativeImageDataImpl(
        const std::vector<char>& compressedImageData, std::string&& imageURL,
        uint32_t needsDownScaleImageResourceLargerThan, float devicePixelRatio,
        size_t width, size_t height, size_t stride)
        : m_image(nullptr)
        , m_width(width)
        , m_stride(stride)
        , m_height(height)
        , m_imageURL(imageURL)
        , m_needsDownScaleImageResourceLargerThan(
              needsDownScaleImageResourceLargerThan)
        , m_devicePixelRatio(devicePixelRatio)
#if defined(PORT_CANVAS_BACKEND_CAIRO)
        , m_imageSurface(nullptr)
#endif
        , m_delay(0)
        , m_imageDecoder(nullptr)
    {
        STARFISH_ASSERT(width != 0);
        STARFISH_ASSERT(height != 0);
        STARFISH_ASSERT(compressedImageData.size() != 0);

        m_inputBuffer.insert(m_inputBuffer.end(), compressedImageData.begin(),
                             compressedImageData.end());

        m_imageDecoder = new ImageDecoder(
            m_inputBuffer, m_needsDownScaleImageResourceLargerThan,
            m_devicePixelRatio);
    }

    virtual ~AnimatedGIFNativeImageDataImpl()
    {
        disposeNativeImageData();
    }

    virtual void pruneInternalDataIfPossible() override
    {
        if (m_imageDecoder->loopCount() == 0) {
#if defined(PORT_CANVAS_BACKEND_CAIRO)
            if (m_imageSurface) {
                cairo_surface_destroy(m_imageSurface);
                m_imageSurface = nullptr;
            }
#endif
            if (m_inputBuffer.size() > 0) {
                m_inputBuffer.clear();
            }
        }
    }

    virtual FrameUpdateResult prepareNextFrame() override
    {
        if (m_width == 0 || m_height == 0 || !m_imageDecoder) {
            return FrameUpdateResult::Finished;
        }

        // Several image elements can share this data (e.g. the same data:
        // URL) and each of them drives its own frame timer, so the animation
        // clock lives here: a tick that arrives before the current frame is
        // due leaves the frame alone. Otherwise the animation would run N
        // times faster with N clients.
        auto now = Clock::now();
        if (m_hasNextFrameTime && now < m_nextFrameTime) {
            return FrameUpdateResult::NoChange;
        }

        bool decoded = false;
        size_t steps = 0;
        do {
            if (!decodeNextFrame()) {
                return decoded ? FrameUpdateResult::Updated
                               : FrameUpdateResult::Finished;
            }
            decoded = true;

            // Count the frame from the deadline it was due at, not from the
            // moment we got around to decoding it, so the decode and dispatch
            // cost isn't added to every frame and compounded into drift.
            m_nextFrameTime = (m_hasNextFrameTime ? m_nextFrameTime : now) +
                              std::chrono::milliseconds(m_delay * 10);
            m_hasNextFrameTime = true;

            // Decoding takes time, so ask again whether the frame we just
            // produced is itself already outdated. If it is, decode through
            // the boundaries that are already past: only the last frame of
            // the run is painted, so a late tick jumps straight to the frame
            // that is due instead of flashing through the skipped ones.
            now = Clock::now();
        } while (m_nextFrameTime <= now && ++steps < MaxCatchUpFrames);

        if (m_nextFrameTime <= now) {
            // Still behind after decoding a whole backlog: the decoder cannot
            // keep up with the rate this GIF asks for. Drop the remaining debt
            // and restart the schedule from here, otherwise it carries into
            // every later tick and the animation plays at decode speed for
            // good.
            m_nextFrameTime = now + std::chrono::milliseconds(m_delay * 10);
        }
        return FrameUpdateResult::Updated;
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
        if (m_imageDecoder) {
            delete m_imageDecoder;
            m_imageDecoder = nullptr;
        }
        free(m_image);
        std::vector<char>().swap(m_inputBuffer);
        std::string().swap(m_imageURL);
        AnimatedGIFNativeImageData::disposeNativeImageData();
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

    virtual uint64_t delayUntilNextFrameInMs() override
    {
        if (!m_hasNextFrameTime) {
            return 0;
        }
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                        m_nextFrameTime - Clock::now())
                        .count();
        return left > 0 ? (uint64_t)left : 0;
    }

private:
    // Decodes the next frame into m_image. Returns false once the animation
    // has no further frame to show.
    bool decodeNextFrame()
    {
        if (!m_image) {
            STARFISH_ASSERT(m_stride == m_width * 4);
            m_image = (uint8_t*)malloc(m_width * m_height * 4);
            STARFISH_RELEASE_ASSERT(m_image != nullptr);
        }

        auto idResult =
            m_imageDecoder->nextFrameOfAnimatedGIF(m_image, m_width, m_height);
        if (idResult.m_width == 0 && idResult.m_height == 0) {
            return false;
        }
        STARFISH_ASSERT(m_image == idResult.m_buffer);

        m_delay = std::max<size_t>(idResult.delay, MinimumDelay);
        return true;
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
    uint8_t* m_image;
    size_t m_width;
    size_t m_stride;
    size_t m_height;
    std::vector<char> m_inputBuffer;
    std::string m_imageURL;
    uint32_t m_needsDownScaleImageResourceLargerThan;
    float m_devicePixelRatio;
#if defined(PORT_CANVAS_BACKEND_CAIRO)
    cairo_surface_t* m_imageSurface;
#endif
    size_t m_delay{ 0 };
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_nextFrameTime;
    bool m_hasNextFrameTime{ false };
    ImageDecoder* m_imageDecoder{ nullptr };
};

NativeImageData* AnimatedGIFNativeImageData::create(
    const std::vector<char>& compressedImageData, std::string&& imageURL,
    uint32_t needsDownScaleImageResourceLargerThan, float devicePixelRatio,
    size_t width, size_t height, size_t stride)
{
    STARFISH_ASSERT(width != 0);
    STARFISH_ASSERT(height != 0);
    STARFISH_ASSERT(compressedImageData.size() != 0);

    NativeImageData* imageData = new AnimatedGIFNativeImageDataImpl(
        compressedImageData, std::move(imageURL),
        needsDownScaleImageResourceLargerThan, devicePixelRatio, width, height,
        stride);
    return imageData;
}
} // namespace Starfish
