/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishHTMLParser__
#define __StarFishHTMLParser__

#include "core/dom/DocumentFragment.h"
#include "core/dom/parser/HTMLToken.h"
#include "core/dom/parser/HTMLTokenizer.h"
#include "core/dom/parser/HTMLInputStream.h"
#include "core/dom/parser/HTMLTreeBuilder.h"

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

    HTMLParser(StarFish* sf, DocumentFragment* df, Element* contextElement,
               String* sourceString)
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
    void parseStep(bool shouldEndParseWhenThereIsNoToken = true);
    void endParse();

    HTMLTokenizer* tokenizer()
    {
        return &m_tokenizer;
    }
    HTMLTreeBuilder* treeBuilder()
    {
        return &m_treeBuilder;
    }

    HTMLInputStream* input()
    {
        return &m_input;
    }

    TextPosition textPosition() const
    {
        const SegmentedString& currentString = m_input.current();
        OrdinalNumber line = currentString.currentLine();
        OrdinalNumber column = currentString.currentColumn();
        return TextPosition(line, column);
    }

private:
    HTMLToken& token()
    {
        return *m_token;
    }

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
