/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAXName__)
#define __StarfishCDPAXName__

#include <string>
#include <vector>

namespace Starfish {

class Document;
class Element;
class Node;

// Collapse runs of ASCII whitespace to one space and trim both ends, the
// way accname says to normalize a text alternative.
std::string axCollapseWhitespace(const std::string& text);

// The text a text node contributes. Inner whitespace is collapsed, but one
// leading and one trailing space survive, because they separate this text
// from the text beside it.
std::string axTextForNode(Node* node);

// The name of the document node, which is the title of the page.
std::string axDocumentName(Document* document);

// The accessible name of an element, following accname: aria-labelledby,
// then aria-label, then the native label of a form control, then the
// element's own content for roles that allow it, then title.
std::string axNameForElement(Element* element, const std::string& tag,
                             const std::string& role);

// An element that contributed text to a name, with the text it gave.
struct AXRelatedText {
    Element* element;
    std::string text;
};

// One place a name can come from, in the form the protocol reports it. An
// absent text means the source gave nothing. A source is superseded when
// an earlier source already gave the name.
struct AXNameSource {
    AXNameSource(bool supersededBy, const char* attributeName,
                 const char* sourceType)
        : attribute(attributeName)
        , type(sourceType)
        , superseded(supersededBy)
    {
    }

    std::string attribute;
    std::string nativeSource;
    const char* type;
    bool hasText = false;
    std::string text;
    bool hasAttributeValue = false;
    std::string attributeValue;
    bool superseded;
    bool invalid = false;
    std::vector<AXRelatedText> related;
};

// Every source Chromium looks at to name an element, in its order. This is
// the list the protocol reports as name.sources.
std::vector<AXNameSource> axNameSources(Element* element,
                                        const std::string& role);

// The value of aria-labelledby, falling back to the misspelled
// aria-labeledby that Chromium also accepts.
std::string axLabelledByAttribute(Element* element);

// The text the elements named by a list of id references contribute.
std::string axTextFromIdRefs(Element* element, const std::string& idRefs);

// One attribute of an element, collapsed. Empty when the attribute is
// absent.
std::string axAttribute(Element* element, const char* name);
bool axHasAttribute(Element* element, const char* name);

// Whether the element carries any aria-* attribute. An author who wrote one
// meant the element to be reported, even when it has no role of its own.
bool axHasAnyAriaAttribute(Element* element);

} // namespace Starfish

#endif
