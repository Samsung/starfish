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

#ifndef __StarFishHTMLFormControlsCollection__
#define __StarFishHTMLFormControlsCollection__

#include "binding/ScriptWrappable.h"
#include "core/dom/HTMLCollection.h"

namespace StarFish {

class Node;

class HTMLFormControlsCollection : public HTMLCollection {
public:
    HTMLFormControlsCollection(Node* root,
                               NodeListImpl::FilterFunctionType filterType);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLFormControlsCollection() const;

    // Note : inherits length and item()

    virtual Element* namedItem(String* name) override;
};
}
#endif
