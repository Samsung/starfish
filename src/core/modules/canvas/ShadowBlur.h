/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishShadowBlur__
#define __StarfishShadowBlur__

#include "core/style/Unit.h"

namespace Starfish {

class ShadowBlur {
public:
    // channels is 4 for a premultiplied ARGB image, 1 for an alpha mask.
    ShadowBlur(uint8_t* source, const size_t& width, const size_t& height,
               const size_t& stride, size_t channels = 4);
    ~ShadowBlur();
    void process(float stdDeviation);

    static const float RADIUS_LIMIT;
    static float computeKernelSizeAtStdDeviation(float stdDeviation);

private:
    size_t m_width;
    size_t m_height;
    size_t m_stride;
    size_t m_channels;
    uint8_t* m_source;
    std::unique_ptr<uint8_t, void (*)(uint8_t*)> m_workspace;
};
} // namespace Starfish
#endif
