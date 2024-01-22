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

#ifndef __StarfishWebGLRenderingContextState__
#define __StarfishWebGLRenderingContextState__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishBase.h"

namespace Starfish {

#define STATEFUL_VALUES(V)                                             \
    V(WebGLVertexArrayObjectOES, Nullable<WebGLVertexArrayObjectOES*>, \
      webGLVertexArrayObjectOES)

// Forward declarations
#define V(Constructor, _, __) class Constructor;
STATEFUL_VALUES(V);
#undef V

class WebGLRenderingContextState : public gc {
public:
    WebGLRenderingContextState();

// Define functions
#define V(Constructor, Type, MemberName) \
    DEFINE_GETTER(Type, MemberName);     \
    DEFINE_SETTER(Type, MemberName, Constructor);
    STATEFUL_VALUES(V);
#undef V

private:
    // Define variables
#define V(Constructor, Type, MemberName) Type m_##MemberName;
    STATEFUL_VALUES(V);
#undef V
};
} // namespace Starfish

#endif
#endif
