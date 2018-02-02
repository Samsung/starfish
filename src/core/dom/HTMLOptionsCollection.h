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

#ifndef __StarFishHTMLOptionsCollection__
#define __StarFishHTMLOptionsCollection__

#include "core/dom/HTMLCollection.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/NodeList.h"
#include "core/dom/NodeListImpl.h"

namespace StarFish {

class Node;
class Element;

class HTMLOptionsCollection : public HTMLCollection {
public:
    HTMLOptionsCollection(Node* root,
                          NodeListImpl::FilterFunctionType filterType,
                          void* data, bool canCache = false);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLOptionsCollection() const;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    size_t length() const;
    void setLength(size_t value);

    int selectedIndex();
    void setSelectedIndex(int index);
};
}

#endif
