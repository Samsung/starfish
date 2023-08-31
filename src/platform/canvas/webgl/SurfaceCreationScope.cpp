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

#include "StarfishBase.h"
#include "platform/canvas/webgl/SurfaceCreationScope.h"

namespace Starfish {

std::shared_ptr<TextureCreationDelegate> SurfaceCreationScope::m_delegate;

SurfaceCreationScope::SurfaceCreationScope(
    std::shared_ptr<TextureCreationDelegate> delegate)
{
    STARFISH_ASSERT(m_delegate == nullptr);
    STARFISH_ASSERT(delegate != nullptr);
    m_delegate = delegate;
}

SurfaceCreationScope::~SurfaceCreationScope()
{
    m_delegate.reset();
}

} // namespace Starfish
