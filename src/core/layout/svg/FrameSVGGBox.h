/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFrameSVGGBox__
#define __StarfishFrameSVGGBox__

#include "core/layout/svg/FrameSVGBox.h"

namespace Starfish {

class FrameSVGGBox final : public FrameSVGBox {
public:
    FrameSVGGBox(Node* node)
        : FrameSVGBox(node)
    {
    }

    virtual const char* name() override
    {
        return "FrameSVGGBox";
    }

    virtual void postLayoutSVG() override
    {
        LayoutRect rt;
        Frame* f = firstChild();
        while (f) {
            if (f->isFrameSVGBox()) {
                rt.unite(f->asFrameBox()->frameRect());
            }
            f = f->next();
        }
        m_frameRect = rt;
    }

protected:
};
} // namespace Starfish

#endif
