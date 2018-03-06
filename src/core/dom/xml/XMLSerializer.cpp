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

#include "StarFishConfig.h"
#include "XMLSerializer.h"

#include "core/dom/Element.h"
#include "core/dom/Comment.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentType.h"

#include <../third_party/rapidxml/rapidxml.hpp>
#include <../third_party/rapidxml/rapidxml_print.hpp>

namespace StarFish {

static bool isSelfClosingTag(char* str)
{
    // https://www.w3.org/TR/html5/syntax.html#void-elements
    // area, base, br, col, embed, hr, img, input, keygen, link, meta, param,
    // source, track, wbr

    if (strncmp(str, "area", 4) == 0) {
        return true;
    } else if (strncmp(str, "base", 4) == 0) {
        return true;
    } else if (strncmp(str, "br", 2) == 0) {
        return true;
    } else if (strncmp(str, "col", 3) == 0) {
        return true;
    } else if (strncmp(str, "embed", 5) == 0) {
        return true;
    } else if (strncmp(str, "hr", 2) == 0) {
        return true;
    } else if (strncmp(str, "img", 3) == 0) {
        return true;
    } else if (strncmp(str, "input", 5) == 0) {
        return true;
    } else if (strncmp(str, "keygen", 6) == 0) {
        return true;
    } else if (strncmp(str, "link", 4) == 0) {
        return true;
    } else if (strncmp(str, "meta", 4) == 0) {
        return true;
    } else if (strncmp(str, "param", 5) == 0) {
        return true;
    } else if (strncmp(str, "source", 6) == 0) {
        return true;
    } else if (strncmp(str, "track", 5) == 0) {
        return true;
    } else if (strncmp(str, "wbr", 3) == 0) {
        return true;
    }
    return false;
}

// https://developer.mozilla.org/en-US/docs/Web/API/Element/innerHTML
// If a <div>, <span>, or <noembed> node has a child text node that includes the
// characters (&), (<), or (>), innerHTML returns these characters as &amp, &lt
// and &gt respectively.
static bool hasSpecialChar(char* str, char* parentTagName)
{
    if (!str)
        return false;

    if (!parentTagName)
        return false;

    if ((strncmp(parentTagName, "div", 3) == 0) ||
        (strncmp(parentTagName, "span", 4) == 0) ||
        (strncmp(parentTagName, "noembed", 7) == 0)) {
        char* current = str;
        while (*current != '\0') {
            if (*current == '&' || *current == '<' || *current == '>') {
                return true;
            }
            current++;
        }
    }
    return false;
}

static rapidxml::xml_node<char>* createXMLNodeFromElement(
    Element* e, rapidxml::xml_document<char>& xmlDocument)
{
    auto utf8Data = e->localName()->toUTF8NonGCString();
    char* allocateName = xmlDocument.allocate_string(utf8Data.data());
    rapidxml::node_type nodeType = rapidxml::node_type::node_element;
    if (isSelfClosingTag(allocateName)) {
        nodeType = rapidxml::node_type::node_element_self_close;
    }
    rapidxml::xml_node<char>* xmlNode =
        xmlDocument.allocate_node(nodeType, allocateName);

    size_t attributeCount = e->attributeCount();
    for (size_t i = 0; i < attributeCount; i++) {
        auto utf8DataName =
            e->getAssuredAttributeName(i).localName()->toUTF8NonGCString();
        char* allocateCountName =
            xmlDocument.allocate_string(utf8DataName.data());
        auto utf8DataValue = e->getAssuredAttribute(i)->toUTF8NonGCString();
        char* allocateCountValue =
            xmlDocument.allocate_string(utf8DataValue.data());
        rapidxml::xml_attribute<char>* attr = xmlDocument.allocate_attribute(
            allocateCountName, allocateCountValue);
        xmlNode->append_attribute(attr);
    }

    Node* child = e->firstChild();

    while (child) {
        rapidxml::xml_node<char>* childXMLNode;
        if (child->isElement()) {
            childXMLNode =
                createXMLNodeFromElement(child->asElement(), xmlDocument);
        } else if (child->isComment()) {
            auto utf8Data =
                child->asCharacterData()->data()->toUTF8NonGCString();
            char* allocateValue = xmlDocument.allocate_string(utf8Data.data());
            childXMLNode = xmlDocument.allocate_node(
                rapidxml::node_type::node_comment, "", allocateValue);
        } else if (child->isText()) {
            auto utf8Data =
                child->asCharacterData()->data()->toUTF8NonGCString();
            char* allocateValue = xmlDocument.allocate_string(utf8Data.data());
            if (hasSpecialChar(allocateValue, allocateName)) {
                childXMLNode = xmlDocument.allocate_node(
                    rapidxml::node_type::node_data_specialChar, "",
                    allocateValue);
            } else {
                childXMLNode = xmlDocument.allocate_node(
                    rapidxml::node_type::node_data, "", allocateValue);
            }
        } else if (child->isDocumentType()) {
            auto utf8Data =
                child->asDocumentType()->nodeName()->toUTF8NonGCString();
            char* allocateName = xmlDocument.allocate_string(utf8Data.data());
            childXMLNode = xmlDocument.allocate_node(
                rapidxml::node_type::node_doctype, allocateName);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        xmlNode->append_node(childXMLNode);
        child = child->nextSibling();
    }

    return xmlNode;
}

String* XMLSerializer::serializeToXML(Element* e, bool includeSelf)
{
    rapidxml::xml_document<char> doc;
    rapidxml::xml_node<char>* root = createXMLNodeFromElement(e, doc);

    std::string s;
    int xmlFlag = rapidxml::print_no_expand_quot |
                  rapidxml::print_no_expand_lt_gt |
                  rapidxml::print_no_expand_amp | rapidxml::print_no_indenting |
                  rapidxml::print_care_script_style;
    if (!includeSelf) {
        rapidxml::xml_node<char>* c = root->first_node();
        while (c) {
            rapidxml::print<std::back_insert_iterator<std::basic_string<char>>,
                            char>(std::back_inserter(s), *c, xmlFlag);
            c = c->next_sibling();
        }
    } else {
        rapidxml::print(std::back_inserter(s), *root, xmlFlag);
    }

    return String::fromUTF8(s.data(), s.length());
}
}
