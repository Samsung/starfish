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

#ifndef __StarfishImageDecoder__
#define __StarfishImageDecoder__

namespace Starfish {

class ImageDecoder {
public:
    ImageDecoder(const std::vector<char>& inputBuffer)
        : m_inputBuffer(inputBuffer)
        , m_gifFile(nullptr)
        , m_gifBuffer(nullptr)
        , m_decodedBuffer(nullptr)
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
        {
        }
    };

    struct GifReadData {
        unsigned long long size;
        unsigned long long pos;
        void* mem;
    };

    DecodeResult decodeJustImageSize();
    DecodeResult decode();
    DecodeResult nextFrameOfAnimatedGIF();

    static bool isAnimatedGIF(const std::vector<char>& inputBuffer);

private:
    bool prepareAnimatedGIF();

    const std::vector<char>& m_inputBuffer;
    // for animated GIF
    void* m_gifFile;
    void* m_gifBuffer;
    GifReadData m_gifReadData;
    uint8_t* m_decodedBuffer;
};
} // namespace Starfish

#endif
