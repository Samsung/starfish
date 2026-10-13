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

#ifndef __StarfishSubtleCryptoKeys__
#define __StarfishSubtleCryptoKeys__

#if defined(STARFISH_ENABLE_WEBCRYPTO)

#include "core/modules/crypto/CryptoKey.h"

// Key import, export and generation for SubtleCrypto. Only SubtleCrypto.cpp
// uses this header.
namespace Starfish {

class SubtleCrypto;

enum class WebCryptoKeyFormat {
    Raw,
    Spki,
    Pkcs8,
    Jwk,
};

// A present JWK string member. `has` is false for an absent member.
struct JwkString {
    JwkString()
        : has(false)
    {
    }
    bool has;
    std::string value;
};

// The JsonWebKey dictionary. Only the members that WebCrypto reads.
// https://w3c.github.io/webcrypto/#JsonWebKey-dictionary
struct JsonWebKey {
    JsonWebKey()
        : hasExt(false)
        , ext(false)
        , hasKeyOps(false)
        , hasOth(false)
    {
    }
    JwkString alg, crv, d, dp, dq, e, k, kty, n, p, q, qi, use, x, y;
    bool hasExt;
    bool ext;
    bool hasKeyOps;
    std::vector<std::string> keyOps;
    bool hasOth;
};

// WebIDL enum conversion of KeyFormat. Throws a TypeError for other values.
WebCryptoKeyFormat toWebCryptoKeyFormat(Escargot::ExecutionStateRef* state,
                                        Escargot::ValueRef* value);

// WebIDL conversion of the JsonWebKey dictionary. Throws a TypeError.
JsonWebKey toJsonWebKey(Escargot::ExecutionStateRef* state,
                        Escargot::ValueRef* value);

// The "import key" steps of each algorithm, and the usage check that
// importKey() does after them. `bytes` is used for every format but jwk.
// Throws on failure.
CryptoKey* importWebCryptoKey(Escargot::ExecutionStateRef* state,
                              ExecutionContext* executionContext,
                              WebCryptoKeyFormat format,
                              const std::vector<uint8_t>& bytes,
                              const JsonWebKey& jwk,
                              const WebCryptoAlgorithm& algorithm,
                              bool extractable, WebCryptoKeyUsages usages);

// The "export key" steps. Returns an ArrayBuffer, or an object for jwk.
// Does not check [[extractable]]; the caller does.
Escargot::ValueRef* exportWebCryptoKey(Escargot::ExecutionStateRef* state,
                                       WebCryptoKeyFormat format,
                                       CryptoKey* key);

// The "generate key" steps. Returns a CryptoKey or a CryptoKeyPair object.
// Returns null when the work was queued on `subtle` for `promise`.
Escargot::ValueRef* generateWebCryptoKey(Escargot::ExecutionStateRef* state,
                                         SubtleCrypto* subtle, Promise* promise,
                                         const WebCryptoAlgorithm& algorithm,
                                         bool extractable,
                                         WebCryptoKeyUsages usages);

// Makes an ArrayBuffer with a copy of `bytes`.
Escargot::ValueRef* toWebCryptoArrayBuffer(Escargot::ExecutionStateRef* state,
                                           const std::vector<uint8_t>& bytes);

} // namespace Starfish

#endif
#endif
