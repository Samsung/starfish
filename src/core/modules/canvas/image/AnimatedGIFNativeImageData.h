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

#ifndef __AmimatedGIFNativeImageData__
#define __AmimatedGIFNativeImageData__

#include "BufferedNativeImageData.h"

namespace Starfish {

class AnimatedGIFNativeImageData : public BufferedNativeImageData {
public:
    static NativeImageData* create(const std::vector<char>& compressedImageData,
                                   std::string&& imageURL, size_t width,
                                   size_t height, size_t stride);

    virtual bool prepareNextFrame() = 0;
    virtual size_t delay() = 0;
    virtual bool isAnimatedGIFNativeImageData() const
    {
        return true;
    }

protected:
    AnimatedGIFNativeImageData()
    {
    }
};
} // namespace Starfish

#endif
