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


#ifndef __StarFishHTMLDocument__
#define __StarFishHTMLDocument__

#include "Document.h"

namespace StarFish {

class Window;

class HTMLDocument : public Document {
public:
    HTMLDocument(Window* window, ScriptBindingInstance* scriptBindingInstance, URL* url, String* charSet, bool doesParticipateInRendering)
        : Document(window, scriptBindingInstance, url, charSet, doesParticipateInRendering)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Element* createElement(AtomicString localName, bool shouldCheckName);
    static Element* createHTMLElement(Document* document, AtomicString localName);
protected:
};

}

#endif
