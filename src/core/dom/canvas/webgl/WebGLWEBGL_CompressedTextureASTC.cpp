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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLWEBGL_CompressedTextureASTC.h"
#include "core/dom/canvas/webgl/WebGLExtensions.h"
#include "core/util/String.h"

namespace Starfish {

WEBGL_compressed_texture_astc::WEBGL_compressed_texture_astc(
    ScriptBindingInstance* instance, WebGLRenderingContext*)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(instance)
{
}

// The extension is only exposed when the LDR profile is there; HDR adds to it.
// https://registry.khronos.org/webgl/extensions/WEBGL_compressed_texture_astc/
GCVector<String*> WEBGL_compressed_texture_astc::getSupportedProfiles()
{
    GCVector<String*> profiles;
    profiles.push_back(String::createASCIIString("ldr"));
    if (WebGLExtensionRegistry::instance().hasASTCHDRProfile()) {
        profiles.push_back(String::createASCIIString("hdr"));
    }
    return profiles;
}

} // namespace Starfish

#endif
