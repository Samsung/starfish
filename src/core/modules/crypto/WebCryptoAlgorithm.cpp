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

// This file uses only Escargot and PlatformCrypto. See WebCryptoAlgorithm.h.
#include "StarfishConfig.h"

#if defined(STARFISH_ENABLE_WEBCRYPTO)

#include "core/modules/crypto/WebCryptoAlgorithm.h"

#include <EscargotPublic.h>

#include <cmath>
#include <cstring>

using namespace Escargot;

namespace Starfish {

namespace {

    // The IDL dictionary that an algorithm converts to for an operation.
    // https://w3c.github.io/webcrypto/#algorithm-dictionary-members
    enum class Dictionary {
        Algorithm,
        AesCbcParams,
        AesCtrParams,
        AesGcmParams,
        AesKeyGenParams,
        AesDerivedKeyParams,
        HmacKeyGenParams,
        HmacImportParams,
        RsaHashedKeyGenParams,
        RsaHashedImportParams,
        RsaPssParams,
        RsaOaepParams,
        EcdsaParams,
        EcKeyGenParams,
        EcKeyImportParams,
        EcdhKeyDeriveParams,
        HkdfParams,
        Pbkdf2Params,
    };

    struct Registration {
        WebCryptoOperation operation;
        WebCryptoAlgorithmName name;
        Dictionary dictionary;
    };

    typedef WebCryptoOperation Op;
    typedef WebCryptoAlgorithmName Name;
    typedef Dictionary Dict;

    // The supported algorithms. This is the part of the spec registry that
    // Starfish implements.
    const Registration kRegistrations[] = {
        { Op::Encrypt, Name::RsaOaep, Dict::RsaOaepParams },
        { Op::Encrypt, Name::AesCtr, Dict::AesCtrParams },
        { Op::Encrypt, Name::AesCbc, Dict::AesCbcParams },
        { Op::Encrypt, Name::AesGcm, Dict::AesGcmParams },
        { Op::Decrypt, Name::RsaOaep, Dict::RsaOaepParams },
        { Op::Decrypt, Name::AesCtr, Dict::AesCtrParams },
        { Op::Decrypt, Name::AesCbc, Dict::AesCbcParams },
        { Op::Decrypt, Name::AesGcm, Dict::AesGcmParams },
        { Op::Sign, Name::RsassaPkcs1v15, Dict::Algorithm },
        { Op::Sign, Name::RsaPss, Dict::RsaPssParams },
        { Op::Sign, Name::Ecdsa, Dict::EcdsaParams },
        { Op::Sign, Name::Ed25519, Dict::Algorithm },
        { Op::Sign, Name::Hmac, Dict::Algorithm },
        { Op::Verify, Name::RsassaPkcs1v15, Dict::Algorithm },
        { Op::Verify, Name::RsaPss, Dict::RsaPssParams },
        { Op::Verify, Name::Ecdsa, Dict::EcdsaParams },
        { Op::Verify, Name::Ed25519, Dict::Algorithm },
        { Op::Verify, Name::Hmac, Dict::Algorithm },
        { Op::Digest, Name::Sha1, Dict::Algorithm },
        { Op::Digest, Name::Sha256, Dict::Algorithm },
        { Op::Digest, Name::Sha384, Dict::Algorithm },
        { Op::Digest, Name::Sha512, Dict::Algorithm },
        { Op::GenerateKey, Name::RsassaPkcs1v15, Dict::RsaHashedKeyGenParams },
        { Op::GenerateKey, Name::RsaPss, Dict::RsaHashedKeyGenParams },
        { Op::GenerateKey, Name::RsaOaep, Dict::RsaHashedKeyGenParams },
        { Op::GenerateKey, Name::Ecdsa, Dict::EcKeyGenParams },
        { Op::GenerateKey, Name::Ecdh, Dict::EcKeyGenParams },
        { Op::GenerateKey, Name::Ed25519, Dict::Algorithm },
        { Op::GenerateKey, Name::X25519, Dict::Algorithm },
        { Op::GenerateKey, Name::Hmac, Dict::HmacKeyGenParams },
        { Op::GenerateKey, Name::AesCtr, Dict::AesKeyGenParams },
        { Op::GenerateKey, Name::AesCbc, Dict::AesKeyGenParams },
        { Op::GenerateKey, Name::AesGcm, Dict::AesKeyGenParams },
        { Op::GenerateKey, Name::AesKw, Dict::AesKeyGenParams },
        { Op::DeriveBits, Name::Ecdh, Dict::EcdhKeyDeriveParams },
        { Op::DeriveBits, Name::X25519, Dict::EcdhKeyDeriveParams },
        { Op::DeriveBits, Name::Hkdf, Dict::HkdfParams },
        { Op::DeriveBits, Name::Pbkdf2, Dict::Pbkdf2Params },
        { Op::ImportKey, Name::RsassaPkcs1v15, Dict::RsaHashedImportParams },
        { Op::ImportKey, Name::RsaPss, Dict::RsaHashedImportParams },
        { Op::ImportKey, Name::RsaOaep, Dict::RsaHashedImportParams },
        { Op::ImportKey, Name::Ecdsa, Dict::EcKeyImportParams },
        { Op::ImportKey, Name::Ecdh, Dict::EcKeyImportParams },
        { Op::ImportKey, Name::Ed25519, Dict::Algorithm },
        { Op::ImportKey, Name::X25519, Dict::Algorithm },
        { Op::ImportKey, Name::Hmac, Dict::HmacImportParams },
        { Op::ImportKey, Name::AesCtr, Dict::Algorithm },
        { Op::ImportKey, Name::AesCbc, Dict::Algorithm },
        { Op::ImportKey, Name::AesGcm, Dict::Algorithm },
        { Op::ImportKey, Name::AesKw, Dict::Algorithm },
        { Op::ImportKey, Name::Hkdf, Dict::Algorithm },
        { Op::ImportKey, Name::Pbkdf2, Dict::Algorithm },
        { Op::WrapKey, Name::AesKw, Dict::Algorithm },
        { Op::UnwrapKey, Name::AesKw, Dict::Algorithm },
        { Op::GetKeyLength, Name::AesCtr, Dict::AesDerivedKeyParams },
        { Op::GetKeyLength, Name::AesCbc, Dict::AesDerivedKeyParams },
        { Op::GetKeyLength, Name::AesGcm, Dict::AesDerivedKeyParams },
        { Op::GetKeyLength, Name::AesKw, Dict::AesDerivedKeyParams },
        { Op::GetKeyLength, Name::Hmac, Dict::HmacImportParams },
        { Op::GetKeyLength, Name::Hkdf, Dict::Algorithm },
        { Op::GetKeyLength, Name::Pbkdf2, Dict::Algorithm },
    };

    // The spec compares names with ASCII case-insensitive matching. Other
    // characters, such as U+212A KELVIN SIGN, must match exactly.
    bool equalsIgnoringAsciiCase(const std::string& a, const char* b)
    {
        size_t i = 0;
        for (; i < a.size() && b[i]; i++) {
            char x = a[i];
            char y = b[i];
            if (x >= 'a' && x <= 'z') {
                x = static_cast<char>(x - 'a' + 'A');
            }
            if (y >= 'a' && y <= 'z') {
                y = static_cast<char>(y - 'a' + 'A');
            }
            if (x != y) {
                return false;
            }
        }
        return i == a.size() && !b[i];
    }

    ValueRef* getMember(ExecutionStateRef* state, ObjectRef* object,
                        const char* name)
    {
        // A member of an absent dictionary is undefined.
        if (!object) {
            return ValueRef::createUndefined();
        }
        return object->get(state,
                           StringRef::createFromASCII(name, strlen(name)));
    }

    ValueRef* getRequiredMember(ExecutionStateRef* state, ObjectRef* object,
                                const char* name)
    {
        ValueRef* value = getMember(state, object, name);
        if (value->isUndefined()) {
            std::string message =
                std::string("Algorithm: ") + name + " is required.";
            throwWebCryptoTypeError(state, message.c_str());
        }
        return value;
    }

    // [EnforceRange] conversion of an integer type with the given maximum.
    // https://webidl.spec.whatwg.org/#abstract-opdef-converttoint
    uint32_t toEnforcedInteger(ExecutionStateRef* state, ValueRef* value,
                               double maximum, const char* name)
    {
        double number = value->toNumber(state);
        if (std::isnan(number) || std::isinf(number)) {
            std::string message =
                std::string("Algorithm: ") + name + " is not a finite number.";
            throwWebCryptoTypeError(state, message.c_str());
        }
        number = std::trunc(number);
        if (number < 0 || number > maximum) {
            std::string message =
                std::string("Algorithm: ") + name + " is out of range.";
            throwWebCryptoTypeError(state, message.c_str());
        }
        return static_cast<uint32_t>(number);
    }

    const double kOctetMax = 255;
    const double kUnsignedShortMax = 65535;
    const double kUnsignedLongMax = 4294967295.0;

    void readBufferSource(ExecutionStateRef* state, ObjectRef* object,
                          const char* name, bool isRequired,
                          std::vector<uint8_t>* out, bool* has = nullptr)
    {
        ValueRef* value = isRequired ? getRequiredMember(state, object, name)
                                     : getMember(state, object, name);
        if (value->isUndefined()) {
            return;
        }
        copyWebCryptoBufferSource(state, value, out);
        if (has) {
            *has = true;
        }
    }

    // Converts the Algorithm dictionary and finds the registered name.
    // Returns the dictionary object, which is null for a string algorithm.
    ObjectRef* toAlgorithmName(ExecutionStateRef* state,
                               WebCryptoOperation operation, ValueRef* value,
                               Name* name, Dictionary* dictionary)
    {
        // 1. A string algorithm is the dictionary { name: alg }.
        ObjectRef* object = nullptr;
        std::string nameString;
        if (value->isString()) {
            nameString = value->asString()->toStdUTF8String();
        } else {
            // 2. Convert to the Algorithm dictionary. undefined and null give
            // an empty dictionary, so the required name is missing.
            if (!value->isUndefinedOrNull()) {
                if (!value->isObject()) {
                    throwWebCryptoTypeError(state, "Algorithm: not an object.");
                }
                object = value->asObject();
            }
            ValueRef* nameValue = getRequiredMember(state, object, "name");
            nameString = nameValue->toString(state)->toStdUTF8String();
        }

        // 3. Find the name in the registry for the operation.
        for (const Registration& registration : kRegistrations) {
            if (registration.operation == operation &&
                equalsIgnoringAsciiCase(
                    nameString,
                    webCryptoAlgorithmNameString(registration.name))) {
                *name = registration.name;
                *dictionary = registration.dictionary;
                return object;
            }
        }
        std::string message = "Unrecognized algorithm name: " + nameString;
        throwWebCryptoError(state, WebCryptoError::NotSupported,
                            message.c_str());
        return nullptr;
    }

    void readHash(ExecutionStateRef* state, ObjectRef* object,
                  WebCryptoAlgorithm* algorithm)
    {
        // HashAlgorithmIdentifier is normalized with the digest operation.
        ValueRef* value = getRequiredMember(state, object, "hash");
        WebCryptoAlgorithm hash = normalizeWebCryptoAlgorithm(
            state, WebCryptoOperation::Digest, value);
        algorithm->hash = toCryptoHash(hash.name);
        algorithm->hasHash = true;
    }

} // namespace

const char* webCryptoAlgorithmNameString(WebCryptoAlgorithmName name)
{
    switch (name) {
    case Name::AesCbc:
        return "AES-CBC";
    case Name::AesCtr:
        return "AES-CTR";
    case Name::AesGcm:
        return "AES-GCM";
    case Name::AesKw:
        return "AES-KW";
    case Name::Hmac:
        return "HMAC";
    case Name::RsassaPkcs1v15:
        return "RSASSA-PKCS1-v1_5";
    case Name::RsaPss:
        return "RSA-PSS";
    case Name::RsaOaep:
        return "RSA-OAEP";
    case Name::Ecdsa:
        return "ECDSA";
    case Name::Ecdh:
        return "ECDH";
    case Name::Ed25519:
        return "Ed25519";
    case Name::X25519:
        return "X25519";
    case Name::Hkdf:
        return "HKDF";
    case Name::Pbkdf2:
        return "PBKDF2";
    case Name::Sha1:
        return "SHA-1";
    case Name::Sha256:
        return "SHA-256";
    case Name::Sha384:
        return "SHA-384";
    case Name::Sha512:
        return "SHA-512";
    }
    return "";
}

const char* webCryptoKeyUsageString(WebCryptoKeyUsage usage)
{
    switch (usage) {
    case WebCryptoKeyUsageEncrypt:
        return "encrypt";
    case WebCryptoKeyUsageDecrypt:
        return "decrypt";
    case WebCryptoKeyUsageSign:
        return "sign";
    case WebCryptoKeyUsageVerify:
        return "verify";
    case WebCryptoKeyUsageDeriveKey:
        return "deriveKey";
    case WebCryptoKeyUsageDeriveBits:
        return "deriveBits";
    case WebCryptoKeyUsageWrapKey:
        return "wrapKey";
    case WebCryptoKeyUsageUnwrapKey:
        return "unwrapKey";
    }
    return "";
}

CryptoHash toCryptoHash(WebCryptoAlgorithmName name)
{
    switch (name) {
    case Name::Sha1:
        return CryptoHash::SHA1;
    case Name::Sha384:
        return CryptoHash::SHA384;
    case Name::Sha512:
        return CryptoHash::SHA512;
    default:
        return CryptoHash::SHA256;
    }
}

WebCryptoAlgorithmName toWebCryptoHashName(CryptoHash hash)
{
    switch (hash) {
    case CryptoHash::SHA1:
        return Name::Sha1;
    case CryptoHash::SHA256:
        return Name::Sha256;
    case CryptoHash::SHA384:
        return Name::Sha384;
    case CryptoHash::SHA512:
        return Name::Sha512;
    }
    return Name::Sha256;
}

void throwWebCryptoTypeError(ExecutionStateRef* state, const char* message)
{
    state->throwException(TypeErrorObjectRef::create(
        state, StringRef::createFromUTF8(message, strlen(message))));
}

void copyWebCryptoBufferSource(ExecutionStateRef* state, ValueRef* value,
                               std::vector<uint8_t>* out)
{
    // BufferSource is (ArrayBufferView or ArrayBuffer). SharedArrayBuffer
    // is not allowed. A detached buffer holds no bytes ("get a copy of the
    // bytes held by the buffer source"). Escargot throws when the view of a
    // detached buffer is read, so check first.
    if (value->isArrayBufferView()) {
        ArrayBufferViewRef* view = value->asArrayBufferView();
        ArrayBufferRef* buffer = view->buffer();
        if (buffer->isSharedArrayBufferObject()) {
            throwWebCryptoTypeError(state, "A shared buffer is not allowed.");
        }
        if (buffer->isArrayBufferObject() &&
            buffer->asArrayBufferObject()->isDetachedBuffer()) {
            out->clear();
            return;
        }
        const uint8_t* data = view->rawBuffer();
        out->assign(data, data ? data + view->byteLength() : data);
        return;
    }
    if (value->isArrayBufferObject()) {
        ArrayBufferObjectRef* buffer = value->asArrayBufferObject();
        if (buffer->isDetachedBuffer()) {
            out->clear();
            return;
        }
        const uint8_t* data = buffer->rawBuffer();
        out->assign(data, data ? data + buffer->byteLength() : data);
        return;
    }
    throwWebCryptoTypeError(state, "Not an ArrayBuffer or ArrayBufferView.");
}

WebCryptoKeyUsages toWebCryptoKeyUsages(ExecutionStateRef* state,
                                        ValueRef* value)
{
    // https://webidl.spec.whatwg.org/#es-sequence
    if (!value->isObject()) {
        throwWebCryptoTypeError(state, "Key usages: not a sequence.");
    }
    ValueRef* method = value->asObject()->get(
        state, state->context()->vmInstance()->iteratorSymbol());
    if (!method->isCallable()) {
        throwWebCryptoTypeError(state, "Key usages: not iterable.");
    }
    ValueRef* iterator = method->call(state, value, 0, nullptr);
    if (!iterator->isObject()) {
        throwWebCryptoTypeError(state, "Key usages: bad iterator.");
    }
    ValueRef* next =
        iterator->asObject()->get(state, StringRef::createFromASCII("next"));

    WebCryptoKeyUsages usages = 0;
    while (true) {
        ValueRef* result = next->call(state, iterator, 0, nullptr);
        if (!result->isObject()) {
            throwWebCryptoTypeError(state, "Key usages: bad iterator result.");
        }
        if (result->asObject()
                ->get(state, StringRef::createFromASCII("done"))
                ->toBoolean(state)) {
            return usages;
        }
        std::string usage =
            result->asObject()
                ->get(state, StringRef::createFromASCII("value"))
                ->toString(state)
                ->toStdUTF8String();
        WebCryptoKeyUsages bit = 0;
        for (unsigned i = 0; i < 8; i++) {
            WebCryptoKeyUsage candidate =
                static_cast<WebCryptoKeyUsage>(1 << i);
            if (usage == webCryptoKeyUsageString(candidate)) {
                bit = candidate;
            }
        }
        if (!bit) {
            std::string message =
                "Key usages: '" + usage + "' is not a valid KeyUsage.";
            throwWebCryptoTypeError(state, message.c_str());
        }
        usages |= bit;
    }
}

WebCryptoAlgorithm normalizeWebCryptoAlgorithm(ExecutionStateRef* state,
                                               WebCryptoOperation operation,
                                               ValueRef* value)
{
    WebCryptoAlgorithm algorithm;
    Dictionary dictionary;
    ObjectRef* object =
        toAlgorithmName(state, operation, value, &algorithm.name, &dictionary);

    // 4. Convert to the dictionary of the algorithm. The own members are
    // read in lexicographic order. Every getter call is visible to the page,
    // so this order matters.
    // The spec converts the whole dictionary again here, which reads `name`
    // a second time. Browsers read it once, and WPT depends on that: the
    // "transferred during call" tests detach a buffer inside the `name`
    // getter, and a second call would throw. So `name` is not read again.

    switch (dictionary) {
    case Dict::Algorithm:
        break;
    case Dict::AesCbcParams:
        readBufferSource(state, object, "iv", true, &algorithm.iv);
        break;
    case Dict::AesCtrParams:
        readBufferSource(state, object, "counter", true, &algorithm.counter);
        algorithm.length =
            toEnforcedInteger(state, getRequiredMember(state, object, "length"),
                              kOctetMax, "length");
        algorithm.hasLength = true;
        break;
    case Dict::AesGcmParams: {
        readBufferSource(state, object, "additionalData", false,
                         &algorithm.additionalData,
                         &algorithm.hasAdditionalData);
        readBufferSource(state, object, "iv", true, &algorithm.iv);
        ValueRef* tagLength = getMember(state, object, "tagLength");
        if (!tagLength->isUndefined()) {
            algorithm.tagLength =
                toEnforcedInteger(state, tagLength, kOctetMax, "tagLength");
        }
        break;
    }
    case Dict::AesKeyGenParams:
    case Dict::AesDerivedKeyParams:
        algorithm.length =
            toEnforcedInteger(state, getRequiredMember(state, object, "length"),
                              kUnsignedShortMax, "length");
        algorithm.hasLength = true;
        break;
    case Dict::HmacKeyGenParams:
    case Dict::HmacImportParams: {
        readHash(state, object, &algorithm);
        ValueRef* length = getMember(state, object, "length");
        if (!length->isUndefined()) {
            algorithm.length =
                toEnforcedInteger(state, length, kUnsignedLongMax, "length");
            algorithm.hasLength = true;
        }
        break;
    }
    case Dict::RsaHashedKeyGenParams: {
        // RsaKeyGenParams members come before the own member hash.
        algorithm.modulusLength = toEnforcedInteger(
            state, getRequiredMember(state, object, "modulusLength"),
            kUnsignedLongMax, "modulusLength");
        ValueRef* exponent = getRequiredMember(state, object, "publicExponent");
        // BigInteger is a typedef of Uint8Array.
        if (!exponent->isUint8ArrayObject()) {
            throwWebCryptoTypeError(
                state, "Algorithm: publicExponent is not a Uint8Array.");
        }
        copyWebCryptoBufferSource(state, exponent, &algorithm.publicExponent);
        readHash(state, object, &algorithm);
        break;
    }
    case Dict::RsaHashedImportParams:
    case Dict::EcdsaParams:
        readHash(state, object, &algorithm);
        break;
    case Dict::RsaPssParams:
        algorithm.saltLength = toEnforcedInteger(
            state, getRequiredMember(state, object, "saltLength"),
            kUnsignedLongMax, "saltLength");
        break;
    case Dict::RsaOaepParams:
        readBufferSource(state, object, "label", false, &algorithm.label);
        break;
    case Dict::EcKeyGenParams:
    case Dict::EcKeyImportParams:
        algorithm.namedCurve = getRequiredMember(state, object, "namedCurve")
                                   ->toString(state)
                                   ->toStdUTF8String();
        break;
    case Dict::EcdhKeyDeriveParams: {
        ValueRef* publicKey = getRequiredMember(state, object, "public");
        if (!isWebCryptoKeyValue(publicKey)) {
            throwWebCryptoTypeError(state,
                                    "Algorithm: public is not a CryptoKey.");
        }
        algorithm.publicKey = publicKey;
        break;
    }
    case Dict::HkdfParams:
        readHash(state, object, &algorithm);
        readBufferSource(state, object, "info", true, &algorithm.info);
        readBufferSource(state, object, "salt", true, &algorithm.salt);
        break;
    case Dict::Pbkdf2Params:
        readHash(state, object, &algorithm);
        algorithm.iterations = toEnforcedInteger(
            state, getRequiredMember(state, object, "iterations"),
            kUnsignedLongMax, "iterations");
        readBufferSource(state, object, "salt", true, &algorithm.salt);
        break;
    }
    return algorithm;
}

} // namespace Starfish

#endif
