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

#include "Escargot.h"
#include <EscargotPublic.h>

#include <random>

namespace Starfish {

ESCARGOT_EXPORT void reseedScriptEngineForTest()
{
    // Escargot's inexpensive time(NULL) seed and mt19937 generator are kept
    // for embedded startup and runtime performance. Changing the generator
    // or using OS randomness on each Math.random() call is too costly there.
    // Keep its native genrand_res53 path, which also avoids long-double math
    // and the associated embedded code-size cost.
    // Parallel WPT processes/workers initialized in the same second otherwise
    // produce identical Math.random() sequences: common/utils.js token()
    // then collides in the server stash used by request-cache tests.
    // Only test builds reseed the existing thread-local generator after
    // Escargot initialization, once per VM, with OS entropy. Both Window and
    // Worker VMs use this path; normal builds retain Escargot's original seed.
    std::random_device random;
    std::seed_seq seed{ random(), random(), random(), random() };
    Escargot::ThreadLocal::randEngine().seed(seed);
}

} // namespace Starfish
