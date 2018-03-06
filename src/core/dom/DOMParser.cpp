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
#ifdef STARFISH_ENABLE_DOMPARSER
#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/CDATASection.h"
#include "core/dom/Comment.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/DocumentType.h"
#include "core/dom/DOMParser.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/dom/Text.h"
#include "core/dom/XMLDocument.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/page/Window.h"

#include <../third_party/rapidxml/rapidxml.hpp>

namespace StarFish {

ScriptBindingInstance* DOMParser::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

static void buildDocumentFromXML(
    rapidxml::xml_node<char>* node, StarFish* sf, Node* parent,
    std::map<std::string, AtomicString> namespaceMap)
{
    Node* newNode;

    if (node->type() == rapidxml::node_type::node_element) {
        AtomicString localName = AtomicString::emptyAtomicString();
        AtomicString namespaceURI = AtomicString::emptyAtomicString();
        std::string localNameStd = node->name();
        auto colonPos = localNameStd.find(':');
        if (std::string::npos != colonPos) {
            localName = AtomicString::createAtomicString(sf, node->name() +
                                                                 colonPos + 1);

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
            newNode =
                HTMLDocument::createHTMLElement(parent->document(), localName);
        } else if (namespaceURI == sf->staticStrings()->m_svgNamespaceURI) {
            newNode =
                SVGDocument::createSVGElement(parent->document(), localName);
        } else {
            newNode = new NamedElement(parent->document(),
                                       QualifiedName(namespaceURI, localName));
        }

        rapidxml::xml_attribute<char>* attr = node->first_attribute();
        while (attr) {
            if (namespaceURI == sf->staticStrings()->m_xhtmlNamespaceURI) {
                newNode->asElement()->setAttribute(
                    QualifiedName(
                        AtomicString::emptyAtomicString(),
                        AtomicString::createAttrAtomicString(sf, attr->name())),
                    String::fromUTF8(attr->value()));
            } else {
                newNode->asElement()->setAttribute(
                    QualifiedName(
                        AtomicString::emptyAtomicString(),
                        AtomicString::createAtomicString(sf, attr->name())),
                    String::fromUTF8(attr->value()));
            }

            std::string attrName = attr->name();
            if (attrName == "xmlns") {
                namespaceMap[std::string("")] =
                    AtomicString::createAttrAtomicString(sf, attr->value());
            } else if (attrName.find("xmlns:") == 0) {
                namespaceMap[attrName.substr(6)] =
                    AtomicString::createAttrAtomicString(sf, attr->value());
            }

            attr = attr->next_attribute();
        }
    } else if (node->type() == rapidxml::node_type::node_comment) {
        newNode =
            parent->document()->createComment(String::fromUTF8(node->value()));
    } else if (node->type() == rapidxml::node_type::node_doctype) {
        newNode = new DocumentType(
            parent->document(), String::fromUTF8(node->name()),
            String::fromUTF8(node->value()), String::emptyString);
    } else if (node->type() == rapidxml::node_type::node_cdata) {
        newNode = parent->document()->createCDATASection(
            String::fromUTF8(node->value()));
    } else if (node->type() == rapidxml::node_type::node_data) {
        newNode =
            parent->document()->createTextNode(String::fromUTF8(node->value()));
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
    if (type->equalsIgnoreCase("text/html")) {
        Document* document =
            new HTMLDocument(window(), window()->scriptBindingInstance(),
                             DOMParser::document()->documentURI(),
                             String::createASCIIString("UTF-8"), false);

        HTMLDocumentBuilder builder(document);
        builder.build(str);

        return document;
    } else if (type->equalsIgnoreCase("text/xml") ||
               type->equalsIgnoreCase("application/xml")) {
        rapidxml::xml_document<char> doc;
        auto utf8String = str->toUTF8NonGCString();
        char* cStr = (char*)utf8String.data();
        try {
            doc.parse<rapidxml::parse_doctype_node |
                      rapidxml::parse_comment_nodes>(cStr);
            XMLDocument* document =
                new XMLDocument(window(), window()->scriptBindingInstance(),
                                DOMParser::document()->documentURI(),
                                String::createASCIIString("UTF-8"), false);

            rapidxml::xml_node<char>* n = doc.first_node();
            while (n) {
                buildDocumentFromXML(n, document->starFish(), document,
                                     std::map<std::string, AtomicString>());
                n = n->next_sibling();
            }

            return document;
        } catch (const rapidxml::parse_error& err) {
            std::string errStr = err.what();
            errStr += " where -> ";
            char buffer[16];
            strncpy(buffer, err.where<char>(), 16);
            errStr += buffer;
            return DOMParser::parseFromString(String::fromUTF8(errStr.data()),
                                              String::fromUTF8("text/html"));
        }
    } else {
        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH_WITH_ENUM, "SupportedType");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "parseFromString", "DOMParser");
        throw new DOMException(document(), DOMException::SCRIPT_TYPE_ERR, msg);
    }
}
}
#endif
