
/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishScriptBindingWindowInstance__
#define __StarfishScriptBindingWindowInstance__

namespace Starfish {

class ScriptBindingWindowInstance final : public ScriptBindingInstance {
public:
    ScriptBindingWindowInstance(ScriptEngineInstance* engineInstance,
                                Window* ownerWindow);

    void destroy() override;

    Window* ownerWindow() override;
    Document* ownerDocument() override;
    void dispatchErrorEventToGlobalScope(ErrorEventInit& errorInfo) override;

private:
    Window* m_ownerWindow;

    void initJavaScriptBinding(Escargot::ContextRef* context,
                               Escargot::ExecutionStateRef* state) override;
};
} // namespace Starfish

#endif
