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

#ifndef __StarFishHTMLDocumentBuilderElement__
#define __StarFishHTMLDocumentBuilderElement__

#include "dom/builder/DocumentBuilder.h"

namespace StarFish {

class Window;
class TextResource;
class HTMLParser;

class HTMLDocumentBuilder : public DocumentBuilder {
    friend class HTMLResourceClient;
public:
    HTMLDocumentBuilder(Document* document)
        : DocumentBuilder(document)
        , m_parser(nullptr)
        , m_textResource(nullptr)
    {
    }

    virtual void build(const URL& url);
    virtual void resume();
    HTMLParser* parser()
    {
        return m_parser;
    }
protected:
    HTMLParser* m_parser;
    TextResource* m_textResource;
};


}

#endif
