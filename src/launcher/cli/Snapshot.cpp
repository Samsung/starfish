/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "Snapshot.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <cmath>

#include <vector>

namespace StarfishCLI {

namespace {

    std::string propertyValue(const rapidjson::Value& node,
                              const char* property)
    {
        if (!node.HasMember(property) || !node[property].IsObject()) {
            return std::string();
        }
        const rapidjson::Value& value = node[property];
        if (!value.HasMember("value") || !value["value"].IsString()) {
            return std::string();
        }
        return value["value"].GetString();
    }

    const rapidjson::Value* findProperty(const rapidjson::Value& node,
                                         const char* name)
    {
        if (!node.HasMember("properties") || !node["properties"].IsArray()) {
            return nullptr;
        }
        const rapidjson::Value& properties = node["properties"];
        for (rapidjson::SizeType index = 0; index < properties.Size();
             index++) {
            const rapidjson::Value& property = properties[index];
            if (property.IsObject() && property.HasMember("name") &&
                property["name"].IsString() &&
                std::string(property["name"].GetString()) == name &&
                property.HasMember("value") && property["value"].IsObject() &&
                property["value"].HasMember("value")) {
                return &property["value"]["value"];
            }
        }
        return nullptr;
    }

    bool booleanProperty(const rapidjson::Value& node, const char* name,
                         bool& result)
    {
        const rapidjson::Value* value = findProperty(node, name);
        if (!value || !value->IsBool()) {
            return false;
        }
        result = value->GetBool();
        return true;
    }

    // Returns the states in the order level, checked, expanded, selected,
    // disabled, required. The result has no brackets.
    std::string stateList(const rapidjson::Value& node)
    {
        std::vector<std::string> states;
        const rapidjson::Value* level = findProperty(node, "level");
        if (level && level->IsInt()) {
            states.push_back("level=" + std::to_string(level->GetInt()));
        }
        const rapidjson::Value* checked = findProperty(node, "checked");
        if (checked && checked->IsString()) {
            states.push_back(std::string("checked=") + checked->GetString());
        }
        bool flag = false;
        if (booleanProperty(node, "expanded", flag)) {
            states.push_back(flag ? "expanded=true" : "expanded=false");
        }
        if (booleanProperty(node, "selected", flag) && flag) {
            states.push_back("selected");
        }
        if (booleanProperty(node, "disabled", flag) && flag) {
            states.push_back("disabled");
        }
        if (booleanProperty(node, "required", flag) && flag) {
            states.push_back("required");
        }
        std::string joined;
        for (size_t index = 0; index < states.size(); index++) {
            joined += (index ? ", " : "") + states[index];
        }
        return joined;
    }

    // The value text of a string or number value. Booleans give no text.
    std::string valueText(const rapidjson::Value& node)
    {
        if (!node.HasMember("value") || !node["value"].IsObject() ||
            !node["value"].HasMember("value")) {
            return std::string();
        }
        const rapidjson::Value& value = node["value"]["value"];
        if (value.IsString()) {
            return value.GetString();
        }
        if (!value.IsNumber()) {
            return std::string();
        }
        if (value.IsDouble()) {
            const double number = value.GetDouble();
            if (std::floor(number) == number && std::fabs(number) < 1e15) {
                return std::to_string(static_cast<long long>(number));
            }
        }
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        value.Accept(writer);
        return buffer.GetString();
    }

    bool isInteractiveRole(const std::string& role)
    {
        static const char* const roles[] = {
            "button",     "link",   "textbox",  "combobox",  "listbox",
            "checkbox",   "radio",  "menuitem", "option",    "slider",
            "spinbutton", "switch", "tab",      "searchbox", nullptr
        };
        for (size_t index = 0; roles[index]; index++) {
            if (role == roles[index]) {
                return true;
            }
        }
        return false;
    }

    std::string escapeSnapshotText(const std::string& value)
    {
        static const char hexadecimal[] = "0123456789abcdef";
        std::string escaped;
        escaped.reserve(value.size());
        for (size_t index = 0; index < value.size(); index++) {
            unsigned char character = value[index];
            if (character == '"' || character == '\\') {
                escaped += '\\';
                escaped += static_cast<char>(character);
            } else if (character < 0x20 || character == 0x7f) {
                escaped += "\\u00";
                escaped += hexadecimal[character >> 4];
                escaped += hexadecimal[character & 0x0f];
            } else if (character == 0xc2 && index + 1 < value.size() &&
                       static_cast<unsigned char>(value[index + 1]) >= 0x80 &&
                       static_cast<unsigned char>(value[index + 1]) <= 0x9f) {
                unsigned char c1 = value[++index];
                escaped += "\\u00";
                escaped += hexadecimal[c1 >> 4];
                escaped += hexadecimal[c1 & 0x0f];
            } else {
                escaped += static_cast<char>(character);
            }
        }
        return escaped;
    }

} // namespace

bool formatInteractiveSnapshot(const std::string& axTree, std::string& output,
                               std::string& error, std::vector<int>& nodeIds,
                               std::vector<SnapshotElement>* elements)
{
    rapidjson::Document document;
    document.Parse(axTree.c_str());
    if (document.HasParseError() || !document.HasMember("nodes") ||
        !document["nodes"].IsArray()) {
        error = "unexpected Accessibility.getFullAXTree response";
        return false;
    }

    output.clear();
    nodeIds.clear();
    if (elements) {
        elements->clear();
    }
    size_t reference = 0;
    const rapidjson::Value& nodes = document["nodes"];
    for (rapidjson::SizeType index = 0; index < nodes.Size(); index++) {
        const rapidjson::Value& node = nodes[index];
        if (!node.IsObject() ||
            (node.HasMember("ignored") && node["ignored"].IsBool() &&
             node["ignored"].GetBool())) {
            continue;
        }

        std::string role = propertyValue(node, "role");
        if (!isInteractiveRole(role) || !node.HasMember("backendDOMNodeId") ||
            !node["backendDOMNodeId"].IsInt()) {
            continue;
        }

        reference++;
        nodeIds.push_back(node["backendDOMNodeId"].GetInt());
        output += "@e" + std::to_string(reference) + " [" + role + "]";
        std::string name = propertyValue(node, "name");
        if (elements) {
            elements->push_back({ role, name });
        }
        if (!name.empty()) {
            output += " \"" + escapeSnapshotText(name) + "\"";
        }
        const std::string states = stateList(node);
        if (!states.empty()) {
            output += " [" + states + "]";
        }
        const std::string value = valueText(node);
        if (!value.empty() && value != name) {
            output += ": " + escapeSnapshotText(value);
        }
        output += "\n";
    }
    return true;
}

} // namespace StarfishCLI
