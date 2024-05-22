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

#ifndef __StarfishShellAppLoop__
#define __StarfishShellAppLoop__

#include <memory>

namespace StarfishShell {

class AppLoop {
public:
    static std::unique_ptr<AppLoop> create();

    virtual ~AppLoop() = default;
    virtual void init() = 0;
    virtual int start(double timeoutInSec = 0) = 0;
    virtual void stop() = 0;
    virtual void deinit() = 0;

protected:
    AppLoop() = default;
};

} // namespace StarfishShell

#endif
