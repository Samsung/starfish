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

#ifndef __StarfishShellRendererDelegate__
#define __StarfishShellRendererDelegate__

#include <cstdint>

namespace StarfishShell {

class RendererDelegate {
public:
    RendererDelegate() = default;
    virtual ~RendererDelegate() = default;

    virtual bool makeCurrent() = 0;
    virtual bool swapBuffers() = 0;
    virtual uintptr_t createSharedContext() = 0;
    virtual bool destroyContext(uintptr_t context) = 0;
    virtual bool clearCurrentContext() = 0;
    virtual bool makeCurrentWithContext(uintptr_t context) = 0;
    virtual void* getProcAddress(const char* name) = 0;
};

} // namespace StarfishShell

#endif
