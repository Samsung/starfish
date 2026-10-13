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

#include "StarfishConfig.h"

#if defined(STARFISH_ENABLE_WEBCRYPTO)

#include "core/modules/crypto/SubtleCrypto.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/modules/crypto/SubtleCryptoKeys.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/page/WebBase.h"

#include <EscargotPublic.h>

#include <cstring>

using namespace Escargot;

namespace Starfish {

// Host functions of WebCryptoAlgorithm.h.

void throwWebCryptoError(ExecutionStateRef* state, WebCryptoError error,
                         const char* message)
{
    const char* name = "OperationError";
    switch (error) {
    case WebCryptoError::NotSupported:
        name = "NotSupportedError";
        break;
    case WebCryptoError::Syntax:
        name = "SyntaxError";
        break;
    case WebCryptoError::InvalidAccess:
        name = "InvalidAccessError";
        break;
    case WebCryptoError::Data:
        name = "DataError";
        break;
    case WebCryptoError::Operation:
        break;
    }
    DOMException* exception =
        new DOMException(fetchExecutionContext(state->context()),
                         String::fromUTF8(message, strlen(message)),
                         String::fromUTF8(name, strlen(name)));
    state->throwException(exception->scriptValue());
}

bool isWebCryptoKeyValue(ValueRef* value)
{
    ScriptWrappable* wrappable = toScriptWrappable(value);
    return wrappable && wrappable->isCryptoKey();
}

namespace {

    typedef WebCryptoAlgorithmName Name;
    typedef WebCryptoOperation Op;

    // WebIDL conversion of a CryptoKey argument.
    CryptoKey* toCryptoKey(ExecutionStateRef* state, ValueRef* value,
                           const char* name)
    {
        if (!isWebCryptoKeyValue(value)) {
            std::string message = std::string(name) + " is not a CryptoKey.";
            throwWebCryptoTypeError(state, message.c_str());
        }
        return toScriptWrappable(value)->asCryptoKey();
    }

    // WebIDL conversion of a BufferSource argument. The bytes are copied
    // later, after the algorithm is normalized, as the spec orders it.
    void checkBufferSource(ExecutionStateRef* state, ValueRef* value,
                           const char* name)
    {
        if (!value->isArrayBufferView() && !value->isArrayBufferObject()) {
            std::string message = std::string(name) +
                                  " is not an ArrayBuffer or ArrayBufferView.";
            throwWebCryptoTypeError(state, message.c_str());
        }
    }

    std::vector<uint8_t> copyBytes(ExecutionStateRef* state, ValueRef* value)
    {
        std::vector<uint8_t> bytes;
        copyWebCryptoBufferSource(state, value, &bytes);
        return bytes;
    }

    // The checks that encrypt, sign and the others do before the operation.
    void checkKey(ExecutionStateRef* state, const WebCryptoAlgorithm& algorithm,
                  CryptoKey* key, WebCryptoKeyUsage usage)
    {
        if (algorithm.name != key->keyAlgorithm().name) {
            throwWebCryptoError(state, WebCryptoError::InvalidAccess,
                                "The key is for a different algorithm.");
        }
        if (!key->hasUsage(usage)) {
            throwWebCryptoError(state, WebCryptoError::InvalidAccess,
                                "The key does not allow this operation.");
        }
    }

    void checkKeyType(ExecutionStateRef* state, CryptoKey* key,
                      CryptoKey::Type type)
    {
        if (key->keyType() != type) {
            throwWebCryptoError(state, WebCryptoError::InvalidAccess,
                                "The key type is not valid here.");
        }
    }

    void checkResult(ExecutionStateRef* state, bool ok)
    {
        if (!ok) {
            throwWebCryptoError(state, WebCryptoError::Operation,
                                "The operation failed.");
        }
    }

    // The encrypt and decrypt steps of each algorithm. wrapKey and unwrapKey
    // use them too.
    std::vector<uint8_t> crypt(ExecutionStateRef* state, bool isEncrypt,
                               const WebCryptoAlgorithm& algorithm,
                               CryptoKey* key, const std::vector<uint8_t>& data)
    {
        std::vector<uint8_t> out;
        switch (algorithm.name) {
        case Name::AesCbc:
            if (algorithm.iv.size() != 16) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "AES-CBC iv must be 16 bytes.");
            }
            checkResult(state,
                        isEncrypt ? PlatformCrypto::aesCbcEncrypt(
                                        key->secret(), algorithm.iv, data, &out)
                                  : PlatformCrypto::aesCbcDecrypt(key->secret(),
                                                                  algorithm.iv,
                                                                  data, &out));
            break;
        case Name::AesCtr:
            if (algorithm.counter.size() != 16) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "AES-CTR counter must be 16 bytes.");
            }
            if (algorithm.length == 0 || algorithm.length > 128) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "AES-CTR length must be 1 to 128.");
            }
            checkResult(state,
                        PlatformCrypto::aesCtr(key->secret(), algorithm.counter,
                                               algorithm.length, data, &out));
            break;
        case Name::AesGcm: {
            uint32_t tagLength = algorithm.tagLength;
            if (tagLength != 32 && tagLength != 64 &&
                (tagLength < 96 || tagLength > 128 || tagLength % 8 != 0)) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "AES-GCM tagLength is not valid.");
            }
            if (!isEncrypt && data.size() < tagLength / 8) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "The data is shorter than the tag.");
            }
            CryptoBytes additionalData;
            if (algorithm.hasAdditionalData) {
                additionalData = algorithm.additionalData;
            }
            checkResult(state,
                        isEncrypt ? PlatformCrypto::aesGcmEncrypt(
                                        key->secret(), algorithm.iv,
                                        additionalData, tagLength, data, &out)
                                  : PlatformCrypto::aesGcmDecrypt(
                                        key->secret(), algorithm.iv,
                                        additionalData, tagLength, data, &out));
            break;
        }
        case Name::RsaOaep:
            checkKeyType(state, key,
                         isEncrypt ? CryptoKey::Type::Public
                                   : CryptoKey::Type::Private);
            checkResult(
                state, isEncrypt
                           ? PlatformCrypto::rsaOaepEncrypt(
                                 key->asymmetricKey(), key->keyAlgorithm().hash,
                                 algorithm.label, data, &out)
                           : PlatformCrypto::rsaOaepDecrypt(
                                 key->asymmetricKey(), key->keyAlgorithm().hash,
                                 algorithm.label, data, &out));
            break;
        default:
            throwWebCryptoError(state, WebCryptoError::NotSupported,
                                "The algorithm cannot encrypt.");
        }
        return out;
    }

    // Normalizes for wrapKey, or for encrypt when the algorithm has no
    // wrapKey operation. The spec tries the first and uses the second on
    // any error, so the first exception is caught here.
    WebCryptoAlgorithm normalizeWrapAlgorithm(ExecutionStateRef* state,
                                              Op wrapOperation,
                                              Op cryptOperation,
                                              ValueRef* value)
    {
        struct Data {
            Op operation;
            ValueRef* value;
            WebCryptoAlgorithm result;
        } data = { wrapOperation, value, WebCryptoAlgorithm() };
        Evaluator::EvaluatorResult result = Evaluator::execute(
            state->context(),
            [](ExecutionStateRef* state, Data* data) -> ValueRef* {
                data->result = normalizeWebCryptoAlgorithm(
                    state, data->operation, data->value);
                return ValueRef::createUndefined();
            },
            &data);
        if (result.isSuccessful()) {
            return data.result;
        }
        return normalizeWebCryptoAlgorithm(state, cryptOperation, value);
    }

    ValueRef* jsonCall(ExecutionStateRef* state, const char* method,
                       ValueRef* argument)
    {
        // The spec asks for the JSON functions of a new global object. This
        // uses the current realm, so a page that replaces window.JSON
        // changes the result.
        ValueRef* json = state->context()->globalObject()->get(
            state, StringRef::createFromASCII("JSON"));
        ValueRef* function = json->asObject()->get(
            state, StringRef::createFromASCII(method, strlen(method)));
        return function->call(state, json, 1, &argument);
    }

    // The "get key length" steps of deriveKey. Returns false for null.
    bool keyLength(ExecutionStateRef* state,
                   const WebCryptoAlgorithm& algorithm, uint32_t* length)
    {
        switch (algorithm.name) {
        case Name::AesCbc:
        case Name::AesCtr:
        case Name::AesGcm:
        case Name::AesKw:
            if (algorithm.length != 128 && algorithm.length != 192 &&
                algorithm.length != 256) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "AES key length must be 128, 192 or 256.");
            }
            *length = algorithm.length;
            return true;
        case Name::Hmac:
            if (algorithm.hasLength) {
                if (algorithm.length == 0) {
                    throwWebCryptoTypeError(
                        state, "The HMAC key length must not be zero.");
                }
                *length = algorithm.length;
            } else {
                *length = (algorithm.hash == CryptoHash::SHA384 ||
                           algorithm.hash == CryptoHash::SHA512)
                              ? 1024
                              : 512;
            }
            return true;
        default:
            return false;
        }
    }

    // Keeps `length` bits of `bytes`. The unused low bits of the last byte
    // become zero.
    bool truncateBits(std::vector<uint8_t>* bytes, uint32_t length)
    {
        if (length > bytes->size() * 8) {
            return false;
        }
        bytes->resize((length + 7) / 8);
        if (length % 8 != 0) {
            bytes->back() &= static_cast<uint8_t>(0xff << (8 - length % 8));
        }
        return true;
    }

    // Derived bits in a plain computation. It runs on the JS thread, or on
    // the thread pool for PBKDF2.
    typedef std::function<bool(std::vector<uint8_t>*)> DeriveBitsWork;

    // The "derive bits" steps of each algorithm up to the computation. The
    // checks throw here; a failed computation returns false.
    DeriveBitsWork prepareDeriveBits(ExecutionStateRef* state,
                                     const WebCryptoAlgorithm& algorithm,
                                     CryptoKey* baseKey, bool hasLength,
                                     uint32_t length, bool* isSlow)
    {
        *isSlow = false;
        switch (algorithm.name) {
        case Name::Ecdh:
        case Name::X25519: {
            checkKeyType(state, baseKey, CryptoKey::Type::Private);
            CryptoKey* publicKey =
                toScriptWrappable(algorithm.publicKey)->asCryptoKey();
            checkKeyType(state, publicKey, CryptoKey::Type::Public);
            if (publicKey->keyAlgorithm().name !=
                baseKey->keyAlgorithm().name) {
                throwWebCryptoError(state, WebCryptoError::InvalidAccess,
                                    "The public key is for another algorithm.");
            }
            if (publicKey->keyAlgorithm().namedCurve !=
                baseKey->keyAlgorithm().namedCurve) {
                throwWebCryptoError(state, WebCryptoError::InvalidAccess,
                                    "The public key is on another curve.");
            }
            AsymmetricKey privateKey = baseKey->asymmetricKey();
            AsymmetricKey peerKey = publicKey->asymmetricKey();
            return [privateKey, peerKey, hasLength,
                    length](std::vector<uint8_t>* out) {
                return PlatformCrypto::deriveSharedSecret(privateKey, peerKey,
                                                          out) &&
                       (!hasLength || truncateBits(out, length));
            };
        }
        case Name::Hkdf:
        case Name::Pbkdf2: {
            if (!hasLength || length % 8 != 0) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "The length must be a multiple of 8.");
            }
            if (algorithm.name == Name::Pbkdf2 && algorithm.iterations == 0) {
                throwWebCryptoError(state, WebCryptoError::Operation,
                                    "PBKDF2 iterations must not be zero.");
            }
            std::vector<uint8_t> secret = baseKey->secret();
            WebCryptoAlgorithm params = algorithm;
            *isSlow = algorithm.name == Name::Pbkdf2;
            return [secret, params, length](std::vector<uint8_t>* out) {
                if (params.name == Name::Hkdf) {
                    return PlatformCrypto::hkdf(params.hash, secret,
                                                params.salt, params.info,
                                                length / 8, out);
                }
                return PlatformCrypto::pbkdf2(params.hash, secret, params.salt,
                                              params.iterations, length / 8,
                                              out);
            };
        }
        default:
            throwWebCryptoError(state, WebCryptoError::NotSupported,
                                "The algorithm cannot derive bits.");
        }
        return DeriveBitsWork();
    }

} // namespace

ScriptBindingInstance* SubtleCrypto::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

Promise* SubtleCrypto::run(const Body& body)
{
    Promise* promise = new Promise(scriptBindingInstance());
    struct Data {
        const Body* body;
        Promise* promise;
        ScriptValue value;
    } data = { &body, promise, nullptr };
    Evaluator::EvaluatorResult result = Evaluator::execute(
        scriptBindingInstance()->scriptContext(),
        [](ExecutionStateRef* state, Data* data) -> ValueRef* {
            data->value = (*data->body)(state, data->promise);
            return ValueRef::createUndefined();
        },
        &data);
    if (!result.isSuccessful()) {
        promise->reject(result.error.value());
    } else if (data.value) {
        promise->fulfill(data.value);
    }
    return promise;
}

void SubtleCrypto::queue(Promise* promise, std::function<void()> work,
                         Settle settle)
{
    size_t id = m_nextPendingId++;
    m_pendingPromises.push_back(PendingPromise{ id, promise });

    // The job is plain memory, so the worker thread can use it. Only the JS
    // thread reads its GC pointers. The GC does not scan the job, so `this`
    // must stay alive by another path:
    // - ThreadPool keeps its work data as a GC root until after this idler
    //   runs. That data points to the execution context, which reaches
    //   `this` through crypto.subtle.
    // - If the page is torn down after the idler is posted, the message
    //   loop drops the idler and calls the discard callback. It frees the
    //   job and the input bytes. The Promise is freed with the page.
    // - If the page is torn down before the work ends, the idler still
    //   runs and settles the Promise of the old page. The old page stays
    //   in memory until then.
    struct Job {
        std::function<void()> work;
        Settle settle;
        SubtleCrypto* subtle;
        size_t id;
        MessageLoop* messageLoop;
        GlobalScope* globalScope;
    };
    WebBase* webBase = m_executionContext->webBase();
    Job* job = new Job{ std::move(work),
                        std::move(settle),
                        this,
                        id,
                        webBase->messageLoop(),
                        m_executionContext->globalScope() };
    webBase->threadPool()->addWork(
        m_executionContext,
        [](void* data) -> void* {
            Job* job = static_cast<Job*>(data);
            job->work();
            job->messageLoop->addIdlerWithNoGCRootingInOtherThread(
                job->globalScope,
                [](size_t, void* data) {
                    Job* job = static_cast<Job*>(data);
                    SubtleCrypto* subtle = job->subtle;
                    Promise* promise = nullptr;
                    for (size_t i = 0; i < subtle->m_pendingPromises.size();
                         i++) {
                        if (subtle->m_pendingPromises[i].id == job->id) {
                            promise = subtle->m_pendingPromises[i].promise;
                            subtle->m_pendingPromises.erase(
                                subtle->m_pendingPromises.begin() + i);
                            break;
                        }
                    }
                    if (promise) {
                        struct Data {
                            Settle* settle;
                            ScriptValue value;
                        } data = { &job->settle, nullptr };
                        Evaluator::EvaluatorResult result = Evaluator::execute(
                            subtle->scriptBindingInstance()->scriptContext(),
                            [](ExecutionStateRef* state,
                               Data* data) -> ValueRef* {
                                data->value = (*data->settle)(state);
                                return ValueRef::createUndefined();
                            },
                            &data);
                        if (result.isSuccessful()) {
                            promise->fulfill(data.value);
                        } else {
                            promise->reject(result.error.value());
                        }
                    }
                    delete job;
                },
                [](void* data) { delete static_cast<Job*>(data); }, job);
            return nullptr;
        },
        job);
}

Promise* SubtleCrypto::encrypt(ScriptValue algorithm, ScriptValue key,
                               ScriptValue data)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        CryptoKey* cryptoKey = toCryptoKey(state, key, "key");
        checkBufferSource(state, data, "data");
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::Encrypt, algorithm);
        std::vector<uint8_t> bytes = copyBytes(state, data);
        checkKey(state, normalized, cryptoKey, WebCryptoKeyUsageEncrypt);
        return toWebCryptoArrayBuffer(
            state, crypt(state, true, normalized, cryptoKey, bytes));
    });
}

Promise* SubtleCrypto::decrypt(ScriptValue algorithm, ScriptValue key,
                               ScriptValue data)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        CryptoKey* cryptoKey = toCryptoKey(state, key, "key");
        checkBufferSource(state, data, "data");
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::Decrypt, algorithm);
        std::vector<uint8_t> bytes = copyBytes(state, data);
        checkKey(state, normalized, cryptoKey, WebCryptoKeyUsageDecrypt);
        return toWebCryptoArrayBuffer(
            state, crypt(state, false, normalized, cryptoKey, bytes));
    });
}

Promise* SubtleCrypto::sign(ScriptValue algorithm, ScriptValue key,
                            ScriptValue data)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        CryptoKey* cryptoKey = toCryptoKey(state, key, "key");
        checkBufferSource(state, data, "data");
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::Sign, algorithm);
        std::vector<uint8_t> bytes = copyBytes(state, data);
        checkKey(state, normalized, cryptoKey, WebCryptoKeyUsageSign);
        std::vector<uint8_t> out;
        const CryptoKeyAlgorithm& keyAlgorithm = cryptoKey->keyAlgorithm();
        switch (normalized.name) {
        case Name::Hmac:
            checkResult(state, PlatformCrypto::hmacSign(keyAlgorithm.hash,
                                                        cryptoKey->secret(),
                                                        bytes, &out));
            break;
        case Name::RsassaPkcs1v15:
        case Name::RsaPss:
            checkKeyType(state, cryptoKey, CryptoKey::Type::Private);
            checkResult(state, PlatformCrypto::rsaSign(
                                   cryptoKey->asymmetricKey(),
                                   normalized.name == Name::RsaPss
                                       ? RsaSignaturePadding::PSS
                                       : RsaSignaturePadding::PKCS1v15,
                                   keyAlgorithm.hash, normalized.saltLength,
                                   bytes, &out));
            break;
        case Name::Ecdsa:
            checkKeyType(state, cryptoKey, CryptoKey::Type::Private);
            checkResult(
                state, PlatformCrypto::ecdsaSign(cryptoKey->asymmetricKey(),
                                                 normalized.hash, bytes, &out));
            break;
        default:
            checkKeyType(state, cryptoKey, CryptoKey::Type::Private);
            checkResult(state, PlatformCrypto::ed25519Sign(
                                   cryptoKey->asymmetricKey(), bytes, &out));
            break;
        }
        return toWebCryptoArrayBuffer(state, out);
    });
}

Promise* SubtleCrypto::verify(ScriptValue algorithm, ScriptValue key,
                              ScriptValue signature, ScriptValue data)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        CryptoKey* cryptoKey = toCryptoKey(state, key, "key");
        checkBufferSource(state, signature, "signature");
        checkBufferSource(state, data, "data");
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::Verify, algorithm);
        std::vector<uint8_t> signatureBytes = copyBytes(state, signature);
        std::vector<uint8_t> bytes = copyBytes(state, data);
        checkKey(state, normalized, cryptoKey, WebCryptoKeyUsageVerify);
        const CryptoKeyAlgorithm& keyAlgorithm = cryptoKey->keyAlgorithm();
        bool verified = false;
        switch (normalized.name) {
        case Name::Hmac:
            verified = PlatformCrypto::hmacVerify(
                keyAlgorithm.hash, cryptoKey->secret(), bytes, signatureBytes);
            break;
        case Name::RsassaPkcs1v15:
        case Name::RsaPss:
            checkKeyType(state, cryptoKey, CryptoKey::Type::Public);
            verified = PlatformCrypto::rsaVerify(
                cryptoKey->asymmetricKey(),
                normalized.name == Name::RsaPss ? RsaSignaturePadding::PSS
                                                : RsaSignaturePadding::PKCS1v15,
                keyAlgorithm.hash, normalized.saltLength, bytes,
                signatureBytes);
            break;
        case Name::Ecdsa:
            checkKeyType(state, cryptoKey, CryptoKey::Type::Public);
            verified = PlatformCrypto::ecdsaVerify(cryptoKey->asymmetricKey(),
                                                   normalized.hash, bytes,
                                                   signatureBytes);
            break;
        default:
            checkKeyType(state, cryptoKey, CryptoKey::Type::Public);
            verified = PlatformCrypto::ed25519Verify(cryptoKey->asymmetricKey(),
                                                     bytes, signatureBytes);
            break;
        }
        return ValueRef::create(verified);
    });
}

Promise* SubtleCrypto::digest(ScriptValue algorithm, ScriptValue data)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        checkBufferSource(state, data, "data");
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::Digest, algorithm);
        std::vector<uint8_t> bytes = copyBytes(state, data);
        std::vector<uint8_t> out;
        checkResult(state, PlatformCrypto::digest(toCryptoHash(normalized.name),
                                                  bytes, &out));
        return toWebCryptoArrayBuffer(state, out);
    });
}

Promise* SubtleCrypto::generateKey(ScriptValue algorithm,
                                   ScriptValue extractable,
                                   ScriptValue keyUsages)
{
    return run([&](ExecutionStateRef* state, Promise* promise) -> ScriptValue {
        bool isExtractable = extractable->toBoolean(state);
        WebCryptoKeyUsages usages = toWebCryptoKeyUsages(state, keyUsages);
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::GenerateKey, algorithm);
        return generateWebCryptoKey(state, this, promise, normalized,
                                    isExtractable, usages);
    });
}

Promise* SubtleCrypto::deriveBits(ScriptValue algorithm, ScriptValue baseKey,
                                  ScriptValue length)
{
    return run([&](ExecutionStateRef* state, Promise* promise) -> ScriptValue {
        CryptoKey* key = toCryptoKey(state, baseKey, "baseKey");
        // `optional unsigned long? length = null`.
        bool hasLength = length && !length->isUndefinedOrNull();
        uint32_t bits = hasLength ? length->toUint32(state) : 0;
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::DeriveBits, algorithm);
        checkKey(state, normalized, key, WebCryptoKeyUsageDeriveBits);
        bool isSlow = false;
        DeriveBitsWork derive =
            prepareDeriveBits(state, normalized, key, hasLength, bits, &isSlow);
        if (!isSlow) {
            std::vector<uint8_t> out;
            checkResult(state, derive(&out));
            return toWebCryptoArrayBuffer(state, out);
        }
        auto out = std::make_shared<std::vector<uint8_t>>();
        auto ok = std::make_shared<bool>(false);
        queue(
            promise, [derive, out, ok]() { *ok = derive(out.get()); },
            [out, ok](ExecutionStateRef* state) -> ScriptValue {
                checkResult(state, *ok);
                return toWebCryptoArrayBuffer(state, *out);
            });
        return nullptr;
    });
}

Promise* SubtleCrypto::deriveKey(ScriptValue algorithm, ScriptValue baseKey,
                                 ScriptValue derivedKeyType,
                                 ScriptValue extractable, ScriptValue keyUsages)
{
    return run([&](ExecutionStateRef* state, Promise* promise) -> ScriptValue {
        CryptoKey* key = toCryptoKey(state, baseKey, "baseKey");
        bool isExtractable = extractable->toBoolean(state);
        WebCryptoKeyUsages usages = toWebCryptoKeyUsages(state, keyUsages);
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::DeriveBits, algorithm);
        WebCryptoAlgorithm importAlgorithm =
            normalizeWebCryptoAlgorithm(state, Op::ImportKey, derivedKeyType);
        WebCryptoAlgorithm lengthAlgorithm = normalizeWebCryptoAlgorithm(
            state, Op::GetKeyLength, derivedKeyType);
        checkKey(state, normalized, key, WebCryptoKeyUsageDeriveKey);
        uint32_t length = 0;
        bool hasLength = keyLength(state, lengthAlgorithm, &length);
        bool isSlow = false;
        DeriveBitsWork derive = prepareDeriveBits(state, normalized, key,
                                                  hasLength, length, &isSlow);

        ExecutionContext* context = m_executionContext;
        auto importDerived = [context, importAlgorithm, isExtractable,
                              usages](ExecutionStateRef* state,
                                      const std::vector<uint8_t>& bits) {
            return importWebCryptoKey(state, context, WebCryptoKeyFormat::Raw,
                                      bits, JsonWebKey(), importAlgorithm,
                                      isExtractable, usages)
                ->scriptValue();
        };
        if (!isSlow) {
            std::vector<uint8_t> bits;
            checkResult(state, derive(&bits));
            ScriptValue result = importDerived(state, bits);
            PlatformCrypto::cleanse(bits.data(), bits.size());
            return result;
        }
        auto bits = std::make_shared<std::vector<uint8_t>>();
        auto ok = std::make_shared<bool>(false);
        queue(
            promise, [derive, bits, ok]() { *ok = derive(bits.get()); },
            [bits, ok, importDerived](ExecutionStateRef* state) -> ScriptValue {
                checkResult(state, *ok);
                ScriptValue result = importDerived(state, *bits);
                PlatformCrypto::cleanse(bits->data(), bits->size());
                return result;
            });
        return nullptr;
    });
}

Promise* SubtleCrypto::importKey(ScriptValue format, ScriptValue keyData,
                                 ScriptValue algorithm, ScriptValue extractable,
                                 ScriptValue keyUsages)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        WebCryptoKeyFormat keyFormat = toWebCryptoKeyFormat(state, format);
        // keyData is (BufferSource or JsonWebKey). A buffer is the first
        // type. Other objects, undefined and null are the dictionary.
        bool isBuffer =
            keyData->isArrayBufferView() || keyData->isArrayBufferObject();
        if (!isBuffer && !keyData->isObject() &&
            !keyData->isUndefinedOrNull()) {
            throwWebCryptoTypeError(
                state, "keyData is not a BufferSource or JsonWebKey.");
        }
        JsonWebKey jwk;
        if (!isBuffer) {
            jwk = toJsonWebKey(state, keyData);
        }
        bool isExtractable = extractable->toBoolean(state);
        WebCryptoKeyUsages usages = toWebCryptoKeyUsages(state, keyUsages);
        WebCryptoAlgorithm normalized =
            normalizeWebCryptoAlgorithm(state, Op::ImportKey, algorithm);
        std::vector<uint8_t> bytes;
        if (keyFormat == WebCryptoKeyFormat::Jwk) {
            if (isBuffer) {
                throwWebCryptoTypeError(state, "keyData is not a JsonWebKey.");
            }
        } else {
            if (!isBuffer) {
                throwWebCryptoTypeError(state,
                                        "keyData is not a BufferSource.");
            }
            bytes = copyBytes(state, keyData);
        }
        CryptoKey* key =
            importWebCryptoKey(state, m_executionContext, keyFormat, bytes, jwk,
                               normalized, isExtractable, usages);
        if (!bytes.empty()) {
            PlatformCrypto::cleanse(bytes.data(), bytes.size());
        }
        return key->scriptValue();
    });
}

Promise* SubtleCrypto::exportKey(ScriptValue format, ScriptValue key)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        WebCryptoKeyFormat keyFormat = toWebCryptoKeyFormat(state, format);
        CryptoKey* cryptoKey = toCryptoKey(state, key, "key");
        Name name = cryptoKey->keyAlgorithm().name;
        if (name == Name::Hkdf || name == Name::Pbkdf2) {
            throwWebCryptoError(state, WebCryptoError::NotSupported,
                                "HKDF and PBKDF2 keys cannot be exported.");
        }
        if (!cryptoKey->extractable()) {
            throwWebCryptoError(state, WebCryptoError::InvalidAccess,
                                "The key is not extractable.");
        }
        return exportWebCryptoKey(state, keyFormat, cryptoKey);
    });
}

Promise* SubtleCrypto::wrapKey(ScriptValue format, ScriptValue key,
                               ScriptValue wrappingKey,
                               ScriptValue wrapAlgorithm)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        WebCryptoKeyFormat keyFormat = toWebCryptoKeyFormat(state, format);
        CryptoKey* cryptoKey = toCryptoKey(state, key, "key");
        CryptoKey* wrapper = toCryptoKey(state, wrappingKey, "wrappingKey");
        WebCryptoAlgorithm normalized = normalizeWrapAlgorithm(
            state, Op::WrapKey, Op::Encrypt, wrapAlgorithm);
        checkKey(state, normalized, wrapper, WebCryptoKeyUsageWrapKey);
        Name name = cryptoKey->keyAlgorithm().name;
        if (name == Name::Hkdf || name == Name::Pbkdf2) {
            throwWebCryptoError(state, WebCryptoError::NotSupported,
                                "HKDF and PBKDF2 keys cannot be exported.");
        }
        if (!cryptoKey->extractable()) {
            throwWebCryptoError(state, WebCryptoError::InvalidAccess,
                                "The key is not extractable.");
        }
        ValueRef* exported = exportWebCryptoKey(state, keyFormat, cryptoKey);
        std::vector<uint8_t> bytes;
        if (keyFormat == WebCryptoKeyFormat::Jwk) {
            std::string json = jsonCall(state, "stringify", exported)
                                   ->toString(state)
                                   ->toStdUTF8String();
            bytes.assign(json.begin(), json.end());
        } else {
            bytes = copyBytes(state, exported);
        }
        std::vector<uint8_t> out;
        if (normalized.name == Name::AesKw) {
            checkResult(state, PlatformCrypto::aesKwWrap(wrapper->secret(),
                                                         bytes, &out));
        } else {
            out = crypt(state, true, normalized, wrapper, bytes);
        }
        PlatformCrypto::cleanse(bytes.data(), bytes.size());
        return toWebCryptoArrayBuffer(state, out);
    });
}

Promise* SubtleCrypto::unwrapKey(ScriptValue format, ScriptValue wrappedKey,
                                 ScriptValue unwrappingKey,
                                 ScriptValue unwrapAlgorithm,
                                 ScriptValue unwrappedKeyAlgorithm,
                                 ScriptValue extractable, ScriptValue keyUsages)
{
    return run([&](ExecutionStateRef* state, Promise*) -> ScriptValue {
        WebCryptoKeyFormat keyFormat = toWebCryptoKeyFormat(state, format);
        checkBufferSource(state, wrappedKey, "wrappedKey");
        CryptoKey* unwrapper =
            toCryptoKey(state, unwrappingKey, "unwrappingKey");
        bool isExtractable = extractable->toBoolean(state);
        WebCryptoKeyUsages usages = toWebCryptoKeyUsages(state, keyUsages);
        WebCryptoAlgorithm normalized = normalizeWrapAlgorithm(
            state, Op::UnwrapKey, Op::Decrypt, unwrapAlgorithm);
        WebCryptoAlgorithm importAlgorithm = normalizeWebCryptoAlgorithm(
            state, Op::ImportKey, unwrappedKeyAlgorithm);
        std::vector<uint8_t> wrapped = copyBytes(state, wrappedKey);
        checkKey(state, normalized, unwrapper, WebCryptoKeyUsageUnwrapKey);
        std::vector<uint8_t> bytes;
        if (normalized.name == Name::AesKw) {
            checkResult(state, PlatformCrypto::aesKwUnwrap(unwrapper->secret(),
                                                           wrapped, &bytes));
        } else {
            bytes = crypt(state, false, normalized, unwrapper, wrapped);
        }
        JsonWebKey jwk;
        if (keyFormat == WebCryptoKeyFormat::Jwk) {
            // "parse a JWK": the bytes are UTF-8 JSON.
            ValueRef* text = StringRef::createFromUTF8(
                reinterpret_cast<const char*>(bytes.data()), bytes.size());
            jwk = toJsonWebKey(state, jsonCall(state, "parse", text));
            if (!jwk.kty.has) {
                throwWebCryptoError(state, WebCryptoError::Data,
                                    "JWK: 'kty' is required.");
            }
        }
        CryptoKey* key =
            importWebCryptoKey(state, m_executionContext, keyFormat, bytes, jwk,
                               importAlgorithm, isExtractable, usages);
        PlatformCrypto::cleanse(bytes.data(), bytes.size());
        return key->scriptValue();
    });
}

} // namespace Starfish

#endif
