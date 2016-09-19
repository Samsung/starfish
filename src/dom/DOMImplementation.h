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

#ifdef STARFISH_EXP

#ifndef __StarFishDOMImplementation__
#define __StarFishDOMImplementation__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class DocumentType;
class Window;

class DOMImplementation : public ScriptWrappable {
public:
    DOMImplementation(Window* window, ScriptBindingInstance* instance)
        : ScriptWrappable(this)
        , m_window(window)
        , m_instance(instance)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::DOMImplementationObject;
    }

    DocumentType* createDocumentType(String* qualifiedName, String* publicId, String* systemId);
    Document* createHTMLDocument(String* title = String::spaceString);

    bool hasFeature() { return true; } // useless; always returns true
private:
    Window* m_window;
    ScriptBindingInstance* m_instance;
};
}

#endif

#endif
