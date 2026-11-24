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
#include "StarfishConfig.h"
#include "Starfish.h"
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

namespace Starfish {

DOMParser::DOMParser(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
{
}

ScriptBindingInstance* DOMParser::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

static void buildDocumentFromXML(
    rapidxml::xml_node<char>* node, Starfish* starfish, Node* parent,
    std::map<std::string, AtomicString> namespaceMap)
{
    Node* newNode;

    if (node->type() == rapidxml::node_type::node_element) {
        AtomicString localName = AtomicString::emptyAtomicString();
        AtomicString namespaceURI = AtomicString::emptyAtomicString();
        AtomicString elemPrefix = AtomicString::emptyAtomicString();
        std::string localNameStd = node->name();

        rapidxml::xml_attribute<char>* attr = node->first_attribute();
        while (attr) {
            std::string attrName = attr->name();
            if (attrName == "xmlns") {
                namespaceMap[std::string("")] =
                    AtomicString::createAtomicString(starfish, attr->value());
            } else if (attrName.find("xmlns:") == 0) {
                namespaceMap[attrName.substr(6)] =
                    AtomicString::createAtomicString(starfish, attr->value());
            }

            attr = attr->next_attribute();
        }

        auto colonPos = localNameStd.find(':');
        if (std::string::npos != colonPos) {
            std::string ns = localNameStd.substr(0, colonPos);
            elemPrefix = AtomicString::createAtomicString(starfish, ns.c_str(),
                                                          ns.size());
            localName = AtomicString::createAtomicString(
                starfish, node->name() + colonPos + 1);

            auto iter = namespaceMap.find(ns);
            if (iter != namespaceMap.end()) {
                namespaceURI = iter->second;
            } else if (ns == "xml") {
                namespaceURI = starfish->staticStrings()->m_xmlNamespaceURI;
            }
        } else {
            localName =
                AtomicString::createAtomicString(starfish, node->name());
            auto iter = namespaceMap.find(std::string(""));
            if (iter != namespaceMap.end()) {
                namespaceURI = iter->second;
            }
        }

        QualifiedName elemQName =
            !elemPrefix.isEmptyAtomicString()
                ? QualifiedName(elemPrefix, namespaceURI, localName)
                : (!namespaceURI.isEmptyAtomicString()
                       ? QualifiedName(namespaceURI, localName)
                       : QualifiedName(localName));

        if (namespaceURI == starfish->staticStrings()->m_xhtmlNamespaceURI) {
            newNode =
                HTMLDocument::createHTMLElement(parent->document(), elemQName);
        } else if (namespaceURI ==
                   starfish->staticStrings()->m_svgNamespaceURI) {
            newNode =
                SVGDocument::createSVGElement(parent->document(), elemQName);
        } else {
            newNode = new NamedElement(parent->document(), elemQName);
        }

        attr = node->first_attribute();
        while (attr) {
            STARFISH_ASSERT(attr->name() != nullptr);
            STARFISH_ASSERT(attr->value() != nullptr);
            std::string attrNameStd = attr->name();
            QualifiedName attrQName;
            if (attrNameStd == "xmlns") {
                attrQName = QualifiedName(
                    starfish->staticStrings()->m_xmlnsNamespaceURI,
                    AtomicString::createAtomicString(starfish, "xmlns", 5));
            } else if (attrNameStd.find("xmlns:") == 0) {
                std::string local = attrNameStd.substr(6);
                attrQName = QualifiedName(
                    AtomicString::createAtomicString(starfish, "xmlns", 5),
                    starfish->staticStrings()->m_xmlnsNamespaceURI,
                    AtomicString::createAtomicString(starfish, local.c_str(),
                                                     local.size()));
            } else {
                auto attrColon = attrNameStd.find(':');
                if (attrColon != std::string::npos) {
                    std::string pfx = attrNameStd.substr(0, attrColon);
                    std::string local = attrNameStd.substr(attrColon + 1);
                    AtomicString attrNs = AtomicString::emptyAtomicString();
                    if (pfx == "xml") {
                        attrNs = starfish->staticStrings()->m_xmlNamespaceURI;
                    } else {
                        auto it = namespaceMap.find(pfx);
                        if (it != namespaceMap.end()) {
                            attrNs = it->second;
                        }
                    }
                    AtomicString pfxAtom = AtomicString::createAtomicString(
                        starfish, pfx.c_str(), pfx.size());
                    AtomicString localAtom = AtomicString::createAtomicString(
                        starfish, local.c_str(), local.size());
                    attrQName =
                        attrNs.isEmptyAtomicString()
                            ? QualifiedName(AtomicString::createAtomicString(
                                  starfish, attr->name()))
                            : QualifiedName(pfxAtom, attrNs, localAtom);
                } else if (namespaceURI ==
                           starfish->staticStrings()->m_xhtmlNamespaceURI) {
                    attrQName =
                        QualifiedName(AtomicString::createAttrAtomicString(
                            starfish, attr->name()));
                } else {
                    attrQName = QualifiedName(AtomicString::createAtomicString(
                        starfish, attr->name()));
                }
            }
            newNode->asElement()->setAttribute(
                attrQName, String::fromUTF8(attr->value(), attr->value_size()));

            attr = attr->next_attribute();
        }
    } else if (node->type() == rapidxml::node_type::node_comment) {
        STARFISH_ASSERT(node->value() != nullptr);
        newNode = parent->document()->createComment(
            String::fromUTF8(node->value(), node->value_size()));
    } else if (node->type() == rapidxml::node_type::node_doctype) {
        STARFISH_ASSERT(node->value() != nullptr);
        STARFISH_ASSERT(node->name() != nullptr);
        newNode = new DocumentType(
            parent->document(),
            String::fromUTF8(node->name(), node->name_size()),
            String::fromUTF8(node->value(), node->value_size()),
            String::emptyString);
    } else if (node->type() == rapidxml::node_type::node_cdata) {
        STARFISH_ASSERT(node->value() != nullptr);
        newNode = parent->document()->createCDATASection(
            String::fromUTF8(node->value(), node->value_size()));
    } else if (node->type() == rapidxml::node_type::node_data) {
        STARFISH_ASSERT(node->value() != nullptr);
        newNode = parent->document()->createTextNode(
            String::fromUTF8(node->value(), node->value_size()));
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    parent->appendChild(newNode);

    rapidxml::xml_node<char>* n = node->first_node();
    while (n) {
        buildDocumentFromXML(n, starfish, newNode, namespaceMap);
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
               type->equalsIgnoreCase("application/xml") ||
               type->equalsIgnoreCase("application/dash+xml") ||
               type->equalsIgnoreCase("image/svg+xml")) {
        rapidxml::xml_document<char> doc;
        auto utf8String = str->toUTF8NonGCString();
        char* cStr = (char*)utf8String.data();
        try {
            doc.parse<rapidxml::parse_doctype_node |
                      rapidxml::parse_comment_nodes |
                      rapidxml::parse_validate_closing_tags>(cStr);
            XMLDocument* document =
                new XMLDocument(window(), window()->scriptBindingInstance(),
                                DOMParser::document()->documentURI(),
                                String::createASCIIString("UTF-8"), false);

            rapidxml::xml_node<char>* n = doc.first_node();
            while (n) {
                buildDocumentFromXML(n, document->starfish(), document,
                                     std::map<std::string, AtomicString>());
                n = n->next_sibling();
            }

            return document;
        } catch (const rapidxml::parse_error& err) {
            std::string errStr = err.what();
            errStr += " where -> ";
            const char* where = err.where<char>();
            errStr.append(where, strnlen(where, 16));
            return DOMParser::parseFromString(
                String::fromUTF8(errStr.data(), errStr.size()),
                String::fromUTF8("text/html"));
        } catch (DOMException* e) {
            return DOMParser::parseFromString(e->message(),
                                              String::fromUTF8("text/html"));
        }
    } else {
        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH_WITH_ENUM, "SupportedType");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "parseFromString", "DOMParser",
                        reason);
        throw new DOMException(document()->executionContext(),
                               DOMException::SCRIPT_TYPE_ERR, msg);
    }
}
} // namespace Starfish
