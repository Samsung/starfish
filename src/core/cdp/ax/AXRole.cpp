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
#include "AXRole.h"
#include "AXName.h"
#include "core/dom/Element.h"

#include <cctype>
#include <cstring>
#include <unordered_map>

namespace Starfish {

namespace {

    std::string attributeOf(Element* element, const char* name)
    {
        Optional<String*> value =
            element->getAttribute(String::fromUTF8(name, strlen(name)));
        if (!value.hasValue() || !value.value()) {
            return std::string();
        }
        return value.value()->toUTF8NonGCString();
    }

    std::string lowered(std::string text)
    {
        for (char& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    }

    // role takes a token list; the first token the user agent understands wins.
    std::string firstRoleToken(Element* element)
    {
        std::string role =
            lowered(axCollapseWhitespace(attributeOf(element, "role")));
        size_t space = role.find(' ');
        return space == std::string::npos ? role : role.substr(0, space);
    }

} // namespace

bool axHasPresentationalRole(Element* element)
{
    std::string role = firstRoleToken(element);
    return role == "none" || role == "presentation";
}

bool axIsPresentationalListItem(Element* element)
{
    Element* parent = element->parentElement();
    if (!parent || !parent->localName()) {
        return false;
    }
    std::string parentTag = lowered(parent->localName()->toUTF8NonGCString());
    if (parentTag != "ul" && parentTag != "ol" && parentTag != "menu") {
        return false;
    }
    std::string role = attributeOf(parent, "role");
    return !role.empty() && role != "list" && role != "directory";
}

// The implicit role of elements whose role does not depend on attributes or
// context, or nullptr.
static const char* fixedRoleForTag(const std::string& tag)
{
    static const struct {
        const char* tag;
        const char* role;
    } roles[] = {
        { "button", "button" },
        { "iframe", "Iframe" },
        { "h1", "heading" },
        { "h2", "heading" },
        { "h3", "heading" },
        { "h4", "heading" },
        { "h5", "heading" },
        { "h6", "heading" },
        { "img", "image" },
        { "svg", "image" },
        { "textarea", "textbox" },
        { "select", "combobox" },
        { "option", "option" },
        { "nav", "navigation" },
        { "main", "main" },
        { "article", "article" },
        { "section", "region" },
        { "header", "banner" },
        { "footer", "contentinfo" },
        { "aside", "complementary" },
        { "dialog", "dialog" },
        { "ul", "list" },
        { "ol", "list" },
        { "p", "paragraph" },
        { "figure", "figure" },
        { "figcaption", "caption" },
        { "table", "table" },
        { "tr", "row" },
        { "td", "cell" },
        { "th", "columnheader" },
        { "form", "form" },
        { "canvas", "canvas" },
        { "label", "LabelText" },
    };
    for (const auto& entry : roles) {
        if (tag == entry.tag) {
            return entry.role;
        }
    }
    return nullptr;
}

static std::string inputRole(Element* element)
{
    std::string type = lowered(attributeOf(element, "type"));
    if (type == "checkbox") {
        return "checkbox";
    }
    if (type == "radio") {
        return "radio";
    }
    if (type == "button" || type == "submit" || type == "reset" ||
        type == "image") {
        return "button";
    }
    if (type == "range") {
        return "slider";
    }
    if (type == "number") {
        return "spinbutton";
    }
    if (type == "hidden") {
        return std::string();
    }
    // text, search, email, url, tel, password, "" ...
    return "textbox";
}

static std::string summaryRole(Element* element)
{
    // Only the summary of a details element is a disclosure widget.
    // A details element with a name belongs to an exclusive group.
    Element* parent = element->parentElement();
    if (parent && parent->localName() &&
        parent->localName()->toUTF8NonGCString() == "details") {
        return attributeOf(parent, "name").empty()
                   ? "DisclosureTriangle"
                   : "DisclosureTriangleGrouped";
    }
    return "generic";
}

std::string axRoleForElement(Element* element, const std::string& tag)
{
    std::string explicitRole = firstRoleToken(element);
    if (!explicitRole.empty()) {
        if (explicitRole == "none" || explicitRole == "presentation") {
            return std::string();
        }
        // The ARIA role img is reported as the role image.
        if (explicitRole == "img") {
            return "image";
        }
        return explicitRole;
    }

    if (const char* role = fixedRoleForTag(tag)) {
        return role;
    }
    if (tag == "a") {
        return element->hasAttribute(String::fromUTF8("href", 4)) ? "link"
                                                                  : "generic";
    }
    if (tag == "input") {
        return inputRole(element);
    }
    if (tag == "li") {
        return axIsPresentationalListItem(element) ? std::string() : "listitem";
    }
    if (tag == "summary") {
        return summaryRole(element);
    }
    return "generic";
}

int axChromeRoleForRole(const std::string& role)
{
    static const std::unordered_map<std::string, int> mapping = {
        { "RootWebArea", 144 },
        { "article", 5 },
        { "main", 118 },
        { "paragraph", 133 },
        { "heading", 96 },
        { "button", 9 },
        { "link", 110 },
        { "image", 99 },
        { "MenuListPopup", 128 },
        { "textbox", 170 },
        { "StaticText", 158 },
        { "checkbox", 14 },
        { "combobox", 209 },
        { "banner", 7 },
        { "contentinfo", 26 },
        { "complementary", 22 },
        { "dialog", 35 },
        { "alertdialog", 3 },
        { "figure", 84 },
        { "caption", 11 },
        { "generic", 88 },
        { "list", 111 },
        { "listitem", 115 },
        { "table", 167 },
        { "row", 145 },
        { "cell", 13 },
        { "columnheader", 19 },
        { "form", 87 },
        { "region", 143 },
        { "Iframe", 97 },
        { "radio", 141 },
        { "radiogroup", 142 },
        { "slider", 155 },
        { "menu", 122 },
        { "menuitem", 124 },
        { "option", 113 },
        { "alert", 2 },
        { "progressbar", 140 },
        { "searchbox", 153 },
        { "spinbutton", 156 },
        { "status", 159 },
        { "switch", 163 },
        { "DisclosureTriangle", 37 },
        { "DisclosureTriangleGrouped", 38 },
        { "treeitem", 180 },
        { "navigation", 130 },
        { "InlineTextBox", 101 },
        { "group", 93 },
        { "tab", 164 },
        { "tablist", 165 },
        { "tabpanel", 166 },
        { "canvas", 10 },
        { "tree", 179 },
        { "LabelText", 104 },
    };
    auto it = mapping.find(role);
    return it != mapping.end() ? it->second : 0;
}

bool axRoleSupportsNameFromContents(const std::string& role)
{
    return role == "button" || role == "cell" || role == "checkbox" ||
           role == "DisclosureTriangle" ||
           role == "DisclosureTriangleGrouped" || role == "columnheader" ||
           role == "gridcell" || role == "heading" || role == "link" ||
           role == "menuitem" || role == "menuitemcheckbox" ||
           role == "menuitemradio" || role == "option" || role == "radio" ||
           role == "row" || role == "rowheader" || role == "sectionhead" ||
           role == "switch" || role == "tab" || role == "tooltip" ||
           role == "treeitem";
}

bool axRoleIsNameProhibited(const std::string& role)
{
    return role == "caption" || role == "code" || role == "definition" ||
           role == "deletion" || role == "emphasis" || role == "generic" ||
           role == "insertion" || role == "mark" || role == "none" ||
           role == "paragraph" || role == "presentation" || role == "strong" ||
           role == "subscript" || role == "suggestion" ||
           role == "superscript" || role == "term" || role == "time";
}

} // namespace Starfish

#endif
