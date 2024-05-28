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
#include <unordered_set>
#include "core/modules/worker/util/Trace.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <EscargotPublic.h>
#include "binding/ScriptBindingInstance.h"
#include "core/dom/canvas/webgl/WebGLOES_VertexArrayObject.h"

#include "platform/canvas/gl/IncludeGL.h"
#include "platform/canvas/gl/GL.h"

namespace Starfish {

WebGLExtensionRegistry& WebGLExtensionRegistry::instance()
{
    static WebGLExtensionRegistry instance;
    return instance;
}

WebGLExtensionRegistry::WebGLExtensionRegistry()
{
}

void WebGLExtensionRegistry::initialize(GL* gl)
{
    // 1. Get a list of extensions supported on this device
    const char* raw =
        reinterpret_cast<const char*>(gl->getString(GL_EXTENSIONS));
    const std::string extensions = raw ? raw : "";

    // WebGL uses extension names without the 'GL_' prefix.
    std::vector<std::string> tokens;
    std::stringstream ss(extensions);
    std::string token;
    while (getline(ss, token, ' ')) {
        if (token.substr(0, 3) == "GL_") {
            tokens.push_back(token.substr(3));
        } else {
            STARFISH_ASSERT_NOT_REACHED();
        }
    }
#ifndef NDEBUG
    std::sort(tokens.begin(), tokens.end(),
              [](const std::string& a, const std::string& b) -> bool {
                  return a < b;
              });
#endif
#if defined(STARFISH_TIZEN)
    STARFISH_LOG_INFO("GL_EXTENSIONS =\n%s",
                      StringUtils::createAlignedString(tokens, 3).c_str());
#else
    TRACEF(WEBGL, "GL_EXTENSIONS =\n%s",
           StringUtils::createAlignedString(tokens, 3));
#endif
    std::unordered_set<std::string> glExtensions;
    for (const std::string& token : tokens) {
        glExtensions.emplace(token);
    }

    // 2. Add generators for extensions not requiring binding to native objects

#define SUPPORTED_GL_EXTENSIONS(V)                        \
    V(OES_texture_float, OES_texture_float)               \
    V(OES_texture_half_float, OES_texture_half_float)     \
    V(OES_standard_derivatives, OES_standard_derivatives) \
    V(OES_texture_float_linear, OES_texture_float_linear) \
    V(OES_depth_texture, WEBGL_depth_texture)             \
    V(EXT_blend_minmax, EXT_blend_minmax)

#define V(name, spec)                                                        \
    if (glExtensions.find(#name) != glExtensions.end()) {                    \
        m_interfaceGenerators[#spec] =                                       \
            [](ScriptBindingInstance* instance,                              \
               WebGLRenderingContext*) -> Escargot::ObjectRef* {             \
            return createScriptObject(instance, instance->fn##spec(), #spec, \
                                      nullptr);                              \
        };                                                                   \
    } else {                                                                 \
        STARFISH_LOG_INFO("WebGL supports " #spec                            \
                          ", but this device does not. (Not an error.)");    \
    }
    SUPPORTED_GL_EXTENSIONS(V);
#undef V
#undef SUPPORTED_GL_EXTENSIONS

    // 3. Add generators for extensions bound with native objects

#define SUPPORTED_GL_EXTENSIONS(V) V(OES_vertex_array_object)

#define V(name)                                                                \
    if (glExtensions.find(#name) != glExtensions.end()) {                      \
        m_interfaceGenerators[#name] =                                         \
            [](ScriptBindingInstance* instance,                                \
               WebGLRenderingContext* glContext) -> Escargot::ObjectRef* {     \
            return (new name(instance, glContext))->scriptValue()->asObject(); \
        };                                                                     \
    } else {                                                                   \
        STARFISH_LOG_WARN(#name " is not supported on this device");           \
    }
    SUPPORTED_GL_EXTENSIONS(V);
#undef V
#undef SUPPORTED_GL_EXTENSIONS

    m_hasEXT_texture_format_BGRA8888 =
        (extensions.find("GL_EXT_texture_format_BGRA8888") !=
         std::string::npos);

    m_isInitialized = true;
}

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
