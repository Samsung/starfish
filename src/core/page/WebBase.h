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

#ifndef __StarfishWebBase__
#define __StarfishWebBase__

#include "binding/StarfishHoldable.h"

namespace Starfish {

class MessageLoop;
class Console;
class Inspector;

class WebBase : public StarfishHoldable {
public:
    virtual ~WebBase()
    {
    }
    virtual MessageLoop* messageLoop() const = 0;
    virtual Console* console() const = 0;

#if defined(STARFISH_ENABLE_INSPECTOR)
    virtual Inspector* inspector() const = 0;
#endif

protected:
    WebBase(Starfish* starfish)
        : StarfishHoldable(starfish)
    {
    }
};
}
#endif
