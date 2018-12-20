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

#ifndef __StarfishWebStorageNamespace__
#define __StarfishWebStorageNamespace__

#include "binding/WindowHoldable.h"

namespace Starfish {

class WebOrigin;
class Storage;
class Window;

class StorageNamespace : public gc {
public:
    virtual Storage* storage(Window* window, WebOrigin* origin) = 0;
    virtual ~StorageNamespace()
    {
    }

protected:
    StorageNamespace()
    {
    }
};
}

#endif
