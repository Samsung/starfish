/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAdoptedStyleSheets__
#define __StarfishAdoptedStyleSheets__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class Node;

// Shared implementation of the CSSOM `adoptedStyleSheets` observable array for
// both Document and ShadowRoot. `host` must be a Document or a ShadowRoot; the
// helper reads/writes the host's own backing list and cached proxy slot through
// the accessors those classes expose. Binding + data model only - no cascade.
namespace AdoptedStyleSheets {

    // Lazily create (and cache) the observable-array proxy for `host`.
    ScriptProxyObject observableArray(Escargot::ExecutionStateRef* state,
                                      Node* host);

    // Attribute setter: replace the backing list with the contents of `value`.
    void setFromObservableArray(Escargot::ExecutionStateRef* state, Node* host,
                                Escargot::ValueRef* value);

} // namespace AdoptedStyleSheets

} // namespace Starfish

#endif
