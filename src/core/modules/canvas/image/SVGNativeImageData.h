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

#ifndef __SVGNativeImageData__
#define __SVGNativeImageData__

#include "NativeImageData.h"

namespace Starfish {

class FrameSVGSVGBox;
class FrameBox;
class SVGNativeImageData : public NativeImageData {
public:
    static NativeImageData* create(size_t actualDeviceWidth,
                                   size_t actualDeviceHeight,
                                   FrameSVGSVGBox* frameSVGSVGBox);
    virtual bool isSVGNativeImageData() const
    {
        return true;
    }

    FrameSVGSVGBox* frameSVGSVGBox()
    {
        return m_frameSVGSVGBox;
    }

    virtual bool hasViewport() = 0;

    virtual void updateContentSize(FrameBox* containingBlock) = 0;

protected:
    SVGNativeImageData()
    {
    }

    FrameSVGSVGBox* m_frameSVGSVGBox = nullptr;
};
} // namespace Starfish

#endif
