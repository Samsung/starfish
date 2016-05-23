#include "StarFishConfig.h"
#include "HTMLParser.h"
#include "AtomicHTMLToken.h"

#include "dom/Document.h"
#include "dom/HTMLElement.h"
#include "dom/HTMLScriptElement.h"

namespace StarFish {

void HTMLParser::startParse()
{
    m_document->setInParsing(true);
}

void HTMLParser::endParse()
{
    m_treeBuilder.flush();
    m_treeBuilder.finished();
    m_treeBuilder.detach();
    m_document->setInParsing(false);
    if (!m_treeBuilder.isParsingFragment())
        m_document->notifyDomContentLoaded();
}

void HTMLParser::parseStep()
{
    while (true) {
        if (m_treeBuilder.hasParserBlockingScript()) {
            TextPosition pos;
            HTMLScriptElement* script = m_treeBuilder.takeScriptToProcess(pos)->asHTMLElement()->asHTMLScriptElement();
            script->clearParserInserted();
            bool shouldStop = script->executeScript(m_treeBuilder.isParsingFragment(), true);
            if (shouldStop)
                break;
        }

        if (!m_tokenizer.nextToken(m_input.current(), token())) {
            endParse();
            break;
        }
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

        m_treeBuilder.constructTree(&at);

        if (!rawToken.isUninitialized()) {
            ASSERT(rawToken.type() == HTMLToken::Character);
            rawToken.clear();
        }
    }
}

}
