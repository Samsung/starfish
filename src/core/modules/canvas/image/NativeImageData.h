/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __NativeImageData__
#define __NativeImageData__

namespace StarFish {

class NativeImageData : public gc {
public:
    enum PreserveAspectRatioValue {
        None,
        xMinYMin,
        xMidYMin,
        xMaxYMin,
        xMinYMid,
        xMidYMid,
        xMaxYMid,
        xMinYMax,
        xMidYMax,
        xMaxYMax,
    };

    static NativeImageData* create(
        String* localImageSrc); // this is only for EFL backend
    static NativeImageData* create(const char* buf, size_t len);
    static NativeImageData* create(size_t width, size_t height);

    virtual size_t bufferSize() = 0;
    virtual uint8_t* data() = 0;
    virtual void clear() = 0;
    virtual void* unwrap() = 0;          // Use in EFL port
    virtual void* internalSurface() = 0; // Use in Cairo port
    virtual size_t width() = 0;
    virtual size_t height() = 0;
    virtual size_t stride() = 0;
    virtual bool hasTransparentPixel() = 0;
    virtual ~NativeImageData()
    {
    }

    PreserveAspectRatioValue preserveAspectRatioValue()
    {
        return m_preserveAspectRatioValue;
    }

    void setPreserveAspectRatioValue(PreserveAspectRatioValue v)
    {
        m_preserveAspectRatioValue = v;
    }

protected:
    NativeImageData()
    {
        m_preserveAspectRatioValue = None;
    }

    PreserveAspectRatioValue m_preserveAspectRatioValue;
};
}

#endif
