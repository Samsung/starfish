/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_CANVAS

#ifndef __StarFishFrameReplacedCanvas__
#define __StarFishFrameReplacedCanvas__

#include "core/layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedCanvas final : public FrameReplaced {
public:
    FrameReplacedCanvas(Node* node)
        : FrameReplaced(node, nullptr)
    {
    }

    virtual const char* name() override
    {
        return "FrameReplacedCanvas";
    }

    virtual bool isFrameCanvas()
    {
        return true;
    }

    void paintContent(PaintingContext& ctx);

    virtual IntrinsicSize intrinsicSize() override;
};
}
#endif
#endif
