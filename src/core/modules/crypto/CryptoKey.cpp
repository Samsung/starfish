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

#include "core/modules/crypto/CryptoKey.h"
#include "binding/ScriptBindingInstance.h"

#include <EscargotPublic.h>

#include <cstring>

using namespace Escargot;

namespace Starfish {

namespace {

    void setString(ExecutionStateRef* state, ObjectRef* object,
                   const char* name, const char* value)
    {
        object->set(state, StringRef::createFromASCII(name, strlen(name)),
                    StringRef::createFromUTF8(value, strlen(value)));
    }

    ObjectRef* hashObject(ExecutionStateRef* state, CryptoHash hash)
    {
        ObjectRef* object = ObjectRef::create(state);
        setString(state, object, "name",
                  webCryptoAlgorithmNameString(toWebCryptoHashName(hash)));
        return object;
    }

    Uint8ArrayObjectRef* toUint8Array(ExecutionStateRef* state,
                                      const std::vector<uint8_t>& bytes)
    {
        ArrayBufferObjectRef* buffer = ArrayBufferObjectRef::create(state);
        buffer->allocateBuffer(state, bytes.size());
        if (!bytes.empty()) {
            memcpy(buffer->rawBuffer(), bytes.data(), bytes.size());
        }
        Uint8ArrayObjectRef* array = Uint8ArrayObjectRef::create(state);
        array->setBuffer(buffer, 0, bytes.size(), bytes.size());
        return array;
    }

    // The KeyAlgorithm dictionary of the key, as a new object.
    // https://w3c.github.io/webcrypto/#key-algorithm-dictionaries
    ObjectRef* createAlgorithmObject(ExecutionStateRef* state,
                                     const CryptoKeyAlgorithm& algorithm)
    {
        ObjectRef* object = ObjectRef::create(state);
        setString(state, object, "name",
                  webCryptoAlgorithmNameString(algorithm.name));
        switch (algorithm.name) {
        case WebCryptoAlgorithmName::AesCbc:
        case WebCryptoAlgorithmName::AesCtr:
        case WebCryptoAlgorithmName::AesGcm:
        case WebCryptoAlgorithmName::AesKw:
            object->set(state, StringRef::createFromASCII("length"),
                        ValueRef::create(algorithm.length));
            break;
        case WebCryptoAlgorithmName::Hmac:
            object->set(state, StringRef::createFromASCII("length"),
                        ValueRef::create(algorithm.length));
            object->set(state, StringRef::createFromASCII("hash"),
                        hashObject(state, algorithm.hash));
            break;
        case WebCryptoAlgorithmName::RsassaPkcs1v15:
        case WebCryptoAlgorithmName::RsaPss:
        case WebCryptoAlgorithmName::RsaOaep:
            object->set(state, StringRef::createFromASCII("modulusLength"),
                        ValueRef::create(algorithm.modulusLength));
            object->set(state, StringRef::createFromASCII("publicExponent"),
                        toUint8Array(state, algorithm.publicExponent));
            object->set(state, StringRef::createFromASCII("hash"),
                        hashObject(state, algorithm.hash));
            break;
        case WebCryptoAlgorithmName::Ecdsa:
        case WebCryptoAlgorithmName::Ecdh:
            setString(state, object, "namedCurve",
                      algorithm.namedCurve.c_str());
            break;
        default:
            break;
        }
        return object;
    }

} // namespace

CryptoKey* CryptoKey::createSecret(ExecutionContext* executionContext,
                                   const CryptoKeyAlgorithm& algorithm,
                                   bool extractable, WebCryptoKeyUsages usages,
                                   const std::vector<uint8_t>& secret)
{
    CryptoKey* key = new CryptoKey(executionContext, Type::Secret, algorithm,
                                   extractable, usages);
    key->m_secret = secret;
    return key;
}

CryptoKey* CryptoKey::createAsymmetric(ExecutionContext* executionContext,
                                       const CryptoKeyAlgorithm& algorithm,
                                       bool extractable,
                                       WebCryptoKeyUsages usages,
                                       const AsymmetricKey& key)
{
    CryptoKey* cryptoKey = new CryptoKey(
        executionContext, key.isPrivate() ? Type::Private : Type::Public,
        algorithm, extractable, usages);
    cryptoKey->m_asymmetricKey = key;
    return cryptoKey;
}

CryptoKey::CryptoKey(ExecutionContext* executionContext, Type type,
                     const CryptoKeyAlgorithm& algorithm, bool extractable,
                     WebCryptoKeyUsages usages)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_type(type)
    , m_algorithm(algorithm)
    , m_extractable(extractable)
    , m_usages(usages)
    , m_algorithmObject(nullptr)
    , m_usagesObject(nullptr)
{
    // The members own native memory (vectors, the OpenSSL key). GC does not
    // run destructors, so a finalizer does.
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* object, void*) {
            static_cast<CryptoKey*>(object)->~CryptoKey();
        },
        nullptr, nullptr, nullptr);
}

CryptoKey::~CryptoKey()
{
    if (!m_secret.empty()) {
        PlatformCrypto::cleanse(m_secret.data(), m_secret.size());
    }
}

ScriptBindingInstance* CryptoKey::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

String* CryptoKey::type()
{
    switch (m_type) {
    case Type::Public:
        return String::fromUTF8("public");
    case Type::Private:
        return String::fromUTF8("private");
    case Type::Secret:
        break;
    }
    return String::fromUTF8("secret");
}

ScriptObject CryptoKey::algorithm()
{
    if (!m_algorithmObject) {
        m_algorithmObject =
            Evaluator::execute(
                scriptBindingInstance()->scriptContext(),
                [](ExecutionStateRef* state, CryptoKey* self) -> ValueRef* {
                    return createAlgorithmObject(state, self->m_algorithm);
                },
                this)
                .result->asObject();
    }
    return m_algorithmObject;
}

ScriptObject CryptoKey::usages()
{
    if (!m_usagesObject) {
        m_usagesObject =
            Evaluator::execute(
                scriptBindingInstance()->scriptContext(),
                [](ExecutionStateRef* state, CryptoKey* self) -> ValueRef* {
                    // In the order of the KeyUsage enum.
                    ArrayObjectRef* array = ArrayObjectRef::create(state);
                    uint32_t index = 0;
                    for (unsigned i = 0; i < 8; i++) {
                        WebCryptoKeyUsage usage =
                            static_cast<WebCryptoKeyUsage>(1 << i);
                        if (self->m_usages & usage) {
                            const char* name = webCryptoKeyUsageString(usage);
                            array->set(
                                state, ValueRef::create(index++),
                                StringRef::createFromASCII(name, strlen(name)));
                        }
                    }
                    return array;
                },
                this)
                .result->asObject();
    }
    return m_usagesObject;
}

} // namespace Starfish

#endif
