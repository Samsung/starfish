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
class FunctionObjectRef;
};

namespace Starfish {

class ScriptBindingInstance;
class String;

using ExtensionGenerator =
    std::function<Escargot::FunctionObjectRef*(ScriptBindingInstance*)>;

class WebGLExtensionRegistry {
public:
    static WebGLExtensionRegistry& instance();

    Optional<ExtensionGenerator> getGenerator(const std::string& name);
    GCVector<String*> getSupportedExtensions();

    WebGLExtensionRegistry(const WebGLExtensionRegistry&) = delete;
    WebGLExtensionRegistry(const WebGLExtensionRegistry&&) = delete;
    WebGLExtensionRegistry& operator=(const WebGLExtensionRegistry&) = delete;
    WebGLExtensionRegistry& operator=(const WebGLExtensionRegistry&&) = delete;

private:
    WebGLExtensionRegistry();

    std::unordered_map<std::string, ExtensionGenerator, CaseInsensitiveHash,
                       CaseInsensitiveEqual>
        m_interfaceGenerators;
};

} // namespace Starfish

#endif
