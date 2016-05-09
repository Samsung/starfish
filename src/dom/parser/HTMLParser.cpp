#include "StarFishConfig.h"
#include "HTMLParser.h"
#include "AtomicHTMLToken.h"

namespace StarFish {

void HTMLParser::parse()
{
    m_document->setInParsing(true);
    if (!m_documentFragment) {
        HTMLTreeBuilder builder(this, m_document, false);
        while (true) {
            if (!m_tokenizer.nextToken(m_input.current(), token()))
                break;
            HTMLToken& rawToken = token();
            AtomicHTMLToken at(m_starFish, rawToken);

            // We clear the rawToken in case constructTreeFromAtomicToken
            // synchronously re-enters the parser. We don't clear the token immedately
            // for Character tokens because the AtomicHTMLToken avoids copying the
            // characters by keeping a pointer to the underlying buffer in the
            // HTMLToken. Fortunately, Character tokens can't cause us to re-enter
            // the parser.
            //
            // FIXME: Stop clearing the rawToken once we start running the parser off
            // the main thread or once we stop allowing synchronous JavaScript
            // execution from parseAttribute.
            if (rawToken.type() != HTMLToken::Character)
                rawToken.clear();

            builder.constructTree(&at);

            if (!rawToken.isUninitialized()) {
                ASSERT(rawToken.type() == HTMLToken::Character);
                rawToken.clear();
            }
        }
        builder.flush();
        builder.finished();
        builder.detach();
    } else {
        HTMLTreeBuilder builder(this, m_documentFragment, m_contextElement);
        while (true) {
            if (!m_tokenizer.nextToken(m_input.current(), token()))
                break;
            HTMLToken& rawToken = token();
            AtomicHTMLToken at(m_starFish, rawToken);

            // We clear the rawToken in case constructTreeFromAtomicToken
            // synchronously re-enters the parser. We don't clear the token immedately
            // for Character tokens because the AtomicHTMLToken avoids copying the
            // characters by keeping a pointer to the underlying buffer in the
            // HTMLToken. Fortunately, Character tokens can't cause us to re-enter
            // the parser.
            //
            // FIXME: Stop clearing the rawToken once we start running the parser off
            // the main thread or once we stop allowing synchronous JavaScript
            // execution from parseAttribute.
            if (rawToken.type() != HTMLToken::Character)
                rawToken.clear();

            builder.constructTree(&at);

            if (!rawToken.isUninitialized()) {
                ASSERT(rawToken.type() == HTMLToken::Character);
                rawToken.clear();
            }
        }
        builder.flush();
        builder.finished();
        builder.detach();
    }
    m_document->setInParsing(false);

}

}
