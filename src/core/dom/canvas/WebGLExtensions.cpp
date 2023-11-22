/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "WebGLExtensions.h"
#include "core/util/String.h"
#include "core/util/String.h"
#include <unordered_set>
#include "platform/canvas/webgl/GLES.h" // glGetString
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

namespace Starfish {

#define SUPPORTED_GL_EXTENSIONS(V) \
    V(OES_texture_float)           \
    V(OES_texture_half_float)

#define V(name)                                 \
    Escargot::FunctionObjectRef* binding##name( \
        ScriptBindingInstance* scriptBindingInstance);

SUPPORTED_GL_EXTENSIONS(V);
#undef V

WebGLExtensionRegistry& WebGLExtensionRegistry::instance()
{
    static WebGLExtensionRegistry instance;
    return instance;
}

WebGLExtensionRegistry::WebGLExtensionRegistry()
{
    // 1. Get a list of extensions supported by this device
    const std::string rawString =
        reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));

    std::vector<std::string> tokens;
    StringUtils::split(rawString, ' ', tokens);
    STARFISH_LOG_DEBUG("GL_EXTENSIONS =\n%s",
                       StringUtils::createAlignedString(tokens, 3).c_str());

    std::unordered_set<std::string> glExtensions;
    for (const std::string& token : tokens) {
        glExtensions.emplace(token);
    }

    // 2. Register only extensions that have the interface binding implemented.
#define V(name)                                                      \
    if (glExtensions.find("GL_" #name) != glExtensions.end()) {      \
        m_interfaceGenerators[#name] = binding##name;                \
    } else {                                                         \
        STARFISH_LOG_WARN(#name " is not supported by this device"); \
    }
    SUPPORTED_GL_EXTENSIONS(V);
#undef V
}

#undef SUPPORTED_GL_EXTENSIONS

GCVector<String*> WebGLExtensionRegistry::getSupportedExtensions()
{
    GCVector<String*> extentions;
    for (const auto& pair : m_interfaceGenerators) {
        extentions.push_back(
            String::createASCIIString(pair.first.c_str(), pair.first.length()));
    }
    return extentions;
}

Optional<ExtensionGenerator> WebGLExtensionRegistry::getGenerator(
    const std::string& name)
{
    const auto& iter = m_interfaceGenerators.find(name);
    if (iter != m_interfaceGenerators.end()) {
        return iter->second;
    }
    return Optional<ExtensionGenerator>();
}

} // namespace Starfish

#endif
