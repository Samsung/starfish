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

#ifndef __StarFishComment__
#define __StarFishComment__

#include "core/dom/CharacterData.h"

namespace StarFish {

class Comment : public CharacterData {
public:
    Comment(Document* document, String* data = String::emptyString)
        : CharacterData(document, data)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isComment() const override;

    /* 4.4 Interface Node */
    virtual NodeType nodeType() const override
    {
        return Node::COMMENT_NODE;
    }

    virtual String* nodeName();
    virtual String* localName();

    virtual Node* clone() override
    {
        return new Comment(document(), data());
    }

    bool isContainerNode()
    {
        return false;
    }

    /* 4.12 Interface Comment */
    // Comment node does not have any public APIs
};
}

#endif
