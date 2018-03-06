/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameSVGCircleBox__
#define __StarFishFrameSVGCircleBox__

#include "core/layout/svg/FrameSVGBox.h"

namespace StarFish {

class FrameSVGCircleBox : public FrameSVGBox {
public:
    FrameSVGCircleBox(Node* node)
        : FrameSVGBox(node)
    {
    }

    virtual bool isFrameSVGCircleBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameSVGCircleBox";
    }

    virtual void paintSVG(PaintingContext& ctx);

protected:
};
}

#endif
