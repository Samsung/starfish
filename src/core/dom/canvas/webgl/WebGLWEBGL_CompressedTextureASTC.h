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

#ifndef __StarfishWebGLWEBGL_CompressedTextureASTC__
#define __StarfishWebGLWEBGL_CompressedTextureASTC__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "core/util/GCDescriptor.h"
#include "StarfishBase.h" // GCVector

namespace Starfish {

class String;
class WebGLRenderingContext;

// References:
// binding/generated/WEBGL_compressed_texture_astcBinding.cpp

class WEBGL_compressed_texture_astc : public ScriptWrappable {
public:
    WEBGL_compressed_texture_astc(ScriptBindingInstance* instance,
                                  WebGLRenderingContext*);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWEBGL_compressed_texture_astc() const override;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    GCVector<String*> getSupportedProfiles();

    BEGIN_IMPLEMENT_NEW_WITH_GC_DESC(WEBGL_compressed_texture_astc);
    FILL_GC_POINTER(WEBGL_compressed_texture_astc, m_object); // ScriptWrappable
    FILL_GC_POINTER(WEBGL_compressed_texture_astc, m_scriptBindingInstance);
    END_IMPLEMENT_NEW_WITH_GC_DESC();

private:
    ScriptBindingInstance* m_scriptBindingInstance;
};

} // namespace Starfish

#endif

#endif
