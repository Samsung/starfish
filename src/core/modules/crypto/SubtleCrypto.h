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

#ifndef __StarfishSubtleCrypto__
#define __StarfishSubtleCrypto__

#if defined(STARFISH_ENABLE_WEBCRYPTO)

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

#include <functional>

namespace Starfish {

// https://w3c.github.io/webcrypto/#subtlecrypto-interface
//
// Every method returns a Promise and never throws. Each method converts its
// arguments, normalizes the algorithm and checks the key inside one JS
// scope. An exception there rejects the Promise.
//
// Most operations finish before the method returns. RSA key generation and
// PBKDF2 are slow, so they run on the thread pool. Their Promises are kept in
// m_pendingPromises, a GC-traced list, until the result comes back. So a page
// that is gone also releases its pending Promises when its jobs end. See
// queue() for how the job and this object stay valid.
class SubtleCrypto : public ScriptWrappable {
public:
    static SubtleCrypto* create(ExecutionContext* executionContext)
    {
        return new SubtleCrypto(executionContext);
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SubtleCrypto)

    Promise* encrypt(ScriptValue algorithm, ScriptValue key, ScriptValue data);
    Promise* decrypt(ScriptValue algorithm, ScriptValue key, ScriptValue data);
    Promise* sign(ScriptValue algorithm, ScriptValue key, ScriptValue data);
    Promise* verify(ScriptValue algorithm, ScriptValue key,
                    ScriptValue signature, ScriptValue data);
    Promise* digest(ScriptValue algorithm, ScriptValue data);
    Promise* generateKey(ScriptValue algorithm, ScriptValue extractable,
                         ScriptValue keyUsages);
    Promise* deriveKey(ScriptValue algorithm, ScriptValue baseKey,
                       ScriptValue derivedKeyType, ScriptValue extractable,
                       ScriptValue keyUsages);
    Promise* deriveBits(ScriptValue algorithm, ScriptValue baseKey,
                        ScriptValue length = nullptr);
    Promise* importKey(ScriptValue format, ScriptValue keyData,
                       ScriptValue algorithm, ScriptValue extractable,
                       ScriptValue keyUsages);
    Promise* exportKey(ScriptValue format, ScriptValue key);
    Promise* wrapKey(ScriptValue format, ScriptValue key,
                     ScriptValue wrappingKey, ScriptValue wrapAlgorithm);
    Promise* unwrapKey(ScriptValue format, ScriptValue wrappedKey,
                       ScriptValue unwrappingKey, ScriptValue unwrapAlgorithm,
                       ScriptValue unwrappedKeyAlgorithm,
                       ScriptValue extractable, ScriptValue keyUsages);

    ExecutionContext* executionContext() const
    {
        return m_executionContext;
    }

    // Runs `work` on the thread pool, then `settle` on the JS thread. `work`
    // must not touch GC memory. `settle` builds the result; an exception in
    // it rejects `promise`.
    typedef std::function<ScriptValue(Escargot::ExecutionStateRef*)> Settle;
    void queue(Promise* promise, std::function<void()> work, Settle settle);

private:
    SubtleCrypto(ExecutionContext* executionContext)
        : ScriptWrappable(this)
        , m_executionContext(executionContext)
    {
    }

    // Creates a Promise and runs `body` in a new JS scope. `body` returns
    // the fulfillment value, or null when it queued the work.
    typedef std::function<ScriptValue(Escargot::ExecutionStateRef*, Promise*)>
        Body;
    Promise* run(const Body& body);

    struct PendingPromise {
        size_t id;
        Promise* promise;
    };

    ExecutionContext* m_executionContext;
    GCVector<PendingPromise> m_pendingPromises;
    size_t m_nextPendingId = 0;
};

} // namespace Starfish

#endif
#endif
