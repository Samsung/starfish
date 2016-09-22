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

#ifndef __StarFishHTMLParser__
#define __StarFishHTMLParser__

#include "dom/DocumentFragment.h"
#include "dom/parser/HTMLToken.h"
#include "dom/parser/HTMLTokenizer.h"
#include "dom/parser/HTMLInputStream.h"
#include "dom/parser/HTMLTreeBuilder.h"

namespace StarFish {
class HTMLParser : public gc {
public:
    HTMLParser(StarFish* sf, Document* document, String* sourceString)
        : m_treeBuilder(this, document, false)
    {
        m_starFish = sf;
        m_document = document;
        m_documentFragment = nullptr;
        m_source = sourceString;
        m_input.appendToEnd(SegmentedString(sourceString));
        m_token = new HTMLToken();
    }

    HTMLParser(StarFish* sf, DocumentFragment* df, Element* contextElement, String* sourceString)
        : m_treeBuilder(this, df, contextElement)
    {
        m_starFish = sf;
        m_documentFragment = df;
        m_document = df->document();
        m_source = sourceString;
        m_input.appendToEnd(SegmentedString(sourceString));
        m_token = new HTMLToken();
    }

    void startParse();
    void parseStep();
    void endParse();

    HTMLTokenizer* tokenizer() { return &m_tokenizer; }
    HTMLTreeBuilder* treeBuilder() { return &m_treeBuilder; }

    TextPosition textPosition() const
    {
        const SegmentedString& currentString = m_input.current();
        OrdinalNumber line = currentString.currentLine();
        OrdinalNumber column = currentString.currentColumn();
        return TextPosition(line, column);
    }
private:
    HTMLToken& token() { return *m_token; }

    StarFish* m_starFish;
    Document* m_document;
    DocumentFragment* m_documentFragment;
    HTMLToken* m_token;
    HTMLTreeBuilder m_treeBuilder;
    HTMLTokenizer m_tokenizer;
    HTMLInputStream m_input;
    String* m_source;
};
}

#endif
