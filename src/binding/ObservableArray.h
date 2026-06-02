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

#ifndef __StarfishObservableArray__
#define __StarfishObservableArray__

#include "binding/ScriptWrappable.h"

namespace Starfish {

// Web IDL observable array
// (https://webidl.spec.whatwg.org/#js-observable-arrays).
//
// A reusable, element-type-agnostic helper that builds a Proxy-backed exotic
// array object. The native backing list owned by the host is the source of
// truth; the Proxy wraps a real Array object (so Array.isArray, Array.prototype
// methods and iteration work for free) and intercepts every mutation
// (defineProperty/set/deleteProperty) to validate and write it through to the
// host via the callbacks below. The element-specific validation/conversion is
// supplied by the host; this helper only drives the array mechanics.
//
// The callbacks below run inside a Proxy trap. On a rejected value they should
// throw a JS exception via state->throwException(...); throwing aborts the
// current operation before the Proxy mirrors anything onto the backing array.

// Validate+convert+store `value` at `index`. Returns false (or throws) to
// reject.
typedef bool (*ObservableArraySetIndexedValueCallback)(
    Escargot::ExecutionStateRef* state, ScriptWrappable* host, uint32_t index,
    Escargot::ValueRef* value);
// Remove the element at `index` from the backing list.
typedef bool (*ObservableArrayDeleteIndexedValueCallback)(
    Escargot::ExecutionStateRef* state, ScriptWrappable* host, uint32_t index);
// Convert the backing-list element at `index` to a JS value.
typedef Escargot::ValueRef* (*ObservableArrayGetIndexedValueCallback)(
    Escargot::ExecutionStateRef* state, ScriptWrappable* host, uint32_t index);
// Current length of the backing list.
typedef uint32_t (*ObservableArrayLengthCallback)(ScriptWrappable* host);
// Resize (truncate) the backing list to `newLength`.
typedef bool (*ObservableArraySetLengthCallback)(
    Escargot::ExecutionStateRef* state, ScriptWrappable* host,
    uint32_t newLength);

struct ObservableArrayCallbacks {
    ObservableArraySetIndexedValueCallback setIndexedValue;
    ObservableArrayDeleteIndexedValueCallback deleteIndexedValue;
    ObservableArrayGetIndexedValueCallback getIndexedValue;
    ObservableArrayLengthCallback length;
    ObservableArraySetLengthCallback setLength;
};

class ObservableArray {
public:
    // Build a Proxy-backed observable array bound to `host`. `callbacks` must
    // point to storage that outlives the proxy (a static const is ideal).
    static ScriptProxyObject create(Escargot::ExecutionStateRef* state,
                                    ScriptWrappable* host,
                                    const ObservableArrayCallbacks* callbacks);

    // Re-seed the proxy's backing Array from the host's native list. Call after
    // the host replaces its backing list wholesale (the attribute setter path).
    static void syncFromHost(Escargot::ExecutionStateRef* state,
                             ScriptProxyObject proxy);
};

} // namespace Starfish

#endif
