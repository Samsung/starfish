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

#ifndef __StarFishHTMLCollection__
#define __StarFishHTMLCollection__

#include "binding/ScriptWrappable.h"
#include "dom/NodeListImpl.h"

namespace StarFish {

class Node;
class Element;

class HTMLCollection : public ScriptWrappable {
public:
    HTMLCollection(ScriptBindingInstance* instance, Node* root,
                   NodeListImpl::FilterFunctionType filterType, void* data,
                   bool canCache = false)
        : ScriptWrappable(this)
        , m_nodeListImpl(root, filterType, data, canCache)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    virtual bool isHTMLCollection() const
    {
        return true;
    }

    unsigned long length() const;
    Element* item(unsigned long index);
    Element* namedItem(String* name);
    NodeListImpl& getNodeListImpl()
    {
        return m_nodeListImpl;
    }

private:
    NodeListImpl m_nodeListImpl;
};
}

#endif
