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

#ifndef __StarfishNativeGradient__
#define __StarfishNativeGradient__

#include "core/style/GradientData.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

struct GradientDrawingInfo;
class NativeImageData;

class NativeGradient : public gc {
public:
    static std::shared_ptr<NativeGradient> create(GradientDrawingInfo* info);
    static std::shared_ptr<NativeGradient> create(double x0, double y0,
                                                  double x1, double y1);
    static std::shared_ptr<NativeGradient> create(double x0, double y0,
                                                  double r0, double x1,
                                                  double y1, double r1);

    virtual ~NativeGradient()
    {
        removeGradientImageDataCached();
    }

    NativeImageData* gradientImageDataCached()
    {
        return m_gradientImageDataCached;
    }

    void setGradientImageDataCached(NativeImageData* imageData)
    {
        STARFISH_ASSERT(m_gradientDrawingInfo != nullptr);
        m_gradientImageDataCached = imageData;
    }

    virtual void addColorStop(const double& offset, const Unit::Color& color)
    {
        STARFISH_UNIMPLEMENTED();
    }

    virtual bool isZeroSize()
    {
        STARFISH_UNIMPLEMENTED();
        return false;
    }

protected:
    NativeGradient()
        : m_gradientDrawingInfo(nullptr)
        , m_gradientImageDataCached(nullptr)
    {
    }

    NativeGradient(GradientDrawingInfo* info)
        : m_gradientDrawingInfo(info)
        , m_gradientImageDataCached(nullptr)
    {
        STARFISH_ASSERT(info != nullptr);
    }

    void removeGradientImageDataCached()
    {
        if (m_gradientDrawingInfo) {
            delete m_gradientDrawingInfo;
            m_gradientDrawingInfo = nullptr;
        }
        if (m_gradientImageDataCached) {
            delete m_gradientImageDataCached;
            m_gradientImageDataCached = nullptr;
        }
    }

    GradientDrawingInfo* m_gradientDrawingInfo;
    NativeImageData* m_gradientImageDataCached;
};
} // namespace Starfish
#endif
