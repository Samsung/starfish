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

#ifndef __StarfishCryptoKey__
#define __StarfishCryptoKey__

#if defined(STARFISH_ENABLE_WEBCRYPTO)

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/crypto/WebCryptoAlgorithm.h"

namespace Starfish {

// The [[algorithm]] internal slot. Which members are used depends on the
// name, as in the KeyAlgorithm dictionaries of the spec.
struct CryptoKeyAlgorithm {
    CryptoKeyAlgorithm()
        : name(WebCryptoAlgorithmName::AesGcm)
        , length(0)
        , hash(CryptoHash::SHA256)
        , modulusLength(0)
    {
    }

    WebCryptoAlgorithmName name;
    // AES and HMAC key length in bits.
    uint32_t length;
    // HMAC and RSA.
    CryptoHash hash;
    // EC.
    std::string namedCurve;
    // RSA.
    uint32_t modulusLength;
    std::vector<uint8_t> publicExponent;
};

// https://w3c.github.io/webcrypto/#cryptokey-interface
//
// The key material is either secret bytes or an AsymmetricKey. It is not a
// GC object, so a finalizer frees it, and it clears the secret bytes first.
class CryptoKey : public ScriptWrappable {
public:
    enum class Type {
        Public,
        Private,
        Secret,
    };

    static CryptoKey* createSecret(ExecutionContext* executionContext,
                                   const CryptoKeyAlgorithm& algorithm,
                                   bool extractable, WebCryptoKeyUsages usages,
                                   const std::vector<uint8_t>& secret);
    static CryptoKey* createAsymmetric(ExecutionContext* executionContext,
                                       const CryptoKeyAlgorithm& algorithm,
                                       bool extractable,
                                       WebCryptoKeyUsages usages,
                                       const AsymmetricKey& key);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(CryptoKey)

    // IDL attributes.
    String* type();
    bool extractable() const
    {
        return m_extractable;
    }
    // Both return the same object on each call, as the spec requires.
    ScriptObject algorithm();
    ScriptObject usages();

    Type keyType() const
    {
        return m_type;
    }
    const CryptoKeyAlgorithm& keyAlgorithm() const
    {
        return m_algorithm;
    }
    WebCryptoKeyUsages keyUsages() const
    {
        return m_usages;
    }
    bool hasUsage(WebCryptoKeyUsage usage) const
    {
        return m_usages & usage;
    }
    const std::vector<uint8_t>& secret() const
    {
        return m_secret;
    }
    const AsymmetricKey& asymmetricKey() const
    {
        return m_asymmetricKey;
    }
    ExecutionContext* executionContext() const
    {
        return m_executionContext;
    }

private:
    CryptoKey(ExecutionContext* executionContext, Type type,
              const CryptoKeyAlgorithm& algorithm, bool extractable,
              WebCryptoKeyUsages usages);
    ~CryptoKey();

    ExecutionContext* m_executionContext;
    Type m_type;
    CryptoKeyAlgorithm m_algorithm;
    bool m_extractable;
    WebCryptoKeyUsages m_usages;
    std::vector<uint8_t> m_secret;
    AsymmetricKey m_asymmetricKey;
    // Made on the first read of the attribute, then reused.
    ScriptObject m_algorithmObject;
    ScriptObject m_usagesObject;
};

} // namespace Starfish

#endif
#endif
