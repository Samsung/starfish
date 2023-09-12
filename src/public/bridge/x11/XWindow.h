/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishXWindow__
#define __StarfishXWindow__

#include "StarfishPlatform.h"
#include "public/bridge/x11/WindowBase.h"
#include <memory>

#if defined(PORT_WEBVIEW_BRIDGE_X11)

namespace LWE {

class XWindow final : public WindowBase {
public:
    XWindow();
    bool init(const char* appName, int width, int height) override;
    void pollEvent() override;
    void terminate() override;
    void getCursorPos(double& xpos, double& ypos) override;
    void setInitHint(int hint, int value) override;
    NativeWindowType getNativeWindowHandle() override;

private:
    struct Internal;
    std::shared_ptr<Internal> m_internal;
};

} // namespace LWE

#endif // defined(PORT_WEBVIEW_BRIDGE_X11)

#endif
