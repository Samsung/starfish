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

#include "core/modules/crypto/SubtleCryptoKeys.h"
#include "core/modules/crypto/SubtleCrypto.h"

#include <EscargotPublic.h>

#include <cstring>

using namespace Escargot;

namespace Starfish {

namespace {

    typedef WebCryptoAlgorithmName Name;

    const WebCryptoKeyUsages kAesUsages =
        WebCryptoKeyUsageEncrypt | WebCryptoKeyUsageDecrypt |
        WebCryptoKeyUsageWrapKey | WebCryptoKeyUsageUnwrapKey;
    const WebCryptoKeyUsages kWrapUsages =
        WebCryptoKeyUsageWrapKey | WebCryptoKeyUsageUnwrapKey;
    const WebCryptoKeyUsages kSignUsages =
        WebCryptoKeyUsageSign | WebCryptoKeyUsageVerify;
    const WebCryptoKeyUsages kDeriveUsages =
        WebCryptoKeyUsageDeriveKey | WebCryptoKeyUsageDeriveBits;
    const WebCryptoKeyUsages kOaepPublicUsages =
        WebCryptoKeyUsageEncrypt | WebCryptoKeyUsageWrapKey;
    const WebCryptoKeyUsages kOaepPrivateUsages =
        WebCryptoKeyUsageDecrypt | WebCryptoKeyUsageUnwrapKey;
    // Single usages as WebCryptoKeyUsages, for use next to the sets above.
    const WebCryptoKeyUsages kSignUsage = WebCryptoKeyUsageSign;
    const WebCryptoKeyUsages kVerifyUsage = WebCryptoKeyUsageVerify;

    void throwError(ExecutionStateRef* state, WebCryptoError error,
                    const char* message)
    {
        throwWebCryptoError(state, error, message);
    }

    // Throws a SyntaxError when `usages` has a usage outside `allowed`.
    void checkUsages(ExecutionStateRef* state, WebCryptoKeyUsages usages,
                     WebCryptoKeyUsages allowed)
    {
        if (usages & ~allowed) {
            throwError(state, WebCryptoError::Syntax,
                       "The key usages are not valid for this key.");
        }
    }

    bool isAes(Name name)
    {
        return name == Name::AesCbc || name == Name::AesCtr ||
               name == Name::AesGcm || name == Name::AesKw;
    }

    bool isRsa(Name name)
    {
        return name == Name::RsassaPkcs1v15 || name == Name::RsaPss ||
               name == Name::RsaOaep;
    }

    // P-256, P-384 and P-521 only.
    bool toCurve(const std::string& name, CryptoCurve* curve)
    {
        if (name == "P-256") {
            *curve = CryptoCurve::P256;
        } else if (name == "P-384") {
            *curve = CryptoCurve::P384;
        } else if (name == "P-521") {
            *curve = CryptoCurve::P521;
        } else {
            return false;
        }
        return true;
    }

    const char* curveName(CryptoCurve curve)
    {
        switch (curve) {
        case CryptoCurve::P256:
            return "P-256";
        case CryptoCurve::P384:
            return "P-384";
        case CryptoCurve::P521:
            return "P-521";
        }
        return "";
    }

    // The default HMAC key length is the block size of the hash.
    uint32_t hmacBlockBits(CryptoHash hash)
    {
        return (hash == CryptoHash::SHA384 || hash == CryptoHash::SHA512) ? 1024
                                                                          : 512;
    }

    // base64url without padding (RFC 7515, appendix C).
    std::string base64UrlEncode(const std::vector<uint8_t>& bytes)
    {
        static const char kAlphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        std::string out;
        size_t i = 0;
        for (; i + 2 < bytes.size(); i += 3) {
            uint32_t v = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];
            out.push_back(kAlphabet[(v >> 18) & 63]);
            out.push_back(kAlphabet[(v >> 12) & 63]);
            out.push_back(kAlphabet[(v >> 6) & 63]);
            out.push_back(kAlphabet[v & 63]);
        }
        if (i + 1 == bytes.size()) {
            uint32_t v = bytes[i] << 16;
            out.push_back(kAlphabet[(v >> 18) & 63]);
            out.push_back(kAlphabet[(v >> 12) & 63]);
        } else if (i + 2 == bytes.size()) {
            uint32_t v = (bytes[i] << 16) | (bytes[i + 1] << 8);
            out.push_back(kAlphabet[(v >> 18) & 63]);
            out.push_back(kAlphabet[(v >> 12) & 63]);
            out.push_back(kAlphabet[(v >> 6) & 63]);
        }
        return out;
    }

    // Throws a DataError for characters outside the base64url alphabet and
    // for a length that no encoding can have.
    std::vector<uint8_t> base64UrlDecode(ExecutionStateRef* state,
                                         const std::string& text)
    {
        std::vector<uint8_t> out;
        uint32_t buffer = 0;
        int bits = 0;
        for (char c : text) {
            int value;
            if (c >= 'A' && c <= 'Z') {
                value = c - 'A';
            } else if (c >= 'a' && c <= 'z') {
                value = c - 'a' + 26;
            } else if (c >= '0' && c <= '9') {
                value = c - '0' + 52;
            } else if (c == '-') {
                value = 62;
            } else if (c == '_') {
                value = 63;
            } else {
                throwError(state, WebCryptoError::Data,
                           "JWK: invalid base64url data.");
                return out;
            }
            buffer = (buffer << 6) | static_cast<uint32_t>(value);
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                out.push_back(static_cast<uint8_t>((buffer >> bits) & 0xff));
            }
        }
        if (text.size() % 4 == 1) {
            throwError(state, WebCryptoError::Data,
                       "JWK: invalid base64url length.");
        }
        return out;
    }

    // A required JWK member, decoded. Throws a DataError when it is absent.
    std::vector<uint8_t> requiredJwkBytes(ExecutionStateRef* state,
                                          const JwkString& member,
                                          const char* name)
    {
        if (!member.has) {
            std::string message =
                std::string("JWK: '") + name + "' is required.";
            throwError(state, WebCryptoError::Data, message.c_str());
        }
        return base64UrlDecode(state, member.value);
    }

    // The checks that every JWK import does after the key type is known.
    // https://w3c.github.io/webcrypto/#aes-cbc-operations (and the others)
    void checkJwk(ExecutionStateRef* state, const JsonWebKey& jwk,
                  const char* kty, const char* use, bool extractable,
                  WebCryptoKeyUsages usages)
    {
        if (!jwk.kty.has || jwk.kty.value != kty) {
            throwError(state, WebCryptoError::Data, "JWK: wrong 'kty'.");
        }
        if (usages && jwk.use.has && jwk.use.value != use) {
            throwError(state, WebCryptoError::Data, "JWK: wrong 'use'.");
        }
        if (jwk.hasKeyOps) {
            // RFC 7517 4.3: the values must not repeat.
            for (size_t i = 0; i < jwk.keyOps.size(); i++) {
                for (size_t j = i + 1; j < jwk.keyOps.size(); j++) {
                    if (jwk.keyOps[i] == jwk.keyOps[j]) {
                        throwError(state, WebCryptoError::Data,
                                   "JWK: 'key_ops' repeats a value.");
                    }
                }
            }
            for (unsigned i = 0; i < 8; i++) {
                WebCryptoKeyUsage usage =
                    static_cast<WebCryptoKeyUsage>(1 << i);
                if (!(usages & usage)) {
                    continue;
                }
                bool found = false;
                for (const std::string& op : jwk.keyOps) {
                    found = found || op == webCryptoKeyUsageString(usage);
                }
                if (!found) {
                    throwError(state, WebCryptoError::Data,
                               "JWK: 'key_ops' does not allow the usages.");
                }
            }
        }
        if (jwk.hasExt && !jwk.ext && extractable) {
            throwError(state, WebCryptoError::Data,
                       "JWK: the key is not extractable.");
        }
    }

    const char* aesJwkAlg(Name name, size_t keyBytes)
    {
        static const char* kCbc[] = { "A128CBC", "A192CBC", "A256CBC" };
        static const char* kCtr[] = { "A128CTR", "A192CTR", "A256CTR" };
        static const char* kGcm[] = { "A128GCM", "A192GCM", "A256GCM" };
        static const char* kKw[] = { "A128KW", "A192KW", "A256KW" };
        size_t index = keyBytes == 16 ? 0 : keyBytes == 24 ? 1 : 2;
        switch (name) {
        case Name::AesCbc:
            return kCbc[index];
        case Name::AesCtr:
            return kCtr[index];
        case Name::AesGcm:
            return kGcm[index];
        default:
            return kKw[index];
        }
    }

    const char* hmacJwkAlg(CryptoHash hash)
    {
        switch (hash) {
        case CryptoHash::SHA1:
            return "HS1";
        case CryptoHash::SHA256:
            return "HS256";
        case CryptoHash::SHA384:
            return "HS384";
        case CryptoHash::SHA512:
            return "HS512";
        }
        return "";
    }

    const char* rsaJwkAlg(Name name, CryptoHash hash)
    {
        static const char* kPkcs1[] = { "RS1", "RS256", "RS384", "RS512" };
        static const char* kPss[] = { "PS1", "PS256", "PS384", "PS512" };
        static const char* kOaep[] = { "RSA-OAEP", "RSA-OAEP-256",
                                       "RSA-OAEP-384", "RSA-OAEP-512" };
        size_t index = static_cast<size_t>(hash);
        switch (name) {
        case Name::RsassaPkcs1v15:
            return kPkcs1[index];
        case Name::RsaPss:
            return kPss[index];
        default:
            return kOaep[index];
        }
    }

    const char* ecdsaJwkAlg(CryptoCurve curve)
    {
        switch (curve) {
        case CryptoCurve::P256:
            return "ES256";
        case CryptoCurve::P384:
            return "ES384";
        case CryptoCurve::P521:
            return "ES512";
        }
        return "";
    }

    CryptoKeyKind okpKind(Name name)
    {
        return name == Name::Ed25519 ? CryptoKeyKind::Ed25519
                                     : CryptoKeyKind::X25519;
    }

    CryptoKey* importAes(ExecutionStateRef* state, ExecutionContext* context,
                         WebCryptoKeyFormat format,
                         const std::vector<uint8_t>& bytes,
                         const JsonWebKey& jwk,
                         const WebCryptoAlgorithm& algorithm, bool extractable,
                         WebCryptoKeyUsages usages)
    {
        checkUsages(state, usages,
                    algorithm.name == Name::AesKw ? kWrapUsages : kAesUsages);
        std::vector<uint8_t> data;
        if (format == WebCryptoKeyFormat::Raw) {
            data = bytes;
        } else if (format == WebCryptoKeyFormat::Jwk) {
            if (!jwk.kty.has || jwk.kty.value != "oct") {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'kty'.");
            }
            data = requiredJwkBytes(state, jwk.k, "k");
        } else {
            throwError(state, WebCryptoError::NotSupported,
                       "AES keys support only raw and jwk.");
        }
        if (data.size() != 16 && data.size() != 24 && data.size() != 32) {
            throwError(state, WebCryptoError::Data,
                       "AES key length must be 128, 192 or 256 bits.");
        }
        if (format == WebCryptoKeyFormat::Jwk) {
            if (jwk.alg.has &&
                jwk.alg.value != aesJwkAlg(algorithm.name, data.size())) {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'alg'.");
            }
            checkJwk(state, jwk, "oct", "enc", extractable, usages);
        }
        CryptoKeyAlgorithm keyAlgorithm;
        keyAlgorithm.name = algorithm.name;
        keyAlgorithm.length = static_cast<uint32_t>(data.size() * 8);
        CryptoKey* key = CryptoKey::createSecret(context, keyAlgorithm,
                                                 extractable, usages, data);
        PlatformCrypto::cleanse(data.data(), data.size());
        return key;
    }

    CryptoKey* importHmac(ExecutionStateRef* state, ExecutionContext* context,
                          WebCryptoKeyFormat format,
                          const std::vector<uint8_t>& bytes,
                          const JsonWebKey& jwk,
                          const WebCryptoAlgorithm& algorithm, bool extractable,
                          WebCryptoKeyUsages usages)
    {
        checkUsages(state, usages, kSignUsages);
        std::vector<uint8_t> data;
        if (format == WebCryptoKeyFormat::Raw) {
            data = bytes;
        } else if (format == WebCryptoKeyFormat::Jwk) {
            if (!jwk.kty.has || jwk.kty.value != "oct") {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'kty'.");
            }
            data = requiredJwkBytes(state, jwk.k, "k");
            if (jwk.alg.has && jwk.alg.value != hmacJwkAlg(algorithm.hash)) {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'alg'.");
            }
            checkJwk(state, jwk, "oct", "sig", extractable, usages);
        } else {
            throwError(state, WebCryptoError::NotSupported,
                       "HMAC keys support only raw and jwk.");
        }
        uint64_t length = static_cast<uint64_t>(data.size()) * 8;
        if (length == 0) {
            throwError(state, WebCryptoError::Data, "The HMAC key is empty.");
        }
        if (algorithm.hasLength) {
            // The length may drop only the unused bits of the last byte.
            if (algorithm.length > length || algorithm.length <= length - 8) {
                throwError(state, WebCryptoError::Data,
                           "The HMAC length does not match the key data.");
            }
            length = algorithm.length;
        }
        CryptoKeyAlgorithm keyAlgorithm;
        keyAlgorithm.name = Name::Hmac;
        keyAlgorithm.hash = algorithm.hash;
        keyAlgorithm.length = static_cast<uint32_t>(length);
        CryptoKey* key = CryptoKey::createSecret(context, keyAlgorithm,
                                                 extractable, usages, data);
        PlatformCrypto::cleanse(data.data(), data.size());
        return key;
    }

    CryptoKey* importKdf(ExecutionStateRef* state, ExecutionContext* context,
                         WebCryptoKeyFormat format,
                         const std::vector<uint8_t>& bytes,
                         const WebCryptoAlgorithm& algorithm, bool extractable,
                         WebCryptoKeyUsages usages)
    {
        if (format != WebCryptoKeyFormat::Raw) {
            throwError(state, WebCryptoError::NotSupported,
                       "HKDF and PBKDF2 keys support only raw.");
        }
        checkUsages(state, usages, kDeriveUsages);
        if (extractable) {
            throwError(state, WebCryptoError::Syntax,
                       "HKDF and PBKDF2 keys cannot be extractable.");
        }
        CryptoKeyAlgorithm keyAlgorithm;
        keyAlgorithm.name = algorithm.name;
        return CryptoKey::createSecret(context, keyAlgorithm, false, usages,
                                       bytes);
    }

    CryptoKey* importRsa(ExecutionStateRef* state, ExecutionContext* context,
                         WebCryptoKeyFormat format,
                         const std::vector<uint8_t>& bytes,
                         const JsonWebKey& jwk,
                         const WebCryptoAlgorithm& algorithm, bool extractable,
                         WebCryptoKeyUsages usages)
    {
        bool isOaep = algorithm.name == Name::RsaOaep;
        WebCryptoKeyUsages publicUsages =
            isOaep ? kOaepPublicUsages : kVerifyUsage;
        WebCryptoKeyUsages privateUsages =
            isOaep ? kOaepPrivateUsages : kSignUsage;
        AsymmetricKey key;
        if (format == WebCryptoKeyFormat::Spki) {
            checkUsages(state, usages, publicUsages);
            if (!PlatformCrypto::importSpki(CryptoKeyKind::RSA, bytes, &key)) {
                throwError(state, WebCryptoError::Data, "Invalid RSA key.");
            }
        } else if (format == WebCryptoKeyFormat::Pkcs8) {
            checkUsages(state, usages, privateUsages);
            if (!PlatformCrypto::importPkcs8(CryptoKeyKind::RSA, bytes, &key)) {
                throwError(state, WebCryptoError::Data, "Invalid RSA key.");
            }
        } else if (format == WebCryptoKeyFormat::Jwk) {
            bool isPrivate = jwk.d.has;
            checkUsages(state, usages,
                        isPrivate ? privateUsages : publicUsages);
            if (!jwk.kty.has || jwk.kty.value != "RSA") {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'kty'.");
            }
            if (jwk.alg.has &&
                jwk.alg.value != rsaJwkAlg(algorithm.name, algorithm.hash)) {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'alg'.");
            }
            checkJwk(state, jwk, "RSA", isOaep ? "enc" : "sig", extractable,
                     usages);
            // Keys with more than two primes are not supported.
            if (jwk.hasOth) {
                throwError(state, WebCryptoError::Data,
                           "JWK: 'oth' is not supported.");
            }
            RsaKeyComponents components;
            components.n = requiredJwkBytes(state, jwk.n, "n");
            components.e = requiredJwkBytes(state, jwk.e, "e");
            if (isPrivate) {
                components.d = requiredJwkBytes(state, jwk.d, "d");
                components.p = requiredJwkBytes(state, jwk.p, "p");
                components.q = requiredJwkBytes(state, jwk.q, "q");
                components.dp = requiredJwkBytes(state, jwk.dp, "dp");
                components.dq = requiredJwkBytes(state, jwk.dq, "dq");
                components.qi = requiredJwkBytes(state, jwk.qi, "qi");
            }
            if (!PlatformCrypto::importRsaComponents(components, &key)) {
                throwError(state, WebCryptoError::Data, "Invalid RSA key.");
            }
        } else {
            throwError(state, WebCryptoError::NotSupported,
                       "RSA keys support spki, pkcs8 and jwk.");
        }
        CryptoKeyAlgorithm keyAlgorithm;
        keyAlgorithm.name = algorithm.name;
        keyAlgorithm.hash = algorithm.hash;
        keyAlgorithm.modulusLength =
            static_cast<uint32_t>(key.rsaModulusBits());
        keyAlgorithm.publicExponent = key.rsaPublicExponent();
        return CryptoKey::createAsymmetric(context, keyAlgorithm, extractable,
                                           usages, key);
    }

    CryptoKey* importEc(ExecutionStateRef* state, ExecutionContext* context,
                        WebCryptoKeyFormat format,
                        const std::vector<uint8_t>& bytes,
                        const JsonWebKey& jwk,
                        const WebCryptoAlgorithm& algorithm, bool extractable,
                        WebCryptoKeyUsages usages)
    {
        bool isEcdsa = algorithm.name == Name::Ecdsa;
        WebCryptoKeyUsages publicUsages = isEcdsa ? kVerifyUsage : 0;
        WebCryptoKeyUsages privateUsages = isEcdsa ? kSignUsage : kDeriveUsages;
        CryptoCurve curve;
        bool isKnownCurve = toCurve(algorithm.namedCurve, &curve);
        AsymmetricKey key;
        switch (format) {
        case WebCryptoKeyFormat::Spki:
        case WebCryptoKeyFormat::Raw:
            checkUsages(state, usages, publicUsages);
            break;
        case WebCryptoKeyFormat::Pkcs8:
            checkUsages(state, usages, privateUsages);
            break;
        case WebCryptoKeyFormat::Jwk:
            checkUsages(state, usages,
                        jwk.d.has ? privateUsages : publicUsages);
            break;
        }
        if (!isKnownCurve) {
            throwError(state, WebCryptoError::NotSupported,
                       "The named curve is not supported.");
        }
        if (format == WebCryptoKeyFormat::Raw) {
            if (!PlatformCrypto::importEcPoint(curve, bytes, &key)) {
                throwError(state, WebCryptoError::Data, "Invalid EC key.");
            }
        } else if (format == WebCryptoKeyFormat::Spki ||
                   format == WebCryptoKeyFormat::Pkcs8) {
            bool ok =
                format == WebCryptoKeyFormat::Spki
                    ? PlatformCrypto::importSpki(CryptoKeyKind::EC, bytes, &key)
                    : PlatformCrypto::importPkcs8(CryptoKeyKind::EC, bytes,
                                                  &key);
            if (!ok || key.curve() != curve) {
                throwError(state, WebCryptoError::Data, "Invalid EC key.");
            }
        } else {
            if (!jwk.kty.has || jwk.kty.value != "EC") {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'kty'.");
            }
            checkJwk(state, jwk, "EC", isEcdsa ? "sig" : "enc", extractable,
                     usages);
            if (!jwk.crv.has || jwk.crv.value != algorithm.namedCurve) {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'crv'.");
            }
            if (isEcdsa && jwk.alg.has && jwk.alg.value != ecdsaJwkAlg(curve)) {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'alg'.");
            }
            std::vector<uint8_t> x = requiredJwkBytes(state, jwk.x, "x");
            std::vector<uint8_t> y = requiredJwkBytes(state, jwk.y, "y");
            std::vector<uint8_t> d;
            if (jwk.d.has) {
                d = base64UrlDecode(state, jwk.d.value);
                if (d.empty()) {
                    throwError(state, WebCryptoError::Data, "JWK: empty 'd'.");
                }
            }
            if (!PlatformCrypto::importEcCoordinates(curve, x, y, d, &key)) {
                throwError(state, WebCryptoError::Data, "Invalid EC key.");
            }
            if (!d.empty()) {
                PlatformCrypto::cleanse(d.data(), d.size());
            }
        }
        CryptoKeyAlgorithm keyAlgorithm;
        keyAlgorithm.name = algorithm.name;
        keyAlgorithm.namedCurve = curveName(curve);
        return CryptoKey::createAsymmetric(context, keyAlgorithm, extractable,
                                           usages, key);
    }

    CryptoKey* importOkp(ExecutionStateRef* state, ExecutionContext* context,
                         WebCryptoKeyFormat format,
                         const std::vector<uint8_t>& bytes,
                         const JsonWebKey& jwk,
                         const WebCryptoAlgorithm& algorithm, bool extractable,
                         WebCryptoKeyUsages usages)
    {
        bool isEd25519 = algorithm.name == Name::Ed25519;
        WebCryptoKeyUsages publicUsages = isEd25519 ? kVerifyUsage : 0;
        WebCryptoKeyUsages privateUsages =
            isEd25519 ? kSignUsage : kDeriveUsages;
        CryptoKeyKind kind = okpKind(algorithm.name);
        const char* curve = isEd25519 ? "Ed25519" : "X25519";
        AsymmetricKey key;
        if (format == WebCryptoKeyFormat::Raw) {
            checkUsages(state, usages, publicUsages);
            if (!PlatformCrypto::importOkp(kind, bytes, CryptoBytes(), &key)) {
                throwError(state, WebCryptoError::Data, "Invalid key.");
            }
        } else if (format == WebCryptoKeyFormat::Spki) {
            checkUsages(state, usages, publicUsages);
            if (!PlatformCrypto::importSpki(kind, bytes, &key)) {
                throwError(state, WebCryptoError::Data, "Invalid key.");
            }
        } else if (format == WebCryptoKeyFormat::Pkcs8) {
            checkUsages(state, usages, privateUsages);
            if (!PlatformCrypto::importPkcs8(kind, bytes, &key)) {
                throwError(state, WebCryptoError::Data, "Invalid key.");
            }
        } else {
            checkUsages(state, usages,
                        jwk.d.has ? privateUsages : publicUsages);
            if (!jwk.kty.has || jwk.kty.value != "OKP") {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'kty'.");
            }
            if (!jwk.crv.has || jwk.crv.value != curve) {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'crv'.");
            }
            if (isEd25519 && jwk.alg.has && jwk.alg.value != "Ed25519" &&
                jwk.alg.value != "EdDSA") {
                throwError(state, WebCryptoError::Data, "JWK: wrong 'alg'.");
            }
            checkJwk(state, jwk, "OKP", isEd25519 ? "sig" : "enc", extractable,
                     usages);
            std::vector<uint8_t> x = requiredJwkBytes(state, jwk.x, "x");
            std::vector<uint8_t> d;
            if (jwk.d.has) {
                d = base64UrlDecode(state, jwk.d.value);
            }
            if (!PlatformCrypto::importOkp(
                    kind, x, jwk.d.has ? CryptoBytes(d) : CryptoBytes(),
                    &key) ||
                (jwk.d.has && d.empty())) {
                throwError(state, WebCryptoError::Data, "Invalid key.");
            }
            if (!d.empty()) {
                PlatformCrypto::cleanse(d.data(), d.size());
            }
        }
        CryptoKeyAlgorithm keyAlgorithm;
        keyAlgorithm.name = algorithm.name;
        return CryptoKey::createAsymmetric(context, keyAlgorithm, extractable,
                                           usages, key);
    }

    void setJwkString(ExecutionStateRef* state, ObjectRef* object,
                      const char* name, const std::string& value)
    {
        object->set(state, StringRef::createFromASCII(name, strlen(name)),
                    StringRef::createFromUTF8(value.data(), value.size()));
    }

    ObjectRef* createJwk(ExecutionStateRef* state, CryptoKey* key,
                         const char* kty)
    {
        ObjectRef* jwk = ObjectRef::create(state);
        setJwkString(state, jwk, "kty", kty);
        // key_ops is the usages of the key, in the KeyUsage order.
        ArrayObjectRef* keyOps = ArrayObjectRef::create(state);
        uint32_t index = 0;
        for (unsigned i = 0; i < 8; i++) {
            WebCryptoKeyUsage usage = static_cast<WebCryptoKeyUsage>(1 << i);
            if (key->hasUsage(usage)) {
                const char* name = webCryptoKeyUsageString(usage);
                keyOps->set(state, ValueRef::create(index++),
                            StringRef::createFromASCII(name, strlen(name)));
            }
        }
        jwk->set(state, StringRef::createFromASCII("key_ops"), keyOps);
        jwk->set(state, StringRef::createFromASCII("ext"),
                 ValueRef::create(key->extractable()));
        return jwk;
    }

    void queueOrThrowGenerateError(ExecutionStateRef* state, bool ok)
    {
        if (!ok) {
            throwError(state, WebCryptoError::Operation,
                       "Key generation failed.");
        }
    }

    // An exponent is valid when it is odd and greater than 1.
    bool isValidRsaExponent(const std::vector<uint8_t>& exponent)
    {
        if (exponent.empty() || !(exponent.back() & 1)) {
            return false;
        }
        for (size_t i = 0; i + 1 < exponent.size(); i++) {
            if (exponent[i]) {
                return true;
            }
        }
        return exponent.back() > 1;
    }

    // A CryptoKeyPair dictionary as an object.
    ValueRef* keyPairObject(ExecutionStateRef* state, CryptoKey* publicKey,
                            CryptoKey* privateKey)
    {
        ObjectRef* pair = ObjectRef::create(state);
        pair->set(state, StringRef::createFromASCII("privateKey"),
                  privateKey->scriptValue());
        pair->set(state, StringRef::createFromASCII("publicKey"),
                  publicKey->scriptValue());
        return pair;
    }

    // Splits a generated key pair into two CryptoKeys. The public key is
    // always extractable (spec, "generate key" of each algorithm).
    ValueRef* createKeyPair(ExecutionStateRef* state, ExecutionContext* context,
                            const CryptoKeyAlgorithm& keyAlgorithm,
                            const AsymmetricKey& privateKey, bool extractable,
                            WebCryptoKeyUsages publicUsages,
                            WebCryptoKeyUsages privateUsages)
    {
        CryptoKey* publicCryptoKey = CryptoKey::createAsymmetric(
            context, keyAlgorithm, true, publicUsages, privateKey.publicKey());
        CryptoKey* privateCryptoKey = CryptoKey::createAsymmetric(
            context, keyAlgorithm, extractable, privateUsages, privateKey);
        return keyPairObject(state, publicCryptoKey, privateCryptoKey);
    }

} // namespace

WebCryptoKeyFormat toWebCryptoKeyFormat(ExecutionStateRef* state,
                                        ValueRef* value)
{
    std::string format = value->toString(state)->toStdUTF8String();
    if (format == "raw") {
        return WebCryptoKeyFormat::Raw;
    }
    if (format == "spki") {
        return WebCryptoKeyFormat::Spki;
    }
    if (format == "pkcs8") {
        return WebCryptoKeyFormat::Pkcs8;
    }
    if (format == "jwk") {
        return WebCryptoKeyFormat::Jwk;
    }
    std::string message = "'" + format + "' is not a valid KeyFormat.";
    throwWebCryptoTypeError(state, message.c_str());
    return WebCryptoKeyFormat::Raw;
}

JsonWebKey toJsonWebKey(ExecutionStateRef* state, ValueRef* value)
{
    JsonWebKey jwk;
    if (value->isUndefinedOrNull()) {
        return jwk;
    }
    if (!value->isObject()) {
        throwWebCryptoTypeError(state, "JsonWebKey: not an object.");
    }
    ObjectRef* object = value->asObject();
    // WebIDL reads dictionary members in lexicographic order.
    auto readString = [&](const char* name, JwkString* target) {
        ValueRef* member =
            object->get(state, StringRef::createFromASCII(name, strlen(name)));
        if (!member->isUndefined()) {
            target->has = true;
            target->value = member->toString(state)->toStdUTF8String();
        }
    };
    readString("alg", &jwk.alg);
    readString("crv", &jwk.crv);
    readString("d", &jwk.d);
    readString("dp", &jwk.dp);
    readString("dq", &jwk.dq);
    readString("e", &jwk.e);
    ValueRef* ext = object->get(state, StringRef::createFromASCII("ext"));
    if (!ext->isUndefined()) {
        jwk.hasExt = true;
        jwk.ext = ext->toBoolean(state);
    }
    readString("k", &jwk.k);
    ValueRef* keyOps =
        object->get(state, StringRef::createFromASCII("key_ops"));
    if (!keyOps->isUndefined()) {
        if (!keyOps->isArrayObject()) {
            throwWebCryptoTypeError(state,
                                    "JsonWebKey: key_ops is not a "
                                    "sequence.");
        }
        jwk.hasKeyOps = true;
        ArrayObjectRef* array = keyOps->asArrayObject();
        uint64_t length = array->length(state);
        for (uint64_t i = 0; i < length; i++) {
            jwk.keyOps.push_back(array->get(state, ValueRef::create(i))
                                     ->toString(state)
                                     ->toStdUTF8String());
        }
    }
    readString("kty", &jwk.kty);
    readString("n", &jwk.n);
    jwk.hasOth =
        !object->get(state, StringRef::createFromASCII("oth"))->isUndefined();
    readString("p", &jwk.p);
    readString("q", &jwk.q);
    readString("qi", &jwk.qi);
    readString("use", &jwk.use);
    readString("x", &jwk.x);
    readString("y", &jwk.y);
    return jwk;
}

ValueRef* toWebCryptoArrayBuffer(ExecutionStateRef* state,
                                 const std::vector<uint8_t>& bytes)
{
    ArrayBufferObjectRef* buffer = ArrayBufferObjectRef::create(state);
    buffer->allocateBuffer(state, bytes.size());
    if (!bytes.empty()) {
        memcpy(buffer->rawBuffer(), bytes.data(), bytes.size());
    }
    return buffer;
}

CryptoKey* importWebCryptoKey(ExecutionStateRef* state,
                              ExecutionContext* executionContext,
                              WebCryptoKeyFormat format,
                              const std::vector<uint8_t>& bytes,
                              const JsonWebKey& jwk,
                              const WebCryptoAlgorithm& algorithm,
                              bool extractable, WebCryptoKeyUsages usages)
{
    CryptoKey* key = nullptr;
    Name name = algorithm.name;
    if (isAes(name)) {
        key = importAes(state, executionContext, format, bytes, jwk, algorithm,
                        extractable, usages);
    } else if (name == Name::Hmac) {
        key = importHmac(state, executionContext, format, bytes, jwk, algorithm,
                         extractable, usages);
    } else if (name == Name::Hkdf || name == Name::Pbkdf2) {
        key = importKdf(state, executionContext, format, bytes, algorithm,
                        extractable, usages);
    } else if (isRsa(name)) {
        key = importRsa(state, executionContext, format, bytes, jwk, algorithm,
                        extractable, usages);
    } else if (name == Name::Ecdsa || name == Name::Ecdh) {
        key = importEc(state, executionContext, format, bytes, jwk, algorithm,
                       extractable, usages);
    } else {
        key = importOkp(state, executionContext, format, bytes, jwk, algorithm,
                        extractable, usages);
    }
    // importKey() step: a secret or private key needs a usage.
    if (key->keyType() != CryptoKey::Type::Public && !usages) {
        throwError(state, WebCryptoError::Syntax,
                   "The key usages must not be empty.");
    }
    return key;
}

ValueRef* exportWebCryptoKey(ExecutionStateRef* state,
                             WebCryptoKeyFormat format, CryptoKey* key)
{
    const CryptoKeyAlgorithm& algorithm = key->keyAlgorithm();
    Name name = algorithm.name;
    if (name == Name::Hkdf || name == Name::Pbkdf2) {
        throwError(state, WebCryptoError::NotSupported,
                   "HKDF and PBKDF2 keys cannot be exported.");
    }

    if (key->keyType() == CryptoKey::Type::Secret) {
        if (format == WebCryptoKeyFormat::Raw) {
            return toWebCryptoArrayBuffer(state, key->secret());
        }
        if (format != WebCryptoKeyFormat::Jwk) {
            throwError(state, WebCryptoError::NotSupported,
                       "Secret keys support only raw and jwk.");
        }
        ObjectRef* jwk = createJwk(state, key, "oct");
        setJwkString(state, jwk, "k", base64UrlEncode(key->secret()));
        setJwkString(state, jwk, "alg",
                     name == Name::Hmac
                         ? hmacJwkAlg(algorithm.hash)
                         : aesJwkAlg(name, key->secret().size()));
        return jwk;
    }

    const AsymmetricKey& asymmetricKey = key->asymmetricKey();
    bool isPrivate = key->keyType() == CryptoKey::Type::Private;
    std::vector<uint8_t> out;
    switch (format) {
    case WebCryptoKeyFormat::Spki:
        if (isPrivate) {
            throwError(state, WebCryptoError::InvalidAccess,
                       "spki needs a public key.");
        }
        if (!PlatformCrypto::exportSpki(asymmetricKey, &out)) {
            throwError(state, WebCryptoError::Operation, "Export failed.");
        }
        return toWebCryptoArrayBuffer(state, out);
    case WebCryptoKeyFormat::Pkcs8:
        if (!isPrivate) {
            throwError(state, WebCryptoError::InvalidAccess,
                       "pkcs8 needs a private key.");
        }
        if (!PlatformCrypto::exportPkcs8(asymmetricKey, &out)) {
            throwError(state, WebCryptoError::Operation, "Export failed.");
        }
        return toWebCryptoArrayBuffer(state, out);
    case WebCryptoKeyFormat::Raw: {
        if (isRsa(name)) {
            throwError(state, WebCryptoError::NotSupported,
                       "RSA keys cannot be exported as raw.");
        }
        if (isPrivate) {
            throwError(state, WebCryptoError::InvalidAccess,
                       "raw needs a public key.");
        }
        std::vector<uint8_t> unused;
        bool ok = (name == Name::Ecdsa || name == Name::Ecdh)
                      ? PlatformCrypto::exportEcPoint(asymmetricKey, &out)
                      : PlatformCrypto::exportOkp(asymmetricKey, &out, &unused);
        if (!ok) {
            throwError(state, WebCryptoError::Operation, "Export failed.");
        }
        return toWebCryptoArrayBuffer(state, out);
    }
    case WebCryptoKeyFormat::Jwk:
        break;
    }

    if (isRsa(name)) {
        RsaKeyComponents components;
        if (!PlatformCrypto::exportRsaComponents(asymmetricKey, &components)) {
            throwError(state, WebCryptoError::Operation, "Export failed.");
        }
        ObjectRef* jwk = createJwk(state, key, "RSA");
        setJwkString(state, jwk, "alg", rsaJwkAlg(name, algorithm.hash));
        setJwkString(state, jwk, "n", base64UrlEncode(components.n));
        setJwkString(state, jwk, "e", base64UrlEncode(components.e));
        if (isPrivate) {
            setJwkString(state, jwk, "d", base64UrlEncode(components.d));
            setJwkString(state, jwk, "p", base64UrlEncode(components.p));
            setJwkString(state, jwk, "q", base64UrlEncode(components.q));
            setJwkString(state, jwk, "dp", base64UrlEncode(components.dp));
            setJwkString(state, jwk, "dq", base64UrlEncode(components.dq));
            setJwkString(state, jwk, "qi", base64UrlEncode(components.qi));
        }
        return jwk;
    }
    if (name == Name::Ecdsa || name == Name::Ecdh) {
        std::vector<uint8_t> x, y, d;
        if (!PlatformCrypto::exportEcCoordinates(asymmetricKey, &x, &y, &d)) {
            throwError(state, WebCryptoError::Operation, "Export failed.");
        }
        ObjectRef* jwk = createJwk(state, key, "EC");
        setJwkString(state, jwk, "crv", algorithm.namedCurve);
        setJwkString(state, jwk, "x", base64UrlEncode(x));
        setJwkString(state, jwk, "y", base64UrlEncode(y));
        if (isPrivate) {
            setJwkString(state, jwk, "d", base64UrlEncode(d));
            PlatformCrypto::cleanse(d.data(), d.size());
        }
        return jwk;
    }
    std::vector<uint8_t> x, d;
    if (!PlatformCrypto::exportOkp(asymmetricKey, &x, &d)) {
        throwError(state, WebCryptoError::Operation, "Export failed.");
    }
    ObjectRef* jwk = createJwk(state, key, "OKP");
    setJwkString(state, jwk, "crv",
                 name == Name::Ed25519 ? "Ed25519" : "X25519");
    if (name == Name::Ed25519) {
        setJwkString(state, jwk, "alg", "Ed25519");
    }
    setJwkString(state, jwk, "x", base64UrlEncode(x));
    if (isPrivate) {
        setJwkString(state, jwk, "d", base64UrlEncode(d));
        PlatformCrypto::cleanse(d.data(), d.size());
    }
    return jwk;
}

ValueRef* generateWebCryptoKey(ExecutionStateRef* state, SubtleCrypto* subtle,
                               Promise* promise,
                               const WebCryptoAlgorithm& algorithm,
                               bool extractable, WebCryptoKeyUsages usages)
{
    ExecutionContext* context = subtle->executionContext();
    Name name = algorithm.name;
    CryptoKeyAlgorithm keyAlgorithm;
    keyAlgorithm.name = name;

    if (isAes(name)) {
        checkUsages(state, usages,
                    name == Name::AesKw ? kWrapUsages : kAesUsages);
        if (algorithm.length != 128 && algorithm.length != 192 &&
            algorithm.length != 256) {
            throwError(state, WebCryptoError::Operation,
                       "AES key length must be 128, 192 or 256 bits.");
        }
        if (!usages) {
            throwError(state, WebCryptoError::Syntax,
                       "The key usages must not be empty.");
        }
        std::vector<uint8_t> secret(algorithm.length / 8);
        queueOrThrowGenerateError(
            state, PlatformCrypto::randomBytes(secret.data(), secret.size()));
        keyAlgorithm.length = algorithm.length;
        CryptoKey* key = CryptoKey::createSecret(context, keyAlgorithm,
                                                 extractable, usages, secret);
        PlatformCrypto::cleanse(secret.data(), secret.size());
        return key->scriptValue();
    }

    if (name == Name::Hmac) {
        checkUsages(state, usages, kSignUsages);
        uint32_t length = hmacBlockBits(algorithm.hash);
        if (algorithm.hasLength) {
            if (algorithm.length == 0) {
                throwError(state, WebCryptoError::Operation,
                           "The HMAC key length must not be zero.");
            }
            length = algorithm.length;
        }
        if (!usages) {
            throwError(state, WebCryptoError::Syntax,
                       "The key usages must not be empty.");
        }
        // A length that is not a multiple of 8 keeps the unused low bits of
        // the last byte, as HMAC importKey allows.
        std::vector<uint8_t> secret((length + 7) / 8);
        queueOrThrowGenerateError(
            state, PlatformCrypto::randomBytes(secret.data(), secret.size()));
        keyAlgorithm.hash = algorithm.hash;
        keyAlgorithm.length = length;
        CryptoKey* key = CryptoKey::createSecret(context, keyAlgorithm,
                                                 extractable, usages, secret);
        PlatformCrypto::cleanse(secret.data(), secret.size());
        return key->scriptValue();
    }

    if (isRsa(name)) {
        bool isOaep = name == Name::RsaOaep;
        checkUsages(state, usages, isOaep ? kAesUsages : kSignUsages);
        WebCryptoKeyUsages publicUsages =
            usages & (isOaep ? kOaepPublicUsages : kVerifyUsage);
        WebCryptoKeyUsages privateUsages =
            usages & (isOaep ? kOaepPrivateUsages : kSignUsage);
        // A key cannot be made with an even exponent or with 1. The spec
        // reports that as a failed generation (OperationError), which comes
        // before the empty private usages check. So check here, before the
        // work is queued.
        if (!isValidRsaExponent(algorithm.publicExponent)) {
            throwError(state, WebCryptoError::Operation,
                       "The RSA public exponent is not valid.");
        }
        if (!privateUsages) {
            throwError(state, WebCryptoError::Syntax,
                       "The private key usages must not be empty.");
        }
        keyAlgorithm.hash = algorithm.hash;
        // Generation takes long, so it runs on the thread pool. The work owns
        // only plain data.
        struct RsaWork {
            size_t modulusLength;
            std::vector<uint8_t> publicExponent;
            AsymmetricKey key;
            bool ok;
        };
        std::shared_ptr<RsaWork> work = std::make_shared<RsaWork>();
        work->modulusLength = algorithm.modulusLength;
        work->publicExponent = algorithm.publicExponent;
        work->ok = false;
        subtle->queue(
            promise,
            [work]() {
                work->ok = PlatformCrypto::generateRsaKey(
                    work->modulusLength, work->publicExponent, &work->key);
            },
            [work, context, keyAlgorithm, extractable, publicUsages,
             privateUsages](ExecutionStateRef* state) -> ScriptValue {
                queueOrThrowGenerateError(state, work->ok);
                CryptoKeyAlgorithm result = keyAlgorithm;
                result.modulusLength =
                    static_cast<uint32_t>(work->key.rsaModulusBits());
                result.publicExponent = work->key.rsaPublicExponent();
                return createKeyPair(state, context, result, work->key,
                                     extractable, publicUsages, privateUsages);
            });
        return nullptr;
    }

    AsymmetricKey key;
    WebCryptoKeyUsages publicUsages = 0;
    WebCryptoKeyUsages privateUsages = 0;
    if (name == Name::Ecdsa || name == Name::Ecdh) {
        bool isEcdsa = name == Name::Ecdsa;
        checkUsages(state, usages, isEcdsa ? kSignUsages : kDeriveUsages);
        CryptoCurve curve;
        if (!toCurve(algorithm.namedCurve, &curve)) {
            throwError(state, WebCryptoError::NotSupported,
                       "The named curve is not supported.");
        }
        queueOrThrowGenerateError(state,
                                  PlatformCrypto::generateEcKey(curve, &key));
        keyAlgorithm.namedCurve = curveName(curve);
        publicUsages = isEcdsa ? usages & kVerifyUsage : 0;
        privateUsages = isEcdsa ? usages & kSignUsage : usages;
    } else {
        bool isEd25519 = name == Name::Ed25519;
        checkUsages(state, usages, isEd25519 ? kSignUsages : kDeriveUsages);
        queueOrThrowGenerateError(
            state, PlatformCrypto::generateOkpKey(okpKind(name), &key));
        publicUsages = isEd25519 ? usages & kVerifyUsage : 0;
        privateUsages = isEd25519 ? usages & kSignUsage : usages;
    }
    if (!privateUsages) {
        throwError(state, WebCryptoError::Syntax,
                   "The private key usages must not be empty.");
    }
    return createKeyPair(state, context, keyAlgorithm, key, extractable,
                         publicUsages, privateUsages);
}

} // namespace Starfish

#endif
