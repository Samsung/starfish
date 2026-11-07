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

#ifndef __CompressedNativeImageData__
#define __CompressedNativeImageData__

#include "BufferedNativeImageData.h"

namespace Starfish {

class CompressedNativeImageData : public BufferedNativeImageData {
public:
    static NativeImageData* create(
        const std::vector<char>& compressedImageData, std::string&& imageURL,
        uint32_t needsDownScaleImageResourceLargerThan, float devicePixelRatio);
    static NativeImageData* create(
        const std::vector<char>& compressedImageData, std::string&& imageURL,
        uint32_t needsDownScaleImageResourceLargerThan, float devicePixelRatio,
        uint8_t* decodedImageBuffer, size_t width, size_t height,
        size_t stride);

    virtual bool isCompressedNativeImageData() const
    {
        return true;
    }

    virtual bool hasCompressedData() = 0;
    virtual bool isDecompressed() = 0;
    virtual const std::string& compressedImageURL() = 0;
    // The image decoded again as its file holds it: RGBA order, straight
    // alpha, width() x height() with rows width() * 4 bytes apart; the
    // caller frees it. nullptr when every pixel is opaque, so data() already
    // holds those colors, or when the file cannot give them at this size.
    virtual uint8_t* decodeStraightRGBA() = 0;

protected:
    CompressedNativeImageData()
    {
    }
};
} // namespace Starfish

#endif
