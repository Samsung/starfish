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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "AXSerializer.h"
#include "AXName.h"
#include "AXRole.h"
#include "AXTree.h"
#include "../NodeRegistry.h"
#include "../domains/PageDomain.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/page/BrowsingContext.h"

#include <cstring>

namespace Starfish {

namespace {

    // The id the protocol uses for a DOM node that has no AX node. Chromium
    // hands out one fixed id for all of them, because such a node has no place
    // in the tree to tell it apart by.
    const char* kIdForNodeWithNoAXNode = "ax-no-node";

    std::string tagOf(Element* element)
    {
        std::string tag = element->localName()
                              ? element->localName()->toUTF8NonGCString()
                              : std::string();
        for (char& c : tag) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return tag;
    }

} // namespace

rapidjson::Value AXSerializer::stringValue(const std::string& text)
{
    return rapidjson::Value(text.c_str(), text.size(), *m_allocator);
}

void AXSerializer::addRole(rapidjson::Value& out, const std::string& role)
{
    // A role the ARIA vocabulary does not have is reported as an internal
    // role, which is how a client tells the two apart.
    const char* type = (role == "StaticText" || role == "InlineTextBox" ||
                        role == "RootWebArea" || role == "Iframe")
                           ? "internalRole"
                           : "role";

    rapidjson::Value roleValue(rapidjson::kObjectType);
    roleValue.AddMember("type",
                        rapidjson::Value(type, strlen(type), *m_allocator),
                        *m_allocator);
    roleValue.AddMember("value", stringValue(role), *m_allocator);
    out.AddMember("role", roleValue, *m_allocator);

    rapidjson::Value chromeRole(rapidjson::kObjectType);
    chromeRole.AddMember("type", "internalRole", *m_allocator);
    chromeRole.AddMember("value", axChromeRoleForRole(role), *m_allocator);
    out.AddMember("chromeRole", chromeRole, *m_allocator);
}

rapidjson::Value AXSerializer::relatedNodes(Element* element,
                                            const std::string& idRefs,
                                            bool includeText)
{
    rapidjson::Value related(rapidjson::kArrayType);
    Document* document = element ? element->document() : nullptr;
    if (!document) {
        return related;
    }

    size_t start = 0;
    while (start < idRefs.size()) {
        size_t end = idRefs.find(' ', start);
        std::string id = idRefs.substr(start, end - start);
        if (!id.empty()) {
            Element* target = document->getElementById(
                String::fromUTF8(id.data(), id.size()));
            if (target) {
                rapidjson::Value item(rapidjson::kObjectType);
                item.AddMember("backendDOMNodeId",
                               m_registry->getOrCreate(target), *m_allocator);
                item.AddMember("idref", stringValue(id), *m_allocator);
                if (includeText) {
                    item.AddMember("text",
                                   stringValue(axTextFromIdRefs(element, id)),
                                   *m_allocator);
                }
                related.PushBack(item, *m_allocator);
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return related;
}

void AXSerializer::addNameWithSources(rapidjson::Value& out, AXNode* node)
{
    Element* element = node->element();
    const std::string& name = node->name();
    const std::string& role = node->role();

    rapidjson::Value nameValue(rapidjson::kObjectType);
    nameValue.AddMember("type", "computedString", *m_allocator);
    nameValue.AddMember("value", stringValue(name), *m_allocator);

    // The sources list records every place a name could have come from, in
    // the order accname consults them. The one that supplied the name
    // carries a value; the ones a higher-priority source beat are marked
    // superseded.
    rapidjson::Value sources(rapidjson::kArrayType);
    if (!element) {
        // Text has one place its name can come from: the text itself.
        if (node->kind() == AXNodeKind::StaticText && !name.empty()) {
            rapidjson::Value source(rapidjson::kObjectType);
            source.AddMember("type", "contents", *m_allocator);
            rapidjson::Value value(rapidjson::kObjectType);
            value.AddMember("type", "computedString", *m_allocator);
            value.AddMember("value", stringValue(name), *m_allocator);
            source.AddMember("value", value, *m_allocator);
            sources.PushBack(source, *m_allocator);
        }
    } else {
        std::string labelledBy = axLabelledByAttribute(element);
        bool hasLabelledBy = !labelledBy.empty();
        bool hasLabel = axHasAttribute(element, "aria-label");
        bool hasTitle = axHasAttribute(element, "title");
        bool supportsContent = axRoleSupportsNameFromContents(role);

        bool usedLabelledBy =
            hasLabelledBy && !axTextFromIdRefs(element, labelledBy).empty();
        bool usedLabel = !usedLabelledBy && hasLabel &&
                         !axAttribute(element, "aria-label").empty();
        bool usedContent =
            !usedLabelledBy && !usedLabel && supportsContent && !name.empty();
        bool usedTitle =
            !usedLabelledBy && !usedLabel && !usedContent && hasTitle;

        {
            rapidjson::Value source(rapidjson::kObjectType);
            source.AddMember("attribute", "aria-labelledby", *m_allocator);
            source.AddMember("type", "relatedElement", *m_allocator);
            if (hasLabelledBy) {
                rapidjson::Value attributeValue(rapidjson::kObjectType);
                attributeValue.AddMember("type", "idrefList", *m_allocator);
                attributeValue.AddMember("value", stringValue(labelledBy),
                                         *m_allocator);
                rapidjson::Value related =
                    relatedNodes(element, labelledBy, true);
                if (!related.Empty()) {
                    attributeValue.AddMember("relatedNodes", related,
                                             *m_allocator);
                }
                source.AddMember("attributeValue", attributeValue,
                                 *m_allocator);
            }
            if (usedLabelledBy) {
                rapidjson::Value value(rapidjson::kObjectType);
                value.AddMember("type", "computedString", *m_allocator);
                value.AddMember("value", stringValue(name), *m_allocator);
                source.AddMember("value", value, *m_allocator);
            } else if (hasLabelledBy) {
                source.AddMember("superseded", true, *m_allocator);
            }
            sources.PushBack(source, *m_allocator);
        }
        {
            rapidjson::Value source(rapidjson::kObjectType);
            source.AddMember("attribute", "aria-label", *m_allocator);
            source.AddMember("type", "attribute", *m_allocator);
            if (hasLabel) {
                rapidjson::Value attributeValue(rapidjson::kObjectType);
                attributeValue.AddMember("type", "string", *m_allocator);
                attributeValue.AddMember(
                    "value", stringValue(axAttribute(element, "aria-label")),
                    *m_allocator);
                source.AddMember("attributeValue", attributeValue,
                                 *m_allocator);
            }
            if (usedLabel) {
                rapidjson::Value value(rapidjson::kObjectType);
                value.AddMember("type", "computedString", *m_allocator);
                value.AddMember("value", stringValue(name), *m_allocator);
                source.AddMember("value", value, *m_allocator);
            } else if (usedLabelledBy) {
                source.AddMember("superseded", true, *m_allocator);
            }
            sources.PushBack(source, *m_allocator);
        }
        if (supportsContent) {
            rapidjson::Value source(rapidjson::kObjectType);
            source.AddMember("type", "contents", *m_allocator);
            if (usedContent) {
                rapidjson::Value value(rapidjson::kObjectType);
                value.AddMember("type", "computedString", *m_allocator);
                value.AddMember("value", stringValue(name), *m_allocator);
                source.AddMember("value", value, *m_allocator);
            }
            sources.PushBack(source, *m_allocator);
        }
        if (!axRoleIsNameProhibited(role)) {
            rapidjson::Value source(rapidjson::kObjectType);
            source.AddMember("attribute", "title", *m_allocator);
            source.AddMember("type", "attribute", *m_allocator);
            if (!usedTitle && (usedLabelledBy || usedLabel || usedContent)) {
                source.AddMember("superseded", true, *m_allocator);
            }
            sources.PushBack(source, *m_allocator);
        }
    }
    nameValue.AddMember("sources", sources, *m_allocator);
    out.AddMember("name", nameValue, *m_allocator);
}

void AXSerializer::addDocumentName(rapidjson::Value& out, AXNode* node)
{
    const std::string& title = node->name();

    rapidjson::Value name(rapidjson::kObjectType);
    name.AddMember("type", "computedString", *m_allocator);
    name.AddMember("value", stringValue(title), *m_allocator);

    // The document's name comes from <title>, so its source list ends with
    // that native source rather than with the title attribute.
    rapidjson::Value sources(rapidjson::kArrayType);
    rapidjson::Value labelledBy(rapidjson::kObjectType);
    labelledBy.AddMember("attribute", "aria-labelledby", *m_allocator);
    labelledBy.AddMember("type", "relatedElement", *m_allocator);
    sources.PushBack(labelledBy, *m_allocator);
    rapidjson::Value ariaLabel(rapidjson::kObjectType);
    ariaLabel.AddMember("attribute", "aria-label", *m_allocator);
    ariaLabel.AddMember("type", "attribute", *m_allocator);
    sources.PushBack(ariaLabel, *m_allocator);
    rapidjson::Value supersededAriaLabel(rapidjson::kObjectType);
    supersededAriaLabel.AddMember("attribute", "aria-label", *m_allocator);
    supersededAriaLabel.AddMember("superseded", true, *m_allocator);
    supersededAriaLabel.AddMember("type", "attribute", *m_allocator);
    sources.PushBack(supersededAriaLabel, *m_allocator);
    rapidjson::Value nativeTitle(rapidjson::kObjectType);
    nativeTitle.AddMember("nativeSource", "title", *m_allocator);
    nativeTitle.AddMember("type", "relatedElement", *m_allocator);
    if (!title.empty()) {
        rapidjson::Value titleValue(rapidjson::kObjectType);
        titleValue.AddMember("type", "computedString", *m_allocator);
        titleValue.AddMember("value", stringValue(title), *m_allocator);
        nativeTitle.AddMember("value", titleValue, *m_allocator);
    }
    sources.PushBack(nativeTitle, *m_allocator);
    name.AddMember("sources", sources, *m_allocator);
    out.AddMember("name", name, *m_allocator);
}

void AXSerializer::addProperties(rapidjson::Value& out, AXNode* node)
{
    rapidjson::Value properties(rapidjson::kArrayType);

    if (node->kind() == AXNodeKind::Root) {
        rapidjson::Value focusable(rapidjson::kObjectType);
        focusable.AddMember("name", "focusable", *m_allocator);
        rapidjson::Value focusableValue(rapidjson::kObjectType);
        focusableValue.AddMember("type", "booleanOrUndefined", *m_allocator);
        focusableValue.AddMember("value", true, *m_allocator);
        focusable.AddMember("value", focusableValue, *m_allocator);
        properties.PushBack(focusable, *m_allocator);

        rapidjson::Value url(rapidjson::kObjectType);
        url.AddMember("name", "url", *m_allocator);
        rapidjson::Value urlValue(rapidjson::kObjectType);
        urlValue.AddMember("type", "string", *m_allocator);
        Document* document = node->domNode()->asDocument();
        std::string address = document->urlString()
                                  ? document->urlString()->toUTF8NonGCString()
                                  : std::string();
        urlValue.AddMember("value", stringValue(address), *m_allocator);
        url.AddMember("value", urlValue, *m_allocator);
        properties.PushBack(url, *m_allocator);

        out.AddMember("properties", properties, *m_allocator);
        return;
    }

    Element* element = node->element();
    if (!element) {
        out.AddMember("properties", properties, *m_allocator);
        return;
    }

    std::string tag = tagOf(element);
    if (axHasAttribute(element, "tabindex") || tag == "input" ||
        tag == "button" || tag == "select" || tag == "textarea" ||
        (tag == "a" && axHasAttribute(element, "href"))) {
        rapidjson::Value property(rapidjson::kObjectType);
        property.AddMember("name", "focusable", *m_allocator);
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type", "booleanOrUndefined", *m_allocator);
        value.AddMember("value", true, *m_allocator);
        property.AddMember("value", value, *m_allocator);
        properties.PushBack(property, *m_allocator);
    }

    if (node->role() == "heading" && tag.size() == 2 && tag[0] == 'h' &&
        tag[1] >= '1' && tag[1] <= '6') {
        rapidjson::Value property(rapidjson::kObjectType);
        property.AddMember("name", "level", *m_allocator);
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type", "integer", *m_allocator);
        value.AddMember("value", tag[1] - '0', *m_allocator);
        property.AddMember("value", value, *m_allocator);
        properties.PushBack(property, *m_allocator);
    }

    struct Relation {
        const char* attribute;
        const char* property;
        const char* type;
        bool includeValue;
        bool includeText;
    };
    // The order is the one Chromium's FillSparseAttributes and
    // FillRelationships produce, which the expected outputs compare
    // position by position.
    static const Relation relations[] = {
        { "aria-activedescendant", "activedescendant", "idref", false, false },
        { "aria-controls", "controls", "idrefList", true, false },
        { "aria-describedby", "describedby", "idrefList", true, true },
        { "aria-details", "details", "idrefList", true, false },
        { "aria-errormessage", "errormessage", "idrefList", true, false },
        { "aria-flowto", "flowto", "idrefList", true, false },
        { "aria-labelledby", "labelledby", "nodeList", false, true },
        { "aria-owns", "owns", "idrefList", true, false },
    };
    for (const Relation& relation : relations) {
        std::string idRefs = axAttribute(element, relation.attribute);
        if (idRefs.empty()) {
            continue;
        }
        rapidjson::Value related =
            relatedNodes(element, idRefs, relation.includeText);
        if (related.Empty()) {
            continue;
        }
        rapidjson::Value property(rapidjson::kObjectType);
        property.AddMember("name",
                           rapidjson::Value(relation.property,
                                            strlen(relation.property),
                                            *m_allocator),
                           *m_allocator);
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type",
                        rapidjson::Value(relation.type, strlen(relation.type),
                                         *m_allocator),
                        *m_allocator);
        if (relation.includeValue) {
            value.AddMember("value", stringValue(idRefs), *m_allocator);
        }
        value.AddMember("relatedNodes", related, *m_allocator);
        property.AddMember("value", value, *m_allocator);
        properties.PushBack(property, *m_allocator);
    }

    out.AddMember("properties", properties, *m_allocator);
}

void AXSerializer::addDescriptionAndValue(rapidjson::Value& out, AXNode* node)
{
    Element* element = node->element();
    if (!element) {
        return;
    }

    std::string describedBy = axAttribute(element, "aria-describedby");
    if (!describedBy.empty()) {
        std::string description = axTextFromIdRefs(element, describedBy);
        if (!description.empty()) {
            rapidjson::Value value(rapidjson::kObjectType);
            value.AddMember("type", "computedString", *m_allocator);
            value.AddMember("value", stringValue(description), *m_allocator);
            out.AddMember("description", value, *m_allocator);
        }
    }

    std::string tag = tagOf(element);
    std::string controlValue;
    if (tag == "input" || tag == "textarea" || tag == "select") {
        controlValue = axAttribute(element, "value");
    } else if (axHasAttribute(element, "aria-valuetext")) {
        controlValue = axAttribute(element, "aria-valuetext");
    } else if (axHasAttribute(element, "aria-valuenow")) {
        controlValue = axAttribute(element, "aria-valuenow");
    }
    if (!controlValue.empty()) {
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type", "string", *m_allocator);
        value.AddMember("value", stringValue(controlValue), *m_allocator);
        out.AddMember("value", value, *m_allocator);
    }
}

void AXSerializer::addIgnoredReasons(rapidjson::Value& out, AXNode* node)
{
    rapidjson::Value reasons(rapidjson::kArrayType);
    for (AXIgnoredReason reason : node->ignoredReasons()) {
        const char* name = axIgnoredReasonName(reason);
        rapidjson::Value property(rapidjson::kObjectType);
        property.AddMember("name",
                           rapidjson::Value(name, strlen(name), *m_allocator),
                           *m_allocator);

        rapidjson::Value value(rapidjson::kObjectType);
        Element* target = node->ignoredReasonTarget();
        if (target) {
            // A reason that names another element reports that element
            // instead of a bare true.
            value.AddMember("type", "idref", *m_allocator);
            rapidjson::Value related(rapidjson::kArrayType);
            rapidjson::Value item(rapidjson::kObjectType);
            item.AddMember("backendDOMNodeId", m_registry->getOrCreate(target),
                           *m_allocator);
            std::string idref = axAttribute(target, "id");
            if (!idref.empty()) {
                item.AddMember("idref", stringValue(idref), *m_allocator);
            }
            related.PushBack(item, *m_allocator);
            value.AddMember("relatedNodes", related, *m_allocator);
        } else {
            value.AddMember("type", "boolean", *m_allocator);
            value.AddMember("value", true, *m_allocator);
        }
        property.AddMember("value", value, *m_allocator);
        reasons.PushBack(property, *m_allocator);
    }
    out.AddMember("ignoredReasons", reasons, *m_allocator);
}

rapidjson::Value AXSerializer::serializeMissing(Node* domNode)
{
    rapidjson::Value out(rapidjson::kObjectType);
    out.AddMember("nodeId",
                  rapidjson::Value(kIdForNodeWithNoAXNode,
                                   strlen(kIdForNodeWithNoAXNode),
                                   *m_allocator),
                  *m_allocator);
    out.AddMember("ignored", true, *m_allocator);
    addRole(out, "none");

    rapidjson::Value reasons(rapidjson::kArrayType);
    rapidjson::Value reason(rapidjson::kObjectType);
    reason.AddMember("name", "notRendered", *m_allocator);
    rapidjson::Value reasonValue(rapidjson::kObjectType);
    reasonValue.AddMember("type", "boolean", *m_allocator);
    reasonValue.AddMember("value", true, *m_allocator);
    reason.AddMember("value", reasonValue, *m_allocator);
    reasons.PushBack(reason, *m_allocator);
    out.AddMember("ignoredReasons", reasons, *m_allocator);

    out.AddMember("backendDOMNodeId", m_registry->getOrCreate(domNode),
                  *m_allocator);
    return out;
}

rapidjson::Value AXSerializer::serialize(AXNode* node, bool forceNameAndRole)
{
    rapidjson::Value out(rapidjson::kObjectType);
    out.AddMember("nodeId", stringValue(node->id()), *m_allocator);
    out.AddMember("ignored", node->ignored(), *m_allocator);

    if (node->ignored() && !forceNameAndRole) {
        addRole(out, "none");
        addIgnoredReasons(out, node);
    } else if (node->ignored()) {
        addRole(out, node->role().empty() ? "none" : node->role());
        addNameWithSources(out, node);
        addIgnoredReasons(out, node);
    } else if (node->kind() == AXNodeKind::Root) {
        addRole(out, node->role());
        addDocumentName(out, node);
        addProperties(out, node);
    } else {
        addRole(out, node->role());
        addNameWithSources(out, node);
        addProperties(out, node);
        addDescriptionAndValue(out, node);
    }

    rapidjson::Value childIds(rapidjson::kArrayType);
    for (AXNode* child : node->children()) {
        childIds.PushBack(stringValue(child->id()), *m_allocator);
    }
    out.AddMember("childIds", childIds, *m_allocator);

    if (node->domNode()) {
        out.AddMember("backendDOMNodeId",
                      m_registry->getOrCreate(node->domNode()), *m_allocator);
    }

    if (node->parent()) {
        out.AddMember("parentId", stringValue(node->parent()->id()),
                      *m_allocator);
    } else if (node->kind() == AXNodeKind::Root && m_page) {
        Document* document = node->domNode()->asDocument();
        std::string frameId =
            m_page->frameIdForBrowsingContext(document->browsingContext());
        if (!frameId.empty()) {
            out.AddMember("frameId", stringValue(frameId), *m_allocator);
        }
    }
    return out;
}

} // namespace Starfish

#endif
