/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __JavaScriptNativeHandler__
#define __JavaScriptNativeHandler__

#include "binding/WebViewHoldable.h"

namespace Starfish {

class ScriptWrappable;
class WebViewHoldable;

class JavaScriptNativeHandler : public ScriptWrappable, public WebViewHoldable {
public:
    typedef std::function<std::string(std::string)> NativeFunctionPtr;
    JavaScriptNativeHandler(WebView* wv, String* functionName,
                            NativeFunctionPtr nativeCallback);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override
    {
    }
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return nullptr;
    }
    virtual bool isJavaScriptNativeHandler() const override
    {
        return true;
    }

    String* callNativeHandler(String* param);

protected:
    String* m_name;
    NativeFunctionPtr m_callback;
};
}
#endif
