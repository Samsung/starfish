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

#ifndef __StarFishCDATASection__
#define __StarFishCDATASection__

#include "dom/Text.h"

namespace StarFish {

class CDATASection : public Text {
public:
    CDATASection(Document* document, String* data)
        : Text(document, data)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isCDATASection() const override
    {
        return true;
    }

    virtual String* nodeName();
    virtual String* localName();

    virtual Node* clone() override
    {
        return new CDATASection(document(), data());
    }
};
}

#endif
