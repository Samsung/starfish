/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#if defined(STARFISH_ENABLE_DOMPARSER) && !defined(__StarFishDOMParser__)
#define __StarFishDOMParser__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace StarFish {

class Document;

class DOMParser : public ScriptWrappable, public DocumentHoldable {
public:
    DOMParser(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isDOMParser() const override;

    Document* parseFromString(String* str, String* type);
};
}

#endif
