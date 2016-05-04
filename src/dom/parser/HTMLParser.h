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
    {
        m_starFish = sf;
        m_document = document;
        m_documentFragment = nullptr;
        m_contextElement = nullptr;
        m_source = sourceString;
        m_input.appendToEnd(SegmentedString(sourceString));
        m_token = new HTMLToken();
    }

    HTMLParser(StarFish* sf, DocumentFragment* df, Element* contextElement, String* sourceString)
    {
        m_starFish = sf;
        m_documentFragment = df;
        m_contextElement = contextElement;
        m_document = df->document();
        m_source = sourceString;
        m_input.appendToEnd(SegmentedString(sourceString));
        m_token = new HTMLToken();
    }

    void parse();
    HTMLTokenizer* tokenizer() { return &m_tokenizer; }

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
    Element* m_contextElement;
    HTMLToken* m_token;
    HTMLTokenizer m_tokenizer;
    HTMLInputStream m_input;
    String* m_source;
};
}

#endif
