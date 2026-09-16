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

#ifndef __AmimatedGIFNativeImageData__
#define __AmimatedGIFNativeImageData__

#include "BufferedNativeImageData.h"

namespace Starfish {

class AnimatedGIFNativeImageData : public BufferedNativeImageData {
public:
    enum class FrameUpdateResult {
        NoChange, // the frame on screen is still the one that is due
        Updated,  // a new frame was decoded and has to be painted
        Finished, // there is no further frame to show
    };

    static NativeImageData* create(
        const std::vector<char>& compressedImageData, std::string&& imageURL,
        uint32_t needsDownScaleImageResourceLargerThan, float devicePixelRatio,
        size_t width, size_t height, size_t stride);

    // Advances the animation to the frame that belongs on screen at the moment
    // of the call. The data can be shared by several image elements, each
    // driving its own timer, so the animation clock lives here rather than in
    // the clients.
    virtual FrameUpdateResult prepareNextFrame() = 0;

    // Milliseconds left until the next frame is due; 0 means "due now".
    virtual uint64_t delayUntilNextFrameInMs() = 0;
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
