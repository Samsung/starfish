/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __JavaScriptNativeHandler__
#define __JavaScriptNativeHandler__

namespace StarFish {
class ScriptWrappable;
class StarFishHoldable;

class JavaScriptNativeHandler : public ScriptWrappable,
                                public StarFishHoldable {
public:
    typedef std::string (*NativeFunctionPtr)(std::string param);
    JavaScriptNativeHandler(StarFish* starFish, String* functionName,
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
