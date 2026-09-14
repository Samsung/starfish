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

#ifndef __StarfishImageDecoder__
#define __StarfishImageDecoder__

namespace Starfish {

class ImageDecoder {
public:
    ImageDecoder(const std::vector<char>& inputBuffer,
                 uint32_t needsDownScaleImageResourceLargerThan,
                 float devicePixelRatio)
        : m_inputBuffer(inputBuffer)
        , m_gifFile(nullptr)
        , m_gifBuffer(nullptr)
        , m_needsDownScaleImageResourceLargerThan(
              needsDownScaleImageResourceLargerThan)
        , m_devicePixelRatio(devicePixelRatio)
    {
    }

    ~ImageDecoder();
    struct DecodeResult {
        bool m_isSuccessful;
        bool m_isAnimatedGIF;
        uint8_t* m_buffer; // decoded image buffer(RGBA or BGRA format depends
                           // on port)
        size_t m_width;
        size_t m_height;
        size_t m_stride;
        size_t delay;

        DecodeResult()
            : m_isSuccessful(false)
            , m_isAnimatedGIF(false)
            , m_buffer(nullptr)
            , m_width(0)
            , m_height(0)
            , m_stride(0)
            , delay(0)
        {
        }
    };

    struct GifReadData {
        unsigned long long size{ 0 };
        unsigned long long pos{ 0 };
        void* mem{ nullptr };
    };

    DecodeResult decodeJustImageSize();
    DecodeResult decode();
    DecodeResult nextFrameOfAnimatedGIF(uint8_t* targetBuffer,
                                        size_t targetWidth,
                                        size_t targetHeight);
    int loopCount()
    {
        return m_loopCount;
    }

    static bool isAnimatedGIF(const std::vector<char>& inputBuffer);

private:
    enum class GifDisposeMethod {
        None,
        Background,
        RestorePrevious,
    };

    bool prepareAnimatedGIF();

    const std::vector<char>& m_inputBuffer;
    // for animated GIF
    void* m_gifFile;
    void* m_gifBuffer;
    GifReadData m_gifReadData;
    // The first frame of each pass is drawn on a fully transparent canvas,
    // and the previous frame's disposal is applied before the next frame.
    // The previous frame's area is kept clamped to the canvas; the snapshot
    // holds what was under it when its disposal is "restore to previous".
    bool m_gifPassStarted{ false };
    GifDisposeMethod m_gifPrevDispose{ GifDisposeMethod::None };
    size_t m_gifPrevLeft{ 0 };
    size_t m_gifPrevTop{ 0 };
    size_t m_gifPrevWidth{ 0 };
    size_t m_gifPrevHeight{ 0 };
    std::vector<uint8_t> m_gifPrevSnapshot;
    uint32_t m_needsDownScaleImageResourceLargerThan;
    float m_devicePixelRatio;
    int m_loopCount = 1;
    bool m_hasLoopCount = false;
};
} // namespace Starfish

#endif
