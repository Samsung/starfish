/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

#include "StarFishConfig.h"
#include "dom/HTMLDocument.h"
#include "dom/HTMLElement.h"
#include "HTMLTreeBuilder.h"

// #include "dom/HTMLFormElement.h"
#include "dom/parser/AtomicHTMLToken.h"
#include "dom/parser/HTMLParser.h"
#include "dom/parser/HTMLParserIdioms.h"
#include "dom/parser/HTMLStackItem.h"
#include "dom/parser/HTMLToken.h"
#include "dom/parser/HTMLTokenizer.h"
#include "dom/DOM.h"

namespace StarFish {

namespace {

inline bool isHTMLSpaceOrReplacementCharacter(char32_t character)
{
    return isHTMLSpace<char32_t>(character) || character == 0xFFFD;
}

}

static TextPosition uninitializedPositionValue1()
{
    return TextPosition(OrdinalNumber::fromOneBasedInt(-1), OrdinalNumber::first());
}

static inline bool isAllWhitespace(String* string)
{
    for (size_t i = 0; i < string->length(); i++) {
        char32_t c = string->charAt(i);
        if (!isHTMLSpace(c)) {
            return false;
        }
    }
    return true;
}

static inline bool isAllWhitespaceOrReplacementCharacters(String* string)
{
    for (size_t i = 0; i < string->length(); i++) {
        char32_t c = string->charAt(i);
        if (!isHTMLSpaceOrReplacementCharacter(c)) {
            return false;
        }
    }
    return true;
}

static bool isNumberedHeaderTag(StaticStrings* s, const AtomicString& tagName)
{
    return tagName == s->m_h1TagName
        || tagName == s->m_h2TagName
        || tagName == s->m_h3TagName
        || tagName == s->m_h4TagName
        || tagName == s->m_h5TagName
        || tagName == s->m_h6TagName;
}

static bool isCaptionColOrColgroupTag(StaticStrings* s, const AtomicString& tagName)
{
    return tagName == s->m_captionTagName
        || tagName == s->m_colTagName
        || tagName == s->m_colgroupTagName;
}

static bool isTableCellContextTag(StaticStrings* s, const AtomicString& tagName)
{
    return tagName == s->m_thTagName || tagName == s->m_tdTagName;
}

static bool isTableBodyContextTag(StaticStrings* s, const AtomicString& tagName)
{
    return tagName == s->m_tbodyTagName
        || tagName == s->m_tfootTagName
        || tagName == s->m_theadTagName;
}

static bool isNonAnchorNonNobrFormattingTag(StaticStrings* s, const AtomicString& tagName)
{
    return tagName == s->m_bTagName
        || tagName == s->m_bigTagName
        || tagName == s->m_codeTagName
        || tagName == s->m_emTagName
        || tagName == s->m_fontTagName
        || tagName == s->m_iTagName
        || tagName == s->m_sTagName
        || tagName == s->m_smallTagName
        || tagName == s->m_strikeTagName
        || tagName == s->m_strongTagName
        || tagName == s->m_ttTagName
        || tagName == s->m_uTagName;
}

static bool isNonAnchorFormattingTag(StaticStrings* s, const AtomicString& tagName)
{
    return tagName == s->m_nobrTagName
        || isNonAnchorNonNobrFormattingTag(s, tagName);
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#formatting
static bool isFormattingTag(StaticStrings* s, const AtomicString& tagName)
{
    return tagName == s->m_aTagName || isNonAnchorFormattingTag(s, tagName);
}

static HTMLFormElement* closestFormAncestor(StaticStrings* s, Element* element)
{
    while (element) {
        if (element->name() == s->m_formTagName)
            return element->asHTMLElement();
        Node* parent = element->parentNode();
        if (!parent || !parent->isElement())
            return 0;
        element = parent->asElement();
    }
    return 0;
}

class HTMLTreeBuilder::CharacterTokenBuffer : public gc {
public:
    explicit CharacterTokenBuffer(AtomicHTMLToken* token)
        : m_characters(token->characters())
        , m_current(0)
        , m_end(token->characters()->length())
    {
        ASSERT(!isEmpty());
    }

    explicit CharacterTokenBuffer(String* characters)
        : m_characters(characters)
        , m_current(0)
        , m_end(characters->length())
    {
        ASSERT(!isEmpty());
    }

    ~CharacterTokenBuffer()
    {
        ASSERT(isEmpty());
    }

    bool isEmpty() const { return m_current == m_end; }

    void skipAtMostOneLeadingNewline()
    {
        ASSERT(!isEmpty());
        if ((*m_characters)[m_current] == '\n')
            ++m_current;
    }

    void skipLeadingWhitespace()
    {
        skipLeading<isHTMLSpace<char32_t> >();
    }

    String* takeLeadingWhitespace()
    {
        return takeLeading<isHTMLSpace<char32_t> >();
    }

    void skipLeadingNonWhitespace()
    {
        skipLeading<isNotHTMLSpace<char32_t> >();
    }

    String* takeRemaining()
    {
        STARFISH_ASSERT(!isEmpty());
        unsigned start = m_current;
        m_current = m_end;
        // Notice that substring is smart enough to return *this when start == 0.
        return m_characters->substring(start, m_end - start);
    }

    void giveRemainingTo(UTF32String& recipient)
    {
        for (size_t i = m_current; i < m_end; i ++) {
            recipient.push_back(m_characters->charAt(i));
        }
        m_current = m_end;
    }

    String* takeRemainingWhitespace()
    {
        STARFISH_ASSERT(!isEmpty());
        const unsigned start = m_current;
        m_current = m_end; // One way or another, we're taking everything!

        unsigned length = 0;
        for (unsigned i = start; i < m_end; ++i) {
            if (isHTMLSpace<char32_t>((*m_characters)[i]))
                ++length;
        }
        // Returning the null string when there aren't any whitespace
        // characters is slightly cleaner semantically because we don't want
        // to insert a text node (as opposed to inserting an empty text node).
        if (!length)
            return String::emptyString;
        if (length == start - m_end) // It's all whitespace.
            return m_characters->substring(start, start - m_end);

        UTF32String result;
        // result.reserveCapacity(length);
        for (unsigned i = start; i < m_end; ++i) {
            char32_t c = m_characters->charAt(i);
            if (isHTMLSpace<char32_t>(c))
                result.push_back(c);
        }

        return new StringDataUTF32(std::move(result));
    }

private:
    template<bool characterPredicate(char32_t)>
    void skipLeading()
    {
        ASSERT(!isEmpty());
        while (characterPredicate((*m_characters)[m_current])) {
            if (++m_current == m_end)
                return;
        }
    }

    template<bool characterPredicate(char32_t)>
    String* takeLeading()
    {
        ASSERT(!isEmpty());
        const unsigned start = m_current;
        skipLeading<characterPredicate>();
        if (start == m_current)
            return String::emptyString;
        return m_characters->substring(start, m_current - start);
    }

    String* m_characters;
    unsigned m_current;
    unsigned m_end;
};

// HTMLDocumentParser*, HTMLDocument*, bool reportErrors
HTMLTreeBuilder::HTMLTreeBuilder(HTMLParser* parser, Document* document, bool)
    : m_framesetOk(true)
#ifndef NDEBUG
    , m_isAttached(true)
#endif
    , m_tree(document)
    , m_insertionMode(InitialMode)
    , m_originalInsertionMode(InitialMode)
    , m_shouldSkipLeadingNewline(false)
    , m_parser(parser)
    , m_scriptToProcessStartPosition(uninitializedPositionValue1())
{
    m_scriptToProcess = nullptr;
}

// FIXME: Member variables should be grouped into self-initializing structs to
// minimize code duplication between these constructors.
HTMLTreeBuilder::HTMLTreeBuilder(HTMLParser* parser, DocumentFragment* fragment, Element* contextElement)
    : m_framesetOk(true)
#ifndef NDEBUG
    , m_isAttached(true)
#endif
    , m_fragmentContext(fragment, contextElement)
    , m_tree(fragment)
    , m_insertionMode(InitialMode)
    , m_originalInsertionMode(InitialMode)
    , m_shouldSkipLeadingNewline(false)
    , m_parser(parser)
    , m_scriptToProcessStartPosition(uninitializedPositionValue1())
{
    m_scriptToProcess = nullptr;
    // FIXME: This assertion will become invalid if <http://webkit.org/b/60316> is fixed.
    ASSERT(contextElement);
    if (contextElement) {
        // Steps 4.2-4.6 of the HTML5 Fragment Case parsing algorithm:
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-end.html#fragment-case
        // For efficiency, we skip step 4.2 ("Let root be a new html element with no attributes")
        // and instead use the DocumentFragment as a root node.
        m_tree.openElements()->pushRootNode(new HTMLStackItem(fragment, HTMLStackItem::ItemForDocumentFragmentNode));

        if (contextElement->name() == (fragment->document()->window()->starFish()->staticStrings()->m_templateTagName))
            m_templateInsertionModes.push_back(TemplateContentsMode);

        resetInsertionModeAppropriately();
        m_tree.setForm(closestFormAncestor(fragment->document()->window()->starFish()->staticStrings(), contextElement));
    }
}
/*
HTMLTreeBuilder::~HTMLTreeBuilder()
{
}
*/
void HTMLTreeBuilder::detach()
{
#ifndef NDEBUG
    // This call makes little sense in fragment mode, but for consistency
    // DocumentParser expects detach() to always be called before it's destroyed.
    m_isAttached = false;
#endif
    // HTMLConstructionSite might be on the callstack when detach() is called
    // otherwise we'd just call m_tree.clear() here instead.
    m_tree.detach();
}

HTMLTreeBuilder::FragmentParsingContext::FragmentParsingContext()
    : m_fragment(0)
{
    m_contextElementStackItem = nullptr;
}

HTMLTreeBuilder::FragmentParsingContext::FragmentParsingContext(DocumentFragment* fragment, Element* contextElement)
    : m_fragment(fragment)
{
    STARFISH_ASSERT(!fragment->firstChild());
    m_contextElementStackItem = new HTMLStackItem(contextElement, HTMLStackItem::ItemForContextElement);
}

HTMLTreeBuilder::FragmentParsingContext::~FragmentParsingContext()
{
}

Element* HTMLTreeBuilder::takeScriptToProcess(TextPosition& scriptStartPosition)
{
    STARFISH_ASSERT(m_scriptToProcess);
    STARFISH_ASSERT(!m_tree.hasPendingTasks());
    // Unpause ourselves, callers may pause us again when processing the script.
    // The HTML5 spec is written as though scripts are executed inside the tree
    // builder. We pause the parser to exit the tree builder, and then resume
    // before running scripts.
    scriptStartPosition = m_scriptToProcessStartPosition;
    m_scriptToProcessStartPosition = uninitializedPositionValue1();
    auto ret = m_scriptToProcess;
    m_scriptToProcess = nullptr;
    return ret;
}

Element* HTMLTreeBuilder::lookScriptToProcess()
{
    return m_scriptToProcess;
}

void HTMLTreeBuilder::constructTree(AtomicHTMLToken* token)
{
    if (shouldProcessTokenInForeignContent(token))
        processTokenInForeignContent(token);
    else
        processToken(token);

    if (m_parser->tokenizer()) {
        bool inForeignContent = false;
        if (!m_tree.isEmpty()) {
            HTMLStackItem* adjustedCurrentNode = adjustedCurrentStackItem();
            inForeignContent = !adjustedCurrentNode->isInHTMLNamespace()
                && !HTMLElementStack::isHTMLIntegrationPoint(adjustedCurrentNode)
                && !HTMLElementStack::isMathMLTextIntegrationPoint(adjustedCurrentNode);
        }

        m_parser->tokenizer()->setForceNullCharacterReplacement(m_insertionMode == TextMode || inForeignContent);
        m_parser->tokenizer()->setShouldAllowCDATA(inForeignContent);
    }

    m_tree.executeQueuedTasks();
    // We might be detached now.
}

void HTMLTreeBuilder::processToken(AtomicHTMLToken* token)
{
    if (token->type() == HTMLToken::Character) {
        processCharacter(token);
        return;
    }

    // Any non-character token needs to cause us to flush any pending text immediately.
    // NOTE: flush() can cause any queued tasks to execute, possibly re-entering the parser.
    m_tree.flush();
    m_shouldSkipLeadingNewline = false;

    switch (token->type()) {
    case HTMLToken::Uninitialized:
    case HTMLToken::Character:
        ASSERT_NOT_REACHED();
        break;
    case HTMLToken::DOCTYPE:
        processDoctypeToken(token);
        break;
    case HTMLToken::StartTag:
        processStartTag(token);
        break;
    case HTMLToken::EndTag:
        processEndTag(token);
        break;
    case HTMLToken::Comment:
        processComment(token);
        break;
    case HTMLToken::EndOfFile:
        processEndOfFile(token);
        break;
    }
}

void HTMLTreeBuilder::processDoctypeToken(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::DOCTYPE);
    if (m_insertionMode == InitialMode) {
        m_tree.insertDoctype(token);
        setInsertionMode(BeforeHTMLMode);
        return;
    }
    if (m_insertionMode == InTableTextMode) {
        defaultForInTableText();
        processDoctypeToken(token);
        return;
    }
    parseError(token);
}

void HTMLTreeBuilder::processFakeStartTag(const QualifiedName& tagName, const AttributeVector& attributes)
{
    // FIXME: We'll need a fancier conversion than just "localName" for SVG/MathML tags.
    AtomicHTMLToken fakeToken(m_tree.document()->window()->starFish(), HTMLToken::StartTag, tagName.localNameAtomic(), attributes);
    processStartTag(&fakeToken);
}

void HTMLTreeBuilder::processFakeEndTag(const AtomicString& tagName)
{
    AtomicHTMLToken fakeToken(m_tree.document()->window()->starFish(), HTMLToken::EndTag, tagName);
    processEndTag(&fakeToken);
}

void HTMLTreeBuilder::processFakeEndTag(const QualifiedName& tagName)
{
    // FIXME: We'll need a fancier conversion than just "localName" for SVG/MathML tags.
    processFakeEndTag(tagName.localNameAtomic());
}

void HTMLTreeBuilder::processFakePEndTagIfPInButtonScope()
{
    if (!m_tree.openElements()->inButtonScope(m_tree.document()->window()->starFish()->staticStrings()->m_pTagName.localNameAtomic()))
        return;
    AtomicHTMLToken endP(m_tree.document()->window()->starFish(), HTMLToken::EndTag, m_tree.document()->window()->starFish()->staticStrings()->m_pTagName.localNameAtomic());
    processEndTag(&endP);
}

namespace {

bool isLi(StaticStrings* s, const HTMLStackItem* item)
{
    return item->hasTagName(s->m_liTagName);
}

bool isDdOrDt(StaticStrings* s, const HTMLStackItem* item)
{
    return item->hasTagName(s->m_ddTagName)
        || item->hasTagName(s->m_dtTagName);
}

}

template <bool shouldClose(StaticStrings* s, const HTMLStackItem*)>
void HTMLTreeBuilder::processCloseWhenNestedTag(AtomicHTMLToken* token)
{
    m_framesetOk = false;
    StaticStrings* s = token->starFish()->staticStrings();
    HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
    while (1) {
        HTMLStackItem* item = nodeRecord->stackItem();
        if (shouldClose(s, item)) {
            STARFISH_ASSERT(item->isElementNode());
            processFakeEndTag(item->localName());
            break;
        }
        if (item->isSpecialNode() && !item->hasTagName(s->m_addressTagName) && !item->hasTagName(s->m_divTagName) && !item->hasTagName(s->m_pTagName))
            break;
        nodeRecord = nodeRecord->next();
    }
    processFakePEndTagIfPInButtonScope();
    m_tree.insertHTMLElement(token);
}
/*
typedef HashMap<AtomicString, QualifiedName> PrefixedNameToQualifiedNameMap;

static void mapLoweredLocalNameToName(PrefixedNameToQualifiedNameMap* map, const QualifiedName* const* names, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        const QualifiedName& name = *names[i];
        const AtomicString& localName = name.localName();
        AtomicString loweredLocalName = localName.lower();
        if (loweredLocalName != localName)
            map->add(loweredLocalName, name);
    }
}

static void adjustSVGTagNameCase(AtomicHTMLToken* token)
{
    static PrefixedNameToQualifiedNameMap* caseMap = 0;
    if (!caseMap) {
        caseMap = new PrefixedNameToQualifiedNameMap;
        OwnPtr<const QualifiedName*[]> svgTags = SVGNames::getSVGTags();
        mapLoweredLocalNameToName(caseMap, svgTags.get(), SVGNames::SVGTagsCount);
    }

    const QualifiedName& casedName = caseMap->get(token->name());
    if (casedName.localName().isNull())
        return;
    token->setName(casedName.localName());
}

template<PassOwnPtr<const QualifiedName*[]> getAttrs(), unsigned length>
static void adjustAttributes(AtomicHTMLToken* token)
{
    static PrefixedNameToQualifiedNameMap* caseMap = 0;
    if (!caseMap) {
        caseMap = new PrefixedNameToQualifiedNameMap;
        OwnPtr<const QualifiedName*[]> attrs = getAttrs();
        mapLoweredLocalNameToName(caseMap, attrs.get(), length);
    }

    for (unsigned i = 0; i < token->attributes().size(); ++i) {
        Attribute& tokenAttribute = token->attributes().at(i);
        const QualifiedName& casedName = caseMap->get(tokenAttribute.localName());
        if (!casedName.localName().isNull())
            tokenAttribute.parserSetName(casedName);
    }
}

static void adjustSVGAttributes(AtomicHTMLToken* token)
{
    adjustAttributes<SVGNames::getSVGAttrs, SVGNames::SVGAttrsCount>(token);
}

static void adjustMathMLAttributes(AtomicHTMLToken* token)
{
    adjustAttributes<MathMLNames::getMathMLAttrs, MathMLNames::MathMLAttrsCount>(token);
}

static void addNamesWithPrefix(PrefixedNameToQualifiedNameMap* map, const AtomicString& prefix, const QualifiedName* const* names, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        const QualifiedName* name = names[i];
        const AtomicString& localName = name->localName();
        AtomicString prefixColonLocalName = prefix + ':' + localName;
        QualifiedName nameWithPrefix(prefix, localName, name->namespaceURI());
        map->add(prefixColonLocalName, nameWithPrefix);
    }
}

static void adjustForeignAttributes(AtomicHTMLToken* token)
{
    static PrefixedNameToQualifiedNameMap* map = 0;
    if (!map) {
        map = new PrefixedNameToQualifiedNameMap;

        OwnPtr<const QualifiedName*[]> attrs = XLinkNames::getXLinkAttrs();
        addNamesWithPrefix(map, xlinkAtom, attrs.get(), XLinkNames::XLinkAttrsCount);

        OwnPtr<const QualifiedName*[]> xmlAttrs = XMLNames::getXMLAttrs();
        addNamesWithPrefix(map, xmlAtom, xmlAttrs.get(), XMLNames::XMLAttrsCount);

        map->add(WTF::xmlnsAtom, XMLNSNames::xmlnsAttr);
        map->add("xmlns:xlink", QualifiedName(xmlnsAtom, xlinkAtom, XMLNSNames::xmlnsNamespaceURI));
    }

    for (unsigned i = 0; i < token->attributes().size(); ++i) {
        Attribute& tokenAttribute = token->attributes().at(i);
        const QualifiedName& name = map->get(tokenAttribute.localName());
        if (!name.localName().isNull())
            tokenAttribute.parserSetName(name);
    }
}
*/
void HTMLTreeBuilder::processStartTagForInBody(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(token->type() == HTMLToken::StartTag);
    StaticStrings* s = token->starFish()->staticStrings();
    if (token->name() == s->m_htmlTagName) {
        processHtmlStartTagForInBody(token);
        return;
    }
    if (token->name() == s->m_baseTagName
        || token->name() == s->m_basefontTagName
        || token->name() == s->m_bgsoundTagName
        || token->name() == s->m_commandTagName
        || token->name() == s->m_linkTagName
        || token->name() == s->m_metaTagName
        || token->name() == s->m_noframesTagName
        || token->name() == s->m_scriptTagName
        || token->name() == s->m_styleTagName
        || token->name() == s->m_titleTagName) {
        bool didProcess = processStartTagForInHead(token);
        STARFISH_ASSERT_UNUSED(didProcess, didProcess);
        return;
    }
    if (token->name() == s->m_bodyTagName) {
        parseError(token);
        if (!m_tree.openElements()->secondElementIsHTMLBodyElement() || m_tree.openElements()->hasOnlyOneElement() || m_tree.openElements()->hasTemplateInHTMLScope()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        m_framesetOk = false;
        m_tree.insertHTMLBodyStartTagInBody(token);
        return;
    }
    if (token->name() == s->m_framesetTagName) {
        parseError(token);
        if (!m_tree.openElements()->secondElementIsHTMLBodyElement() || m_tree.openElements()->hasOnlyOneElement()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        if (!m_framesetOk)
            return;
        // m_tree.openElements()->bodyElement()->remove(ASSERT_NO_EXCEPTION);
        m_tree.openElements()->bodyElement()->remove();
        m_tree.openElements()->popUntil(m_tree.openElements()->bodyElement());
        m_tree.openElements()->popHTMLBodyElement();
        ASSERT(m_tree.openElements()->top() == m_tree.openElements()->htmlElement());
        m_tree.insertHTMLElement(token);
        setInsertionMode(InFramesetMode);
        return;
    }
    if (token->name() == s->m_addressTagName
        || token->name() == s->m_articleTagName
        || token->name() == s->m_asideTagName
        || token->name() == s->m_blockquoteTagName
        || token->name() == s->m_centerTagName
        || token->name() == s->m_detailsTagName
        || token->name() == s->m_dirTagName
        || token->name() == s->m_divTagName
        || token->name() == s->m_dlTagName
        || token->name() == s->m_fieldsetTagName
        || token->name() == s->m_figcaptionTagName
        || token->name() == s->m_figureTagName
        || token->name() == s->m_footerTagName
        || token->name() == s->m_headerTagName
        || token->name() == s->m_hgroupTagName
        || token->name() == s->m_mainTagName
        || token->name() == s->m_menuTagName
        || token->name() == s->m_navTagName
        || token->name() == s->m_olTagName
        || token->name() == s->m_pTagName
        || token->name() == s->m_sectionTagName
        || token->name() == s->m_summaryTagName
        || token->name() == s->m_ulTagName) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        return;
    }
    if (isNumberedHeaderTag(s, token->name())) {
        processFakePEndTagIfPInButtonScope();
        if (m_tree.currentStackItem()->isNumberedHeaderElement()) {
            parseError(token);
            m_tree.openElements()->pop();
        }
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token->name() == s->m_preTagName || token->name() == s->m_listingTagName) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        m_shouldSkipLeadingNewline = true;
        m_framesetOk = false;
        return;
    }
    if (token->name() == s->m_formTagName) {
        if (m_tree.form()) {
            parseError(token);
            return;
        }
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLFormElement(token);
        return;
    }
    if (token->name() == s->m_liTagName) {
        processCloseWhenNestedTag<isLi>(token);
        return;
    }
    if (token->name() == s->m_ddTagName || token->name() == s->m_dtTagName) {
        processCloseWhenNestedTag<isDdOrDt>(token);
        return;
    }
    if (token->name() == s->m_plaintextTagName) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        if (m_parser->tokenizer())
            m_parser->tokenizer()->setState(HTMLTokenizer::PLAINTEXTState);
        return;
    }
    if (token->name() == s->m_buttonTagName) {
        if (m_tree.openElements()->inScope(s->m_buttonTagName)) {
            parseError(token);
            processFakeEndTag(s->m_buttonTagName);
            processStartTag(token); // FIXME: Could we just fall through here?
            return;
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token->name() == s->m_aTagName) {
        Element* activeATag = m_tree.activeFormattingElements()->closestElementInScopeWithName(s->m_aTagName.localNameAtomic());
        if (activeATag) {
            parseError(token);
            processFakeEndTag(s->m_aTagName);
            m_tree.activeFormattingElements()->remove(activeATag);
            if (m_tree.openElements()->contains(activeATag))
                m_tree.openElements()->remove(activeATag);
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertFormattingElement(token);
        return;
    }
    if (isNonAnchorNonNobrFormattingTag(s, token->name())) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertFormattingElement(token);
        return;
    }
    if (token->name() == s->m_nobrTagName) {
        m_tree.reconstructTheActiveFormattingElements();
        if (m_tree.openElements()->inScope(s->m_nobrTagName)) {
            parseError(token);
            processFakeEndTag(s->m_nobrTagName);
            m_tree.reconstructTheActiveFormattingElements();
        }
        m_tree.insertFormattingElement(token);
        return;
    }
    if (token->name() == s->m_appletTagName
        || token->name() == s->m_embedTagName
        || token->name() == s->m_objectTagName) {
        /*
        if (!pluginContentIsAllowed(m_tree.parserContentPolicy()))
            return;
        */
    }
    if (token->name() == s->m_appletTagName
        || token->name() == s->m_marqueeTagName
        || token->name() == s->m_objectTagName) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_tree.activeFormattingElements()->appendMarker();
        m_framesetOk = false;
        return;
    }
    if (token->name() == s->m_tableTagName) {
        if (!m_tree.inQuirksMode() && m_tree.openElements()->inButtonScope(s->m_pTagName))
            processFakeEndTag(s->m_pTagName);
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        setInsertionMode(InTableMode);
        return;
    }
    if (token->name() == s->m_imageTagName) {
        parseError(token);
        // Apparently we're not supposed to ask.
        token->setName(s->m_imgTagName.localNameAtomic());
        // Note the fall through to the imgTag handling below!
    }
    if (token->name() == s->m_areaTagName
        || token->name() == s->m_brTagName
        || token->name() == s->m_embedTagName
        || token->name() == s->m_imgTagName
        || token->name() == s->m_keygenTagName
        || token->name() == s->m_wbrTagName) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertSelfClosingHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token->name() == s->m_inputTagName) {
        Attribute* typeAttribute = token->getAttributeItem(s->m_type);
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertSelfClosingHTMLElement(token);
        if (!typeAttribute || !typeAttribute->value()->equalsWithoutCase(String::createASCIIString("hidden")))
            m_framesetOk = false;
        return;
    }
    if (token->name() == s->m_paramTagName
        || token->name() == s->m_sourceTagName
        || token->name() == s->m_trackTagName) {
        m_tree.insertSelfClosingHTMLElement(token);
        return;
    }
    if (token->name() == s->m_hrTagName) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertSelfClosingHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token->name() == s->m_textareaTagName) {
        m_tree.insertHTMLElement(token);
        m_shouldSkipLeadingNewline = true;
        if (m_parser->tokenizer())
            m_parser->tokenizer()->setState(HTMLTokenizer::RCDATAState);
        m_originalInsertionMode = m_insertionMode;
        m_framesetOk = false;
        setInsertionMode(TextMode);
        return;
    }
    if (token->name() == s->m_xmpTagName) {
        processFakePEndTagIfPInButtonScope();
        m_tree.reconstructTheActiveFormattingElements();
        m_framesetOk = false;
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == s->m_iframeTagName) {
        m_framesetOk = false;
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == s->m_noembedTagName) {
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == s->m_noscriptTagName) {
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == s->m_selectTagName) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        if (m_insertionMode == InTableMode
            || m_insertionMode == InCaptionMode
            || m_insertionMode == InColumnGroupMode
            || m_insertionMode == InTableBodyMode
            || m_insertionMode == InRowMode
            || m_insertionMode == InCellMode)
            setInsertionMode(InSelectInTableMode);
        else
            setInsertionMode(InSelectMode);
        return;
    }
    if (token->name() == s->m_optgroupTagName || token->name() == s->m_optionTagName) {
        if (m_tree.currentStackItem()->hasTagName(s->m_optionTagName)) {
            AtomicHTMLToken endOption(token->starFish(), HTMLToken::EndTag, s->m_optionTagName.localNameAtomic());
            processEndTag(&endOption);
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token->name() == s->m_rpTagName || token->name() == s->m_rtTagName) {
        if (m_tree.openElements()->inScope(s->m_rubyTagName)) {
            m_tree.generateImpliedEndTags();
            if (!m_tree.currentStackItem()->hasTagName(s->m_rubyTagName))
                parseError(token);
        }
        m_tree.insertHTMLElement(token);
        return;
    }
    /*
    if (token->name() == MathMLNames::mathTag.localName()) {
        m_tree.reconstructTheActiveFormattingElements();
        adjustMathMLAttributes(token);
        adjustForeignAttributes(token);
        m_tree.insertForeignElement(token, MathMLNames::mathmlNamespaceURI);
        return;
    }
    if (token->name() == SVGNames::svgTag.localName()) {
        m_tree.reconstructTheActiveFormattingElements();
        adjustSVGAttributes(token);
        adjustForeignAttributes(token);
        m_tree.insertForeignElement(token, SVGNames::svgNamespaceURI);
        return;
    }*/
    if (isCaptionColOrColgroupTag(s, token->name())
        || token->name() == s->m_frameTagName
        || token->name() == s->m_headTagName
        || isTableBodyContextTag(s, token->name())
        || isTableCellContextTag(s, token->name())
        || token->name() == s->m_trTagName) {
        parseError(token);
        return;
    }
    if (token->name() == s->m_templateTagName) {
        processTemplateStartTag(token);
        return;
    }
    m_tree.reconstructTheActiveFormattingElements();
    m_tree.insertHTMLElement(token);
}

void HTMLTreeBuilder::processTemplateStartTag(AtomicHTMLToken* token)
{
    m_tree.activeFormattingElements()->appendMarker();
    m_tree.insertHTMLElement(token);
    m_templateInsertionModes.push_back(TemplateContentsMode);
    setInsertionMode(TemplateContentsMode);
}

bool HTMLTreeBuilder::processTemplateEndTag(AtomicHTMLToken* token)
{
    StaticStrings* s = token->starFish()->staticStrings();
    ASSERT(token->name() == s->m_templateTagName);
    if (!m_tree.openElements()->hasTemplateInHTMLScope()) {
        STARFISH_ASSERT((m_templateInsertionModes.size() == 0) || (m_templateInsertionModes.size() == 1 && m_fragmentContext.contextElement()->name() == (s->m_templateTagName)));
        parseError(token);
        return false;
    }
    m_tree.generateImpliedEndTags();
    if (!(m_tree.currentStackItem()->localName() == s->m_templateTagName))
        parseError(token);
    m_tree.openElements()->popUntilPopped(s->m_templateTagName);
    m_tree.activeFormattingElements()->clearToLastMarker();
    m_templateInsertionModes.pop_back();
    resetInsertionModeAppropriately();
    return true;
}

bool HTMLTreeBuilder::processEndOfFileForInTemplateContents(AtomicHTMLToken* token)
{
    StaticStrings* s = token->starFish()->staticStrings();
    AtomicHTMLToken endTemplate(token->starFish(), HTMLToken::EndTag, s->m_templateTagName.localNameAtomic());
    if (!processTemplateEndTag(&endTemplate))
        return false;

    processEndOfFile(token);
    return true;
}

bool HTMLTreeBuilder::processColgroupEndTagForInColumnGroup()
{
    if (m_tree.currentIsRootNode() || (m_tree.currentNode()->isElement() && m_tree.currentNode()->asElement()->name() == (m_tree.document()->window()->starFish()->staticStrings()->m_templateTagName))) {
        ASSERT(isParsingFragmentOrTemplateContents());
        // FIXME: parse error
        return false;
    }
    m_tree.openElements()->pop();
    setInsertionMode(InTableMode);
    return true;
}

// http://www.whatwg.org/specs/web-apps/current-work/#adjusted-current-node
HTMLStackItem* HTMLTreeBuilder::adjustedCurrentStackItem() const
{
    ASSERT(!m_tree.isEmpty());
    if (isParsingFragment() && m_tree.openElements()->hasOnlyOneElement())
        return m_fragmentContext.contextElementStackItem();

    return m_tree.currentStackItem();
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#close-the-cell
void HTMLTreeBuilder::closeTheCell()
{
    STARFISH_ASSERT(insertionMode() == InCellMode);
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    if (m_tree.openElements()->inTableScope(s->m_tdTagName)) {
        STARFISH_ASSERT(!m_tree.openElements()->inTableScope(s->m_thTagName));
        processFakeEndTag(s->m_tdTagName);
        return;
    }
    STARFISH_ASSERT(m_tree.openElements()->inTableScope(s->m_thTagName));
    processFakeEndTag(s->m_thTagName);
    STARFISH_ASSERT(insertionMode() == InRowMode);
}

void HTMLTreeBuilder::processStartTagForInTable(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    if (token->name() == s->m_captionTagName) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.activeFormattingElements()->appendMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InCaptionMode);
        return;
    }
    if (token->name() == s->m_colgroupTagName) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InColumnGroupMode);
        return;
    }
    if (token->name() == s->m_colTagName) {
        processFakeStartTag(s->m_colgroupTagName);
        ASSERT(InColumnGroupMode);
        processStartTag(token);
        return;
    }
    if (isTableBodyContextTag(s, token->name())) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InTableBodyMode);
        return;
    }
    if (isTableCellContextTag(s, token->name())
        || token->name() == s->m_trTagName) {
        processFakeStartTag(s->m_tbodyTagName);
        ASSERT(insertionMode() == InTableBodyMode);
        processStartTag(token);
        return;
    }
    if (token->name() == s->m_tableTagName) {
        parseError(token);
        if (!processTableEndTagForInTable()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        processStartTag(token);
        return;
    }
    if (token->name() == s->m_styleTagName || token->name() == s->m_scriptTagName) {
        processStartTagForInHead(token);
        return;
    }
    if (token->name() == s->m_inputTagName) {
        Attribute* typeAttribute = token->getAttributeItem(s->m_type);
        if (typeAttribute && typeAttribute->value()->equalsWithoutCase(String::createASCIIString("hidden"))) {
            parseError(token);
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        // Fall through to "anything else" case.
    }
    if (token->name() == s->m_formTagName) {
        parseError(token);
        if (m_tree.form())
            return;
        m_tree.insertHTMLFormElement(token, true);
        m_tree.openElements()->pop();
        return;
    }
    if (token->name() == s->m_templateTagName) {
        processTemplateStartTag(token);
        return;
    }
    parseError(token);
    HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
    processStartTagForInBody(token);
}

void HTMLTreeBuilder::processStartTag(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::StartTag);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        if (token->name() == s->m_htmlTagName) {
            m_tree.insertHTMLHtmlStartTagBeforeHTML(token);
            setInsertionMode(BeforeHeadMode);
            return;
        }
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == s->m_headTagName) {
            m_tree.insertHTMLHeadElement(token);
            setInsertionMode(InHeadMode);
            return;
        }
        defaultForBeforeHead();
        // Fall through.
    case InHeadMode:
        ASSERT(insertionMode() == InHeadMode);
        if (processStartTagForInHead(token))
            return;
        defaultForInHead();
        // Fall through.
    case AfterHeadMode:
        ASSERT(insertionMode() == AfterHeadMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == s->m_bodyTagName) {
            m_framesetOk = false;
            m_tree.insertHTMLBodyElement(token);
            setInsertionMode(InBodyMode);
            return;
        }
        if (token->name() == s->m_framesetTagName) {
            m_tree.insertHTMLElement(token);
            setInsertionMode(InFramesetMode);
            return;
        }
        if (token->name() == s->m_baseTagName
            || token->name() == s->m_basefontTagName
            || token->name() == s->m_bgsoundTagName
            || token->name() == s->m_linkTagName
            || token->name() == s->m_metaTagName
            || token->name() == s->m_noframesTagName
            || token->name() == s->m_scriptTagName
            || token->name() == s->m_styleTagName
            || token->name() == s->m_templateTagName
            || token->name() == s->m_titleTagName) {
            parseError(token);
            ASSERT(m_tree.head());
            m_tree.openElements()->pushHTMLHeadElement(m_tree.headStackItem());
            processStartTagForInHead(token);
            m_tree.openElements()->removeHTMLHeadElement(m_tree.head());
            return;
        }
        if (token->name() == s->m_headTagName) {
            parseError(token);
            return;
        }
        defaultForAfterHead();
        // Fall through
    case InBodyMode:
        ASSERT(insertionMode() == InBodyMode);
        processStartTagForInBody(token);
        break;
    case InTableMode:
        ASSERT(insertionMode() == InTableMode);
        processStartTagForInTable(token);
        break;
    case InCaptionMode:
        ASSERT(insertionMode() == InCaptionMode);
        if (isCaptionColOrColgroupTag(s, token->name())
            || isTableBodyContextTag(s, token->name())
            || isTableCellContextTag(s, token->name())
            || token->name() == s->m_trTagName) {
            parseError(token);
            if (!processCaptionEndTagForInCaption()) {
                ASSERT(isParsingFragment());
                return;
            }
            processStartTag(token);
            return;
        }
        processStartTagForInBody(token);
        break;
    case InColumnGroupMode:
        ASSERT(insertionMode() == InColumnGroupMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == s->m_colTagName) {
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        if (token->name() == s->m_templateTagName) {
            processTemplateStartTag(token);
            return;
        }
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        processStartTag(token);
        break;
    case InTableBodyMode:
        ASSERT(insertionMode() == InTableBodyMode);
        if (token->name() == s->m_trTagName) {
            m_tree.openElements()->popUntilTableBodyScopeMarker(); // How is there ever anything to pop?
            m_tree.insertHTMLElement(token);
            setInsertionMode(InRowMode);
            return;
        }
        if (isTableCellContextTag(s, token->name())) {
            parseError(token);
            processFakeStartTag(s->m_trTagName);
            ASSERT(insertionMode() == InRowMode);
            processStartTag(token);
            return;
        }
        if (isCaptionColOrColgroupTag(s, token->name()) || isTableBodyContextTag(s, token->name())) {
            // FIXME: This is slow.
            if (!m_tree.openElements()->inTableScope(s->m_tbodyTagName) && !m_tree.openElements()->inTableScope(s->m_theadTagName) && !m_tree.openElements()->inTableScope(s->m_tfootTagName)) {
                ASSERT(isParsingFragmentOrTemplateContents());
                parseError(token);
                return;
            }
            m_tree.openElements()->popUntilTableBodyScopeMarker();
            ASSERT(isTableBodyContextTag(s, m_tree.currentStackItem()->localName()));
            processFakeEndTag(m_tree.currentStackItem()->localName());
            processStartTag(token);
            return;
        }
        processStartTagForInTable(token);
        break;
    case InRowMode:
        ASSERT(insertionMode() == InRowMode);
        if (isTableCellContextTag(s, token->name())) {
            m_tree.openElements()->popUntilTableRowScopeMarker();
            m_tree.insertHTMLElement(token);
            setInsertionMode(InCellMode);
            m_tree.activeFormattingElements()->appendMarker();
            return;
        }
        if (token->name() == s->m_trTagName
            || isCaptionColOrColgroupTag(s, token->name())
            || isTableBodyContextTag(s, token->name())) {
            if (!processTrEndTagForInRow()) {
                ASSERT(isParsingFragmentOrTemplateContents());
                return;
            }
            ASSERT(insertionMode() == InTableBodyMode);
            processStartTag(token);
            return;
        }
        processStartTagForInTable(token);
        break;
    case InCellMode:
        ASSERT(insertionMode() == InCellMode);
        if (isCaptionColOrColgroupTag(s, token->name())
            || isTableCellContextTag(s, token->name())
            || token->name() == s->m_trTagName
            || isTableBodyContextTag(s, token->name())) {
            // FIXME: This could be more efficient.
            if (!m_tree.openElements()->inTableScope(s->m_tdTagName) && !m_tree.openElements()->inTableScope(s->m_thTagName)) {
                ASSERT(isParsingFragment());
                parseError(token);
                return;
            }
            closeTheCell();
            processStartTag(token);
            return;
        }
        processStartTagForInBody(token);
        break;
    case AfterBodyMode:
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        setInsertionMode(InBodyMode);
        processStartTag(token);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == s->m_basefontTagName
            || token->name() == s->m_bgsoundTagName
            || token->name() == s->m_linkTagName
            || token->name() == s->m_metaTagName
            || token->name() == s->m_noframesTagName
            || token->name() == s->m_styleTagName) {
            bool didProcess = processStartTagForInHead(token);
            STARFISH_ASSERT_UNUSED(didProcess, didProcess);
            return;
        }
        if (token->name() == s->m_htmlTagName || token->name() == s->m_noscriptTagName) {
            parseError(token);
            return;
        }
        defaultForInHeadNoscript();
        processToken(token);
        break;
    case InFramesetMode:
        ASSERT(insertionMode() == InFramesetMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == s->m_framesetTagName) {
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token->name() == s->m_frameTagName) {
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        if (token->name() == s->m_noframesTagName) {
            processStartTagForInHead(token);
            return;
        }
        if (token->name() == s->m_templateTagName) {
            processTemplateStartTag(token);
            return;
        }
        parseError(token);
        break;
    case AfterFramesetMode:
    case AfterAfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == s->m_noframesTagName) {
            processStartTagForInHead(token);
            return;
        }
        parseError(token);
        break;
    case InSelectInTableMode:
        ASSERT(insertionMode() == InSelectInTableMode);
        if (token->name() == s->m_captionTagName
            || token->name() == s->m_tableTagName
            || isTableBodyContextTag(s, token->name())
            || token->name() == s->m_trTagName
            || isTableCellContextTag(s, token->name())) {
            parseError(token);
            AtomicHTMLToken endSelect(token->starFish(), HTMLToken::EndTag, s->m_selectTagName.localNameAtomic());
            processEndTag(&endSelect);
            processStartTag(token);
            return;
        }
        // Fall through
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        if (token->name() == s->m_htmlTagName) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == s->m_optionTagName) {
            if (m_tree.currentStackItem()->hasTagName(s->m_optionTagName)) {
                AtomicHTMLToken endOption(token->starFish(), HTMLToken::EndTag, s->m_optionTagName.localNameAtomic());
                processEndTag(&endOption);
            }
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token->name() == s->m_optgroupTagName) {
            if (m_tree.currentStackItem()->hasTagName(s->m_optionTagName)) {
                AtomicHTMLToken endOption(token->starFish(), HTMLToken::EndTag, s->m_optionTagName.localNameAtomic());
                processEndTag(&endOption);
            }
            if (m_tree.currentStackItem()->hasTagName(s->m_optgroupTagName)) {
                AtomicHTMLToken endOptgroup(token->starFish(), HTMLToken::EndTag, s->m_optgroupTagName.localNameAtomic());
                processEndTag(&endOptgroup);
            }
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token->name() == s->m_selectTagName) {
            parseError(token);
            AtomicHTMLToken endSelect(token->starFish(), HTMLToken::EndTag, s->m_selectTagName.localNameAtomic());
            processEndTag(&endSelect);
            return;
        }
        if (token->name() == s->m_inputTagName
            || token->name() == s->m_keygenTagName
            || token->name() == s->m_textareaTagName) {
            parseError(token);
            if (!m_tree.openElements()->inSelectScope(s->m_selectTagName)) {
                ASSERT(isParsingFragment());
                return;
            }
            AtomicHTMLToken endSelect(token->starFish(), HTMLToken::EndTag, s->m_selectTagName.localNameAtomic());
            processEndTag(&endSelect);
            processStartTag(token);
            return;
        }
        if (token->name() == s->m_scriptTagName) {
            bool didProcess = processStartTagForInHead(token);
            STARFISH_ASSERT_UNUSED(didProcess, didProcess);
            return;
        }
        if (token->name() == s->m_templateTagName) {
            processTemplateStartTag(token);
            return;
        }
        break;
    case InTableTextMode:
        defaultForInTableText();
        processStartTag(token);
        break;
    case TextMode:
        ASSERT_NOT_REACHED();
        break;
    case TemplateContentsMode:
        if (token->name() == s->m_templateTagName) {
            processTemplateStartTag(token);
            return;
        }

        if (token->name() == s->m_linkTagName
            || token->name() == s->m_scriptTagName
            || token->name() == s->m_styleTagName
            || token->name() == s->m_metaTagName) {
            processStartTagForInHead(token);
            return;
        }

        InsertionMode insertionMode = TemplateContentsMode;
        if (token->name() == s->m_frameTagName)
            insertionMode = InFramesetMode;
        else if (token->name() == s->m_colTagName)
            insertionMode = InColumnGroupMode;
        else if (isCaptionColOrColgroupTag(s, token->name()) || isTableBodyContextTag(s, token->name()))
            insertionMode = InTableMode;
        else if (token->name() == s->m_trTagName)
            insertionMode = InTableBodyMode;
        else if (isTableCellContextTag(s, token->name()))
            insertionMode = InRowMode;
        else
            insertionMode = InBodyMode;

        ASSERT(insertionMode != TemplateContentsMode);
        ASSERT(m_templateInsertionModes.back() == TemplateContentsMode);
        m_templateInsertionModes.back() = insertionMode;
        setInsertionMode(insertionMode);

        processStartTag(token);
        break;
    }
}

void HTMLTreeBuilder::processHtmlStartTagForInBody(AtomicHTMLToken* token)
{
    parseError(token);
    if (m_tree.openElements()->hasTemplateInHTMLScope()) {
        ASSERT(isParsingTemplateContents());
        return;
    }
    m_tree.insertHTMLHtmlStartTagInBody(token);
}

bool HTMLTreeBuilder::processBodyEndTagForInBody(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::EndTag);
    ASSERT(token->name() == s->m_bodyTagName);
    if (!m_tree.openElements()->inScope(s->m_bodyTagName)) {
        parseError(token);
        return false;
    }
    // notImplemented(); // Emit a more specific parse error based on stack contents.
    setInsertionMode(AfterBodyMode);
    return true;
}

void HTMLTreeBuilder::processAnyOtherEndTagForInBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    HTMLElementStack::ElementRecord* record = m_tree.openElements()->topRecord();
    while (1) {
        HTMLStackItem* item = record->stackItem();
        if (item->matchesHTMLTag(token->name())) {
            m_tree.generateImpliedEndTagsWithExclusion(token->name());
            if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
                parseError(token);
            m_tree.openElements()->popUntilPopped(item->element());
            return;
        }
        if (item->isSpecialNode()) {
            parseError(token);
            return;
        }
        record = record->next();
    }
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-inbody
void HTMLTreeBuilder::callTheAdoptionAgency(AtomicHTMLToken* token)
{
    // The adoption agency algorithm is N^2. We limit the number of iterations
    // to stop from hanging the whole browser. This limit is specified in the
    // adoption agency algorithm:
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#parsing-main-inbody
    static const int outerIterationLimit = 8;
    static const int innerIterationLimit = 3;

    // 1, 2, 3 and 16 are covered by the for() loop.
    for (int i = 0; i < outerIterationLimit; ++i) {
        // 4.
        Element* formattingElement = m_tree.activeFormattingElements()->closestElementInScopeWithName(token->name());
        // 4.a
        if (!formattingElement)
            return processAnyOtherEndTagForInBody(token);
        // 4.c
        if ((m_tree.openElements()->contains(formattingElement)) && !m_tree.openElements()->inScope(formattingElement)) {
            parseError(token);
            // notImplemented(); // Check the stack of open elements for a more specific parse error.
            return;
        }
        // 4.b
        HTMLElementStack::ElementRecord* formattingElementRecord = m_tree.openElements()->find(formattingElement);
        if (!formattingElementRecord) {
            parseError(token);
            m_tree.activeFormattingElements()->remove(formattingElement);
            return;
        }
        // 4.d
        if (formattingElement != m_tree.currentElement())
            parseError(token);
        // 5.
        HTMLElementStack::ElementRecord* furthestBlock = m_tree.openElements()->furthestBlockForFormattingElement(formattingElement);
        // 6.
        if (!furthestBlock) {
            m_tree.openElements()->popUntilPopped(formattingElement);
            m_tree.activeFormattingElements()->remove(formattingElement);
            return;
        }
        // 7.
        ASSERT(furthestBlock->isAbove(formattingElementRecord));
        HTMLStackItem* commonAncestor = formattingElementRecord->next()->stackItem();
        // 8.
        HTMLFormattingElementList::Bookmark bookmark = m_tree.activeFormattingElements()->bookmarkFor(formattingElement);
        // 9.
        HTMLElementStack::ElementRecord* node = furthestBlock;
        HTMLElementStack::ElementRecord* nextNode = node->next();
        HTMLElementStack::ElementRecord* lastNode = furthestBlock;
        // 9.1, 9.2, 9.3 and 9.11 are covered by the for() loop.
        for (int i = 0; i < innerIterationLimit; ++i) {
            // 9.4
            node = nextNode;
            ASSERT(node);
            nextNode = node->next(); // Save node->next() for the next iteration in case node is deleted in 9.5.
            // 9.5
            if (!m_tree.activeFormattingElements()->contains(node->element())) {
                m_tree.openElements()->remove(node->element());
                node = 0;
                continue;
            }
            // 9.6
            if (node == formattingElementRecord)
                break;
            // 9.7
            HTMLStackItem* newItem = m_tree.createElementFromSavedToken(node->stackItem());

            HTMLFormattingElementList::Entry* nodeEntry = m_tree.activeFormattingElements()->find(node->element());
            nodeEntry->replaceElement(newItem);
            node->replaceElement(newItem);
            newItem = nullptr;

            // 9.8
            if (lastNode == furthestBlock)
                bookmark.moveToAfter(nodeEntry);
            // 9.9
            m_tree.reparent(node, lastNode);
            // 9.10
            lastNode = node;
        }
        // 10.
        m_tree.insertAlreadyParsedChild(commonAncestor, lastNode);
        // 11.
        HTMLStackItem* newItem = m_tree.createElementFromSavedToken(formattingElementRecord->stackItem());
        // 12.
        m_tree.takeAllChildren(newItem, furthestBlock);
        // 13.
        m_tree.reparent(furthestBlock, newItem);
        // 14.
        m_tree.activeFormattingElements()->swapTo(formattingElement, newItem, bookmark);
        // 15.
        m_tree.openElements()->remove(formattingElement);
        m_tree.openElements()->insertAbove(newItem, furthestBlock);
    }
}

void HTMLTreeBuilder::resetInsertionModeAppropriately()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#reset-the-insertion-mode-appropriately
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    bool last = false;
    HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
    while (1) {
        HTMLStackItem* item = nodeRecord->stackItem();
        if (item->node() == m_tree.openElements()->rootNode()) {
            last = true;
            if (isParsingFragment())
                item = m_fragmentContext.contextElementStackItem();
        }
        if (item->hasTagName(s->m_templateTagName))
            return setInsertionMode(m_templateInsertionModes.back());
        if (item->hasTagName(s->m_selectTagName)) {
            if (!last) {
                while (item->node() != m_tree.openElements()->rootNode() && !item->hasTagName(s->m_templateTagName)) {
                    nodeRecord = nodeRecord->next();
                    item = nodeRecord->stackItem();
                    if (item->hasTagName(s->m_tableTagName))
                        return setInsertionMode(InSelectInTableMode);
                }
            }
            return setInsertionMode(InSelectMode);
        }
        if (item->hasTagName(s->m_tdTagName) || item->hasTagName(s->m_thTagName))
            return setInsertionMode(InCellMode);
        if (item->hasTagName(s->m_trTagName))
            return setInsertionMode(InRowMode);
        if (item->hasTagName(s->m_tbodyTagName) || item->hasTagName(s->m_theadTagName) || item->hasTagName(s->m_tfootTagName))
            return setInsertionMode(InTableBodyMode);
        if (item->hasTagName(s->m_captionTagName))
            return setInsertionMode(InCaptionMode);
        if (item->hasTagName(s->m_colgroupTagName)) {
            return setInsertionMode(InColumnGroupMode);
        }
        if (item->hasTagName(s->m_tableTagName))
            return setInsertionMode(InTableMode);
        if (item->hasTagName(s->m_headTagName)) {
            if (!m_fragmentContext.fragment() || m_fragmentContext.contextElement() != item->node())
                return setInsertionMode(InHeadMode);
            return setInsertionMode(InBodyMode);
        }
        if (item->hasTagName(s->m_bodyTagName))
            return setInsertionMode(InBodyMode);
        if (item->hasTagName(s->m_framesetTagName)) {
            return setInsertionMode(InFramesetMode);
        }
        if (item->hasTagName(s->m_htmlTagName)) {
            if (m_tree.headStackItem())
                return setInsertionMode(AfterHeadMode);

            ASSERT(isParsingFragment());
            return setInsertionMode(BeforeHeadMode);
        }
        if (last) {
            ASSERT(isParsingFragment());
            return setInsertionMode(InBodyMode);
        }
        nodeRecord = nodeRecord->next();
    }
}

void HTMLTreeBuilder::processEndTagForInTableBody(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::EndTag);
    if (isTableBodyContextTag(s, token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.openElements()->popUntilTableBodyScopeMarker();
        m_tree.openElements()->pop();
        setInsertionMode(InTableMode);
        return;
    }
    if (token->name() == s->m_tableTagName) {
        // FIXME: This is slow.
        if (!m_tree.openElements()->inTableScope(s->m_tbodyTagName) && !m_tree.openElements()->inTableScope(s->m_theadTagName) && !m_tree.openElements()->inTableScope(s->m_tfootTagName)) {
            ASSERT(isParsingFragmentOrTemplateContents());
            parseError(token);
            return;
        }
        m_tree.openElements()->popUntilTableBodyScopeMarker();
        ASSERT(isTableBodyContextTag(s, m_tree.currentStackItem()->localName()));
        processFakeEndTag(m_tree.currentStackItem()->localName());
        processEndTag(token);
        return;
    }
    if (token->name() == s->m_bodyTagName
        || isCaptionColOrColgroupTag(s, token->name())
        || token->name() == s->m_htmlTagName
        || isTableCellContextTag(s, token->name())
        || token->name() == s->m_trTagName) {
        parseError(token);
        return;
    }
    processEndTagForInTable(token);
}

void HTMLTreeBuilder::processEndTagForInRow(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::EndTag);
    if (token->name() == s->m_trTagName) {
        processTrEndTagForInRow();
        return;
    }
    if (token->name() == s->m_tableTagName) {
        if (!processTrEndTagForInRow()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        ASSERT(insertionMode() == InTableBodyMode);
        processEndTag(token);
        return;
    }
    if (isTableBodyContextTag(s, token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
            parseError(token);
            return;
        }
        processFakeEndTag(s->m_trTagName);
        ASSERT(insertionMode() == InTableBodyMode);
        processEndTag(token);
        return;
    }
    if (token->name() == s->m_bodyTagName
        || isCaptionColOrColgroupTag(s, token->name())
        || token->name() == s->m_htmlTagName
        || isTableCellContextTag(s, token->name())) {
        parseError(token);
        return;
    }
    processEndTagForInTable(token);
}

void HTMLTreeBuilder::processEndTagForInCell(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    if (isTableCellContextTag(s, token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        m_tree.activeFormattingElements()->clearToLastMarker();
        setInsertionMode(InRowMode);
        return;
    }
    if (token->name() == s->m_bodyTagName
        || isCaptionColOrColgroupTag(s, token->name())
        || token->name() == s->m_htmlTagName) {
        parseError(token);
        return;
    }
    if (token->name() == s->m_tableTagName
        || token->name() == s->m_trTagName
        || isTableBodyContextTag(s, token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
            ASSERT(isTableBodyContextTag(s, token->name()) || m_tree.openElements()->inTableScope(s->m_templateTagName) || isParsingFragment());
            parseError(token);
            return;
        }
        closeTheCell();
        processEndTag(token);
        return;
    }
    processEndTagForInBody(token);
}

void HTMLTreeBuilder::processEndTagForInBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    if (token->name() == s->m_bodyTagName) {
        processBodyEndTagForInBody(token);
        return;
    }
    if (token->name() == s->m_htmlTagName) {
        AtomicHTMLToken endBody(token->starFish(), HTMLToken::EndTag, s->m_bodyTagName.localNameAtomic());
        if (processBodyEndTagForInBody(&endBody))
            processEndTag(token);
        return;
    }
    if (token->name() == s->m_addressTagName
        || token->name() == s->m_articleTagName
        || token->name() == s->m_asideTagName
        || token->name() == s->m_blockquoteTagName
        || token->name() == s->m_buttonTagName
        || token->name() == s->m_centerTagName
        || token->name() == s->m_detailsTagName
        || token->name() == s->m_dirTagName
        || token->name() == s->m_divTagName
        || token->name() == s->m_dlTagName
        || token->name() == s->m_fieldsetTagName
        || token->name() == s->m_figcaptionTagName
        || token->name() == s->m_figureTagName
        || token->name() == s->m_footerTagName
        || token->name() == s->m_headerTagName
        || token->name() == s->m_hgroupTagName
        || token->name() == s->m_listingTagName
        || token->name() == s->m_mainTagName
        || token->name() == s->m_menuTagName
        || token->name() == s->m_navTagName
        || token->name() == s->m_olTagName
        || token->name() == s->m_preTagName
        || token->name() == s->m_sectionTagName
        || token->name() == s->m_summaryTagName
        || token->name() == s->m_ulTagName) {
        if (!m_tree.openElements()->inScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (token->name() == s->m_formTagName) {
        Element* node = m_tree.takeForm();
        if (!node || !m_tree.openElements()->inScope(node)) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (m_tree.currentElement() != node)
            parseError(token);
        m_tree.openElements()->remove(node);
    }
    if (token->name() == s->m_pTagName) {
        if (!m_tree.openElements()->inButtonScope(token->name())) {
            parseError(token);
            processFakeStartTag(s->m_pTagName);
            ASSERT(m_tree.openElements()->inScope(token->name()));
            processEndTag(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token->name());
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (token->name() == s->m_liTagName) {
        if (!m_tree.openElements()->inListItemScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token->name());
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (token->name() == s->m_ddTagName
        || token->name() == s->m_dtTagName) {
        if (!m_tree.openElements()->inScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token->name());
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (isNumberedHeaderTag(s, token->name())) {
        if (!m_tree.openElements()->hasNumberedHeaderElementInScope()) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilNumberedHeaderElementPopped();
        return;
    }
    if (isFormattingTag(s, token->name())) {
        callTheAdoptionAgency(token);
        return;
    }
    if (token->name() == s->m_appletTagName
        || token->name() == s->m_marqueeTagName
        || token->name() == s->m_objectTagName) {
        if (!m_tree.openElements()->inScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        m_tree.activeFormattingElements()->clearToLastMarker();
        return;
    }
    if (token->name() == s->m_brTagName) {
        parseError(token);
        processFakeStartTag(s->m_brTagName);
        return;
    }
    if (token->name() == s->m_templateTagName) {
        processTemplateEndTag(token);
        return;
    }
    processAnyOtherEndTagForInBody(token);
}

bool HTMLTreeBuilder::processCaptionEndTagForInCaption()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    if (!m_tree.openElements()->inTableScope(s->m_captionTagName)) {
        ASSERT(isParsingFragment());
        // FIXME: parse error
        return false;
    }
    m_tree.generateImpliedEndTags();
    // FIXME: parse error if (!m_tree.currentStackItem()->hasTagName(captionTag))
    m_tree.openElements()->popUntilPopped(s->m_captionTagName);
    m_tree.activeFormattingElements()->clearToLastMarker();
    setInsertionMode(InTableMode);
    return true;
}

bool HTMLTreeBuilder::processTrEndTagForInRow()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    if (!m_tree.openElements()->inTableScope(s->m_trTagName)) {
        ASSERT(isParsingFragmentOrTemplateContents());
        // FIXME: parse error
        return false;
    }
    m_tree.openElements()->popUntilTableRowScopeMarker();
    ASSERT(m_tree.currentStackItem()->hasTagName(s->m_trTagName));
    m_tree.openElements()->pop();
    setInsertionMode(InTableBodyMode);
    return true;
}

bool HTMLTreeBuilder::processTableEndTagForInTable()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    if (!m_tree.openElements()->inTableScope(s->m_tableTagName)) {
        ASSERT(isParsingFragmentOrTemplateContents());
        // FIXME: parse error.
        return false;
    }
    m_tree.openElements()->popUntilPopped(s->m_tableTagName);
    resetInsertionModeAppropriately();
    return true;
}

void HTMLTreeBuilder::processEndTagForInTable(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::EndTag);
    if (token->name() == s->m_tableTagName) {
        processTableEndTagForInTable();
        return;
    }
    if (token->name() == s->m_bodyTagName
        || isCaptionColOrColgroupTag(s, token->name())
        || token->name() == s->m_htmlTagName
        || isTableBodyContextTag(s, token->name())
        || isTableCellContextTag(s, token->name())
        || token->name() == s->m_trTagName) {
        parseError(token);
        return;
    }
    parseError(token);
    // Is this redirection necessary here?
    HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
    processEndTagForInBody(token);
}

void HTMLTreeBuilder::processEndTag(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::EndTag);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        if (token->name() != s->m_headTagName && token->name() != s->m_bodyTagName && token->name() != s->m_htmlTagName && token->name() != s->m_brTagName) {
            parseError(token);
            return;
        }
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        if (token->name() != s->m_headTagName && token->name() != s->m_bodyTagName && token->name() != s->m_htmlTagName && token->name() != s->m_brTagName) {
            parseError(token);
            return;
        }
        defaultForBeforeHead();
        // Fall through.
    case InHeadMode:
        ASSERT(insertionMode() == InHeadMode);
        // FIXME: This case should be broken out into processEndTagForInHead,
        // because other end tag cases now refer to it ("process the token for using the rules of the "in head" insertion mode").
        // but because the logic falls through to AfterHeadMode, that gets a little messy.
        if (token->name() == s->m_templateTagName) {
            processTemplateEndTag(token);
            return;
        }
        if (token->name() == s->m_headTagName) {
            m_tree.openElements()->popHTMLHeadElement();
            setInsertionMode(AfterHeadMode);
            return;
        }
        if (token->name() != s->m_bodyTagName && token->name() != s->m_htmlTagName && token->name() != s->m_brTagName) {
            parseError(token);
            return;
        }
        defaultForInHead();
        // Fall through.
    case AfterHeadMode:
        ASSERT(insertionMode() == AfterHeadMode);
        if (token->name() != s->m_bodyTagName && token->name() != s->m_htmlTagName && token->name() != s->m_brTagName) {
            parseError(token);
            return;
        }
        defaultForAfterHead();
        // Fall through
    case InBodyMode:
        ASSERT(insertionMode() == InBodyMode);
        processEndTagForInBody(token);
        break;
    case InTableMode:
        ASSERT(insertionMode() == InTableMode);
        processEndTagForInTable(token);
        break;
    case InCaptionMode:
        ASSERT(insertionMode() == InCaptionMode);
        if (token->name() == s->m_captionTagName) {
            processCaptionEndTagForInCaption();
            return;
        }
        if (token->name() == s->m_tableTagName) {
            parseError(token);
            if (!processCaptionEndTagForInCaption()) {
                ASSERT(isParsingFragment());
                return;
            }
            processEndTag(token);
            return;
        }
        if (token->name() == s->m_bodyTagName
            || token->name() == s->m_colTagName
            || token->name() == s->m_colgroupTagName
            || token->name() == s->m_htmlTagName
            || isTableBodyContextTag(s, token->name())
            || isTableCellContextTag(s, token->name())
            || token->name() == s->m_trTagName) {
            parseError(token);
            return;
        }
        processEndTagForInBody(token);
        break;
    case InColumnGroupMode:
        ASSERT(insertionMode() == InColumnGroupMode);
        if (token->name() == s->m_colgroupTagName) {
            processColgroupEndTagForInColumnGroup();
            return;
        }
        if (token->name() == s->m_colTagName) {
            parseError(token);
            return;
        }
        if (token->name() == s->m_templateTagName) {
            processTemplateEndTag(token);
            return;
        }
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        processEndTag(token);
        break;
    case InRowMode:
        ASSERT(insertionMode() == InRowMode);
        processEndTagForInRow(token);
        break;
    case InCellMode:
        ASSERT(insertionMode() == InCellMode);
        processEndTagForInCell(token);
        break;
    case InTableBodyMode:
        ASSERT(insertionMode() == InTableBodyMode);
        processEndTagForInTableBody(token);
        break;
    case AfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode);
        if (token->name() == s->m_htmlTagName) {
            if (isParsingFragment()) {
                parseError(token);
                return;
            }
            setInsertionMode(AfterAfterBodyMode);
            return;
        }
        // Fall through.
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        parseError(token);
        setInsertionMode(InBodyMode);
        processEndTag(token);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        if (token->name() == s->m_noscriptTagName) {
            ASSERT(m_tree.currentStackItem()->localName() == s->m_noscriptTagName);
            m_tree.openElements()->pop();
            ASSERT(m_tree.currentStackItem()->localName() == s->m_headTagName);
            setInsertionMode(InHeadMode);
            return;
        }
        if (token->name() != s->m_brTagName) {
            parseError(token);
            return;
        }
        defaultForInHeadNoscript();
        processToken(token);
        break;
    case TextMode:
        if (token->name() == s->m_scriptTagName) {
            // Pause ourselves so that parsing stops until the script can be processed by the caller.
            ASSERT(m_tree.currentStackItem()->hasTagName(s->m_scriptTagName));
            // if (scriptingContentIsAllowed(m_tree.parserContentPolicy()))
            if (true) {
                STARFISH_ASSERT(!m_scriptToProcess);
                m_scriptToProcess = m_tree.currentElement();
            }
            m_tree.openElements()->pop();
            setInsertionMode(m_originalInsertionMode);

            if (m_parser->tokenizer()) {
                // We must set the tokenizer's state to
                // DataState explicitly if the tokenizer didn't have a chance to.
                ASSERT(m_parser->tokenizer()->state() == HTMLTokenizer::DataState);
                m_parser->tokenizer()->setState(HTMLTokenizer::DataState);
            }
            return;
        }
        m_tree.openElements()->pop();
        setInsertionMode(m_originalInsertionMode);
        break;
    case InFramesetMode:
        ASSERT(insertionMode() == InFramesetMode);
        if (token->name() == s->m_framesetTagName) {
            bool ignoreFramesetForFragmentParsing  = m_tree.currentIsRootNode();
            ignoreFramesetForFragmentParsing = ignoreFramesetForFragmentParsing || m_tree.openElements()->hasTemplateInHTMLScope();
            if (ignoreFramesetForFragmentParsing) {
                ASSERT(isParsingFragmentOrTemplateContents());
                parseError(token);
                return;
            }
            m_tree.openElements()->pop();
            if (!isParsingFragment() && !m_tree.currentStackItem()->hasTagName(s->m_framesetTagName))
                setInsertionMode(AfterFramesetMode);
            return;
        }
        if (token->name() == s->m_templateTagName) {
            processTemplateEndTag(token);
            return;
        }
        break;
    case AfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode);
        if (token->name() == s->m_htmlTagName) {
            setInsertionMode(AfterAfterFramesetMode);
            return;
        }
        // Fall through.
    case AfterAfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        parseError(token);
        break;
    case InSelectInTableMode:
        ASSERT(insertionMode() == InSelectInTableMode);
        if (token->name() == s->m_captionTagName
            || token->name() == s->m_tableTagName
            || isTableBodyContextTag(s, token->name())
            || token->name() == s->m_trTagName
            || isTableCellContextTag(s, token->name())) {
            parseError(token);
            if (m_tree.openElements()->inTableScope(token->name())) {
                AtomicHTMLToken endSelect(token->starFish(), HTMLToken::EndTag, s->m_selectTagName.localNameAtomic());
                processEndTag(&endSelect);
                processEndTag(token);
            }
            return;
        }
        // Fall through.
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        if (token->name() == s->m_optgroupTagName) {
            if (m_tree.currentStackItem()->hasTagName(s->m_optionTagName) && m_tree.oneBelowTop() && m_tree.oneBelowTop()->hasTagName(s->m_optgroupTagName))
                processFakeEndTag(s->m_optionTagName);
            if (m_tree.currentStackItem()->hasTagName(s->m_optgroupTagName)) {
                m_tree.openElements()->pop();
                return;
            }
            parseError(token);
            return;
        }
        if (token->name() == s->m_optionTagName) {
            if (m_tree.currentStackItem()->hasTagName(s->m_optionTagName)) {
                m_tree.openElements()->pop();
                return;
            }
            parseError(token);
            return;
        }
        if (token->name() == s->m_selectTagName) {
            if (!m_tree.openElements()->inSelectScope(token->name())) {
                ASSERT(isParsingFragment());
                parseError(token);
                return;
            }
            m_tree.openElements()->popUntilPopped(s->m_selectTagName);
            resetInsertionModeAppropriately();
            return;
        }
        if (token->name() == s->m_templateTagName) {
            processTemplateEndTag(token);
            return;
        }
        break;
    case InTableTextMode:
        defaultForInTableText();
        processEndTag(token);
        break;
    case TemplateContentsMode:
        if (token->name() == s->m_templateTagName) {
            processTemplateEndTag(token);
            return;
        }
        break;
    }
}

void HTMLTreeBuilder::processComment(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::Comment);
    if (m_insertionMode == InitialMode
        || m_insertionMode == BeforeHTMLMode
        || m_insertionMode == AfterAfterBodyMode
        || m_insertionMode == AfterAfterFramesetMode) {
        m_tree.insertCommentOnDocument(token);
        return;
    }
    if (m_insertionMode == AfterBodyMode) {
        m_tree.insertCommentOnHTMLHtmlElement(token);
        return;
    }
    if (m_insertionMode == InTableTextMode) {
        defaultForInTableText();
        processComment(token);
        return;
    }
    m_tree.insertComment(token);
}

void HTMLTreeBuilder::processCharacter(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::Character);
    CharacterTokenBuffer buffer(token);
    processCharacterBuffer(buffer);
}

void HTMLTreeBuilder::processCharacterBuffer(CharacterTokenBuffer& buffer)
{
ReprocessBuffer:
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-inbody
    // Note that this logic is different than the generic \r\n collapsing
    // handled in the input stream preprocessor. This logic is here as an
    // "authoring convenience" so folks can write:
    //
    // <pre>
    // lorem ipsum
    // lorem ipsum
    // </pre>
    //
    // without getting an extra newline at the start of their <pre> element.
    if (m_shouldSkipLeadingNewline) {
        m_shouldSkipLeadingNewline = false;
        buffer.skipAtMostOneLeadingNewline();
        if (buffer.isEmpty())
            return;
    }

    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    switch (insertionMode()) {
    case InitialMode: {
        ASSERT(insertionMode() == InitialMode);
        buffer.skipLeadingWhitespace();
        if (buffer.isEmpty())
            return;
        defaultForInitial();
        // Fall through.
    }
    case BeforeHTMLMode: {
        ASSERT(insertionMode() == BeforeHTMLMode);
        buffer.skipLeadingWhitespace();
        if (buffer.isEmpty())
            return;
        defaultForBeforeHTML();
        // Fall through.
    }
    case BeforeHeadMode: {
        ASSERT(insertionMode() == BeforeHeadMode);
        buffer.skipLeadingWhitespace();
        if (buffer.isEmpty())
            return;
        defaultForBeforeHead();
        // Fall through.
    }
    case InHeadMode: {
        ASSERT(insertionMode() == InHeadMode);
        String* leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!(leadingWhitespace->length() == 0))
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForInHead();
        // Fall through.
    }
    case AfterHeadMode: {
        ASSERT(insertionMode() == AfterHeadMode);
        String* leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!(leadingWhitespace->length() == 0))
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForAfterHead();
        // Fall through.
    }
    case InBodyMode:
    case InCaptionMode:
    case TemplateContentsMode:
    case InCellMode: {
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCaptionMode || insertionMode() == InCellMode || insertionMode() == TemplateContentsMode);
        processCharacterBufferForInBody(buffer);
        break;
    }
    case InTableMode:
    case InTableBodyMode:
    case InRowMode: {
        ASSERT(insertionMode() == InTableMode || insertionMode() == InTableBodyMode || insertionMode() == InRowMode);
        // TODO enable this
        // ASSERT(m_pendingTableCharacters.size() == 0);
        if (m_tree.currentStackItem()->isElementNode()
            && (m_tree.currentStackItem()->hasTagName(s->m_tableTagName)
                || m_tree.currentStackItem()->hasTagName(s->m_tbodyTagName)
                || m_tree.currentStackItem()->hasTagName(s->m_tfootTagName)
                || m_tree.currentStackItem()->hasTagName(s->m_theadTagName)
                || m_tree.currentStackItem()->hasTagName(s->m_trTagName))) {
            m_originalInsertionMode = m_insertionMode;
            setInsertionMode(InTableTextMode);
            // Note that we fall through to the InTableTextMode case below.
        } else {
            HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
            processCharacterBufferForInBody(buffer);
            break;
        }
        // Fall through.
    }
    case InTableTextMode: {
        buffer.giveRemainingTo(m_pendingTableCharacters);
        break;
    }
    case InColumnGroupMode: {
        ASSERT(insertionMode() == InColumnGroupMode);
        String* leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!(leadingWhitespace->length() == 0))
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        if (buffer.isEmpty())
            return;
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            // The spec tells us to drop these characters on the floor.
            buffer.skipLeadingNonWhitespace();
            if (buffer.isEmpty())
                return;
        }
        goto ReprocessBuffer;
    }
    case AfterBodyMode:
    case AfterAfterBodyMode: {
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        // FIXME: parse error
        setInsertionMode(InBodyMode);
        goto ReprocessBuffer;
    }
    case TextMode: {
        ASSERT(insertionMode() == TextMode);
        m_tree.insertTextNode(buffer.takeRemaining());
        break;
    }
    case InHeadNoscriptMode: {
        ASSERT(insertionMode() == InHeadNoscriptMode);
        String* leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!(leadingWhitespace->length() == 0))
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForInHeadNoscript();
        goto ReprocessBuffer;
    }
    case InFramesetMode:
    case AfterFramesetMode: {
        ASSERT(insertionMode() == InFramesetMode || insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        String* leadingWhitespace = buffer.takeRemainingWhitespace();
        if (!(leadingWhitespace->length() == 0))
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        // FIXME: We should generate a parse error if we skipped over any
        // non-whitespace characters.
        break;
    }
    case InSelectInTableMode:
    case InSelectMode: {
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        m_tree.insertTextNode(buffer.takeRemaining());
        break;
    }
    case AfterAfterFramesetMode: {
        String* leadingWhitespace = buffer.takeRemainingWhitespace();
        if (!(leadingWhitespace->length() == 0)) {
            m_tree.reconstructTheActiveFormattingElements();
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        }
        // FIXME: We should generate a parse error if we skipped over any
        // non-whitespace characters.
        break;
    }
    }
}


void HTMLTreeBuilder::processCharacterBufferForInBody(CharacterTokenBuffer& buffer)
{
    m_tree.reconstructTheActiveFormattingElements();
    String* characters = buffer.takeRemaining();
    m_tree.insertTextNode(characters);
    if (m_framesetOk && !isAllWhitespaceOrReplacementCharacters(characters))
        m_framesetOk = false;
}

void HTMLTreeBuilder::processEndOfFile(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::EndOfFile);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        defaultForBeforeHead();
        // Fall through.
    case InHeadMode:
        ASSERT(insertionMode() == InHeadMode);
        defaultForInHead();
        // Fall through.
    case AfterHeadMode:
        ASSERT(insertionMode() == AfterHeadMode);
        defaultForAfterHead();
        // Fall through
    case InBodyMode:
    case InCellMode:
    case InCaptionMode:
    case InRowMode:
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCellMode || insertionMode() == InCaptionMode || insertionMode() == InRowMode || insertionMode() == TemplateContentsMode);
        // notImplemented(); // Emit parse error based on what elements are still open.
        if (!(m_templateInsertionModes.size() == 0) && processEndOfFileForInTemplateContents(token))
            return;
        break;
    case AfterBodyMode:
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        defaultForInHeadNoscript();
        processEndOfFile(token);
        return;
    case AfterFramesetMode:
    case AfterAfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        break;
    case InColumnGroupMode:
        if (m_tree.currentIsRootNode()) {
            ASSERT(isParsingFragment());
            return; // FIXME: Should we break here instead of returning?
        }
        ASSERT(m_tree.currentNode()->asElement()->name() == (s->m_colgroupTagName) || m_tree.currentNode()->asElement()->name() == (s->m_templateTagName));
        processColgroupEndTagForInColumnGroup();
        // Fall through
    case InFramesetMode:
    case InTableMode:
    case InTableBodyMode:
    case InSelectInTableMode:
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode || insertionMode() == InTableMode || insertionMode() == InFramesetMode || insertionMode() == InTableBodyMode || insertionMode() == InColumnGroupMode);
        if (m_tree.currentNode() != m_tree.openElements()->rootNode())
            parseError(token);
        if (!(m_templateInsertionModes.size() == 0) && processEndOfFileForInTemplateContents(token))
            return;
        break;
    case InTableTextMode:
        defaultForInTableText();
        processEndOfFile(token);
        return;
    case TextMode:
        parseError(token);
        if (m_tree.currentStackItem()->hasTagName(s->m_scriptTagName)) {
            // notImplemented(); // mark the script element as "already started".
        }
        m_tree.openElements()->pop();
        ASSERT(m_originalInsertionMode != TextMode);
        setInsertionMode(m_originalInsertionMode);
        processEndOfFile(token);
        return;
    case TemplateContentsMode:
        if (processEndOfFileForInTemplateContents(token))
            return;
        break;
    }
    m_tree.processEndOfFile();
}

void HTMLTreeBuilder::defaultForInitial()
{
    // notImplemented();
    m_tree.setDefaultCompatibilityMode();
    // FIXME: parse error
    setInsertionMode(BeforeHTMLMode);
}

void HTMLTreeBuilder::defaultForBeforeHTML()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    AtomicHTMLToken startHTML(m_tree.document()->window()->starFish(), HTMLToken::StartTag, s->m_htmlTagName.localNameAtomic());
    m_tree.insertHTMLHtmlStartTagBeforeHTML(&startHTML);
    setInsertionMode(BeforeHeadMode);
}

void HTMLTreeBuilder::defaultForBeforeHead()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    AtomicHTMLToken startHead(m_tree.document()->window()->starFish(), HTMLToken::StartTag, s->m_headTagName.localNameAtomic());
    processStartTag(&startHead);
}

void HTMLTreeBuilder::defaultForInHead()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    AtomicHTMLToken endHead(m_tree.document()->window()->starFish(), HTMLToken::EndTag, s->m_headTagName.localNameAtomic());
    processEndTag(&endHead);
}

void HTMLTreeBuilder::defaultForInHeadNoscript()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    AtomicHTMLToken endNoscript(m_tree.document()->window()->starFish(), HTMLToken::EndTag, s->m_noscriptTagName.localNameAtomic());
    processEndTag(&endNoscript);
}

void HTMLTreeBuilder::defaultForAfterHead()
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    AtomicHTMLToken startBody(m_tree.document()->window()->starFish(), HTMLToken::StartTag, s->m_bodyTagName.localNameAtomic());
    processStartTag(&startBody);
    m_framesetOk = true;
}

void HTMLTreeBuilder::defaultForInTableText()
{
    String* characters = new StringDataUTF32(std::move(m_pendingTableCharacters));
    // m_pendingTableCharacters.clear();
    if (!isAllWhitespace(characters)) {
        // FIXME: parse error
        HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertTextNode(characters, NotAllWhitespace);
        m_framesetOk = false;
        setInsertionMode(m_originalInsertionMode);
        return;
    }
    m_tree.insertTextNode(characters);
    setInsertionMode(m_originalInsertionMode);
}

bool HTMLTreeBuilder::processStartTagForInHead(AtomicHTMLToken* token)
{
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();
    ASSERT(token->type() == HTMLToken::StartTag);
    if (token->name() == s->m_htmlTagName) {
        processHtmlStartTagForInBody(token);
        return true;
    }
    if (token->name() == s->m_baseTagName
        || token->name() == s->m_basefontTagName
        || token->name() == s->m_bgsoundTagName
        || token->name() == s->m_commandTagName
        || token->name() == s->m_linkTagName
        || token->name() == s->m_metaTagName) {
        m_tree.insertSelfClosingHTMLElement(token);
        // Note: The custom processing for the <meta> tag is done in HTMLMetaElement::process().
        return true;
    }
    if (token->name() == s->m_titleTagName) {
        processGenericRCDATAStartTag(token);
        return true;
    }
    if (token->name() == s->m_noscriptTagName) {
        // if (m_options.scriptEnabled) {
        if (true) {
            processGenericRawTextStartTag(token);
            return true;
        }
        m_tree.insertHTMLElement(token);
        setInsertionMode(InHeadNoscriptMode);
        return true;
    }
    if (token->name() == s->m_noframesTagName || token->name() == s->m_styleTagName) {
        processGenericRawTextStartTag(token);
        return true;
    }
    if (token->name() == s->m_scriptTagName) {
        processScriptStartTag(token);
        return true;
    }
    if (token->name() == s->m_templateTagName) {
        processTemplateStartTag(token);
        return true;
    }
    if (token->name() == s->m_headTagName) {
        parseError(token);
        return true;
    }
    return false;
}

void HTMLTreeBuilder::processGenericRCDATAStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    m_tree.insertHTMLElement(token);
    if (m_parser->tokenizer())
        m_parser->tokenizer()->setState(HTMLTokenizer::RCDATAState);
    m_originalInsertionMode = m_insertionMode;
    setInsertionMode(TextMode);
}

void HTMLTreeBuilder::processGenericRawTextStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    m_tree.insertHTMLElement(token);
    if (m_parser->tokenizer())
        m_parser->tokenizer()->setState(HTMLTokenizer::RAWTEXTState);
    m_originalInsertionMode = m_insertionMode;
    setInsertionMode(TextMode);
}

void HTMLTreeBuilder::processScriptStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    m_tree.insertScriptElement(token);
    if (m_parser->tokenizer())
        m_parser->tokenizer()->setState(HTMLTokenizer::ScriptDataState);
    m_originalInsertionMode = m_insertionMode;

    TextPosition position = m_parser->textPosition();

    m_scriptToProcessStartPosition = position;

    setInsertionMode(TextMode);
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#tree-construction
bool HTMLTreeBuilder::shouldProcessTokenInForeignContent(AtomicHTMLToken* token)
{
    if (m_tree.isEmpty())
        return false;
    HTMLStackItem* adjustedCurrentNode = adjustedCurrentStackItem();

    if (adjustedCurrentNode->isInHTMLNamespace())
        return false;
    if (HTMLElementStack::isMathMLTextIntegrationPoint(adjustedCurrentNode)) {
        if (token->type() == HTMLToken::StartTag /*
            && token->name() != MathMLNames::mglyphTag
            && token->name() != MathMLNames::malignmarkTag*/ )
            return false;
        if (token->type() == HTMLToken::Character)
            return false;
    }
    /*
    if (adjustedCurrentNode->hasTagName(MathMLNames::annotation_xmlTag)
        && token->type() == HTMLToken::StartTag
        && token->name() == SVGNames::svgTag)
        return false;
    */
    if (HTMLElementStack::isHTMLIntegrationPoint(adjustedCurrentNode)) {
        if (token->type() == HTMLToken::StartTag)
            return false;
        if (token->type() == HTMLToken::Character)
            return false;
    }
    if (token->type() == HTMLToken::EndOfFile)
        return false;
    return true;
}

void HTMLTreeBuilder::processTokenInForeignContent(AtomicHTMLToken* token)
{
    if (token->type() == HTMLToken::Character) {
        String* characters = token->characters();
        m_tree.insertTextNode(characters);
        if (m_framesetOk && !isAllWhitespaceOrReplacementCharacters(characters))
            m_framesetOk = false;
        return;
    }

    m_tree.flush();
    HTMLStackItem* adjustedCurrentNode = adjustedCurrentStackItem();
    StaticStrings* s = m_tree.document()->window()->starFish()->staticStrings();

    switch (token->type()) {
    case HTMLToken::Uninitialized:
        ASSERT_NOT_REACHED();
        break;
    case HTMLToken::DOCTYPE:
        parseError(token);
        break;
    case HTMLToken::StartTag: {
        if (token->name() == s->m_bTagName
            || token->name() == s->m_bigTagName
            || token->name() == s->m_blockquoteTagName
            || token->name() == s->m_bodyTagName
            || token->name() == s->m_brTagName
            || token->name() == s->m_centerTagName
            || token->name() == s->m_codeTagName
            || token->name() == s->m_ddTagName
            || token->name() == s->m_divTagName
            || token->name() == s->m_dlTagName
            || token->name() == s->m_dtTagName
            || token->name() == s->m_emTagName
            || token->name() == s->m_embedTagName
            || isNumberedHeaderTag(s, token->name())
            || token->name() == s->m_headTagName
            || token->name() == s->m_hrTagName
            || token->name() == s->m_iTagName
            || token->name() == s->m_imgTagName
            || token->name() == s->m_liTagName
            || token->name() == s->m_listingTagName
            || token->name() == s->m_menuTagName
            || token->name() == s->m_metaTagName
            || token->name() == s->m_nobrTagName
            || token->name() == s->m_olTagName
            || token->name() == s->m_pTagName
            || token->name() == s->m_preTagName
            || token->name() == s->m_rubyTagName
            || token->name() == s->m_sTagName
            || token->name() == s->m_smallTagName
            || token->name() == s->m_spanTagName
            || token->name() == s->m_strongTagName
            || token->name() == s->m_strikeTagName
            || token->name() == s->m_subTagName
            || token->name() == s->m_supTagName
            || token->name() == s->m_tableTagName
            || token->name() == s->m_ttTagName
            || token->name() == s->m_uTagName
            || token->name() == s->m_ulTagName
            || token->name() == s->m_varTagName
            || (token->name() == s->m_fontTagName && (token->getAttributeItem(s->m_color) || token->getAttributeItem(s->m_face) || token->getAttributeItem(s->m_size)))) {
            parseError(token);
            m_tree.openElements()->popUntilForeignContentScopeMarker();
            processStartTag(token);
            return;
        }
        /*
        const AtomicString& currentNamespace = adjustedCurrentNode->namespaceURI();
        if (currentNamespace == MathMLNames::mathmlNamespaceURI)
            adjustMathMLAttributes(token);
        if (currentNamespace == SVGNames::svgNamespaceURI) {
            adjustSVGTagNameCase(token);
            adjustSVGAttributes(token);
        }
        adjustForeignAttributes(token);*/
        const AtomicString& currentNamespace = adjustedCurrentNode->namespaceURI();
        m_tree.insertForeignElement(token, currentNamespace);
        // TODO
        // adjustForeignAttributes(token);
        break;
    }
    case HTMLToken::EndTag: {
        /*
        if (adjustedCurrentNode->namespaceURI() == SVGNames::svgNamespaceURI)
            adjustSVGTagNameCase(token);*/

        /*
        if (token->name() == SVGNames::scriptTag && m_tree.currentStackItem()->hasTagName(SVGNames::scriptTag)) {
            if (scriptingContentIsAllowed(m_tree.parserContentPolicy()))
                m_scriptToProcess = m_tree.currentElement();
            m_tree.openElements()->pop();
            return;
        }*/
        if (!m_tree.currentStackItem()->isInHTMLNamespace()) {
            // FIXME: This code just wants an Element* iterator, instead of an ElementRecord*
            HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
            if (!nodeRecord->stackItem()->hasLocalName(token->name()))
                parseError(token);
            while (1) {
                if (nodeRecord->stackItem()->hasLocalName(token->name())) {
                    m_tree.openElements()->popUntilPopped(nodeRecord->element());
                    return;
                }
                nodeRecord = nodeRecord->next();

                if (nodeRecord->stackItem()->isInHTMLNamespace())
                    break;
            }
        }
        // Otherwise, process the token according to the rules given in the section corresponding to the current insertion mode in HTML content.
        processEndTag(token);
        break;
    }
    case HTMLToken::Comment:
        m_tree.insertComment(token);
        break;
    case HTMLToken::Character:
    case HTMLToken::EndOfFile:
        ASSERT_NOT_REACHED();
        break;
    }
}

void HTMLTreeBuilder::finished()
{
    if (isParsingFragment())
        return;

    ASSERT(m_templateInsertionModes.size() == 0);
    ASSERT(m_isAttached);
    // Warning, this may detach the parser. Do not do anything else after this.
    m_tree.finishedParsing();
}

void HTMLTreeBuilder::parseError(AtomicHTMLToken*)
{
}

} // namespace WebCore
