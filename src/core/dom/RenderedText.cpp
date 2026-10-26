/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/RenderedText.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLBRElement.h"
#include "core/dom/Text.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"

namespace Starfish {

// Which child nodes an element lets through. HTML gives select, optgroup
// and option boxes whose children are only their optgroup/option children,
// whatever the UA actually renders.
enum class ChildFilter {
    Any,
    OptGroupOrOption,
    Option,
};

class RenderedTextCollector {
public:
    String* run(Element* root)
    {
        collectElementChildren(root);
        return m_out.finalize();
    }

private:
    // CSS white-space processing state of the inline formatting context the
    // walk is in. Soft wraps never change the result (collapsible spaces at
    // a line end are only removed at the end of the block or before a br),
    // so the state is only reset at block container boundaries and br.
    struct LineState {
        bool atLineStart = true;   // collapsible spaces here are removed
        bool pendingSpace = false; // a collapsed space waiting for content
    };

    // The children an element's box lets through: select, optgroup and
    // option only their option-kind children (option also its text), and a
    // replaced element whose content is fallback (media, iframe, object)
    // none.
    void collectElementChildren(Element* element)
    {
        if (element->isHTMLSelectElement()) {
            collectChildren(element, ChildFilter::OptGroupOrOption, false);
        } else if (element->isHTMLOptGroupElement()) {
            collectChildren(element, ChildFilter::Option, false);
        } else if (element->isHTMLOptionElement()) {
            collectChildren(element, ChildFilter::Any, true);
        } else if (!element->isHTMLMediaElement() &&
                   !element->isHTMLIFrameElement() &&
                   !element->isHTMLObjectElement()) {
            collectChildren(element, ChildFilter::Any, false);
        }
    }

    void collectChildren(Node* parent, ChildFilter filter, bool textRendered)
    {
        for (Node* child = parent->firstChild(); child;
             child = child->nextSibling()) {
            if (child->isText()) {
                if (filter == ChildFilter::Any) {
                    visitText(child->asText(), textRendered);
                }
            } else if (child->isElement()) {
                Element* element = child->asElement();
                if (filter == ChildFilter::Any ||
                    element->isHTMLOptionElement() ||
                    (filter == ChildFilter::OptGroupOrOption &&
                     element->isHTMLOptGroupElement())) {
                    visitElement(element);
                }
            }
        }
    }

    void visitText(Text* text, bool renderedByRule)
    {
        Frame* frame = text->frame();
        if (!frame && !renderedByRule) {
            return;
        }
        ComputedStyle* style = text->style();
        if (!style) {
            style = text->parentNode()->style();
        }
        if (style && style->visibility() != VisibleVisibilityValue) {
            return;
        }
        // The frame's text already has text-transform applied.
        String* str = (frame && frame->isFrameText())
                          ? frame->asFrameText()->text()
                          : text->data();
        appendText(str, style ? style->whiteSpace() : NormalWhiteSpaceValue);
    }

    void visitElement(Element* element)
    {
        ComputedStyle* style = element->style();
        Frame* frame = element->frame();
        bool visible = !style || style->visibility() == VisibleVisibilityValue;
        bool hasBox = style && style->display() != NoneDisplayValue;

        // select/optgroup/option act as boxes whenever display is not none.
        if (element->isHTMLSelectElement()) {
            if (hasBox) {
                collectElementChildren(element);
            }
            return;
        }
        if (element->isHTMLOptGroupElement() ||
            element->isHTMLOptionElement()) {
            if (hasBox) {
                visitBlock(element, 1, visible);
            }
            return;
        }

        if (!frame) {
            // display:contents generates no box of its own but its children
            // are rendered; anything else without a box is not rendered and
            // contributes only what its children do.
            collectElementChildren(element);
            return;
        }

        if (frame->isFrameLineBreak() || element->isHTMLBRElement()) {
            endLine();
            if (visible) {
                appendOutput('\n');
            }
            return;
        }
        if (frame->isFrameTableCellBox()) {
            visitContainer(element);
            if (visible && !isLastCellInRow(frame)) {
                appendOutput('\t');
            }
            return;
        }
        if (frame->isFrameTableRowBox()) {
            visitContainer(element);
            if (visible && !isLastRowInTable(frame)) {
                appendOutput('\n');
            }
            return;
        }
        if (element->isHTMLParagraphElement()) {
            visitBlock(element, 2, visible);
            return;
        }
        if (frame->isFrameTableCaptionBox() || isBlockLevel(frame)) {
            visitBlock(element, 1, visible);
            return;
        }
        if (frame->isAtomicInlineLevel()) {
            // An atomic inline keeps the collapsible spaces on both sides of
            // it and lays its own content out as a block container.
            flushPendingSpace();
            visitContainer(element);
            m_line.atLineStart = false;
            m_line.pendingSpace = false;
            return;
        }
        collectElementChildren(element);
    }

    // A block-level box: a required line break count on both sides, and a
    // block container for its content.
    void visitBlock(Element* element, unsigned breakCount, bool visible)
    {
        endLine();
        if (visible) {
            requireLineBreaks(breakCount);
        }
        LineState saved = m_line;
        m_line = LineState();
        collectElementChildren(element);
        m_line = saved;
        endLine();
        if (visible) {
            requireLineBreaks(breakCount);
        }
    }

    // A block container that is not block-level (cell, row, atomic inline):
    // its content starts and ends its own lines.
    void visitContainer(Element* element)
    {
        LineState saved = m_line;
        m_line = LineState();
        collectElementChildren(element);
        m_line = saved;
    }

    static bool isBlockLevel(Frame* frame)
    {
        // Floats and absolutely positioned boxes are blockified, as are
        // flex and grid items.
        if (frame->isFloating() || frame->isAbsolutePositioned()) {
            return true;
        }
        switch (frame->style()->display()) {
        case BlockDisplayValue:
        case ListItemDisplayValue:
        case TableDisplayValue:
        case FlexDisplayValue:
        case GridDisplayValue:
        case BoxDisplayValue:
            return true;
        default:
            break;
        }
        Frame* parent = frame->parent();
        return parent &&
               (parent->isFrameFlexibleBox() || parent->isFrameGridBox());
    }

    static bool isLastCellInRow(Frame* cell)
    {
        for (Frame* f = cell->next(); f; f = f->next()) {
            if (f->isFrameTableCellBox()) {
                return false;
            }
        }
        return true;
    }

    static bool hasRowBox(Frame* frame)
    {
        if (frame->isFrameTableRowBox()) {
            return true;
        }
        if (frame->isFrameTableBox() || frame->isFrameTableCellBox()) {
            return false;
        }
        for (Frame* f = frame->firstChild(); f; f = f->next()) {
            if (hasRowBox(f)) {
                return true;
            }
        }
        return false;
    }

    static bool isLastRowInTable(Frame* row)
    {
        for (Frame* f = row; f && !f->isFrameTableBox(); f = f->parent()) {
            for (Frame* s = f->next(); s; s = s->next()) {
                if (hasRowBox(s)) {
                    return false;
                }
            }
        }
        return true;
    }

    // CSS Text white-space processing, restricted to what survives into
    // the result: collapsing, segment breaks and removal at line ends.
    void appendText(String* str, WhiteSpaceValue whiteSpace)
    {
        bool collapseSpaces = !(whiteSpace & PreWhiteSpaceValue);
        bool collapseNewlines = whiteSpace & NoWrapWhiteSpaceValue;
        size_t length = str->length();
        for (size_t i = 0; i < length; i++) {
            char32_t c = str->charAt(i);
            bool isNewline = c == '\n' || c == '\r';
            if (isNewline && !collapseNewlines) {
                // A preserved segment break; collapsible spaces around it
                // are removed.
                m_line.pendingSpace = false;
                appendOutput('\n');
                m_line.atLineStart = true;
            } else if (isNewline || c == ' ' || c == '\t' || c == '\f') {
                if (!collapseSpaces) {
                    flushPendingSpace();
                    appendOutput(c);
                    m_line.atLineStart = false;
                } else if (!m_line.atLineStart) {
                    m_line.pendingSpace = true;
                }
            } else {
                flushPendingSpace();
                appendOutput(c);
                m_line.atLineStart = false;
            }
        }
    }

    void flushPendingSpace()
    {
        if (m_line.pendingSpace) {
            m_line.pendingSpace = false;
            appendOutput(' ');
        }
    }

    void endLine()
    {
        m_line.pendingSpace = false;
        m_line.atLineStart = true;
    }

    // Required line break counts between two strings merge into the
    // largest one; those before the first string or after the last are
    // dropped.
    void requireLineBreaks(unsigned count)
    {
        if (m_hasOutput && count > m_pendingLineBreaks) {
            m_pendingLineBreaks = count;
        }
    }

    void appendOutput(char32_t c)
    {
        for (; m_pendingLineBreaks; m_pendingLineBreaks--) {
            m_out.appendChar('\n');
        }
        m_out.appendChar(c);
        m_hasOutput = true;
    }

    StringBuilder m_out;
    LineState m_line;
    unsigned m_pendingLineBreaks = 0;
    bool m_hasOutput = false;
};

String* collectRenderedText(Element* element)
{
    RenderedTextCollector collector;
    return collector.run(element);
}

DocumentFragment* renderedTextFragment(Document* document, String* input)
{
    DocumentFragment* fragment = new DocumentFragment(document);
    const QualifiedName& brTagName =
        document->starfish()->staticStrings()->m_brTagName;
    size_t length = input->length();
    size_t position = 0;
    while (position < length) {
        size_t start = position;
        while (position < length) {
            char32_t c = input->charAt(position);
            if (c == '\n' || c == '\r') {
                break;
            }
            position++;
        }
        if (position > start) {
            fragment->appendChild(
                new Text(document, input->substring(start, position - start)));
        }
        while (position < length) {
            char32_t c = input->charAt(position);
            if (c != '\n' && c != '\r') {
                break;
            }
            // A CR LF pair is one line break.
            if (c == '\r' && position + 1 < length &&
                input->charAt(position + 1) == '\n') {
                position++;
            }
            position++;
            fragment->appendChild(new HTMLBRElement(document, brTagName));
        }
    }
    return fragment;
}

} // namespace Starfish
