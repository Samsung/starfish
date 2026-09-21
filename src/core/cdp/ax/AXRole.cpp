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

std::string axRoleForElement(Element* element, const std::string& tag)
{
    std::string explicitRole = firstRoleToken(element);
    if (!explicitRole.empty()) {
        if (explicitRole == "none" || explicitRole == "presentation") {
            return std::string();
        }
        return explicitRole;
    }

    if (tag == "button") {
        return "button";
    }
    if (tag == "iframe") {
        return "Iframe";
    }
    if (tag == "a") {
        return element->hasAttribute(String::fromUTF8("href", 4)) ? "link"
                                                                  : "generic";
    }
    if (tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" ||
        tag == "h5" || tag == "h6") {
        return "heading";
    }
    if (tag == "img") {
        return "image";
    }
    if (tag == "svg") {
        return "image";
    }
    if (tag == "input") {
        std::string type = lowered(attributeOf(element, "type"));
        if (type == "checkbox") {
            return "checkbox";
        }
        if (type == "radio") {
            return "radio";
        }
        if (type == "button" || type == "submit" || type == "reset") {
            return "button";
        }
        if (type == "range") {
            return "slider";
        }
        if (type == "hidden") {
            return std::string();
        }
        // text, search, email, url, tel, password, number, "" ...
        return "textbox";
    }
    if (tag == "textarea") {
        return "textbox";
    }
    if (tag == "select") {
        return "combobox";
    }
    if (tag == "option") {
        return "option";
    }
    if (tag == "nav") {
        return "navigation";
    }
    if (tag == "main") {
        return "main";
    }
    if (tag == "article") {
        return "article";
    }
    if (tag == "section") {
        return "region";
    }
    if (tag == "header") {
        return "banner";
    }
    if (tag == "footer") {
        return "contentinfo";
    }
    if (tag == "aside") {
        return "complementary";
    }
    if (tag == "dialog") {
        return "dialog";
    }
    if (tag == "ul" || tag == "ol") {
        return "list";
    }
    if (tag == "li") {
        return "listitem";
    }
    if (tag == "p") {
        return "paragraph";
    }
    if (tag == "figure") {
        return "figure";
    }
    if (tag == "figcaption") {
        return "caption";
    }
    if (tag == "table") {
        return "table";
    }
    if (tag == "tr") {
        return "row";
    }
    if (tag == "td") {
        return "cell";
    }
    if (tag == "th") {
        return "columnheader";
    }
    if (tag == "form") {
        return "form";
    }
    if (tag == "canvas") {
        return "canvas";
    }
    return "generic";
}

int axChromeRoleForRole(const std::string& role)
{
    static const std::unordered_map<std::string, int> mapping = {
        { "RootWebArea", 144 },   { "article", 5 },
        { "main", 118 },          { "paragraph", 133 },
        { "heading", 96 },        { "button", 9 },
        { "link", 110 },          { "image", 99 },
        { "textbox", 170 },       { "StaticText", 158 },
        { "checkbox", 14 },       { "combobox", 209 },
        { "banner", 7 },          { "contentinfo", 26 },
        { "complementary", 22 },  { "dialog", 35 },
        { "alertDialog", 3 },     { "figure", 84 },
        { "caption", 11 },        { "generic", 88 },
        { "list", 111 },          { "listitem", 115 },
        { "table", 167 },         { "row", 145 },
        { "cell", 13 },           { "columnheader", 19 },
        { "form", 87 },           { "region", 143 },
        { "Iframe", 97 },         { "radio", 141 },
        { "radiogroup", 142 },    { "slider", 155 },
        { "menu", 122 },          { "menuitem", 124 },
        { "option", 113 },        { "alert", 2 },
        { "progressbar", 140 },   { "searchbox", 153 },
        { "spinbutton", 156 },    { "status", 159 },
        { "switch", 163 },        { "disclosureTriangle", 37 },
        { "treeitem", 180 },      { "navigation", 130 },
        { "InlineTextBox", 101 }, { "group", 93 },
        { "tab", 164 },           { "tablist", 165 },
        { "tabPanel", 166 },      { "canvas", 10 },
        { "tree", 179 },
    };
    auto it = mapping.find(role);
    return it != mapping.end() ? it->second : 0;
}

bool axRoleSupportsNameFromContents(const std::string& role)
{
    return role == "button" || role == "cell" || role == "checkbox" ||
           role == "columnheader" || role == "gridcell" || role == "heading" ||
           role == "link" || role == "menuitem" || role == "menuitemcheckbox" ||
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
