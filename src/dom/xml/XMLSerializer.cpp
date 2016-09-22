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
#include "XMLSerializer.h"

#include "dom/Element.h"
#include "dom/Document.h"
#include "dom/Comment.h"
#include "dom/DocumentType.h"

#include <../third_party/rapidxml/rapidxml.hpp>
#include <../third_party/rapidxml/rapidxml_print.hpp>

namespace StarFish {

static rapidxml::xml_node<char>* createXMLNodeFromElement(Element* e, rapidxml::xml_document<char>& xmlDocument)
{
    rapidxml::xml_node<char>* xmlNode = xmlDocument.allocate_node(rapidxml::node_type::node_element, e->localName()->utf8Data());

    size_t attributeCount = e->attributeCount();
    for (size_t i = 0; i < attributeCount; i++) {
        rapidxml::xml_attribute<char>* attr = xmlDocument.allocate_attribute(e->getAttributeName(i).localName()->utf8Data(), e->getAttribute(i)->utf8Data());
        xmlNode->append_attribute(attr);
    }

    Node* child = e->firstChild();

    while (child) {
        rapidxml::xml_node<char>* childXMLNode;
        if (child->isElement()) {
            childXMLNode = createXMLNodeFromElement(child->asElement(), xmlDocument);
        } else if (child->isComment()) {
            childXMLNode = xmlDocument.allocate_node(rapidxml::node_type::node_comment, "", child->asCharacterData()->data()->utf8Data());
        } else if (child->isText()) {
            childXMLNode = xmlDocument.allocate_node(rapidxml::node_type::node_data, "", child->asCharacterData()->data()->utf8Data());
        } else if (child->isDocumentType()) {
            childXMLNode = xmlDocument.allocate_node(rapidxml::node_type::node_doctype, child->asDocumentType()->nodeName()->utf8Data());
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
    if (!includeSelf) {
        rapidxml::xml_node<char>* c = root->first_node();
        while (c) {
            rapidxml::print<std::back_insert_iterator<std::basic_string<char> >, char>(std::back_inserter(s), *c, rapidxml::print_no_expand_quot | rapidxml::print_no_expand_amp | rapidxml::print_no_indenting | rapidxml::print_care_script_style);
            c = c->next_sibling();
        }
    } else {
        // rapidxml::print(std::back_inserter(s), *root);
    }

    return String::fromUTF8(s.data(), s.length());
}

}
