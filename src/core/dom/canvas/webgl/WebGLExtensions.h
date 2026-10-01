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

#ifndef __StarfishWebGLExtensions__
#define __StarfishWebGLExtensions__

#include <functional>
#include <string>
#include <unordered_map>
#include "core/dom/canvas/webgl/WebGLUtils.h"
#include "StarfishBase.h" // Optional, GCVector

namespace Escargot {
class ObjectRef;
}; // namespace Escargot

namespace Starfish {

class ScriptBindingInstance;
class String;
class WebGLRenderingContext;
class GL;

using ExtensionGenerator = std::function<Escargot::ObjectRef*(
    ScriptBindingInstance*, WebGLRenderingContext*)>;

// Some extensions are only defined for one context version: what they add is
// core in the other one (OES_element_index_uint, ANGLE_instanced_arrays) or
// does not exist there at all (EXT_color_buffer_float).
enum class WebGLExtensionAvailability {
    AnyVersion,
    WebGL1Only,
    WebGL2Only,
};

class WebGLExtensionRegistry {
public:
    static WebGLExtensionRegistry& instance();

    void initialize(GL* gl);
    bool isInitialized()
    {
        return m_isInitialized;
    }

    Optional<ExtensionGenerator> getGenerator(const std::string& name,
                                              bool isWebGL2);
    GCVector<String*> getSupportedExtensions(bool isWebGL2);

    WebGLExtensionRegistry(const WebGLExtensionRegistry&) = delete;
    WebGLExtensionRegistry(const WebGLExtensionRegistry&&) = delete;
    WebGLExtensionRegistry& operator=(const WebGLExtensionRegistry&) = delete;
    WebGLExtensionRegistry& operator=(const WebGLExtensionRegistry&&) = delete;

    bool hasEXT_texture_format_BGRA8888()
    {
        return m_hasEXT_texture_format_BGRA8888;
    }

    bool hasTextureCompressionExtension()
    {
        // NOTE: Currently verifiable targets don't support this feature.
        return false;
    }

private:
    WebGLExtensionRegistry();

    struct ExtensionEntry {
        ExtensionGenerator generator;
        WebGLExtensionAvailability availability;

        bool isAvailableTo(bool isWebGL2) const
        {
            switch (availability) {
            case WebGLExtensionAvailability::WebGL1Only:
                return !isWebGL2;
            case WebGLExtensionAvailability::WebGL2Only:
                return isWebGL2;
            default:
                return true;
            }
        }
    };

    std::unordered_map<std::string, ExtensionEntry, CaseInsensitiveHash,
                       CaseInsensitiveEqual>
        m_interfaceGenerators;

    bool m_hasEXT_texture_format_BGRA8888 = false;
    bool m_isInitialized = false;
};

} // namespace Starfish

#endif
