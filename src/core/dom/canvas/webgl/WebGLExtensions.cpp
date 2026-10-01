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
#include "core/util/debug/Trace.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <EscargotPublic.h>
#include "binding/ScriptBindingInstance.h"
#include "core/dom/canvas/webgl/WebGLOES_VertexArrayObject.h"
#include "core/dom/canvas/webgl/WebGLANGLE_InstancedArrays.h"

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
    if (raw == nullptr) {
        // Without a current context getString returns null, and the registry
        // would silently come up empty -- every extension reported as
        // unsupported. Say so instead of leaving it to guesswork.
        STARFISH_LOG_ERROR(
            "GL_EXTENSIONS is unavailable (no current GL context?); no WebGL "
            "extension will be reported as supported.");
    }
    const std::string extensions = raw ? raw : "";

    // WebGL uses extension names without the 'GL_' prefix.
    std::vector<std::string> tokens;
    std::stringstream ss(extensions);
    std::string token;
    while (getline(ss, token, ' ')) {
        // A trailing or doubled separator is legal in the extension string and
        // yields an empty token; it is not a malformed name.
        if (token.empty()) {
            continue;
        }
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

    // ES 3.0 / desktop GL 3.0 made several ES 2.0 extensions core, so their
    // token is gone from GL_EXTENSIONS while the functionality is there. The
    // WebGL extension must still be reported to WebGL 1 content.
    GLint majorVersion = 0;
    gl->getIntegerv(GL_MAJOR_VERSION, &majorVersion);
    while (gl->getError() != GL_NO_ERROR) {
        // GL_MAJOR_VERSION does not exist on ES 2.0; swallow the INVALID_ENUM
        // so that it is not reported to content as a WebGL error.
    }
    const bool isCoreProfile3 = majorVersion >= 3;
    const char* rawVersion =
        reinterpret_cast<const char*>(gl->getString(GL_VERSION));
    const std::string version = rawVersion ? rawVersion : "";
    const bool isDesktopGL = version.find("OpenGL ES") == std::string::npos;

    const auto has = [&glExtensions](const char* name) -> bool {
        return glExtensions.find(name) != glExtensions.end();
    };

    // 2. Add generators for extensions not requiring binding to native objects

#define SUPPORTED_GL_EXTENSIONS(V)                                           \
    V(OES_texture_float, OES_texture_float, AnyVersion, false)               \
    V(OES_texture_half_float, OES_texture_half_float, AnyVersion, false)     \
    V(OES_standard_derivatives, OES_standard_derivatives, AnyVersion, false) \
    V(OES_depth_texture, WEBGL_depth_texture, AnyVersion, false)             \
    V(EXT_texture_filter_anisotropic, EXT_texture_filter_anisotropic,        \
      AnyVersion, false)                                                     \
    V(OES_texture_float_linear, OES_texture_float_linear, AnyVersion, false) \
    V(EXT_blend_minmax, EXT_blend_minmax, AnyVersion, false)                 \
    V(OES_element_index_uint, OES_element_index_uint, WebGL1Only,            \
      isCoreProfile3)                                                        \
    V(EXT_color_buffer_float, EXT_color_buffer_float, WebGL2Only,            \
      (isDesktopGL && isCoreProfile3))

#define V(name, spec, availability, alsoSupportedWhen)                    \
    if (has(#name) || (alsoSupportedWhen)) {                              \
        m_interfaceGenerators[#spec] = ExtensionEntry{                    \
            [](ScriptBindingInstance* instance,                           \
               WebGLRenderingContext*) -> Escargot::ObjectRef* {          \
                return createScriptObject(instance, instance->fn##spec(), \
                                          #spec, nullptr);                \
            },                                                            \
            WebGLExtensionAvailability::availability                      \
        };                                                                \
    } else {                                                              \
        STARFISH_LOG_INFO("WebGL supports " #spec                         \
                          ", but this device does not. (Not an error.)"); \
    }
    SUPPORTED_GL_EXTENSIONS(V);
#undef V
#undef SUPPORTED_GL_EXTENSIONS

    // 3. Add generators for extensions bound with native objects

    // Instanced drawing is core in ES 3.0 / GL 3.1, and is spelled with three
    // different vendor prefixes before that.
    const bool hasInstancedArrays =
        isCoreProfile3 || has("ANGLE_instanced_arrays") ||
        has("EXT_instanced_arrays") || has("NV_instanced_arrays") ||
        has("ARB_instanced_arrays");

#define SUPPORTED_GL_EXTENSIONS(V)                \
    V(OES_vertex_array_object, AnyVersion, false) \
    V(ANGLE_instanced_arrays, WebGL1Only, hasInstancedArrays)

#define V(name, availability, alsoSupportedWhen)                           \
    if (has(#name) || (alsoSupportedWhen)) {                               \
        m_interfaceGenerators[#name] = ExtensionEntry{                     \
            [](ScriptBindingInstance* instance,                            \
               WebGLRenderingContext* glContext) -> Escargot::ObjectRef* { \
                return (new name(instance, glContext))                     \
                    ->scriptValue()                                        \
                    ->asObject();                                          \
            },                                                             \
            WebGLExtensionAvailability::availability                       \
        };                                                                 \
    } else {                                                               \
        STARFISH_LOG_WARN(#name " is not supported on this device");       \
    }
    SUPPORTED_GL_EXTENSIONS(V);
#undef V
#undef SUPPORTED_GL_EXTENSIONS

    m_hasEXT_texture_format_BGRA8888 =
        (extensions.find("GL_EXT_texture_format_BGRA8888") !=
         std::string::npos);

    m_isInitialized = true;
}

GCVector<String*> WebGLExtensionRegistry::getSupportedExtensions(bool isWebGL2)
{
    GCVector<String*> extentions;
    for (const auto& pair : m_interfaceGenerators) {
        if (!pair.second.isAvailableTo(isWebGL2)) {
            continue;
        }
        extentions.push_back(
            String::createASCIIString(pair.first.c_str(), pair.first.length()));
    }
    return extentions;
}

Optional<ExtensionGenerator> WebGLExtensionRegistry::getGenerator(
    const std::string& name, bool isWebGL2)
{
    const auto& iter = m_interfaceGenerators.find(name);
    if (iter != m_interfaceGenerators.end() &&
        iter->second.isAvailableTo(isWebGL2)) {
        return iter->second.generator;
    }
    return Optional<ExtensionGenerator>();
}

} // namespace Starfish

#endif
