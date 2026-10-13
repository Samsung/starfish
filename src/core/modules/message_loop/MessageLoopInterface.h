/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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
#pragma once

namespace Starfish {
class GlobalScope;

class IMessageLoop {
public:
    virtual size_t addIdlerWithNoGCRootingInOtherThread(
        GlobalScope* globalScope, void (*fn)(size_t handle, void*),
        void* data) = 0;

    // Same as above, but the loop calls `discard(data)` on the main thread
    // when it drops the idler without calling `fn`. This happens when the
    // global scope is torn down or the idler is removed. Use `discard` to free
    // `data`, which nobody else owns at that point.
    virtual size_t addIdlerWithNoGCRootingInOtherThread(
        GlobalScope* globalScope, void (*fn)(size_t handle, void*),
        void (*discard)(void*), void* data) = 0;

    virtual size_t addIdlerWithNoGCRootingInOtherThread(
        GlobalScope* globalScope, void (*fn)(size_t handle, void*, void*),
        void* data, void* data1) = 0;
};

} // namespace Starfish
