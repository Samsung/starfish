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

#ifndef __StarFishStyleSheet__
#define __StarFishStyleSheet__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class MediaList;
class StyleSheet : public ScriptWrappable {
public:
    StyleSheet()
        : ScriptWrappable(this)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isStyleSheet() const override;

    /* DOM APIs */
    virtual String* type() const = 0;
    virtual String* href() const = 0;
    virtual Node* ownerNode() const = 0;
    virtual StyleSheet* parentStyleSheet() const
    {
        return nullptr;
    }
    virtual MediaList* media() = 0;
};

} /* namespace StarFish */

#endif /* __StarFishStyleSheet__ */
