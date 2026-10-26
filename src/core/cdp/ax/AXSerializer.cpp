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
#include "core/dom/HTMLDialogElement.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/HTMLTextEditable.h"
#include "core/dom/Node.h"
#include "core/page/BrowsingContext.h"
#include "platform/loader/ResourceURL.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace Starfish {

namespace {

    // The id the protocol uses for a DOM node that has no AX node. Chromium
    // hands out one fixed id for all of them, because such a node has no place
    // in the tree to tell it apart by.
    const char* kIdForNodeWithNoAXNode = "ax-no-node";

    // Chrome shows each character of a password field as this bullet.
    const char* kPasswordBullet = "\xE2\x80\xA2";

    // The largest whole number a double holds exactly, 2^53.
    const double kMaxExactInteger = 9007199254740992.0;

    // The internal role of a native option inside a drop down list.
    const int kChromeRoleMenuListOption = 127;

    // The input types whose value a user types in. Chrome exposes the value
    // of these inputs. It exposes no value for the other types, such as a
    // checkbox or a button.
    const char* const kTextEntryInputTypes[] = {
        "", "text", "search", "email", "url", "tel", "password", "number",
    };

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

    std::string lowered(std::string text)
    {
        for (char& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    }

    std::string ariaToken(Element* element, const char* attribute)
    {
        return lowered(axCollapseWhitespace(axAttribute(element, attribute)));
    }

    bool isTrueToken(Element* element, const char* attribute)
    {
        return ariaToken(element, attribute) == "true";
    }

    // Reads an integer the way Chromium does for aria-level.
    // Leading white space and trailing text are allowed.
    // A missing number or an overflow gives 0.
    int parseLooseInteger(const std::string& text)
    {
        size_t i = 0;
        while (i < text.size() &&
               std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
        bool negative = false;
        if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
            negative = text[i] == '-';
            ++i;
        }
        if (i >= text.size() ||
            !std::isdigit(static_cast<unsigned char>(text[i]))) {
            return 0;
        }
        int parsed = 0;
        for (; i < text.size() &&
               std::isdigit(static_cast<unsigned char>(text[i]));
             ++i) {
            int digit = text[i] - '0';
            if (parsed > (INT_MAX - digit) / 10) {
                return 0;
            }
            parsed = parsed * 10 + digit;
        }
        return negative ? -parsed : parsed;
    }

    // A value below 1 becomes 1, as in Chromium.
    int ariaLevelValue(const std::string& text)
    {
        int parsed = parseLooseInteger(text);
        return parsed < 1 ? 1 : parsed;
    }

    // Chromium treats an empty value and "undefined" as an unset token.
    bool isUnsetToken(const std::string& token)
    {
        return token.empty() || token == "undefined";
    }

    // The elements Chromium treats as listed form controls
    // (ListedElement in the HTML standard).
    bool isListedFormControl(const std::string& tag)
    {
        return tag == "button" || tag == "fieldset" || tag == "input" ||
               tag == "object" || tag == "output" || tag == "select" ||
               tag == "textarea";
    }

    // The roles that support aria-readonly, except cells and headers.
    bool roleSupportsAriaReadonly(const std::string& role)
    {
        return role == "checkbox" || role == "combobox" || role == "grid" ||
               role == "gridcell" || role == "listbox" ||
               role == "menuitemcheckbox" || role == "menuitemradio" ||
               role == "radio" || role == "radiogroup" || role == "searchbox" ||
               role == "slider" || role == "spinbutton" || role == "switch" ||
               role == "textbox" || role == "treegrid";
    }

    bool roleIsCellOrHeader(const std::string& role)
    {
        return role == "cell" || role == "columnheader" || role == "rowheader";
    }

    bool roleIsCellOrHeaderOrRow(const std::string& role)
    {
        return role == "columnheader" || role == "rowheader" || role == "row";
    }

    // A cell, header, or row counts as a widget only inside a grid or
    // treegrid. The nearest table-like ancestor decides.
    bool isInsideGrid(AXNode* node)
    {
        for (AXNode* ancestor = node->parent(); ancestor;
             ancestor = ancestor->parent()) {
            const std::string& ancestorRole = ancestor->role();
            if (ancestorRole == "table" || ancestorRole == "grid" ||
                ancestorRole == "treegrid") {
                return ancestorRole != "table";
            }
        }
        return false;
    }

    // The roles for which Chromium can set the value.
    bool roleCanSetValue(const std::string& role)
    {
        return role == "scrollbar" || role == "searchbox" || role == "slider" ||
               role == "spinbutton" || role == "splitter" || role == "textbox";
    }

    bool roleAllowsMultiselectable(const std::string& role)
    {
        return role == "grid" || role == "listbox" || role == "tablist" ||
               role == "tree" || role == "treegrid";
    }

    // Chromium reports readonly, multiline, and required for its text field
    // role only. The searchbox role is a different role there.
    bool roleAllowsReadonly(const std::string& role)
    {
        return role == "grid" || role == "gridcell" || role == "textbox" ||
               role == "columnheader" || role == "rowheader" ||
               role == "treegrid";
    }

    bool roleAllowsRequired(const std::string& role)
    {
        return role == "combobox" || role == "gridcell" || role == "listbox" ||
               role == "radiogroup" || role == "spinbutton" ||
               role == "textbox" || role == "tree" || role == "columnheader" ||
               role == "rowheader" || role == "treegrid";
    }

    bool roleAllowsOrientation(const std::string& role)
    {
        return role == "listbox" || role == "menu" || role == "menubar" ||
               role == "radiogroup" || role == "scrollbar" ||
               role == "separator" || role == "slider" || role == "tablist" ||
               role == "toolbar" || role == "tree" || role == "treegrid";
    }

    // Only some roles have an implicit orientation in Chromium.
    const char* defaultOrientation(const std::string& role)
    {
        if (role == "listbox" || role == "menu" || role == "scrollbar" ||
            role == "tree") {
            return "vertical";
        }
        if (role == "menubar" || role == "separator" || role == "slider" ||
            role == "tablist") {
            return "horizontal";
        }
        return nullptr;
    }

    bool roleAllowsLevel(const std::string& role)
    {
        return role == "heading" || role == "listitem" || role == "row" ||
               role == "treeitem";
    }

    bool roleAllowsSelected(const std::string& role)
    {
        return role == "columnheader" || role == "option" ||
               role == "gridcell" || role == "row" || role == "rowheader" ||
               role == "tab" || role == "treeitem";
    }

    bool roleIsAlwaysCheckable(const std::string& role)
    {
        return role == "checkbox" || role == "radio" || role == "switch" ||
               role == "menuitemcheckbox" || role == "menuitemradio";
    }

    // Chrome also accepts aria-expanded on listitem.
    bool roleAllowsExpanded(const std::string& role)
    {
        return role == "application" || role == "button" ||
               role == "checkbox" || role == "columnheader" ||
               role == "combobox" || role == "gridcell" || role == "link" ||
               role == "listitem" || role == "menuitem" ||
               role == "menuitemcheckbox" || role == "menuitemradio" ||
               role == "row" || role == "rowheader" || role == "switch" ||
               role == "tab" || role == "treeitem";
    }

    // True when the contenteditable attribute makes the element an editing
    // host. Its descendants are editable, but they are not hosts.
    bool isEditingHost(Element* element)
    {
        if (!element->isHTMLElement() ||
            !axHasAttribute(element, "contenteditable")) {
            return false;
        }
        std::string value = lowered(axAttribute(element, "contenteditable"));
        return value.empty() || value == "true" || value == "plaintext-only";
    }

    enum class EditableKind { None, PlainText, RichText };

    // Walks up from the element to the nearest contenteditable attribute.
    // The value false stops the walk. Other values pass the check upward.
    // Starfish reads the contenteditable attribute in a case sensitive way,
    // so the attribute is checked here instead of isContentEditable().
    EditableKind editableKindFromAttribute(Element* element)
    {
        for (Node* current = element; current && current->isElement();
             current = current->parentNode()) {
            Element* currentElement = current->asElement();
            if (!currentElement->isHTMLElement() ||
                !axHasAttribute(currentElement, "contenteditable")) {
                continue;
            }
            std::string value =
                lowered(axAttribute(currentElement, "contenteditable"));
            if (value.empty() || value == "true") {
                return EditableKind::RichText;
            }
            if (value == "plaintext-only") {
                return EditableKind::PlainText;
            }
            if (value == "false") {
                return EditableKind::None;
            }
        }
        return EditableKind::None;
    }

    // Parses the whole string as a decimal number.
    // "5", "-2.5", and "1e3" pass. "", "5px", and "1,5" do not.
    bool parseWholeNumber(const std::string& text, double& out)
    {
        if (text.empty()) {
            return false;
        }
        const char* begin = text.c_str();
        char* end = nullptr;
        errno = 0;
        double parsed = std::strtod(begin, &end);
        if (errno == ERANGE || end != begin + text.size()) {
            return false;
        }
        out = parsed;
        return true;
    }

} // namespace

rapidjson::Value AXSerializer::stringValue(const std::string& text)
{
    return rapidjson::Value(text.c_str(), text.size(), *m_allocator);
}

void AXSerializer::addBooleanProperty(rapidjson::Value& properties,
                                      const char* name, const char* type,
                                      bool boolValue)
{
    rapidjson::Value property(rapidjson::kObjectType);
    property.AddMember("name", rapidjson::StringRef(name), *m_allocator);
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("type", rapidjson::StringRef(type), *m_allocator);
    value.AddMember("value", boolValue, *m_allocator);
    property.AddMember("value", value, *m_allocator);
    properties.PushBack(property, *m_allocator);
}

void AXSerializer::addIntegerProperty(rapidjson::Value& properties,
                                      const char* name, int integerValue)
{
    rapidjson::Value property(rapidjson::kObjectType);
    property.AddMember("name", rapidjson::StringRef(name), *m_allocator);
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("type", "integer", *m_allocator);
    value.AddMember("value", integerValue, *m_allocator);
    property.AddMember("value", value, *m_allocator);
    properties.PushBack(property, *m_allocator);
}

void AXSerializer::addStringProperty(rapidjson::Value& properties,
                                     const char* name, const char* type,
                                     const char* text)
{
    rapidjson::Value property(rapidjson::kObjectType);
    property.AddMember("name", rapidjson::StringRef(name), *m_allocator);
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("type", rapidjson::StringRef(type), *m_allocator);
    value.AddMember("value", rapidjson::StringRef(text), *m_allocator);
    property.AddMember("value", value, *m_allocator);
    properties.PushBack(property, *m_allocator);
}

void AXSerializer::addStringProperty(rapidjson::Value& properties,
                                     const char* name, const char* type,
                                     const std::string& text)
{
    rapidjson::Value property(rapidjson::kObjectType);
    property.AddMember("name", rapidjson::StringRef(name), *m_allocator);
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("type", rapidjson::StringRef(type), *m_allocator);
    value.AddMember("value", stringValue(text), *m_allocator);
    property.AddMember("value", value, *m_allocator);
    properties.PushBack(property, *m_allocator);
}

void AXSerializer::addRole(rapidjson::Value& out, const std::string& role)
{
    addRole(out, role, axChromeRoleForRole(role));
}

// A native option of a drop down list has its own internal role in Chrome,
// while the ARIA role stays option.
void AXSerializer::addRole(rapidjson::Value& out, AXNode* node,
                           const std::string& role)
{
    int chromeRole = axChromeRoleForRole(role);
    Element* element = node->element();
    if (element && role == "option" && element->isHTMLOptionElement() &&
        !axHasAttribute(element, "role")) {
        HTMLSelectElement* select =
            element->asHTMLOptionElement()->selectElement();
        if (select && !axHasAttribute(select, "multiple") &&
            parseLooseInteger(axAttribute(select, "size")) <= 1) {
            chromeRole = kChromeRoleMenuListOption;
        }
    }
    addRole(out, role, chromeRole);
}

void AXSerializer::addRole(rapidjson::Value& out, const std::string& role,
                           int chromeRole)
{
    // A role the ARIA vocabulary does not have is reported as an internal
    // role, which is how a client tells the two apart.
    const char* type =
        (role == "StaticText" || role == "InlineTextBox" ||
         role == "RootWebArea" || role == "Iframe" || role == "LabelText" ||
         role == "DisclosureTriangle" || role == "DisclosureTriangleGrouped")
            ? "internalRole"
            : "role";

    rapidjson::Value roleValue(rapidjson::kObjectType);
    roleValue.AddMember("type", rapidjson::StringRef(type), *m_allocator);
    roleValue.AddMember("value", stringValue(role), *m_allocator);
    out.AddMember("role", roleValue, *m_allocator);

    rapidjson::Value chromeRoleValue(rapidjson::kObjectType);
    chromeRoleValue.AddMember("type", "internalRole", *m_allocator);
    chromeRoleValue.AddMember("value", chromeRole, *m_allocator);
    out.AddMember("chromeRole", chromeRoleValue, *m_allocator);
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

rapidjson::Value AXSerializer::relatedTextNodes(
    const std::vector<AXRelatedText>& related)
{
    rapidjson::Value nodes(rapidjson::kArrayType);
    for (const AXRelatedText& item : related) {
        rapidjson::Value node(rapidjson::kObjectType);
        node.AddMember("backendDOMNodeId",
                       m_registry->getOrCreate(item.element), *m_allocator);
        std::string id = axAttribute(item.element, "id");
        if (!id.empty()) {
            node.AddMember("idref", stringValue(id), *m_allocator);
        }
        node.AddMember("text", stringValue(item.text), *m_allocator);
        nodes.PushBack(node, *m_allocator);
    }
    return nodes;
}

rapidjson::Value AXSerializer::nameSourceValue(const AXNameSource& source)
{
    rapidjson::Value out(rapidjson::kObjectType);
    out.AddMember("type", rapidjson::StringRef(source.type), *m_allocator);
    if (!source.attribute.empty()) {
        out.AddMember("attribute", stringValue(source.attribute), *m_allocator);
    }
    if (!source.nativeSource.empty()) {
        out.AddMember("nativeSource", stringValue(source.nativeSource),
                      *m_allocator);
    }

    bool isLabelledBy = source.attribute == "aria-labelledby" ||
                        source.attribute == "aria-labeledby";
    if (!source.related.empty() && (isLabelledBy || source.attribute.empty())) {
        rapidjson::Value list(rapidjson::kObjectType);
        list.AddMember(
            "type",
            rapidjson::StringRef(isLabelledBy ? "idrefList" : "nodeList"),
            *m_allocator);
        list.AddMember("relatedNodes", relatedTextNodes(source.related),
                       *m_allocator);
        if (isLabelledBy && source.hasAttributeValue) {
            list.AddMember("value", stringValue(source.attributeValue),
                           *m_allocator);
        }
        out.AddMember(rapidjson::StringRef(isLabelledBy ? "attributeValue"
                                                        : "nativeSourceValue"),
                      list, *m_allocator);
    } else if (source.related.empty() && source.hasAttributeValue) {
        rapidjson::Value attributeValue(rapidjson::kObjectType);
        attributeValue.AddMember("type", "string", *m_allocator);
        attributeValue.AddMember("value", stringValue(source.attributeValue),
                                 *m_allocator);
        out.AddMember("attributeValue", attributeValue, *m_allocator);
    }

    if (source.hasText) {
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type", "computedString", *m_allocator);
        value.AddMember("value", stringValue(source.text), *m_allocator);
        out.AddMember("value", value, *m_allocator);
    }
    if (source.superseded) {
        out.AddMember("superseded", true, *m_allocator);
    }
    if (source.invalid) {
        out.AddMember("invalid", true, *m_allocator);
    }
    return out;
}

void AXSerializer::addNameWithSources(rapidjson::Value& out, AXNode* node)
{
    Element* element = node->element();
    const std::string& name = node->name();
    const std::string& role = node->role();

    rapidjson::Value nameValue(rapidjson::kObjectType);
    nameValue.AddMember("type", "computedString", *m_allocator);
    nameValue.AddMember("value", stringValue(name), *m_allocator);

    // The sources list records every place a name could have come from.
    // It follows the order that accname consults them.
    // The source that supplied the name carries a value.
    // A source that a higher-priority source beat is marked superseded.
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
    } else if (!node->ignored()) {
        for (const AXNameSource& source : axNameSources(element, role)) {
            sources.PushBack(nameSourceValue(source), *m_allocator);
        }
    }
    // Chromium reports no sources for a node that is ignored.
    if (!sources.Empty()) {
        nameValue.AddMember("sources", sources, *m_allocator);
    }
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
        addBooleanProperty(properties, "focusable", "booleanOrUndefined", true);
        Document* document = node->domNode()->asDocument();
        std::string address = document->urlString()
                                  ? document->urlString()->toUTF8NonGCString()
                                  : std::string();
        addStringProperty(properties, "url", "string", address);
        out.AddMember("properties", properties, *m_allocator);
        return;
    }

    Element* element = node->element();
    if (!element) {
        out.AddMember("properties", properties, *m_allocator);
        return;
    }

    const std::string tag = tagOf(element);
    const std::string& role = node->role();
    HTMLTextEditable* textEditable =
        element->isHTMLTextEditable() ? element->asHTMLTextEditable() : nullptr;

    const bool ariaDisabled = isTrueToken(element, "aria-disabled");
    const bool nativeDisabled =
        element->isHTMLFormControl() && element->isDisabledFormControl();
    const bool nativeReadonly = textEditable &&
                                textEditable->isEditableType() &&
                                textEditable->readonly();
    const bool editingHost = isEditingHost(element);

    // Chromium honors aria-readonly only for some roles.
    // It ignores aria-readonly on an editing host.
    // A cell or header needs a grid or treegrid ancestor.
    bool supportsAriaReadonly = false;
    if (!editingHost) {
        if (roleSupportsAriaReadonly(role)) {
            supportsAriaReadonly = true;
        } else if (roleIsCellOrHeader(role)) {
            supportsAriaReadonly = isInsideGrid(node);
        }
    }
    // The native readonly attribute wins over aria-readonly.
    // Otherwise aria-readonly can turn the state on or off.
    bool readonlyRestriction = nativeReadonly;
    const std::string ariaReadonlyToken = ariaToken(element, "aria-readonly");
    if (!nativeReadonly && supportsAriaReadonly &&
        (ariaReadonlyToken == "true" || ariaReadonlyToken == "false")) {
        readonlyRestriction = ariaReadonlyToken == "true";
    }
    const bool disabledRestriction = nativeDisabled || ariaDisabled;
    const bool unrestricted = !disabledRestriction && !readonlyRestriction;

    // FillGlobalStates. AXTree owns hidden/hiddenRoot and exposes those states
    // as ignored reasons. Do not walk the DOM ancestors again here.
    if (disabledRestriction) {
        addBooleanProperty(properties, "disabled", "boolean", true);
    }

    // An empty value and "undefined" mean the attribute is not set.
    // The spelling and grammar values count as true on a text field only.
    // Without the attribute, a listed form control reports its validity.
    // Starfish has no ValidityState, so such a control always reports false.
    const std::string invalid = lowered(axAttribute(element, "aria-invalid"));
    if (!isUnsetToken(invalid)) {
        if (invalid == "false") {
            addStringProperty(properties, "invalid", "token", "false");
        } else if (invalid != "grammar" && invalid != "spelling") {
            addStringProperty(properties, "invalid", "token", "true");
        } else if (role == "textbox") {
            addStringProperty(properties, "invalid", "token", "true");
        }
    } else if (isListedFormControl(tag)) {
        addStringProperty(properties, "invalid", "token", "false");
    }

    const bool nativeDialogIsOpen = element->isHTMLDialogElement() &&
                                    element->asHTMLDialogElement()->open();
    const bool focusableCandidate =
        axHasAttribute(element, "tabindex") || tag == "input" ||
        tag == "button" || tag == "select" || tag == "textarea" ||
        tag == "option" || editingHost || nativeDialogIsOpen ||
        role == "DisclosureTriangle" || role == "DisclosureTriangleGrouped" ||
        (tag == "a" && axHasAttribute(element, "href"));
    if (focusableCandidate && !nativeDisabled) {
        addBooleanProperty(properties, "focusable", "booleanOrUndefined", true);
    }

    if (element->document()->browsingContext()->focusedNode() == element) {
        addBooleanProperty(properties, "focused", "booleanOrUndefined", true);
    }

    // The editable state does not depend on disabled or readonly.
    // It is true for a text control and for content inside an editing host.
    EditableKind editableKind = editableKindFromAttribute(element);
    if (editableKind == EditableKind::None && textEditable &&
        textEditable->isEditableType()) {
        editableKind = EditableKind::PlainText;
    }
    if (editableKind != EditableKind::None) {
        addStringProperty(properties, "editable", "token",
                          editableKind == EditableKind::RichText ? "richtext"
                                                                 : "plaintext");
    }

    // The settable state depends on the role and on the restriction.
    // It does not depend on editable content.
    if (roleCanSetValue(role) && unrestricted) {
        addBooleanProperty(properties, "settable", "booleanOrUndefined", true);
    }

    // FillWidgetProperties.
    // The level comes from aria-level when the attribute has a value.
    const std::string levelText = axAttribute(element, "aria-level");
    const bool hasAriaLevel = roleAllowsLevel(role) && !levelText.empty();
    const int ariaLevel = hasAriaLevel ? ariaLevelValue(levelText) : 0;
    int level = 0;
    if (role == "heading") {
        // A heading uses aria-level only from 1 to 9.
        if (hasAriaLevel && ariaLevel <= 9) {
            level = ariaLevel;
        } else if (tag.size() == 2 && tag[0] == 'h' && tag[1] >= '1' &&
                   tag[1] <= '6') {
            level = tag[1] - '0';
        } else {
            level = 2;
        }
    } else if (hasAriaLevel) {
        level = ariaLevel;
    } else if (role == "listitem") {
        // Count the list ancestors. A list item outside any list has level 1.
        for (AXNode* ancestor = node->parent(); ancestor;
             ancestor = ancestor->parent()) {
            if (ancestor->role() == "list") {
                ++level;
            }
        }
        if (level == 0) {
            level = 1;
        }
    } else if (role == "treeitem") {
        // Count the groups up to the nearest tree.
        level = 1;
        for (AXNode* ancestor = node->parent(); ancestor;
             ancestor = ancestor->parent()) {
            if (ancestor->role() == "tree") {
                break;
            }
            if (ancestor->role() == "group") {
                ++level;
            }
        }
    }
    if (level > 0) {
        addIntegerProperty(properties, "level", level);
    }

    if (roleAllowsMultiselectable(role)) {
        addBooleanProperty(properties, "multiselectable", "boolean",
                           isTrueToken(element, "aria-multiselectable"));
    }

    if (roleAllowsOrientation(role)) {
        std::string orientation = ariaToken(element, "aria-orientation");
        if (orientation != "vertical" && orientation != "horizontal") {
            if (const char* fallback = defaultOrientation(role)) {
                orientation = fallback;
            } else {
                orientation.clear();
            }
        }
        if (!orientation.empty()) {
            addStringProperty(properties, "orientation", "token", orientation);
        }
    }

    if (role == "textbox") {
        // An input is always single line.
        // Otherwise aria-multiline decides.
        // Without it, a textarea or an editing host is multiline.
        bool multiline = false;
        const std::string multilineToken = ariaToken(element, "aria-multiline");
        if (tag == "input") {
            multiline = false;
        } else if (multilineToken == "true" || multilineToken == "false") {
            multiline = multilineToken == "true";
        } else {
            multiline = tag == "textarea" || editingHost;
        }
        addBooleanProperty(properties, "multiline", "boolean", multiline);
    }

    if (roleAllowsReadonly(role)) {
        addBooleanProperty(properties, "readonly", "boolean",
                           readonlyRestriction && !disabledRestriction);
    }

    if (roleAllowsRequired(role) && tag != "select") {
        const bool required = (element->isHTMLFormControl() &&
                               element->asHTMLFormControl()->required()) ||
                              isTrueToken(element, "aria-required");
        addBooleanProperty(properties, "required", "boolean", required);
    }

    // FillWidgetStates. Native mutable state wins over content attributes.
    const std::string checkedToken = ariaToken(element, "aria-checked");
    const bool conditionallyCheckable =
        (role == "option" || role == "treeitem") && !isUnsetToken(checkedToken);
    if (roleIsAlwaysCheckable(role) || conditionallyCheckable) {
        std::string checked;
        if (element->isHTMLInputElement()) {
            checked =
                element->asHTMLInputElement()->checked() ? "true" : "false";
        } else if (isUnsetToken(checkedToken) || checkedToken == "false") {
            checked = "false";
        } else if (checkedToken == "mixed") {
            // The roles radio, menuitemradio, and switch have no mixed state.
            const bool supportsMixed = role == "checkbox" ||
                                       role == "menuitemcheckbox" ||
                                       role == "option" || role == "treeitem";
            checked = supportsMixed ? "mixed" : "false";
        } else {
            // Any other value counts as true.
            checked = "true";
        }
        addStringProperty(properties, "checked", "tristate", checked);
    }

    std::string expanded = ariaToken(element, "aria-expanded");
    // Chrome counts any value other than false and undefined as true.
    if (roleAllowsExpanded(role) && !expanded.empty() &&
        expanded != "undefined") {
        addBooleanProperty(properties, "expanded", "booleanOrUndefined",
                           expanded != "false");
    } else if (tag == "select") {
        addBooleanProperty(properties, "expanded", "booleanOrUndefined", false);
    } else if (tag == "summary" && element->parentElement() &&
               tagOf(element->parentElement()) == "details") {
        // The open state of the details element decides it.
        addBooleanProperty(properties, "expanded", "booleanOrUndefined",
                           axHasAttribute(element->parentElement(), "open"));
    }

    // A cell, header, or row can be selected only inside a grid.
    const bool selectedSupported =
        !roleIsCellOrHeaderOrRow(role) || isInsideGrid(node);
    // Chrome reports selected for a native option or an explicit attribute.
    if (element->isHTMLOptionElement() ||
        (selectedSupported && roleAllowsSelected(role) &&
         axHasAttribute(element, "aria-selected"))) {
        const bool selected =
            element->isHTMLOptionElement()
                ? element->asHTMLOptionElement()->selectedness()
                : isTrueToken(element, "aria-selected");
        addBooleanProperty(properties, "selected", "booleanOrUndefined",
                           selected);
    }

    if (role == "dialog" || role == "alertdialog") {
        std::string ariaModal = ariaToken(element, "aria-modal");
        bool modal = false;
        if (ariaModal == "true" || ariaModal == "false") {
            modal = ariaModal == "true";
        } else if (element->isHTMLDialogElement()) {
            modal = element->asHTMLDialogElement()->isInShowModal();
        }
        addBooleanProperty(properties, "modal", "boolean", modal);
    }

    struct Relation {
        const char* attribute;
        const char* property;
        const char* type;
        bool includeValue;
        bool includeText;
    };
    auto addRelation = [&](const Relation& relation) {
        std::string idRefs = axAttribute(element, relation.attribute);
        if (idRefs.empty()) {
            return;
        }
        rapidjson::Value related =
            relatedNodes(element, idRefs, relation.includeText);
        if (related.Empty()) {
            return;
        }
        rapidjson::Value property(rapidjson::kObjectType);
        property.AddMember("name", rapidjson::StringRef(relation.property),
                           *m_allocator);
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type", rapidjson::StringRef(relation.type),
                        *m_allocator);
        if (relation.includeValue) {
            value.AddMember("value", stringValue(idRefs), *m_allocator);
        }
        value.AddMember("relatedNodes", related, *m_allocator);
        property.AddMember("value", value, *m_allocator);
        properties.PushBack(property, *m_allocator);
    };

    // The order follows FillRelationships and FillSparseAttributes in
    // Chromium. The labelledby property comes last because Chromium adds it
    // from the name sources.
    static const Relation firstRelations[] = {
        { "aria-describedby", "describedby", "idrefList", true, true },
        { "aria-owns", "owns", "idrefList", true, false },
    };
    for (const Relation& relation : firstRelations) {
        addRelation(relation);
    }

    // The url of an image or a link, resolved against the document.
    // A blank or missing attribute gives no url.
    const char* urlAttribute =
        role == "image" && tag == "img"
            ? "src"
            : (role == "link" && tag == "a" ? "href" : nullptr);
    if (urlAttribute) {
        std::string target =
            axCollapseWhitespace(axAttribute(element, urlAttribute));
        if (!target.empty()) {
            ResourceURL resolved(String::fromUTF8(target.data(), target.size()),
                                 element->document()->baseURL()->baseURI());
            addStringProperty(properties, "url", "string",
                              resolved.urlString()->toUTF8NonGCString());
        }
    }

    static const char* const sparseAttributes[][2] = {
        { "aria-keyshortcuts", "keyshortcuts" },
        { "aria-roledescription", "roledescription" },
    };
    for (const auto& sparse : sparseAttributes) {
        if (axHasAttribute(element, sparse[0])) {
            addStringProperty(properties, sparse[1], "string",
                              axAttribute(element, sparse[0]));
        }
    }

    static const Relation lastRelations[] = {
        { "aria-activedescendant", "activedescendant", "idref", false, false },
        { "aria-errormessage", "errormessage", "idrefList", true, false },
        { "aria-controls", "controls", "idrefList", true, false },
        { "aria-details", "details", "idrefList", true, false },
        { "aria-flowto", "flowto", "idrefList", true, false },
    };
    for (const Relation& relation : lastRelations) {
        addRelation(relation);
    }

    // Chromium reports each source that gave the name from other elements
    // as a labelledby property.
    for (const AXNameSource& source : axNameSources(element, node->role())) {
        if (!source.hasText || source.superseded || source.related.empty()) {
            continue;
        }
        rapidjson::Value property(rapidjson::kObjectType);
        property.AddMember("name", "labelledby", *m_allocator);
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type", "nodeList", *m_allocator);
        value.AddMember("relatedNodes", relatedTextNodes(source.related),
                        *m_allocator);
        property.AddMember("value", value, *m_allocator);
        properties.PushBack(property, *m_allocator);
    }

    out.AddMember("properties", properties, *m_allocator);
}

// The attribute of the source that gave the name. It is empty when the
// source is not an attribute.
static std::string nameSourceAttribute(Element* element,
                                       const std::string& role)
{
    for (const AXNameSource& source : axNameSources(element, role)) {
        if (source.hasText && !source.superseded) {
            return source.attribute;
        }
    }
    return std::string();
}

// The value Chrome exposes for a form control. This is the current value,
// not the value attribute, so it follows typing and script changes.
static std::string formControlValue(Element* element, const std::string& tag)
{
    if (!element->isHTMLFormControl()) {
        return std::string();
    }
    if (tag == "select") {
        // Chrome exposes the text of the selected option.
        HTMLOptionElement* option =
            element->asHTMLSelectElement()->firstSelectedOptionElement();
        String* text = option ? option->text() : nullptr;
        return text ? axCollapseWhitespace(text->toUTF8NonGCString())
                    : std::string();
    }

    std::string type;
    if (tag == "input") {
        type = lowered(axAttribute(element, "type"));
        bool isTextEntry = false;
        for (const char* textEntryType : kTextEntryInputTypes) {
            if (type == textEntryType) {
                isTextEntry = true;
                break;
            }
        }
        if (!isTextEntry) {
            return std::string();
        }
    }

    String* value = element->asHTMLFormControl()->value();
    std::string text = value ? value->toUTF8NonGCString() : std::string();
    if (type != "password") {
        return text;
    }
    std::string bullets;
    for (char byte : text) {
        // Count one bullet for each UTF-8 lead byte, so one per character.
        if ((static_cast<unsigned char>(byte) & 0xC0) != 0x80) {
            bullets += kPasswordBullet;
        }
    }
    return bullets;
}

void AXSerializer::addDescriptionAndValue(rapidjson::Value& out, AXNode* node)
{
    Element* element = node->element();
    if (!element) {
        return;
    }

    std::string description;
    std::string describedBy = axAttribute(element, "aria-describedby");
    if (!describedBy.empty()) {
        description = axTextFromIdRefs(element, describedBy);
    }
    std::string tag = tagOf(element);
    std::string type =
        tag == "input" ? lowered(axAttribute(element, "type")) : std::string();
    bool isTextButton = type == "button" || type == "submit" || type == "reset";
    bool hasTitle = axHasAttribute(element, "title");
    std::string nameAttribute;
    if (description.empty() &&
        ((isTextButton && axHasAttribute(element, "value")) || hasTitle)) {
        nameAttribute = nameSourceAttribute(element, node->role());
    }
    // The value of a button that did not name it describes it.
    if (description.empty() && isTextButton && nameAttribute != "value") {
        description = axAttribute(element, "value");
    }
    // A title that did not name the element describes it.
    if (description.empty() && hasTitle && nameAttribute != "title") {
        std::string title = axAttribute(element, "title");
        Optional<String*> content = element->textContent();
        std::string text = axCollapseWhitespace(
            content.hasValue() ? content.value()->toUTF8NonGCString()
                               : std::string());
        if (title != text) {
            description = title;
        }
    }
    if (!description.empty()) {
        rapidjson::Value value(rapidjson::kObjectType);
        value.AddMember("type", "computedString", *m_allocator);
        value.AddMember("value", stringValue(description), *m_allocator);
        out.AddMember("description", value, *m_allocator);
    }

    std::string controlValue;
    bool numberValue = false;
    if (tag == "input" || tag == "textarea" || tag == "select") {
        controlValue = formControlValue(element, tag);
        // Chrome reports the value of <input type="number"> as a number.
        // Every other control keeps the string form.
        numberValue =
            tag == "input" && lowered(axAttribute(element, "type")) == "number";
    } else if (axHasAttribute(element, "aria-valuetext")) {
        controlValue = axAttribute(element, "aria-valuetext");
    } else if (axHasAttribute(element, "aria-valuenow")) {
        controlValue = axAttribute(element, "aria-valuenow");
    }
    if (!controlValue.empty()) {
        rapidjson::Value value(rapidjson::kObjectType);
        double parsed = 0.0;
        if (numberValue && parseWholeNumber(controlValue, parsed)) {
            value.AddMember("type", "number", *m_allocator);
            // A whole number is sent as an integer, as Chrome does.
            // Chrome sends 5, not 5.0.
            if (std::isfinite(parsed) && parsed == std::floor(parsed) &&
                std::fabs(parsed) <= kMaxExactInteger) {
                value.AddMember("value", static_cast<int64_t>(parsed),
                                *m_allocator);
            } else {
                value.AddMember("value", parsed, *m_allocator);
            }
        } else {
            value.AddMember("type", "string", *m_allocator);
            value.AddMember("value", stringValue(controlValue), *m_allocator);
        }
        out.AddMember("value", value, *m_allocator);
    }
}

void AXSerializer::addIgnoredReasons(rapidjson::Value& out, AXNode* node)
{
    rapidjson::Value reasons(rapidjson::kArrayType);
    for (AXIgnoredReason reason : node->ignoredReasons()) {
        const char* name = axIgnoredReasonName(reason);
        rapidjson::Value property(rapidjson::kObjectType);
        property.AddMember("name", rapidjson::StringRef(name), *m_allocator);

        rapidjson::Value value(rapidjson::kObjectType);
        // Chrome names no target for the element that has aria-hidden itself.
        Element* target = reason == AXIgnoredReason::AriaHiddenElement
                              ? nullptr
                              : node->ignoredReasonTarget();
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
    out.AddMember("nodeId", rapidjson::StringRef(kIdForNodeWithNoAXNode),
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
        addRole(out, node, node->role().empty() ? "none" : node->role());
        addNameWithSources(out, node);
        addIgnoredReasons(out, node);
    } else if (node->kind() == AXNodeKind::Root) {
        addRole(out, node->role());
        addDocumentName(out, node);
        addProperties(out, node);
    } else {
        addRole(out, node, node->role());
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
