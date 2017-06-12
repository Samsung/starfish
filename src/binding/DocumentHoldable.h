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

#ifndef __StarFishDocumentHodable__
#define __StarFishDocumentHodable__

namespace StarFish {

class Document;
class StarFish;
class Window;
class ScriptBindingInstance;

class DocumentHoldable {
public:
    DocumentHoldable(Document* document)
        : m_document(document)
    {
    }

    Document* document()
    {
        return m_document;
    }

    Window* window();
    StarFish* starFish();
    ScriptBindingInstance* scriptBindingInstance();

protected:
    Document* m_document;
};
}
#endif
