#include "StarFishConfig.h"
#include "HTMLConstructionSite.h"

#include "dom/Comment.h"
#include "dom/DocumentType.h"
#include "dom/Element.h"
#include "dom/Text.h"
#include "dom/DocumentFragment.h"

#include "dom/HTMLScriptElement.h"
#include "dom/HTMLHtmlElement.h"
#include "dom/parser/AtomicHTMLToken.h"
#include "dom/parser/HTMLParserIdioms.h"
#include "dom/parser/HTMLStackItem.h"
#include "dom/parser/HTMLToken.h"
#include <limits>

namespace StarFish {

static const unsigned maximumHTMLParserDOMTreeDepth = 512;

static inline void setAttributes(Element* element, AtomicHTMLToken* token)
{
    /*
    if (!scriptingContentIsAllowed(parserContentPolicy))
        element->stripScriptingAttributes(token->attributes());*/
    for (size_t i = 0; i < token->attributes().size(); i ++) {
        element->setAttribute(token->attributes()[i].name(), token->attributes()[i].value());
    }
}

static bool hasImpliedEndTag(const HTMLStackItem* item)
{
    StaticStrings* s = item->node()->document()->window()->starFish()->staticStrings();
    return item->hasTagName(s->m_ddLocalName)
        || item->hasTagName(s->m_dtLocalName)
        || item->hasTagName(s->m_liLocalName)
        || item->hasTagName(s->m_optionLocalName)
        || item->hasTagName(s->m_optgroupLocalName)
        || item->hasTagName(s->m_pLocalName)
        || item->hasTagName(s->m_rpLocalName)
        || item->hasTagName(s->m_rtLocalName);
}

static bool shouldUseLengthLimit(const Node* node)
{
    StaticStrings* s = const_cast<Node*>(node)->document()->window()->starFish()->staticStrings();
    return !const_cast<Node*>(node)->localName()->equals(s->m_scriptLocalName.string())
        && !const_cast<Node*>(node)->localName()->equals(s->m_styleLocalName.string());
        // && !node->hasTagName(SVGNames::scriptTag);
}

static unsigned textLengthLimitForContainer(const Node* node)
{
    return shouldUseLengthLimit(node) ? String::defaultLengthLimit : std::numeric_limits<unsigned>::max();
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

static inline void insert(HTMLConstructionSiteTask& task)
{
    /*
    if (task.parent->hasTagName(templateTag))
        task.parent = toHTMLTemplateElement(task.parent.get())->content();
     */
    if (Node* parent = task.child->parentNode()) {
        // parent->parserRemoveChild(*task.child);
        parent->removeChild(task.child);
    }

    if (task.nextChild) {
        task.parent->parserInsertBefore(task.child, task.nextChild);
    } else {
        task.parent->parserAppendChild(task.child);
    }
}

static inline void executeInsertTask(HTMLConstructionSiteTask& task)
{
    STARFISH_ASSERT(task.operation == HTMLConstructionSiteTask::Insert);

    insert(task);

    if (task.child->isElement()) {
        Element* child = task.child->asElement();
        child->beginParsing();
        if (task.selfClosing)
            child->finishParsing();
    }
}

static inline void executeInsertTextTask(HTMLConstructionSiteTask& task)
{
    STARFISH_ASSERT(task.operation == HTMLConstructionSiteTask::InsertText);
    STARFISH_ASSERT(task.child->isText());

    // Merge text nodes into previous ones if possible:
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#insert-a-character
    Text* newText = task.child->asText();
    Node* previousChild = task.nextChild ? task.nextChild->previousSibling() : task.parent->lastChild();
    if (previousChild && previousChild->isText()) {
        Text* previousText = previousChild->asText();
        unsigned lengthLimit = textLengthLimitForContainer(task.parent);
        if (previousText->length() + newText->length() < lengthLimit) {
            previousText->parserSetData(previousText->data()->concat(newText->data()));
            return;
        }
    }

    insert(task);
}

static inline void executeReparentTask(HTMLConstructionSiteTask& task)
{
    STARFISH_ASSERT(task.operation == HTMLConstructionSiteTask::Reparent);

    if (Node* parent = task.child->parentNode()) {
        parent->parserRemoveChild(task.child);
    }

    task.parent->parserAppendChild(task.child);
}

static inline void executeInsertAlreadyParsedChildTask(HTMLConstructionSiteTask& task)
{
    STARFISH_ASSERT(task.operation == HTMLConstructionSiteTask::InsertAlreadyParsedChild);

    insert(task);
}

static inline void executeTakeAllChildrenTask(HTMLConstructionSiteTask& task)
{
    STARFISH_ASSERT(task.operation == HTMLConstructionSiteTask::TakeAllChildren);
    task.parent->parserTakeAllChildrenFrom(task.oldParent());
}

void HTMLConstructionSite::executeTask(HTMLConstructionSiteTask& task)
{
    STARFISH_ASSERT(m_taskQueue.size() == 0);
    if (task.operation == HTMLConstructionSiteTask::Insert)
        return executeInsertTask(task);

    if (task.operation == HTMLConstructionSiteTask::InsertText)
        return executeInsertTextTask(task);

    // All the cases below this point are only used by the adoption agency.

    if (task.operation == HTMLConstructionSiteTask::InsertAlreadyParsedChild)
        return executeInsertAlreadyParsedChildTask(task);

    if (task.operation == HTMLConstructionSiteTask::Reparent)
        return executeReparentTask(task);

    if (task.operation == HTMLConstructionSiteTask::TakeAllChildren)
        return executeTakeAllChildrenTask(task);

    STARFISH_ASSERT_NOT_REACHED();
}

// This is only needed for TextDocuments where we might have text nodes
// approaching the default length limit (~64k) and we don't want to
// break a text node in the middle of a combining character.
/*
static unsigned findBreakIndexBetween(const StringBuilder& string, unsigned currentPosition, unsigned proposedBreakIndex)
{
    ASSERT(currentPosition < proposedBreakIndex);
    ASSERT(proposedBreakIndex <= string.length());
    // The end of the string is always a valid break.
    if (proposedBreakIndex == string.length())
        return proposedBreakIndex;

    // Latin-1 does not have breakable boundaries. If we ever moved to a differnet 8-bit encoding this could be wrong.
    // if (string.is8Bit())
    //     return proposedBreakIndex;

    const UChar* breakSearchCharacters = string.characters16() + currentPosition;
    // We need at least two characters look-ahead to account for UTF-16 surrogates, but can't search off the end of the buffer!
    unsigned breakSearchLength = std::min(proposedBreakIndex - currentPosition + 2, string.length() - currentPosition);
    NonSharedCharacterBreakIterator it(breakSearchCharacters, breakSearchLength);

    if (it.isBreak(proposedBreakIndex - currentPosition))
        return proposedBreakIndex;

    int adjustedBreakIndexInSubstring = it.preceding(proposedBreakIndex - currentPosition);
    if (adjustedBreakIndexInSubstring > 0)
        return currentPosition + adjustedBreakIndexInSubstring;
    // We failed to find a breakable point, let the caller figure out what to do.
    return 0;
}*/

static String* atomizeIfAllWhitespace(String* string, WhitespaceMode whitespaceMode)
{
    // Strings composed entirely of whitespace are likely to be repeated.
    // Turn them into AtomicString so we share a single string for each.
    if (whitespaceMode == AllWhitespace || (whitespaceMode == WhitespaceUnknown && isAllWhitespace(string))) {
        return string;
        // TODO
        // return AtomicString(string).string();
    }
    return string;
}

void HTMLConstructionSite::flushPendingText()
{
    if (m_pendingText.isEmpty())
        return;

    PendingText pendingText;
    // Hold onto the current pending text on the stack so that queueTask doesn't recurse infinitely.
    m_pendingText.swap(pendingText);
    ASSERT(m_pendingText.isEmpty());

    // Splitting text nodes into smaller chunks contradicts HTML5 spec, but is necessary
    // for performance, see: https://bugs.webkit.org/show_bug.cgi?id=55898
    // unsigned lengthLimit = textLengthLimitForContainer(pendingText.parent);

    unsigned currentPosition = 0;
    const UTF32String& string = pendingText.stringBuilder;
    while (currentPosition < string.length()) {
        // unsigned proposedBreakIndex = std::min(currentPosition + lengthLimit, (unsigned)string.length());
        // unsigned breakIndex = findBreakIndexBetween(string, currentPosition, proposedBreakIndex);
        unsigned breakIndex = string.length();
        ASSERT(breakIndex <= string.length());
        String* substring = new StringDataUTF32(string.substr(currentPosition, breakIndex - currentPosition));
        substring = atomizeIfAllWhitespace(substring, pendingText.whitespaceMode);

        HTMLConstructionSiteTask task(HTMLConstructionSiteTask::InsertText);
        task.parent = pendingText.parent;
        task.nextChild = pendingText.nextChild;
        task.child = new Text(task.parent->document(), substring);
        queueTask(task);

        STARFISH_ASSERT(breakIndex > currentPosition);
        STARFISH_ASSERT(breakIndex - currentPosition == substring->length());
        STARFISH_ASSERT((task.child)->asCharacterData()->data()->length() == substring->length());
        currentPosition = breakIndex;
    }
}

void HTMLConstructionSite::queueTask(const HTMLConstructionSiteTask& task)
{
    flushPendingText();
    STARFISH_ASSERT(m_pendingText.isEmpty());
    m_taskQueue.push_back(task);
}

void HTMLConstructionSite::attachLater(Node* parent, Node* prpChild, bool selfClosing)
{
    // ASSERT(scriptingContentIsAllowed(m_parserContentPolicy) || !prpChild.get()->isElementNode() || !toScriptLoaderIfPossible(toElement(prpChild.get())));
    // ASSERT(pluginContentIsAllowed(m_parserContentPolicy) || !prpChild->isPluginElement());

    HTMLConstructionSiteTask task(HTMLConstructionSiteTask::Insert);
    task.parent = parent;
    task.child = prpChild;
    task.selfClosing = selfClosing;

    if (shouldFosterParent()) {
        fosterParent(task.child);
        return;
    }

    // Add as a sibling of the parent if we have reached the maximum depth allowed.
    if (m_openElements.stackDepth() > maximumHTMLParserDOMTreeDepth && task.parent->parentNode())
        task.parent = task.parent->parentNode();

    STARFISH_ASSERT(task.parent);
    queueTask(task);
}

void HTMLConstructionSite::executeQueuedTasks()
{
    // This has no affect on pendingText, and we may have pendingText
    // remaining after executing all other queued tasks.
    const size_t size = m_taskQueue.size();
    if (!size)
        return;

    // Copy the task queue into a local variable in case executeTask
    // re-enters the parser.
    TaskQueue queue;
    queue.swap(m_taskQueue);

    for (size_t i = 0; i < size; ++i)
        executeTask(queue[i]);

    // We might be detached now.
}

HTMLConstructionSite::HTMLConstructionSite(Document* document)
    : m_document(document)
    , m_attachmentRoot(document)
    , m_isParsingFragment(false)
    , m_redirectAttachToFosterParent(false)
    , m_inQuirksMode(document->inQuirksMode())
{
    m_form = nullptr;
    m_head = nullptr;
    // ASSERT(m_document->isHTMLDocument() || m_document->isXHTMLDocument());
}

HTMLConstructionSite::HTMLConstructionSite(DocumentFragment* fragment)
    : m_document(fragment->document())
    , m_attachmentRoot(fragment)
    , m_isParsingFragment(true)
    , m_redirectAttachToFosterParent(false)
    , m_inQuirksMode(fragment->document()->inQuirksMode())
{
    m_form = nullptr;
    m_head = nullptr;
}


HTMLConstructionSite::~HTMLConstructionSite()
{
    // Depending on why we're being destroyed it might be OK
    // to forget queued tasks, but currently we don't expect to.
    STARFISH_ASSERT(m_taskQueue.size() == 0);
    // Currently we assume that text will never be the last token in the
    // document and that we'll always queue some additional task to cause it to flush.
    STARFISH_ASSERT(m_pendingText.isEmpty());
}

void HTMLConstructionSite::detach()
{
    // FIXME: We'd like to ASSERT here that we're canceling and not just discarding
    // text that really should have made it into the DOM earlier, but there
    // doesn't seem to be a nice way to do that.
    m_pendingText.discard();
    m_document = 0;
    m_attachmentRoot = 0;
}

void HTMLConstructionSite::setForm(HTMLFormElement* form)
{
    // This method should only be needed for HTMLTreeBuilder in the fragment case.
    STARFISH_ASSERT(!m_form);
    m_form = form;
}

HTMLFormElement* HTMLConstructionSite::takeForm()
{
    auto ret = m_form;
    m_form = nullptr;
    return ret;
}

void HTMLConstructionSite::dispatchDocumentElementAvailableIfNeeded()
{
    /*
    ASSERT(m_document);
    if (m_document->frame() && !m_isParsingFragment)
        m_document->frame()->loader().dispatchDocumentElementAvailable();
    */
}

void HTMLConstructionSite::insertHTMLHtmlStartTagBeforeHTML(AtomicHTMLToken* token)
{
    // TODO
    STARFISH_ASSERT(m_document);
    HTMLHtmlElement* element = new HTMLHtmlElement(m_document);
    setAttributes(element, token);
    attachLater(m_attachmentRoot, element);
    m_openElements.pushHTMLHtmlElement(new HTMLStackItem(element, token));

    executeQueuedTasks();
    // element->insertedByParser();
    dispatchDocumentElementAvailableIfNeeded();
}

void HTMLConstructionSite::mergeAttributesFromTokenIntoElement(AtomicHTMLToken* token, Element* element)
{
    if (token->attributes().size() == 0)
        return;

    for (unsigned i = 0; i < token->attributes().size(); ++i) {
        const Attribute& tokenAttribute = token->attributes().at(i);
        // if (!element->elementData() || !element->getAttributeItem(tokenAttribute.name()))
        //     element->setAttribute(tokenAttribute.name(), tokenAttribute.value());
        element->setAttribute(tokenAttribute.name(), tokenAttribute.value());
    }
}

void HTMLConstructionSite::insertHTMLHtmlStartTagInBody(AtomicHTMLToken* token)
{
    // Fragments do not have a root HTML element, so any additional HTML elements
    // encountered during fragment parsing should be ignored.
    if (m_isParsingFragment)
        return;

    mergeAttributesFromTokenIntoElement(token, m_openElements.htmlElement());
}

void HTMLConstructionSite::insertHTMLBodyStartTagInBody(AtomicHTMLToken* token)
{
    mergeAttributesFromTokenIntoElement(token, m_openElements.bodyElement());
}

void HTMLConstructionSite::setDefaultCompatibilityMode()
{
    if (m_isParsingFragment)
        return;
    // if (m_document->isSrcdocDocument())
    //    return;
    setCompatibilityMode(Document::QuirksMode);
}

void HTMLConstructionSite::setCompatibilityMode(Document::CompatibilityMode mode)
{
    m_inQuirksMode = (mode == Document::QuirksMode);
    m_document->setCompatibilityMode(mode);
}

void HTMLConstructionSite::setCompatibilityModeFromDoctype(String* name, String* publicId, String* systemId)
{
    // There are three possible compatibility modes:
    // Quirks - quirks mode emulates WinIE and NS4. CSS parsing is also relaxed in this mode, e.g., unit types can
    // be omitted from numbers.
    // Limited Quirks - This mode is identical to no-quirks mode except for its treatment of line-height in the inline box model.
    // No Quirks - no quirks apply. Web pages will obey the specifications to the letter.

    // Check for Quirks Mode.
    if (name->equals("html")
        || publicId->startsWith("+//Silmaril//dtd html Pro v0r11 19970101//", false)
        || publicId->startsWith("-//AdvaSoft Ltd//DTD HTML 3.0 asWedit + extensions//", false)
        || publicId->startsWith("-//AS//DTD HTML 3.0 asWedit + extensions//", false)
        || publicId->startsWith("-//IETF//DTD HTML 2.0 Level 1//", false)
        || publicId->startsWith("-//IETF//DTD HTML 2.0 Level 2//", false)
        || publicId->startsWith("-//IETF//DTD HTML 2.0 Strict Level 1//", false)
        || publicId->startsWith("-//IETF//DTD HTML 2.0 Strict Level 2//", false)
        || publicId->startsWith("-//IETF//DTD HTML 2.0 Strict//", false)
        || publicId->startsWith("-//IETF//DTD HTML 2.0//", false)
        || publicId->startsWith("-//IETF//DTD HTML 2.1E//", false)
        || publicId->startsWith("-//IETF//DTD HTML 3.0//", false)
        || publicId->startsWith("-//IETF//DTD HTML 3.2 Final//", false)
        || publicId->startsWith("-//IETF//DTD HTML 3.2//", false)
        || publicId->startsWith("-//IETF//DTD HTML 3//", false)
        || publicId->startsWith("-//IETF//DTD HTML Level 0//", false)
        || publicId->startsWith("-//IETF//DTD HTML Level 1//", false)
        || publicId->startsWith("-//IETF//DTD HTML Level 2//", false)
        || publicId->startsWith("-//IETF//DTD HTML Level 3//", false)
        || publicId->startsWith("-//IETF//DTD HTML Strict Level 0//", false)
        || publicId->startsWith("-//IETF//DTD HTML Strict Level 1//", false)
        || publicId->startsWith("-//IETF//DTD HTML Strict Level 2//", false)
        || publicId->startsWith("-//IETF//DTD HTML Strict Level 3//", false)
        || publicId->startsWith("-//IETF//DTD HTML Strict//", false)
        || publicId->startsWith("-//IETF//DTD HTML//", false)
        || publicId->startsWith("-//Metrius//DTD Metrius Presentational//", false)
        || publicId->startsWith("-//Microsoft//DTD Internet Explorer 2.0 HTML Strict//", false)
        || publicId->startsWith("-//Microsoft//DTD Internet Explorer 2.0 HTML//", false)
        || publicId->startsWith("-//Microsoft//DTD Internet Explorer 2.0 Tables//", false)
        || publicId->startsWith("-//Microsoft//DTD Internet Explorer 3.0 HTML Strict//", false)
        || publicId->startsWith("-//Microsoft//DTD Internet Explorer 3.0 HTML//", false)
        || publicId->startsWith("-//Microsoft//DTD Internet Explorer 3.0 Tables//", false)
        || publicId->startsWith("-//Netscape Comm. Corp.//DTD HTML//", false)
        || publicId->startsWith("-//Netscape Comm. Corp.//DTD Strict HTML//", false)
        || publicId->startsWith("-//O'Reilly and Associates//DTD HTML 2.0//", false)
        || publicId->startsWith("-//O'Reilly and Associates//DTD HTML Extended 1.0//", false)
        || publicId->startsWith("-//O'Reilly and Associates//DTD HTML Extended Relaxed 1.0//", false)
        || publicId->startsWith("-//SoftQuad Software//DTD HoTMetaL PRO 6.0::19990601::extensions to HTML 4.0//", false)
        || publicId->startsWith("-//SoftQuad//DTD HoTMetaL PRO 4.0::19971010::extensions to HTML 4.0//", false)
        || publicId->startsWith("-//Spyglass//DTD HTML 2.0 Extended//", false)
        || publicId->startsWith("-//SQ//DTD HTML 2.0 HoTMetaL + extensions//", false)
        || publicId->startsWith("-//Sun Microsystems Corp.//DTD HotJava HTML//", false)
        || publicId->startsWith("-//Sun Microsystems Corp.//DTD HotJava Strict HTML//", false)
        || publicId->startsWith("-//W3C//DTD HTML 3 1995-03-24//", false)
        || publicId->startsWith("-//W3C//DTD HTML 3.2 Draft//", false)
        || publicId->startsWith("-//W3C//DTD HTML 3.2 Final//", false)
        || publicId->startsWith("-//W3C//DTD HTML 3.2//", false)
        || publicId->startsWith("-//W3C//DTD HTML 3.2S Draft//", false)
        || publicId->startsWith("-//W3C//DTD HTML 4.0 Frameset//", false)
        || publicId->startsWith("-//W3C//DTD HTML 4.0 Transitional//", false)
        || publicId->startsWith("-//W3C//DTD HTML Experimental 19960712//", false)
        || publicId->startsWith("-//W3C//DTD HTML Experimental 970421//", false)
        || publicId->startsWith("-//W3C//DTD W3 HTML//", false)
        || publicId->startsWith("-//W3O//DTD W3 HTML 3.0//", false)
        || publicId->equalsWithoutCase(String::createASCIIString("-//W3O//DTD W3 HTML Strict 3.0//EN//"))
        || publicId->startsWith("-//WebTechs//DTD Mozilla HTML 2.0//", false)
        || publicId->startsWith("-//WebTechs//DTD Mozilla HTML//", false)
        || publicId->equalsWithoutCase(String::createASCIIString("-/W3C/DTD HTML 4.0 Transitional/EN"))
        || publicId->equalsWithoutCase(String::createASCIIString("HTML"))
        || systemId->equalsWithoutCase(String::createASCIIString("http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd"))
        || ((systemId->length() == 0) && publicId->startsWith("-//W3C//DTD HTML 4.01 Frameset//", false))
        || ((systemId->length() == 0) && publicId->startsWith("-//W3C//DTD HTML 4.01 Transitional//", false))) {
        setCompatibilityMode(Document::QuirksMode);
        return;
    }

    // Check for Limited Quirks Mode.
    if (publicId->startsWith("-//W3C//DTD XHTML 1.0 Frameset//", false)
        || publicId->startsWith("-//W3C//DTD XHTML 1.0 Transitional//", false)
        || (!(systemId->length() == 0) && publicId->startsWith("-//W3C//DTD HTML 4.01 Frameset//", false))
        || (!(systemId->length() == 0) && publicId->startsWith("-//W3C//DTD HTML 4.01 Transitional//", false))) {
        setCompatibilityMode(Document::LimitedQuirksMode);
        return;
    }

    // Otherwise we are No Quirks Mode.
    setCompatibilityMode(Document::NoQuirksMode);
}

void HTMLConstructionSite::processEndOfFile()
{
    STARFISH_ASSERT(currentNode());
    flush();
    openElements()->popAll();
}

void HTMLConstructionSite::finishedParsing()
{
    // We shouldn't have any queued tasks but we might have pending text which we need to promote to tasks and execute.
    STARFISH_ASSERT(m_taskQueue.size() == 0);
    flush();
    // m_document->finishedParsing();
}

void HTMLConstructionSite::insertDoctype(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(token->type() == HTMLToken::DOCTYPE);

    String* publicId = token->publicIdentifier();
    String* systemId = token->systemIdentifier();
    DocumentType* doctype = new DocumentType(m_document, token->name(), publicId, systemId);
    attachLater(m_attachmentRoot, doctype);

    // DOCTYPE nodes are only processed when parsing fragments w/o contextElements, which
    // never occurs. However, if we ever chose to support such, this code is subtly wrong,
    // because context-less fragments can determine their own quirks mode, and thus change
    // parsing rules (like <p> inside <table>). For now we ASSERT that we never hit this code
    // in a fragment, as changing the owning document's compatibility mode would be wrong.
    ASSERT(!m_isParsingFragment);
    if (m_isParsingFragment)
        return;

    if (token->forceQuirks())
        setCompatibilityMode(Document::QuirksMode);
    else {
        setCompatibilityModeFromDoctype(token->name(), publicId, systemId);
    }
}

void HTMLConstructionSite::insertComment(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(token->type() == HTMLToken::Comment);
    attachLater(currentNode(), new Comment(&ownerDocumentForCurrentNode(), token->comment()));
}

void HTMLConstructionSite::insertCommentOnDocument(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(token->type() == HTMLToken::Comment);
    STARFISH_ASSERT(m_document);
    attachLater(m_attachmentRoot, new Comment(m_document, token->comment()));
}

void HTMLConstructionSite::insertCommentOnHTMLHtmlElement(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(token->type() == HTMLToken::Comment);
    Node* parent = m_openElements.rootNode();
    attachLater(parent, new Comment(parent->document(), token->comment()));
}

void HTMLConstructionSite::insertHTMLHeadElement(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(!shouldFosterParent());
    m_head = new HTMLStackItem(createHTMLElement(token), token);
    attachLater(currentNode(), m_head->element());
    m_openElements.pushHTMLHeadElement(m_head);
}

void HTMLConstructionSite::insertHTMLBodyElement(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(!shouldFosterParent());
    Element* body = createHTMLElement(token);
    attachLater(currentNode(), body);
    m_openElements.pushHTMLBodyElement(new HTMLStackItem(body, token));
    // if (LocalFrame* frame = m_document->frame())
    //    frame->loader().client()->dispatchWillInsertBody();
}

void HTMLConstructionSite::insertHTMLFormElement(AtomicHTMLToken* token, bool isDemoted)
{
    Element* element = createHTMLElement(token);
    // STARFISH_ASSERT(element->hasTagName(formTag));
    // m_form = static_pointer_cast<HTMLFormElement>(element.release());
    m_form = element->asHTMLElement();
    // m_form->setDemoted(isDemoted);
    attachLater(currentNode(), m_form);
    m_openElements.push(new HTMLStackItem(m_form, token));
}

void HTMLConstructionSite::insertHTMLElement(AtomicHTMLToken* token)
{
    Element* element = createHTMLElement(token);
    attachLater(currentNode(), element);
    m_openElements.push(new HTMLStackItem(element, token));
}

void HTMLConstructionSite::insertSelfClosingHTMLElement(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    // Normally HTMLElementStack is responsible for calling finishParsingChildren,
    // but self-closing elements are never in the element stack so the stack
    // doesn't get a chance to tell them that we're done parsing their children.
    attachLater(currentNode(), createHTMLElement(token), true);
    // FIXME: Do we want to acknowledge the token's self-closing flag?
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#acknowledge-self-closing-flag
}

void HTMLConstructionSite::insertFormattingElement(AtomicHTMLToken* token)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#the-stack-of-open-elements
    // Possible active formatting elements include:
    // a, b, big, code, em, font, i, nobr, s, small, strike, strong, tt, and u.
    insertHTMLElement(token);
    m_activeFormattingElements.append(currentElementRecord()->stackItem());
}

void HTMLConstructionSite::insertScriptElement(AtomicHTMLToken* token)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/scripting-1.html#already-started
    // http://html5.org/specs/dom-parsing.html#dom-range-createcontextualfragment
    // For createContextualFragment, the specifications say to mark it parser-inserted and already-started and later unmark them.
    // However, we short circuit that logic to avoid the subtree traversal to find script elements since scripts can never see
    // those flags or effects thereof.
    // const bool parserInserted = m_parserContentPolicy != AllowScriptingContentAndDoNotMarkAlreadyStarted;
    // const bool parserInserted = true;
    // const bool alreadyStarted = m_isParsingFragment && parserInserted;
    // HTMLScriptElement* element = new HTMLScriptElement(ownerDocumentForCurrentNode(), parserInserted, alreadyStarted);
    HTMLScriptElement* element = new HTMLScriptElement(&ownerDocumentForCurrentNode());
    setAttributes(element, token);
    // if (scriptingContentIsAllowed(m_parserContentPolicy))
    if (true)
        attachLater(currentNode(), element);
    m_openElements.push(new HTMLStackItem(element, token));
}

void HTMLConstructionSite::insertForeignElement(AtomicHTMLToken* token)
{
    STARFISH_ASSERT(token->type() == HTMLToken::StartTag);
    // parseError when xmlns or xmlns:xlink are wrong.

    Element* element = createElement(token);

    /*if (scriptingContentIsAllowed(m_parserContentPolicy) || !toScriptLoaderIfPossible(element.get()))
        attachLater(currentNode(), element, token->selfClosing());*/
    if (true)
        attachLater(currentNode(), element, token->selfClosing());

    if (!token->selfClosing())
        m_openElements.push(new HTMLStackItem(element, token));
}

void HTMLConstructionSite::insertTextNode(String* string, WhitespaceMode whitespaceMode)
{
    HTMLConstructionSiteTask dummyTask(HTMLConstructionSiteTask::Insert);
    dummyTask.parent = currentNode();

    if (shouldFosterParent())
        findFosterSite(dummyTask);

    // FIXME: This probably doesn't need to be done both here and in insert(Task).
    // if (dummyTask.parent->hasTagName(templateTag))
    //    dummyTask.parent = toHTMLTemplateElement(dummyTask.parent.get())->content();

    // Unclear when parent != case occurs. Somehow we insert text into two separate nodes while processing the same Token.
    // The nextChild != dummy.nextChild case occurs whenever foster parenting happened and we hit a new text node "<table>a</table>b"
    // In either case we have to flush the pending text into the task queue before making more.
    if (!m_pendingText.isEmpty() && (m_pendingText.parent != dummyTask.parent ||  m_pendingText.nextChild != dummyTask.nextChild))
        flushPendingText();
    m_pendingText.append(dummyTask.parent, dummyTask.nextChild, string, whitespaceMode);
}

void HTMLConstructionSite::reparent(HTMLElementStack::ElementRecord* newParent, HTMLElementStack::ElementRecord* child)
{
    HTMLConstructionSiteTask task(HTMLConstructionSiteTask::Reparent);
    task.parent = newParent->node();
    task.child = child->node();
    queueTask(task);
}

void HTMLConstructionSite::reparent(HTMLElementStack::ElementRecord* newParent, HTMLStackItem* child)
{
    HTMLConstructionSiteTask task(HTMLConstructionSiteTask::Reparent);
    task.parent = newParent->node();
    task.child = child->node();
    queueTask(task);
}

void HTMLConstructionSite::insertAlreadyParsedChild(HTMLStackItem* newParent, HTMLElementStack::ElementRecord* child)
{
    if (newParent->causesFosterParenting()) {
        fosterParent(child->node());
        return;
    }

    HTMLConstructionSiteTask task(HTMLConstructionSiteTask::InsertAlreadyParsedChild);
    task.parent = newParent->node();
    task.child = child->node();
    queueTask(task);
}

void HTMLConstructionSite::takeAllChildren(HTMLStackItem* newParent, HTMLElementStack::ElementRecord* oldParent)
{
    HTMLConstructionSiteTask task(HTMLConstructionSiteTask::TakeAllChildren);
    task.parent = newParent->node();
    task.child = oldParent->node();
    queueTask(task);
}

Element* HTMLConstructionSite::createElement(AtomicHTMLToken* token)
{
    QualifiedName tagName = QualifiedName::fromString(m_document->window()->starFish(), token->name());
    Element* element = ownerDocumentForCurrentNode().createElement(tagName);
    setAttributes(element, token);
    return element;
}

inline Document& HTMLConstructionSite::ownerDocumentForCurrentNode()
{
    /*if (currentNode()->hasTagName(templateTag))
        return toHTMLTemplateElement(currentElement())->content()->document();
        */
    return *currentNode()->document();
}

Element* HTMLConstructionSite::createHTMLElement(AtomicHTMLToken* token)
{
    // Document& document = ownerDocumentForCurrentNode();
    // Only associate the element with the current form if we're creating the new element
    // in a document with a browsing context (rather than in <template> contents).
    // HTMLFormElement* form = document.frame() ? m_form.get() : 0;
    // FIXME: This can't use HTMLConstructionSite::createElement because we
    // have to pass the current form element. We should rework form association
    // to occur after construction to allow better code sharing here.
    // Element* element = HTMLElementFactory::createHTMLElement(token->name(), document, form, true);
    QualifiedName tagName = QualifiedName::fromString(m_document->window()->starFish(), token->name());
    Element* element = m_document->createElement(tagName);
    setAttributes(element, token);
    ASSERT(element->isHTMLElement());
    return element;
}

HTMLStackItem* HTMLConstructionSite::createElementFromSavedToken(HTMLStackItem* item)
{
    Element* element;
    // NOTE: Moving from item -> token -> item copies the Attribute vector twice!
    AtomicHTMLToken fakeToken(item->node()->document()->window()->starFish(), HTMLToken::StartTag, item->localName(), item->attributes());
    /*if (item->namespaceURI() == HTMLNames::xhtmlNamespaceURI)
        element = createHTMLElement(&fakeToken);
    else
        element = createElement(&fakeToken, item->namespaceURI());
        */
    element = createElement(&fakeToken);
    return new HTMLStackItem(element, &fakeToken);
}

bool HTMLConstructionSite::indexOfFirstUnopenFormattingElement(unsigned& firstUnopenElementIndex) const
{
    if (m_activeFormattingElements.isEmpty())
        return false;
    unsigned index = m_activeFormattingElements.size();
    do {
        --index;
        const HTMLFormattingElementList::Entry& entry = m_activeFormattingElements.at(index);
        if (entry.isMarker() || m_openElements.contains(entry.element())) {
            firstUnopenElementIndex = index + 1;
            return firstUnopenElementIndex < m_activeFormattingElements.size();
        }
    } while (index);
    firstUnopenElementIndex = index;
    return true;
}

void HTMLConstructionSite::reconstructTheActiveFormattingElements()
{
    unsigned firstUnopenElementIndex;
    if (!indexOfFirstUnopenFormattingElement(firstUnopenElementIndex))
        return;

    unsigned unopenEntryIndex = firstUnopenElementIndex;
    ASSERT(unopenEntryIndex < m_activeFormattingElements.size());
    for (; unopenEntryIndex < m_activeFormattingElements.size(); ++unopenEntryIndex) {
        HTMLFormattingElementList::Entry& unopenedEntry = m_activeFormattingElements.at(unopenEntryIndex);
        HTMLStackItem* reconstructed = createElementFromSavedToken(unopenedEntry.stackItem());
        attachLater(currentNode(), reconstructed->node());
        m_openElements.push(reconstructed);
        unopenedEntry.replaceElement(reconstructed);
    }
}

void HTMLConstructionSite::generateImpliedEndTagsWithExclusion(const QualifiedName& tagName)
{
    while (hasImpliedEndTag(currentStackItem()) && !currentStackItem()->matchesHTMLTag(tagName))
        m_openElements.pop();
}

void HTMLConstructionSite::generateImpliedEndTags()
{
    while (hasImpliedEndTag(currentStackItem()))
        m_openElements.pop();
}

bool HTMLConstructionSite::inQuirksMode()
{
    return m_inQuirksMode;
}

void HTMLConstructionSite::findFosterSite(HTMLConstructionSiteTask& task)
{
    // When a node is to be foster parented, the last template element with no table element is below it in the stack of open elements is the foster parent element (NOT the template's parent!)
    auto s = m_document->window()->starFish()->staticStrings();
    HTMLElementStack::ElementRecord* lastTemplateElement = m_openElements.topmost(s->m_templateLocalName);
    if (lastTemplateElement && !m_openElements.inTableScope(s->m_tableLocalName)) {
        task.parent = lastTemplateElement->element();
        return;
    }

    HTMLElementStack::ElementRecord* lastTableElementRecord = m_openElements.topmost(s->m_tableLocalName);
    if (lastTableElementRecord) {
        Element* lastTableElement = lastTableElementRecord->element();
        Node* parent;
        if (lastTableElementRecord->next()->stackItem()->hasTagName(s->m_templateLocalName))
            parent = lastTableElementRecord->next()->element();
        else
            parent = lastTableElement->parentNode();

        // When parsing HTML fragments, we skip step 4.2 ("Let root be a new html element with no attributes") for efficiency,
        // and instead use the DocumentFragment as a root node. So we must treat the root node (DocumentFragment) as if it is a html element here.
        if (parent && (parent->isElement() || (m_isParsingFragment && parent == m_openElements.rootNode()))) {
            task.parent = parent;
            task.nextChild = lastTableElement;
            return;
        }
        task.parent = lastTableElementRecord->next()->element();
        return;
    }
    // Fragment case
    task.parent = m_openElements.rootNode(); // DocumentFragment
}

bool HTMLConstructionSite::shouldFosterParent() const
{
    return m_redirectAttachToFosterParent
        && currentStackItem()->isElementNode()
        && currentStackItem()->causesFosterParenting();
}

void HTMLConstructionSite::fosterParent(Node* node)
{
    HTMLConstructionSiteTask task(HTMLConstructionSiteTask::Insert);
    findFosterSite(task);
    task.child = node;
    STARFISH_ASSERT(task.parent);
    queueTask(task);
}

}
