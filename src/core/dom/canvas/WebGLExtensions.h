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
#include "core/dom/canvas/WebGLUtils.h"
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

class WebGLExtensionRegistry {
public:
    static WebGLExtensionRegistry& instance();

    void initialize(GL* gl);
    bool isInitialized()
    {
        return m_isInitialized;
    }

    Optional<ExtensionGenerator> getGenerator(const std::string& name);
    GCVector<String*> getSupportedExtensions();

    WebGLExtensionRegistry(const WebGLExtensionRegistry&) = delete;
    WebGLExtensionRegistry(const WebGLExtensionRegistry&&) = delete;
    WebGLExtensionRegistry& operator=(const WebGLExtensionRegistry&) = delete;
    WebGLExtensionRegistry& operator=(const WebGLExtensionRegistry&&) = delete;

    bool hasEXT_texture_format_BGRA8888()
    {
        return m_hasEXT_texture_format_BGRA8888;
    }

private:
    WebGLExtensionRegistry();

    std::unordered_map<std::string, ExtensionGenerator, CaseInsensitiveHash,
                       CaseInsensitiveEqual>
        m_interfaceGenerators;

    bool m_hasEXT_texture_format_BGRA8888 = false;
    bool m_isInitialized = false;
};

} // namespace Starfish

#endif
