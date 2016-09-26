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
#include "DOMParser.h"
#include "dom/DOMException.h"
#include "DocumentFragment.h"
#include "dom/builder/html/HTMLDocumentBuilder.h"
#include "HTMLDocument.h"
#include "DOM.h"

#include <../third_party/rapidxml/rapidxml.hpp>

namespace StarFish {

static void buildDocumentFromXML(rapidxml::xml_node<char>* node, StarFish* sf, Node* parent, std::map<std::string, AtomicString> namespaceMap)
{
    Node* newNode;

    if (node->type() == rapidxml::node_type::node_element) {
        AtomicString localName = AtomicString::emptyAtomicString();
        AtomicString namespaceURI = AtomicString::emptyAtomicString();
        std::string localNameStd = node->name();
        auto colonPos = localNameStd.find(':');
        if (std::string::npos != colonPos) {
            localName = AtomicString::createAtomicString(sf, node->name() + colonPos + 1);

            std::string ns = localNameStd.substr(0, colonPos);
            auto iter = namespaceMap.find(ns);
            if (iter != namespaceMap.end()) {
                namespaceURI = iter->second;
            }
        } else {
            localName = AtomicString::createAtomicString(sf, node->name());
        }
        size_t colon = localName.string()->find(":");
        if (colon != SIZE_MAX) {
            localName.string()->substring(0, colon);
        }

        if (namespaceURI == sf->staticStrings()->m_xhtmlNamespaceURI) {
            newNode = HTMLDocument::createHTMLElement(parent->document(), localName);
        } else {
            newNode = new NamedElement(parent->document(), QualifiedName(namespaceURI, localName));
        }

        rapidxml::xml_attribute<char>* attr = node->first_attribute();
        while (attr) {
            if (namespaceURI == sf->staticStrings()->m_xhtmlNamespaceURI) {
                newNode->asElement()->setAttribute(QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(sf, attr->name())), String::fromUTF8(attr->value()));
            } else {
                newNode->asElement()->setAttribute(QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(sf, attr->name())), String::fromUTF8(attr->value()));
            }

            std::string attrName = attr->name();
            if (attrName == "xmlns") {
                namespaceMap[std::string("")] = AtomicString::createAttrAtomicString(sf, attr->value());
            } else if (attrName.find("xmlns:") == 0) {
                namespaceMap[attrName.substr(6)] = AtomicString::createAttrAtomicString(sf, attr->value());
            }

            attr = attr->next_attribute();
        }
    } else if (node->type() == rapidxml::node_type::node_comment) {
        newNode = parent->document()->createComment(String::fromUTF8(node->value()));
    } else if (node->type() == rapidxml::node_type::node_doctype) {
        newNode = new DocumentType(parent->document(), String::fromUTF8(node->name()), String::fromUTF8(node->value()), String::emptyString);
    } else if (node->type() == rapidxml::node_type::node_cdata || node->type() == rapidxml::node_type::node_data) {
        newNode = parent->document()->createTextNode(String::fromUTF8(node->value()));
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    parent->appendChild(newNode);

    rapidxml::xml_node<char>* n = node->first_node();
    while (n) {
        buildDocumentFromXML(n, sf, newNode, namespaceMap);
        n = n->next_sibling();
    }
}

Document* DOMParser::parseFromString(String* str, String* type)
{
    if (type->equalsWithoutCase("text/html")) {
        Document* document = new HTMLDocument(starFish()->window(), starFish()->window()->scriptBindingInstance(), starFish()->window()->document()->documentURI(), String::createASCIIString("UTF-8"), false);

        HTMLDocumentBuilder builder(document);
        builder.build(str);

        return document;
    } else if (type->equalsWithoutCase("text/xml") || type->equalsWithoutCase("application/xml")) {
        rapidxml::xml_document<char> doc;
        char* cStr = (char*)str->utf8Data();
        try {
            doc.parse<rapidxml::parse_doctype_node | rapidxml::parse_comment_nodes>(cStr);
            Document* document = new Document(starFish()->window(), starFish()->window()->scriptBindingInstance(), starFish()->window()->document()->documentURI(), String::createASCIIString("UTF-8"), false);

            rapidxml::xml_node<char>* n = doc.first_node();
            while (n) {
                buildDocumentFromXML(n, document->window()->starFish(), document, std::map<std::string, AtomicString>());
                n = n->next_sibling();
            }

            return document;
        } catch(const rapidxml::parse_error& err) {
            std::string errStr = err.what();
            errStr += " where -> ";
            char buffer[16];
            strncpy(buffer, err.where<char>(), 16);
            errStr += buffer;
            return DOMParser::parseFromString(String::fromUTF8(errStr.data()), String::fromUTF8("text/html"));
        }
    } else {
        throw new DOMException(starFish()->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "Failed to execute 'parseFromString' on 'DOMParser': The provided value is not a valid enum value of type SupportedType.");
    }
}

}
