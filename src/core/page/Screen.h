/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishScreen__
#define __StarFishScreen__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class Screen : public ScriptWrappable, public DocumentHoldable {
public:
    Screen(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isScreen() const override;
    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return DocumentHoldable::scriptBindingInstance();
    }

    int32_t availWidth() const;
    int32_t availHeight() const;
    int32_t width() const;
    int32_t height() const;
    uint32_t colorDepth() const;
    uint32_t pixelDepth() const;
    float devicePixelRatio() const;
    bool isMonochrome() const;
};
}

#endif
