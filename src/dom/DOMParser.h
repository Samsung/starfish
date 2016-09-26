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
#if defined(STARFISH_ENABLE_DOMPARSER) && !defined(__StarFishDOMParser__)
#define __StarFishDOMParser__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class StarFish;
class Document;

class DOMParser : public ScriptWrappable {
public:
    DOMParser(StarFish* sf)
        : ScriptWrappable(this)
        , m_starFish(sf)
    {

    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::DOMParserObject;
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    Document* parseFromString(String* str, String* type);
private:
    StarFish* m_starFish;
};

}

#endif
