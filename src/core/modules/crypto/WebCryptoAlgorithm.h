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

#ifndef __StarfishWebCryptoAlgorithm__
#define __StarfishWebCryptoAlgorithm__

#include "platform/crypto/PlatformCrypto.h"

#include <string>
#include <vector>

namespace Escargot {
class ExecutionStateRef;
class ValueRef;
} // namespace Escargot

// WebIDL conversion and "normalize an algorithm" for WebCrypto.
// https://w3c.github.io/webcrypto/#algorithm-normalization-normalize-an-algorithm
//
// This code uses only the Escargot API and PlatformCrypto. The host (Starfish)
// provides the two functions at the end of this file. So another Escargot
// host can use this file with its own versions of them.
//
// Errors are thrown into the ExecutionStateRef, like any JS exception. The
// caller runs this code inside Evaluator::execute() and turns the exception
// into a rejected Promise.
namespace Starfish {

enum class WebCryptoAlgorithmName {
    AesCbc,
    AesCtr,
    AesGcm,
    AesKw,
    Hmac,
    RsassaPkcs1v15,
    RsaPss,
    RsaOaep,
    Ecdsa,
    Ecdh,
    Ed25519,
    X25519,
    Hkdf,
    Pbkdf2,
    Sha1,
    Sha256,
    Sha384,
    Sha512,
};

// The registered name, for example "AES-GCM".
const char* webCryptoAlgorithmNameString(WebCryptoAlgorithmName name);

// The operations that algorithms register for. GetKeyLength is the internal
// "get key length" operation of deriveKey.
enum class WebCryptoOperation {
    Encrypt,
    Decrypt,
    Sign,
    Verify,
    Digest,
    GenerateKey,
    DeriveBits,
    ImportKey,
    WrapKey,
    UnwrapKey,
    GetKeyLength,
};

// The DOMException names that WebCrypto uses.
enum class WebCryptoError {
    NotSupported,
    Syntax,
    InvalidAccess,
    Data,
    Operation,
};

// KeyUsage values as bits. The order is the order of the KeyUsage enum.
enum WebCryptoKeyUsage {
    WebCryptoKeyUsageEncrypt = 1 << 0,
    WebCryptoKeyUsageDecrypt = 1 << 1,
    WebCryptoKeyUsageSign = 1 << 2,
    WebCryptoKeyUsageVerify = 1 << 3,
    WebCryptoKeyUsageDeriveKey = 1 << 4,
    WebCryptoKeyUsageDeriveBits = 1 << 5,
    WebCryptoKeyUsageWrapKey = 1 << 6,
    WebCryptoKeyUsageUnwrapKey = 1 << 7,
};
typedef unsigned WebCryptoKeyUsages;

// The KeyUsage string of one bit.
const char* webCryptoKeyUsageString(WebCryptoKeyUsage usage);

// A normalized algorithm. Which members are set depends on the name and
// the operation. BufferSource members hold a copy of the bytes, made when
// the algorithm is normalized.
struct WebCryptoAlgorithm {
    WebCryptoAlgorithm()
        : name(WebCryptoAlgorithmName::Sha256)
        , hash(CryptoHash::SHA256)
        , hasHash(false)
        , hasAdditionalData(false)
        , length(0)
        , hasLength(false)
        , tagLength(128)
        , saltLength(0)
        , iterations(0)
        , modulusLength(0)
        , publicKey(nullptr)
    {
    }

    WebCryptoAlgorithmName name;
    CryptoHash hash;
    bool hasHash;
    std::vector<uint8_t> iv;
    std::vector<uint8_t> counter;
    std::vector<uint8_t> additionalData;
    bool hasAdditionalData;
    std::vector<uint8_t> label;
    std::vector<uint8_t> salt;
    std::vector<uint8_t> info;
    std::vector<uint8_t> publicExponent;
    uint32_t length;
    bool hasLength;
    uint32_t tagLength;
    uint32_t saltLength;
    uint32_t iterations;
    uint32_t modulusLength;
    // Not checked against the supported curves. The operation does that,
    // because an unknown curve is a NotSupportedError there.
    std::string namedCurve;
    // EcdhKeyDeriveParams.public. The host checked that it is a CryptoKey.
    Escargot::ValueRef* publicKey;
};

// Normalizes `value` for `operation`. Throws a TypeError when `value` does
// not convert to the dictionary of the algorithm, and a NotSupportedError
// when the name is not registered for the operation.
WebCryptoAlgorithm normalizeWebCryptoAlgorithm(
    Escargot::ExecutionStateRef* state, WebCryptoOperation operation,
    Escargot::ValueRef* value);

// Converts a sequence<KeyUsage>. Throws a TypeError for a value that is not
// a sequence, or for a string that is not a KeyUsage.
WebCryptoKeyUsages toWebCryptoKeyUsages(Escargot::ExecutionStateRef* state,
                                        Escargot::ValueRef* value);

// Copies the bytes of a BufferSource. Throws a TypeError for other values.
// A detached buffer gives no bytes.
void copyWebCryptoBufferSource(Escargot::ExecutionStateRef* state,
                               Escargot::ValueRef* value,
                               std::vector<uint8_t>* out);

// Converts a normalized hash algorithm name to a CryptoHash.
CryptoHash toCryptoHash(WebCryptoAlgorithmName name);
WebCryptoAlgorithmName toWebCryptoHashName(CryptoHash hash);

// Throws a TypeError into `state`. Does not return.
void throwWebCryptoTypeError(Escargot::ExecutionStateRef* state,
                             const char* message);

// Provided by the host.
// Throws a DOMException into `state`. Does not return.
void throwWebCryptoError(Escargot::ExecutionStateRef* state,
                         WebCryptoError error, const char* message);
// True when `value` is a CryptoKey object of the host.
bool isWebCryptoKeyValue(Escargot::ValueRef* value);

} // namespace Starfish

#endif
