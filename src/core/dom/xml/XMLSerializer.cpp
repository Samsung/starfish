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
#include "XMLSerializer.h"

#include "core/dom/Element.h"
#include "core/dom/Comment.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentType.h"
#include "core/dom/ProcessingInstruction.h"

#include <unordered_map>
#include <vector>

#include <../third_party/rapidxml/rapidxml.hpp>
#include <../third_party/rapidxml/rapidxml_print.hpp>

namespace Starfish {

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
    Node* e, rapidxml::xml_document<char>& xmlDocument)
{
    UTF8StringDataNonGCStd utf8Data;
    if (e->isElement()) {
        utf8Data = e->localName()->toUTF8NonGCString();
    } else {
        utf8Data = e->nodeName()->toUTF8NonGCString();
    }
    char* allocateName = xmlDocument.allocate_string(utf8Data.data());
    rapidxml::node_type nodeType = rapidxml::node_type::node_element;
    if (isSelfClosingTag(allocateName)) {
        nodeType = rapidxml::node_type::node_element_self_close;
    }
    rapidxml::xml_node<char>* xmlNode =
        xmlDocument.allocate_node(nodeType, allocateName);

    if (e->isElement()) {
        Element* ele = e->asElement();
        size_t attributeCount = ele->attributeCount();
        for (size_t i = 0; i < attributeCount; i++) {
            auto utf8DataName = ele->getAssuredAttributeName(i)
                                    .localName()
                                    ->toUTF8NonGCString();
            char* allocateCountName =
                xmlDocument.allocate_string(utf8DataName.data());
            auto utf8DataValue =
                ele->getAssuredAttribute(i)->toUTF8NonGCString();
            char* allocateCountValue =
                xmlDocument.allocate_string(utf8DataValue.data());
            rapidxml::xml_attribute<char>* attr =
                xmlDocument.allocate_attribute(allocateCountName,
                                               allocateCountValue);
            xmlNode->append_attribute(attr);
        }
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
        } else if (child->isCDATASection() && child->document()->typeIsXML()) {
            // XML serialization keeps a CDATA section as one
            // (https://w3c.github.io/DOM-Parsing/#xml-serializing-a-cdatasection-node);
            // the HTML fragment serialization used for HTML documents treats
            // it as a Text node and escapes it instead (falls through below).
            auto utf8Data =
                child->asCharacterData()->data()->toUTF8NonGCString();
            char* allocateValue = xmlDocument.allocate_string(utf8Data.data());
            childXMLNode = xmlDocument.allocate_node(
                rapidxml::node_type::node_cdata, "", allocateValue);
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
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }

        xmlNode->append_node(childXMLNode);
        child = child->nextSibling();
    }

    return xmlNode;
}

String* XMLSerializer::serializeToXML(Node* e, bool includeSelf)
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

// https://w3c.github.io/DOM-Parsing/#xml-serialization
static const char kXMLNamespace[] = "http://www.w3.org/XML/1998/namespace";
static const char kXMLNSNamespace[] = "http://www.w3.org/2000/xmlns/";
static const char kHTMLNamespace[] = "http://www.w3.org/1999/xhtml";

using NamespacePrefixMap =
    std::unordered_map<std::string, std::vector<std::string>>;

static bool isHTMLVoidElementName(String* localName)
{
    return localName->equals("area") || localName->equals("base") ||
           localName->equals("basefont") || localName->equals("bgsound") ||
           localName->equals("br") || localName->equals("col") ||
           localName->equals("embed") || localName->equals("frame") ||
           localName->equals("hr") || localName->equals("img") ||
           localName->equals("input") || localName->equals("keygen") ||
           localName->equals("link") || localName->equals("meta") ||
           localName->equals("param") || localName->equals("source") ||
           localName->equals("track") || localName->equals("wbr");
}

static void appendEscapedText(StringBuilder& out, String* text)
{
    auto data = text->bufferAccessData();
    for (size_t i = 0; i < data.length; i++) {
        char32_t ch = data.charAt(i);
        if (ch == '&') {
            out.appendString("&amp;");
        } else if (ch == '<') {
            out.appendString("&lt;");
        } else if (ch == '>') {
            out.appendString("&gt;");
        } else {
            out.appendChar(ch);
        }
    }
}

static void appendEscapedAttrValue(StringBuilder& out, String* value)
{
    auto data = value->bufferAccessData();
    for (size_t i = 0; i < data.length; i++) {
        char32_t ch = data.charAt(i);
        if (ch == '&') {
            out.appendString("&amp;");
        } else if (ch == '"') {
            out.appendString("&quot;");
        } else if (ch == '<') {
            out.appendString("&lt;");
        } else if (ch == '>') {
            out.appendString("&gt;");
        } else if (ch == '\t') {
            out.appendString("&#9;");
        } else if (ch == '\n') {
            out.appendString("&#xA;");
        } else if (ch == '\r') {
            out.appendString("&#xD;");
        } else {
            out.appendChar(ch);
        }
    }
}

static void appendEscapedAttrValue(StringBuilder& out, const std::string& value)
{
    for (size_t i = 0; i < value.size(); i++) {
        unsigned char ch = static_cast<unsigned char>(value[i]);
        if (ch == '&') {
            out.appendString("&amp;");
        } else if (ch == '"') {
            out.appendString("&quot;");
        } else if (ch == '<') {
            out.appendString("&lt;");
        } else if (ch == '>') {
            out.appendString("&gt;");
        } else if (ch == '\t') {
            out.appendString("&#9;");
        } else if (ch == '\n') {
            out.appendString("&#xA;");
        } else if (ch == '\r') {
            out.appendString("&#xD;");
        } else if (ch < 0x80) {
            out.appendChar(static_cast<char>(ch));
        } else {
            String* utf8 = String::fromUTF8(value.data() + i, value.size() - i);
            appendEscapedAttrValue(out, utf8);
            return;
        }
    }
}

static void appendUTF8(StringBuilder& out, const std::string& str)
{
    for (size_t i = 0; i < str.size(); i++) {
        unsigned char ch = static_cast<unsigned char>(str[i]);
        if (ch < 0x80) {
            out.appendChar(static_cast<char>(ch));
        } else {
            out.appendString(String::fromUTF8(str.data() + i, str.size() - i));
            return;
        }
    }
}

struct ScopedPrefixMapGuard {
    NamespacePrefixMap& map;
    std::vector<std::string> addedNamespaces;

    explicit ScopedPrefixMapGuard(NamespacePrefixMap& m)
        : map(m)
    {
    }

    void add(const std::string& ns, const std::string& prefix)
    {
        map[ns].push_back(prefix);
        addedNamespaces.push_back(ns);
    }

    ~ScopedPrefixMapGuard()
    {
        for (auto it = addedNamespaces.rbegin(); it != addedNamespaces.rend();
             ++it) {
            auto mapIt = map.find(*it);
            if (mapIt != map.end() && !mapIt->second.empty()) {
                mapIt->second.pop_back();
                if (mapIt->second.empty()) {
                    map.erase(mapIt);
                }
            }
        }
    }
};

using LocalPrefixList = std::vector<std::pair<std::string, std::string>>;

static const std::string* findLocalPrefix(const LocalPrefixList& list,
                                          const std::string& prefix)
{
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        if (it->first == prefix) {
            return &it->second;
        }
    }
    return nullptr;
}

static bool hasPrefixInMap(const NamespacePrefixMap& map, const std::string& ns,
                           const std::string& prefix)
{
    auto it = map.find(ns);
    if (it == map.end()) {
        return false;
    }
    for (const auto& p : it->second) {
        if (p == prefix) {
            return true;
        }
    }
    return false;
}

static bool retrievePreferredPrefix(const NamespacePrefixMap& map,
                                    const std::string& ns, std::string& out)
{
    auto it = map.find(ns);
    if (it == map.end() || it->second.empty()) {
        return false;
    }
    out = it->second.back();
    return true;
}

static std::string generatePrefix(ScopedPrefixMapGuard& scope,
                                  const std::string& newNs,
                                  unsigned& prefixIndex)
{
    std::string generated = "ns" + std::to_string(prefixIndex++);
    scope.add(newNs, generated);
    return generated;
}

static std::string getNamespaceString(const QualifiedName& qname,
                                      bool& hasNamespace)
{
    if (qname.hasNamespaceURI()) {
        String* ns = qname.namespaceURI().getValue().string();
        if (ns->length() > 0) {
            hasNamespace = true;
            return ns->toUTF8NonGCString();
        }
    }
    hasNamespace = false;
    return std::string();
}

static void serializeXMLNode(Node* node, bool hasContextNamespace,
                             const std::string& contextNamespace,
                             NamespacePrefixMap& prefixMap,
                             unsigned& prefixIndex, StringBuilder& markup);

static void serializeXMLElement(Element* element, bool hasContextNamespace,
                                const std::string& contextNamespace,
                                NamespacePrefixMap& map, unsigned& prefixIndex,
                                StringBuilder& markup)
{
    // https://w3c.github.io/DOM-Parsing/#xml-serializing-an-element-node
    // Mutate the shared NamespacePrefixMap in place with an RAII undo guard so
    // elements without namespace declarations incur zero map allocations.
    ScopedPrefixMapGuard prefixScope(map);
    LocalPrefixList localPrefixes;

    bool hasDefaultNamespaceAttrValue = false;
    std::string defaultNamespaceAttrValue;

    // Step 4: Record the namespace information for element.
    size_t attrCount = element->attributeCount();
    for (size_t i = 0; i < attrCount; i++) {
        QualifiedName attrName = element->getAssuredAttributeName(i);
        bool attrHasNs = false;
        std::string attrNs = getNamespaceString(attrName, attrHasNs);
        if (attrHasNs && attrNs == kXMLNSNamespace) {
            std::string attrValue =
                element->getAssuredAttribute(i)->toUTF8NonGCString();
            if (!attrName.hasPrefix()) {
                hasDefaultNamespaceAttrValue = true;
                defaultNamespaceAttrValue = attrValue;
            } else {
                std::string local = attrName.localName()->toUTF8NonGCString();
                if (attrValue != kXMLNamespace) {
                    localPrefixes.emplace_back(local, attrValue);
                    prefixScope.add(attrValue, local);
                }
            }
        }
    }

    bool hasInheritedNs = hasContextNamespace;
    std::string inheritedNs = contextNamespace;

    bool elemHasNs = false;
    std::string ns = getNamespaceString(element->name(), elemHasNs);

    bool elemHasPrefix = element->name().hasPrefix();
    std::string elemPrefix =
        elemHasPrefix
            ? element->name().prefix().getValue().string()->toUTF8NonGCString()
            : std::string();
    String* elemLocalName = element->localName();

    bool ignoreNamespaceDefinitionAttribute = false;
    std::string elemOutPrefix;
    std::string extraXmlnsDecl;

    if (hasInheritedNs == elemHasNs && (!elemHasNs || inheritedNs == ns)) {
        // Step 9: inherited ns matches element's ns.
        if (hasDefaultNamespaceAttrValue) {
            ignoreNamespaceDefinitionAttribute = true;
        }
        if (elemHasNs && ns == kXMLNamespace) {
            elemOutPrefix = "xml";
        }
    } else {
        // Step 10: inherited ns differs from element's ns.
        std::string candidatePrefix;
        bool hasCandidatePrefix =
            elemHasNs && retrievePreferredPrefix(map, ns, candidatePrefix);
        const std::string* localBinding =
            elemHasPrefix ? findLocalPrefix(localPrefixes, elemPrefix)
                          : nullptr;

        if (elemHasNs && ns == kXMLNamespace) {
            elemOutPrefix = "xml";
        } else if (elemHasPrefix && elemHasNs && localBinding &&
                   *localBinding == ns) {
            elemOutPrefix = elemPrefix;
        } else if (elemHasPrefix && elemHasNs && !localBinding) {
            // Element's own prefix is not conflicting with a local xmlns:p.
            if (hasCandidatePrefix && hasPrefixInMap(map, ns, elemPrefix)) {
                elemOutPrefix = elemPrefix;
            } else {
                prefixScope.add(ns, elemPrefix);
                localPrefixes.emplace_back(elemPrefix, ns);
                elemOutPrefix = elemPrefix;
                extraXmlnsDecl = " xmlns:" + elemPrefix + "=\"";
            }
        } else if (hasCandidatePrefix) {
            elemOutPrefix = candidatePrefix;
        } else if (elemHasPrefix && elemHasNs) {
            // Prefix collided with a local xmlns:prefix="other".
            std::string gen = generatePrefix(prefixScope, ns, prefixIndex);
            localPrefixes.emplace_back(gen, ns);
            elemOutPrefix = gen;
            extraXmlnsDecl = " xmlns:" + gen + "=\"";
        } else if (hasDefaultNamespaceAttrValue &&
                   ((elemHasNs && defaultNamespaceAttrValue == ns) ||
                    (!elemHasNs && defaultNamespaceAttrValue.empty()))) {
            hasInheritedNs = elemHasNs;
            inheritedNs = ns;
        } else {
            hasInheritedNs = elemHasNs;
            inheritedNs = ns;
            ignoreNamespaceDefinitionAttribute = true;
            extraXmlnsDecl = " xmlns=\"";
        }
    }

    auto appendQualifiedTagName = [&]() {
        if (!elemOutPrefix.empty()) {
            appendUTF8(markup, elemOutPrefix);
            markup.appendChar(':');
        }
        markup.appendString(elemLocalName);
    };

    markup.appendChar('<');
    appendQualifiedTagName();

    if (!extraXmlnsDecl.empty()) {
        appendUTF8(markup, extraXmlnsDecl);
        if (elemHasNs) {
            appendEscapedAttrValue(markup, ns);
        }
        markup.appendChar('"');
    }

    // Step 11: Serialize attributes.
    for (size_t i = 0; i < attrCount; i++) {
        QualifiedName attrName = element->getAssuredAttributeName(i);
        String* attrValue = element->getAssuredAttribute(i);
        bool attrHasNs = false;
        std::string attrNs = getNamespaceString(attrName, attrHasNs);
        String* attrLocal = attrName.localName();

        if (!attrHasNs && attrLocal->equals("xmlns")) {
            // Drop inconsistent null-namespace xmlns="..." attributes
            // (https://github.com/w3c/DOM-Parsing/issues/47).
            if (ignoreNamespaceDefinitionAttribute ||
                (!elemHasNs && attrValue->length() > 0) ||
                (elemHasNs && attrValue->toUTF8NonGCString() != ns)) {
                continue;
            }
        }

        if (attrHasNs && attrNs == kXMLNSNamespace) {
            if (!attrName.hasPrefix()) {
                if (ignoreNamespaceDefinitionAttribute) {
                    continue;
                }
                if (attrValue->length() == 0 && !elemHasNs &&
                    !hasContextNamespace) {
                    continue;
                }
            }
            markup.appendChar(' ');
            if (attrName.hasPrefix()) {
                markup.appendString("xmlns:");
            }
            markup.appendString(attrLocal);
            markup.appendString("=\"");
            appendEscapedAttrValue(markup, attrValue);
            markup.appendChar('"');
            continue;
        }

        std::string attrPrefix;
        if (attrHasNs) {
            if (attrNs == kXMLNamespace) {
                attrPrefix = "xml";
            } else {
                std::string candidate;
                if (retrievePreferredPrefix(map, attrNs, candidate)) {
                    attrPrefix = candidate;
                } else {
                    attrPrefix =
                        generatePrefix(prefixScope, attrNs, prefixIndex);
                    markup.appendString(" xmlns:");
                    appendUTF8(markup, attrPrefix);
                    markup.appendString("=\"");
                    appendEscapedAttrValue(markup, attrNs);
                    markup.appendChar('"');
                }
            }
        }

        markup.appendChar(' ');
        if (!attrPrefix.empty()) {
            appendUTF8(markup, attrPrefix);
            markup.appendChar(':');
        }
        markup.appendString(attrLocal);
        markup.appendString("=\"");
        appendEscapedAttrValue(markup, attrValue);
        markup.appendChar('"');
    }

    // Step 12-17: Children and end tag.
    bool isHTML = elemHasNs && ns == kHTMLNamespace;
    if (!element->firstChild()) {
        if (isHTML && !isHTMLVoidElementName(elemLocalName)) {
            markup.appendString("></");
            appendQualifiedTagName();
            markup.appendChar('>');
        } else if (isHTML) {
            markup.appendString(" />");
        } else {
            markup.appendString("/>");
        }
        return;
    }

    markup.appendChar('>');
    for (Node* child = element->firstChild(); child;
         child = child->nextSibling()) {
        serializeXMLNode(child, hasInheritedNs, inheritedNs, map, prefixIndex,
                         markup);
    }
    markup.appendString("</");
    appendQualifiedTagName();
    markup.appendChar('>');
}

static void serializeXMLNode(Node* node, bool hasContextNamespace,
                             const std::string& contextNamespace,
                             NamespacePrefixMap& prefixMap,
                             unsigned& prefixIndex, StringBuilder& markup)
{
    if (node->isElement()) {
        serializeXMLElement(node->asElement(), hasContextNamespace,
                            contextNamespace, prefixMap, prefixIndex, markup);
    } else if (node->isDocument() || node->isDocumentFragment()) {
        for (Node* child = node->firstChild(); child;
             child = child->nextSibling()) {
            serializeXMLNode(child, hasContextNamespace, contextNamespace,
                             prefixMap, prefixIndex, markup);
        }
    } else if (node->isCDATASection()) {
        markup.appendString("<![CDATA[");
        markup.appendString(node->asCharacterData()->data());
        markup.appendString("]]>");
    } else if (node->isText()) {
        appendEscapedText(markup, node->asCharacterData()->data());
    } else if (node->isComment()) {
        markup.appendString("<!--");
        markup.appendString(node->asCharacterData()->data());
        markup.appendString("-->");
    } else if (node->isDocumentType()) {
        DocumentType* dt = node->asDocumentType();
        markup.appendString("<!DOCTYPE ");
        markup.appendString(dt->name());
        String* publicId = dt->publicId();
        String* systemId = dt->systemId();
        if (publicId->length() > 0) {
            markup.appendString(" PUBLIC \"");
            markup.appendString(publicId);
            markup.appendChar('"');
            if (systemId->length() > 0) {
                markup.appendString(" \"");
                markup.appendString(systemId);
                markup.appendChar('"');
            }
        } else if (systemId->length() > 0) {
            markup.appendString(" SYSTEM \"");
            markup.appendString(systemId);
            markup.appendChar('"');
        }
        markup.appendChar('>');
    } else if (node->isProcessingInstruction()) {
        ProcessingInstruction* pi = node->asProcessingInstruction();
        markup.appendString("<?");
        markup.appendString(pi->target());
        markup.appendChar(' ');
        markup.appendString(pi->data());
        markup.appendString("?>");
    }
    // Attr and any other node types serialize to the empty string.
}

XMLSerializer::XMLSerializer(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
{
}

ScriptBindingInstance* XMLSerializer::scriptBindingInstance()
{
    return document()->scriptBindingInstance();
}

String* XMLSerializer::serializeToString(Node* root)
{
    NamespacePrefixMap map;
    map[kXMLNamespace].push_back("xml");
    unsigned prefixIndex = 1;
    StringBuilder markup;
    serializeXMLNode(root, false, std::string(), map, prefixIndex, markup);
    return markup.finalize();
}

} // namespace Starfish
