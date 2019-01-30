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

#ifndef __StarfishMessageLoopInterface__
#define __StarfishMessageLoopInterface__

namespace Starfish {

class ScriptContext;

class MessageLoopInterface {
public:
    virtual ~MessageLoopInterface()
    {
    }
    virtual size_t addIdler(ScriptContext* ctx,
                            void (*fn)(size_t handle, void*), void* data) = 0;
    virtual size_t addIdler(ScriptContext* ctx,
                            void (*fn)(size_t handle, void*, void*), void* data,
                            void* data1) = 0;
    virtual size_t addIdler(ScriptContext* ctx,
                            void (*fn)(size_t handle, void*, void*, void*),
                            void* data, void* data1, void* data2) = 0;
    virtual size_t addIdlerWithNoGCRootingInOtherThread(
        ScriptContext* ctx, void (*fn)(size_t handle, void*), void* data) = 0;
    virtual size_t addIdlerWithNoGCRootingInOtherThread(
        ScriptContext* ctx, void (*fn)(size_t handle, void*, void*), void* data,
        void* data1) = 0;

    virtual void removeIdler(size_t handle) = 0;
    virtual void removeIdlerWithNoGCRooting(size_t handle) = 0;
    virtual void clearPendingIdlers(ScriptContext* ctx = nullptr) = 0;
    virtual void destroy() = 0;
};

} // namespace Starfish

#endif
