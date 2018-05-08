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

#ifndef __StarFishFrameOptGroupBox__
#define __StarFishFrameOptGroupBox__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;
class ComputedStyle;

class FrameOptGroupBox : public FrameBlockBox {
public:
    FrameOptGroupBox(Node* node, ComputedStyle* style);

    virtual const char* name() override
    {
        return "FrameOptGroupBox";
    }

    virtual bool isFrameOptGroupBox() override
    {
        return true;
    }

private:
};
}

#endif
