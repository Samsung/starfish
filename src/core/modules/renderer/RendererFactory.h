/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRendererFactory__
#define __StarfishRendererFactory__

namespace Starfish {

class Renderer;
class Starfish;

namespace RendererFactory {
#if !defined(STARFISH_EFL_HEADLESS)
    Renderer* createSoftware(Starfish* starfish, uint32_t width,
                             uint32_t height);
    Renderer* createGL(Starfish* starfish, uint32_t width, uint32_t height);
#endif
#if defined(STARFISH_EFL_HEADLESS)
    Renderer* createHeadless(Starfish* starfish, uint32_t width,
                             uint32_t height);
#endif
}; // namespace RendererFactory
} // namespace Starfish

#endif
