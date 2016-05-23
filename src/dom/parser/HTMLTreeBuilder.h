#ifndef __StarFishHTMLTreeBuilder__
#define __StarFishHTMLTreeBuilder__

#include "dom/parser/HTMLConstructionSite.h"
#include "dom/parser/HTMLElementStack.h"
#include "util/String.h"

namespace StarFish {

class AtomicHTMLToken;
class Document;
class DocumentFragment;
class Element;
class HTMLToken;
class HTMLDocument;
class Node;
class HTMLParser;

// TextPosition structure specifies coordinates within an text resource. It is used mostly
// for saving script source position.
class TextPosition {
public:
    TextPosition(OrdinalNumber line, OrdinalNumber column)
        : m_line(line)
        , m_column(column)
    {
    }
    TextPosition() { }
    bool operator==(const TextPosition& other) { return m_line == other.m_line && m_column == other.m_column; }
    bool operator!=(const TextPosition& other) { return !((*this) == other); }

    // A 'minimum' value of position, used as a default value.
    static TextPosition minimumPosition() { return TextPosition(OrdinalNumber::first(), OrdinalNumber::first()); }

    // A value with line value less than a minimum; used as an impossible position.
    static TextPosition belowRangePosition() { return TextPosition(OrdinalNumber::beforeFirst(), OrdinalNumber::beforeFirst()); }

    // A value corresponding to a position with given offset within text having the specified line ending offsets.
    // WTF_EXPORT static TextPosition fromOffsetAndLineEndings(unsigned, const Vector<unsigned>&);

    OrdinalNumber m_line;
    OrdinalNumber m_column;
};

class HTMLTreeBuilder : public gc {
public:
    HTMLTreeBuilder(HTMLParser*, Document*, bool reportErrors);
    HTMLTreeBuilder(HTMLParser*, DocumentFragment*, Element* contextElement);

    const HTMLElementStack* openElements() const { return m_tree.openElements(); }

    bool isParsingFragment() const { return !!m_fragmentContext.fragment(); }
    bool isParsingTemplateContents() const { return m_tree.openElements()->hasTemplateInHTMLScope(); }
    bool isParsingFragmentOrTemplateContents() const { return isParsingFragment() || isParsingTemplateContents(); }

    void detach();

    void constructTree(AtomicHTMLToken*);

    bool hasParserBlockingScript() const { return !!m_scriptToProcess; }
    // Must be called to take the parser-blocking script before calling the parser again.
    Element* takeScriptToProcess(TextPosition& scriptStartPosition);
    Element* lookScriptToProcess();

    // Done, close any open tags, etc.
    void finished();

    // Synchronously empty any queues, possibly creating more DOM nodes.
    void flush() { m_tree.flush(); }

    void setShouldSkipLeadingNewline(bool shouldSkip) { m_shouldSkipLeadingNewline = shouldSkip; }

private:
    class CharacterTokenBuffer;
    // Represents HTML5 "insertion mode"
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#insertion-mode
    enum InsertionMode {
        InitialMode,
        BeforeHTMLMode,
        BeforeHeadMode,
        InHeadMode,
        InHeadNoscriptMode,
        AfterHeadMode,
        TemplateContentsMode,
        InBodyMode,
        TextMode,
        InTableMode,
        InTableTextMode,
        InCaptionMode,
        InColumnGroupMode,
        InTableBodyMode,
        InRowMode,
        InCellMode,
        InSelectMode,
        InSelectInTableMode,
        AfterBodyMode,
        InFramesetMode,
        AfterFramesetMode,
        AfterAfterBodyMode,
        AfterAfterFramesetMode,
    };


    void processToken(AtomicHTMLToken*);

    void processDoctypeToken(AtomicHTMLToken*);
    void processStartTag(AtomicHTMLToken*);
    void processEndTag(AtomicHTMLToken*);
    void processComment(AtomicHTMLToken*);
    void processCharacter(AtomicHTMLToken*);
    void processEndOfFile(AtomicHTMLToken*);

    bool processStartTagForInHead(AtomicHTMLToken*);
    void processStartTagForInBody(AtomicHTMLToken*);
    void processStartTagForInTable(AtomicHTMLToken*);
    void processEndTagForInBody(AtomicHTMLToken*);
    void processEndTagForInTable(AtomicHTMLToken*);
    void processEndTagForInTableBody(AtomicHTMLToken*);
    void processEndTagForInRow(AtomicHTMLToken*);
    void processEndTagForInCell(AtomicHTMLToken*);

    void processIsindexStartTagForInBody(AtomicHTMLToken*);
    void processHtmlStartTagForInBody(AtomicHTMLToken*);
    bool processBodyEndTagForInBody(AtomicHTMLToken*);
    bool processTableEndTagForInTable();
    bool processCaptionEndTagForInCaption();
    bool processColgroupEndTagForInColumnGroup();
    bool processTrEndTagForInRow();
    // FIXME: This function should be inlined into its one call site or it
    // needs to assert which tokens it can be called with.
    void processAnyOtherEndTagForInBody(AtomicHTMLToken*);

    void processCharacterBuffer(CharacterTokenBuffer&);
    inline void processCharacterBufferForInBody(CharacterTokenBuffer&);

    void processFakeStartTag(const QualifiedName&, const AttributeVector& attributes = AttributeVector());
    void processFakeEndTag(const QualifiedName&);
    void processFakeEndTag(const AtomicString&);
    void processFakePEndTagIfPInButtonScope();

    void processGenericRCDATAStartTag(AtomicHTMLToken*);
    void processGenericRawTextStartTag(AtomicHTMLToken*);
    void processScriptStartTag(AtomicHTMLToken*);

    // Default processing for the different insertion modes.
    void defaultForInitial();
    void defaultForBeforeHTML();
    void defaultForBeforeHead();
    void defaultForInHead();
    void defaultForInHeadNoscript();
    void defaultForAfterHead();
    void defaultForInTableText();

    inline HTMLStackItem* adjustedCurrentStackItem() const;
    inline bool shouldProcessTokenInForeignContent(AtomicHTMLToken*);
    void processTokenInForeignContent(AtomicHTMLToken*);

    AttributeVector attributesForIsindexInput(AtomicHTMLToken*);

    void callTheAdoptionAgency(AtomicHTMLToken*);

    void closeTheCell();

    template <bool shouldClose(StaticStrings*, const HTMLStackItem*)>
    void processCloseWhenNestedTag(AtomicHTMLToken*);

    void parseError(AtomicHTMLToken*);

    InsertionMode insertionMode() const { return m_insertionMode; }
    void setInsertionMode(InsertionMode mode) { m_insertionMode = mode; }

    void resetInsertionModeAppropriately();

    void processTemplateStartTag(AtomicHTMLToken*);
    bool processTemplateEndTag(AtomicHTMLToken*);
    bool processEndOfFileForInTemplateContents(AtomicHTMLToken*);

    class FragmentParsingContext : public gc {
    public:
        FragmentParsingContext();
        FragmentParsingContext(DocumentFragment*, Element* contextElement);
        ~FragmentParsingContext();

        DocumentFragment* fragment() const { return m_fragment; }
        Element* contextElement() const { STARFISH_ASSERT(m_fragment); return m_contextElementStackItem->element(); }
        HTMLStackItem* contextElementStackItem() const { STARFISH_ASSERT(m_fragment); return m_contextElementStackItem; }

    private:
        DocumentFragment* m_fragment;
        HTMLStackItem* m_contextElementStackItem;
    };

    bool m_framesetOk;
#ifndef NDEBUG
    bool m_isAttached;
#endif
    FragmentParsingContext m_fragmentContext;
    HTMLConstructionSite m_tree;

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#insertion-mode
    InsertionMode m_insertionMode;

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#original-insertion-mode
    InsertionMode m_originalInsertionMode;

    std::vector<InsertionMode, gc_allocator<InsertionMode>> m_templateInsertionModes;

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#pending-table-character-tokens
    UTF32String m_pendingTableCharacters;

    bool m_shouldSkipLeadingNewline;

    // We access parser because HTML5 spec requires that we be able to change the state of the tokenizer
    // from within parser actions. We also need it to track the current position.
    HTMLParser* m_parser;

    Element* m_scriptToProcess; // <script> tag which needs processing before resuming the parser.
    TextPosition m_scriptToProcessStartPosition; // Starting line number of the script tag needing processing.
};

}

#endif
