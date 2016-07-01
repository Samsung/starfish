/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishText__
#define __StarFishText__

#include "dom/CharacterData.h"

namespace StarFish {

class Text : public CharacterData {
public:
    Text(Document* document, String* data)
        : CharacterData(document, data)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual NodeType nodeType()
    {
        return Node::TEXT_NODE;
    }

    virtual String* nodeName();

    virtual String* localName();

    virtual String* textContent()
    {
        return data();
    }

    /* 4.10 Interface Text */

    // Text* splitText(unsigned long offset);

    String* wholeText();

    /* Other methods (not in DOM API) */

    virtual bool isText() const
    {
        return true;
    }

    virtual Node* clone()
    {
        return new Text(document(), data());
    }

protected:
};

}

#endif
