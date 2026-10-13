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

// RSA, EC, Ed25519 and X25519.
//
// OpenSSL 1.1.1 has no generic API to read or build key components. So this
// file uses the RSA and EC_KEY APIs for that. OpenSSL 3 marks them
// deprecated but still has them, and Starfish builds with
// -Wno-deprecated-declarations. Everything else uses EVP.
//
// This file must not include Starfish headers. See PlatformCrypto.h.
#include "PlatformCryptoInternal.h"

#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <algorithm>
#include <cstring>

namespace Starfish {

using namespace PlatformCryptoInternal;

namespace {

    struct EcKeyDeleter {
        void operator()(EC_KEY* p) const
        {
            EC_KEY_free(p);
        }
        void operator()(EC_POINT* p) const
        {
            EC_POINT_free(p);
        }
        void operator()(ECDSA_SIG* p) const
        {
            ECDSA_SIG_free(p);
        }
        void operator()(RSA* p) const
        {
            RSA_free(p);
        }
        void operator()(PKCS8_PRIV_KEY_INFO* p) const
        {
            PKCS8_PRIV_KEY_INFO_free(p);
        }
    };
    typedef std::unique_ptr<EC_KEY, EcKeyDeleter> EcKeyPointer;
    typedef std::unique_ptr<EC_POINT, EcKeyDeleter> EcPointPointer;
    typedef std::unique_ptr<ECDSA_SIG, EcKeyDeleter> EcdsaSignaturePointer;
    typedef std::unique_ptr<RSA, EcKeyDeleter> RsaPointer;
    typedef std::unique_ptr<PKCS8_PRIV_KEY_INFO, EcKeyDeleter> Pkcs8Pointer;

    int curveNid(CryptoCurve curve)
    {
        switch (curve) {
        case CryptoCurve::P256:
            return NID_X9_62_prime256v1;
        case CryptoCurve::P384:
            return NID_secp384r1;
        case CryptoCurve::P521:
            return NID_secp521r1;
        }
        return NID_undef;
    }

    // Size in bytes of a field element and of the group order. They are
    // the same for these three curves.
    size_t curveSize(CryptoCurve curve)
    {
        switch (curve) {
        case CryptoCurve::P256:
            return 32;
        case CryptoCurve::P384:
            return 48;
        case CryptoCurve::P521:
            return 66;
        }
        return 0;
    }

    int okpKeyType(CryptoKeyKind kind)
    {
        switch (kind) {
        case CryptoKeyKind::Ed25519:
            return EVP_PKEY_ED25519;
        case CryptoKeyKind::X25519:
            return EVP_PKEY_X25519;
        default:
            return NID_undef;
        }
    }

    bool isKind(EVP_PKEY* key, CryptoKeyKind kind)
    {
        int id = EVP_PKEY_id(key);
        switch (kind) {
        case CryptoKeyKind::RSA:
            return id == EVP_PKEY_RSA;
        case CryptoKeyKind::EC:
            return id == EVP_PKEY_EC;
        case CryptoKeyKind::Ed25519:
            return id == EVP_PKEY_ED25519;
        case CryptoKeyKind::X25519:
            return id == EVP_PKEY_X25519;
        }
        return false;
    }

    // Only P-256, P-384 and P-521 are allowed, even if OpenSSL has more.
    bool curveOfKey(EVP_PKEY* key, CryptoCurve* out)
    {
        const EC_KEY* ecKey = EVP_PKEY_get0_EC_KEY(key);
        if (!ecKey) {
            return false;
        }
        switch (EC_GROUP_get_curve_name(EC_KEY_get0_group(ecKey))) {
        case NID_X9_62_prime256v1:
            *out = CryptoCurve::P256;
            return true;
        case NID_secp384r1:
            *out = CryptoCurve::P384;
            return true;
        case NID_secp521r1:
            *out = CryptoCurve::P521;
            return true;
        }
        return false;
    }

    // True when a 32-byte Ed25519 point encoding is a point of small order
    // (1, 2, 4 or 8), or one of the non-canonical encodings of such a point.
    // The list is from libsodium (crypto_core/ed25519/ref10). The sign bit
    // is ignored, so each entry also covers the encoding with that bit set.
    // Each entry was checked by decoding it and multiplying by its order.
    bool isSmallOrderEd25519Point(const uint8_t* point)
    {
        static const uint8_t kBlocklist[][32] = {
            // 0 (order 4)
            { 0 },
            // 1 (order 1)
            { 0x01 },
            // order 8
            { 0x26, 0xe8, 0x95, 0x8f, 0xc2, 0xb2, 0x27, 0xb0, 0x45, 0xc3, 0xf4,
              0x89, 0xf2, 0xef, 0x98, 0xf0, 0xd5, 0xdf, 0xac, 0x05, 0xd3, 0xc6,
              0x33, 0x39, 0xb1, 0x38, 0x02, 0x88, 0x6d, 0x53, 0xfc, 0x05 },
            // order 8
            { 0xc7, 0x17, 0x6a, 0x70, 0x3d, 0x4d, 0xd8, 0x4f, 0xba, 0x3c, 0x0b,
              0x76, 0x0d, 0x10, 0x67, 0x0f, 0x2a, 0x20, 0x53, 0xfa, 0x2c, 0x39,
              0xcc, 0xc6, 0x4e, 0xc7, 0xfd, 0x77, 0x92, 0xac, 0x03, 0x7a },
            // p - 1 (order 2)
            { 0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
            // p, the non-canonical 0 (order 4)
            { 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
            // p + 1, the non-canonical 1 (order 1)
            { 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
        };
        for (const uint8_t* entry : kBlocklist) {
            bool isSame = true;
            for (size_t i = 0; i < 31 && isSame; i++) {
                isSame = point[i] == entry[i];
            }
            if (isSame && (point[31] & 0x7f) == entry[31]) {
                return true;
            }
        }
        return false;
    }

    // Rebuilds an EC key so that OpenSSL encodes its point uncompressed.
    // A key parsed from a compressed point keeps that form, and spki or
    // pkcs8 export would write it compressed. WebCrypto exports
    // uncompressed.
    bool rebuildEcKey(const AsymmetricKey& key, AsymmetricKey* out)
    {
        std::vector<uint8_t> x, y, d;
        if (!PlatformCrypto::exportEcCoordinates(key, &x, &y, &d)) {
            return false;
        }
        bool ok = PlatformCrypto::importEcCoordinates(
            key.curve(), x, y, key.isPrivate() ? CryptoBytes(d) : CryptoBytes(),
            out);
        if (!d.empty()) {
            OPENSSL_cleanse(d.data(), d.size());
        }
        return ok;
    }

    struct BignumContextDeleter {
        void operator()(BN_CTX* p) const
        {
            BN_CTX_free(p);
        }
    };
    typedef std::unique_ptr<BN_CTX, BignumContextDeleter> BignumContext;

    // Checks that the values of a private RSA key belong together:
    // n = p * q, dp = d mod (p - 1), dq = d mod (q - 1) and
    // qi * q = 1 mod p.
    // RSA_check_key() also tests that p and q are prime. That is slow: the
    // WPT rsa_importKey test timed out with it in a Debug build. A key with
    // wrong primes cannot pass the checks above anyway. BoringSSL, used by
    // Chrome, does the same reduced check.
    bool isConsistentRsaKey(const RSA* rsa)
    {
        const BIGNUM *n, *e, *d, *p, *q, *dp, *dq, *qi;
        RSA_get0_key(rsa, &n, &e, &d);
        RSA_get0_factors(rsa, &p, &q);
        RSA_get0_crt_params(rsa, &dp, &dq, &qi);
        if (!n || !e || !d || !p || !q || !dp || !dq || !qi) {
            return false;
        }
        BignumContext context(BN_CTX_new());
        BignumPointer value(BN_new());
        BignumPointer one(BN_new());
        BignumPointer minusOne(BN_new());
        if (!context || !value || !one || !minusOne || !BN_one(one.get())) {
            return false;
        }
        // n = p * q
        if (!BN_mul(value.get(), p, q, context.get()) ||
            BN_cmp(value.get(), n) != 0) {
            return false;
        }
        // dp = d mod (p - 1) and dq = d mod (q - 1)
        const BIGNUM* primes[] = { p, q };
        const BIGNUM* exponents[] = { dp, dq };
        for (size_t i = 0; i < 2; i++) {
            if (!BN_sub(minusOne.get(), primes[i], one.get()) ||
                !BN_mod(value.get(), d, minusOne.get(), context.get()) ||
                BN_cmp(value.get(), exponents[i]) != 0) {
                return false;
            }
        }
        // qi * q = 1 mod p
        return BN_mod_mul(value.get(), qi, q, p, context.get()) &&
               BN_is_one(value.get());
    }

    AsymmetricKey wrapKey(EVP_PKEY* key, bool isPrivate)
    {
        return AsymmetricKey(
            std::make_shared<AsymmetricKey::Impl>(key, isPrivate));
    }

    EVP_PKEY* keyOf(const AsymmetricKey& key)
    {
        return key.isNull() ? nullptr : key.impl()->key;
    }

    bool toVector(const uint8_t* data, int size, std::vector<uint8_t>* out)
    {
        if (size < 0) {
            return false;
        }
        out->assign(data, data + size);
        return true;
    }

    // Encodes with an OpenSSL i2d_* function. The function allocates the
    // buffer when given a null pointer.
    template <typename T, typename Encode>
    bool encodeDer(T* object, Encode encode, std::vector<uint8_t>* out)
    {
        unsigned char* buffer = nullptr;
        int size = encode(object, &buffer);
        bool ok = size > 0 && toVector(buffer, size, out);
        OPENSSL_free(buffer);
        return ok;
    }

    bool keyFromEcKey(EcKeyPointer ecKey, bool isPrivate, AsymmetricKey* out)
    {
        KeyPointer key(EVP_PKEY_new());
        // set1 takes its own reference, so ecKey is freed here as usual.
        if (!key || EVP_PKEY_set1_EC_KEY(key.get(), ecKey.get()) != 1) {
            return false;
        }
        *out = wrapKey(key.release(), isPrivate);
        return true;
    }

    bool bignumToPadded(const BIGNUM* value, size_t size,
                        std::vector<uint8_t>* out)
    {
        out->resize(size);
        return BN_bn2binpad(value, out->data(), static_cast<int>(size)) ==
               static_cast<int>(size);
    }

    // JWK stores RSA integers without leading zeros.
    std::vector<uint8_t> bignumToBytes(const BIGNUM* value)
    {
        std::vector<uint8_t> bytes(static_cast<size_t>(BN_num_bytes(value)));
        BN_bn2bin(value, bytes.data());
        return bytes;
    }

    BIGNUM* bytesToBignum(CryptoBytes bytes)
    {
        if (bytes.size == 0 || !fitsInInt(bytes.size)) {
            return nullptr;
        }
        return BN_bin2bn(bytes.data, static_cast<int>(bytes.size), nullptr);
    }

    bool generateKey(int keyType, int curve, AsymmetricKey* out)
    {
        KeyContext context(EVP_PKEY_CTX_new_id(keyType, nullptr));
        if (!context || EVP_PKEY_keygen_init(context.get()) != 1) {
            return false;
        }
        if (keyType == EVP_PKEY_EC &&
            (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(context.get(), curve) !=
                 1 ||
             EVP_PKEY_CTX_set_ec_param_enc(context.get(),
                                           OPENSSL_EC_NAMED_CURVE) != 1)) {
            return false;
        }
        EVP_PKEY* key = nullptr;
        if (EVP_PKEY_keygen(context.get(), &key) != 1) {
            return false;
        }
        *out = wrapKey(key, true);
        return true;
    }

    // Sets up an EVP_DigestSign or EVP_DigestVerify context. Ed25519 uses no
    // hash, so `hash` is null for it.
    bool initDigestContext(EVP_MD_CTX* context, EVP_PKEY* key,
                           const EVP_MD* hash, bool isSign,
                           EVP_PKEY_CTX** keyContext)
    {
        if (isSign) {
            return EVP_DigestSignInit(context, keyContext, hash, nullptr,
                                      key) == 1;
        }
        return EVP_DigestVerifyInit(context, keyContext, hash, nullptr, key) ==
               1;
    }

    bool setRsaSignaturePadding(EVP_PKEY_CTX* context,
                                RsaSignaturePadding padding, const EVP_MD* hash,
                                size_t saltLength)
    {
        if (padding == RsaSignaturePadding::PKCS1v15) {
            return EVP_PKEY_CTX_set_rsa_padding(context, RSA_PKCS1_PADDING) ==
                   1;
        }
        return fitsInInt(saltLength) &&
               EVP_PKEY_CTX_set_rsa_padding(context, RSA_PKCS1_PSS_PADDING) ==
                   1 &&
               EVP_PKEY_CTX_set_rsa_pss_saltlen(
                   context, static_cast<int>(saltLength)) == 1 &&
               EVP_PKEY_CTX_set_rsa_mgf1_md(context, hash) == 1;
    }

    bool signWith(EVP_MD_CTX* context, CryptoBytes data,
                  std::vector<uint8_t>* out)
    {
        size_t size = 0;
        if (EVP_DigestSign(context, nullptr, &size, nonNullData(data),
                           data.size) != 1) {
            return false;
        }
        out->resize(size);
        if (EVP_DigestSign(context, out->data(), &size, nonNullData(data),
                           data.size) != 1) {
            return false;
        }
        out->resize(size);
        return true;
    }

    bool verifyWith(EVP_MD_CTX* context, CryptoBytes data,
                    CryptoBytes signature)
    {
        // 1 means valid, 0 means invalid, and a negative value is an error.
        return EVP_DigestVerify(context, nonNullData(signature), signature.size,
                                nonNullData(data), data.size) == 1;
    }

    bool setOaep(EVP_PKEY_CTX* context, CryptoHash hash, CryptoBytes label)
    {
        const EVP_MD* md = hashFunction(hash);
        if (EVP_PKEY_CTX_set_rsa_padding(context, RSA_PKCS1_OAEP_PADDING) !=
                1 ||
            EVP_PKEY_CTX_set_rsa_oaep_md(context, md) != 1 ||
            EVP_PKEY_CTX_set_rsa_mgf1_md(context, md) != 1) {
            return false;
        }
        if (label.size == 0) {
            return true;
        }
        if (!fitsInInt(label.size)) {
            return false;
        }
        // set0 takes ownership of the buffer when it succeeds.
        unsigned char* copy =
            static_cast<unsigned char*>(OPENSSL_malloc(label.size));
        if (!copy) {
            return false;
        }
        memcpy(copy, label.data, label.size);
        if (EVP_PKEY_CTX_set0_rsa_oaep_label(
                context, copy, static_cast<int>(label.size)) != 1) {
            OPENSSL_free(copy);
            return false;
        }
        return true;
    }

    bool oaepCrypt(const AsymmetricKey& key, CryptoHash hash, CryptoBytes label,
                   CryptoBytes data, bool isEncrypt, std::vector<uint8_t>* out)
    {
        EVP_PKEY* pkey = keyOf(key);
        if (!pkey || !isKind(pkey, CryptoKeyKind::RSA) ||
            (!isEncrypt && !key.isPrivate())) {
            return false;
        }
        KeyContext context(EVP_PKEY_CTX_new(pkey, nullptr));
        if (!context ||
            (isEncrypt ? EVP_PKEY_encrypt_init(context.get())
                       : EVP_PKEY_decrypt_init(context.get())) != 1 ||
            !setOaep(context.get(), hash, label)) {
            return false;
        }
        size_t size = 0;
        int (*crypt)(EVP_PKEY_CTX*, unsigned char*, size_t*,
                     const unsigned char*, size_t) =
            isEncrypt ? EVP_PKEY_encrypt : EVP_PKEY_decrypt;
        if (crypt(context.get(), nullptr, &size, nonNullData(data),
                  data.size) != 1) {
            return false;
        }
        // The size above is the maximum. An empty plaintext gives 0 later.
        std::vector<uint8_t> result(size);
        if (crypt(context.get(), result.data(), &size, nonNullData(data),
                  data.size) != 1) {
            return false;
        }
        result.resize(size);
        out->swap(result);
        return true;
    }

} // namespace

bool AsymmetricKey::isPrivate() const
{
    return m_impl && m_impl->isPrivate;
}

CryptoKeyKind AsymmetricKey::kind() const
{
    switch (EVP_PKEY_id(m_impl->key)) {
    case EVP_PKEY_EC:
        return CryptoKeyKind::EC;
    case EVP_PKEY_ED25519:
        return CryptoKeyKind::Ed25519;
    case EVP_PKEY_X25519:
        return CryptoKeyKind::X25519;
    default:
        return CryptoKeyKind::RSA;
    }
}

CryptoCurve AsymmetricKey::curve() const
{
    CryptoCurve curve = CryptoCurve::P256;
    curveOfKey(m_impl->key, &curve);
    return curve;
}

size_t AsymmetricKey::rsaModulusBits() const
{
    return static_cast<size_t>(EVP_PKEY_bits(m_impl->key));
}

std::vector<uint8_t> AsymmetricKey::rsaPublicExponent() const
{
    const RSA* rsa = EVP_PKEY_get0_RSA(m_impl->key);
    if (!rsa) {
        return std::vector<uint8_t>();
    }
    const BIGNUM* e = nullptr;
    RSA_get0_key(rsa, nullptr, &e, nullptr);
    return bignumToBytes(e);
}

AsymmetricKey AsymmetricKey::publicKey() const
{
    if (!isPrivate()) {
        return *this;
    }
    // Encoding the public half and reading it back works the same way for
    // every key type in both OpenSSL versions.
    ErrorQueueScope errorScope;
    std::vector<uint8_t> der;
    if (!encodeDer(m_impl->key, i2d_PUBKEY, &der)) {
        return AsymmetricKey();
    }
    const unsigned char* cursor = der.data();
    EVP_PKEY* key = d2i_PUBKEY(nullptr, &cursor, static_cast<long>(der.size()));
    return key ? wrapKey(key, false) : AsymmetricKey();
}

bool PlatformCrypto::generateRsaKey(size_t modulusBits,
                                    CryptoBytes publicExponent,
                                    AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    BignumPointer exponent(bytesToBignum(publicExponent));
    // An even exponent or 1 cannot make an RSA key. OpenSSL does not reject
    // all of them, so check here.
    if (!exponent || !BN_is_odd(exponent.get()) || BN_is_one(exponent.get()) ||
        !fitsInInt(modulusBits)) {
        return false;
    }
    KeyContext context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
    if (!context || EVP_PKEY_keygen_init(context.get()) != 1 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(),
                                         static_cast<int>(modulusBits)) != 1) {
        return false;
    }
    // Both versions take ownership of the exponent only when this succeeds.
    if (EVP_PKEY_CTX_set_rsa_keygen_pubexp(context.get(), exponent.get()) !=
        1) {
        return false;
    }
    exponent.release();
    EVP_PKEY* key = nullptr;
    if (EVP_PKEY_keygen(context.get(), &key) != 1) {
        return false;
    }
    *out = wrapKey(key, true);
    return true;
}

bool PlatformCrypto::generateEcKey(CryptoCurve curve, AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    return generateKey(EVP_PKEY_EC, curveNid(curve), out);
}

bool PlatformCrypto::generateOkpKey(CryptoKeyKind kind, AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    int type = okpKeyType(kind);
    return type != NID_undef && generateKey(type, NID_undef, out);
}

bool PlatformCrypto::importSpki(CryptoKeyKind kind, CryptoBytes der,
                                AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    if (der.size == 0 || !fitsInInt(der.size)) {
        return false;
    }
    const unsigned char* cursor = der.data;
    KeyPointer key(d2i_PUBKEY(nullptr, &cursor, static_cast<long>(der.size)));
    CryptoCurve curve;
    if (!key || cursor != der.data + der.size || !isKind(key.get(), kind) ||
        (kind == CryptoKeyKind::EC && !curveOfKey(key.get(), &curve))) {
        return false;
    }
    AsymmetricKey result = wrapKey(key.release(), false);
    if (kind == CryptoKeyKind::EC) {
        return rebuildEcKey(result, out);
    }
    *out = result;
    return true;
}

bool PlatformCrypto::importPkcs8(CryptoKeyKind kind, CryptoBytes der,
                                 AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    if (der.size == 0 || !fitsInInt(der.size)) {
        return false;
    }
    const unsigned char* cursor = der.data;
    Pkcs8Pointer info(
        d2i_PKCS8_PRIV_KEY_INFO(nullptr, &cursor, static_cast<long>(der.size)));
    if (!info || cursor != der.data + der.size) {
        return false;
    }
    KeyPointer key(EVP_PKCS82PKEY(info.get()));
    CryptoCurve curve;
    if (!key || !isKind(key.get(), kind) ||
        (kind == CryptoKeyKind::EC && !curveOfKey(key.get(), &curve))) {
        return false;
    }
    // An RSA or EC key read from DER is not checked for consistency.
    if (kind == CryptoKeyKind::RSA &&
        !isConsistentRsaKey(EVP_PKEY_get0_RSA(key.get()))) {
        return false;
    }
    if (kind == CryptoKeyKind::EC &&
        EC_KEY_check_key(EVP_PKEY_get0_EC_KEY(key.get())) != 1) {
        return false;
    }
    AsymmetricKey result = wrapKey(key.release(), true);
    if (kind == CryptoKeyKind::EC) {
        return rebuildEcKey(result, out);
    }
    *out = result;
    return true;
}

bool PlatformCrypto::exportSpki(const AsymmetricKey& key,
                                std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    return pkey && encodeDer(pkey, i2d_PUBKEY, out);
}

bool PlatformCrypto::exportPkcs8(const AsymmetricKey& key,
                                 std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    if (!pkey || !key.isPrivate()) {
        return false;
    }
    Pkcs8Pointer info(EVP_PKEY2PKCS8(pkey));
    return info && encodeDer(info.get(), i2d_PKCS8_PRIV_KEY_INFO, out);
}

bool PlatformCrypto::importEcPoint(CryptoCurve curve, CryptoBytes point,
                                   AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    EcKeyPointer ecKey(EC_KEY_new_by_curve_name(curveNid(curve)));
    if (!ecKey || point.size == 0) {
        return false;
    }
    const EC_GROUP* group = EC_KEY_get0_group(ecKey.get());
    EcPointPointer publicPoint(EC_POINT_new(group));
    // oct2point rejects points that are not on the curve.
    if (!publicPoint ||
        EC_POINT_oct2point(group, publicPoint.get(), point.data, point.size,
                           nullptr) != 1 ||
        EC_KEY_set_public_key(ecKey.get(), publicPoint.get()) != 1 ||
        EC_KEY_check_key(ecKey.get()) != 1) {
        return false;
    }
    return keyFromEcKey(std::move(ecKey), false, out);
}

bool PlatformCrypto::exportEcPoint(const AsymmetricKey& key,
                                   std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    const EC_KEY* ecKey = pkey ? EVP_PKEY_get0_EC_KEY(pkey) : nullptr;
    if (!ecKey) {
        return false;
    }
    const EC_GROUP* group = EC_KEY_get0_group(ecKey);
    const EC_POINT* point = EC_KEY_get0_public_key(ecKey);
    size_t size = EC_POINT_point2oct(
        group, point, POINT_CONVERSION_UNCOMPRESSED, nullptr, 0, nullptr);
    if (size == 0) {
        return false;
    }
    out->resize(size);
    return EC_POINT_point2oct(group, point, POINT_CONVERSION_UNCOMPRESSED,
                              out->data(), size, nullptr) == size;
}

bool PlatformCrypto::importEcCoordinates(CryptoCurve curve, CryptoBytes x,
                                         CryptoBytes y, CryptoBytes d,
                                         AsymmetricKey* out)
{
    size_t size = curveSize(curve);
    if (x.size != size || y.size != size || (d.size != 0 && d.size != size)) {
        return false;
    }
    // Build the uncompressed point 0x04 || x || y.
    std::vector<uint8_t> point(1 + 2 * size);
    point[0] = POINT_CONVERSION_UNCOMPRESSED;
    std::copy(x.data, x.data + size, point.begin() + 1);
    std::copy(y.data, y.data + size, point.begin() + 1 + size);
    AsymmetricKey publicKey;
    if (!importEcPoint(curve, point, &publicKey)) {
        return false;
    }
    if (d.size == 0) {
        *out = publicKey;
        return true;
    }

    ErrorQueueScope errorScope;
    EcKeyPointer ecKey(EC_KEY_dup(EVP_PKEY_get0_EC_KEY(publicKey.impl()->key)));
    BignumPointer privateValue(bytesToBignum(d));
    // check_key verifies that d times the generator is the public point.
    if (!ecKey || !privateValue ||
        EC_KEY_set_private_key(ecKey.get(), privateValue.get()) != 1 ||
        EC_KEY_check_key(ecKey.get()) != 1) {
        return false;
    }
    return keyFromEcKey(std::move(ecKey), true, out);
}

bool PlatformCrypto::exportEcCoordinates(const AsymmetricKey& key,
                                         std::vector<uint8_t>* x,
                                         std::vector<uint8_t>* y,
                                         std::vector<uint8_t>* d)
{
    std::vector<uint8_t> point;
    CryptoCurve curve;
    if (!exportEcPoint(key, &point) || !curveOfKey(keyOf(key), &curve)) {
        return false;
    }
    size_t size = curveSize(curve);
    if (point.size() != 1 + 2 * size) {
        return false;
    }
    x->assign(point.begin() + 1, point.begin() + 1 + size);
    y->assign(point.begin() + 1 + size, point.end());
    if (!key.isPrivate()) {
        return true;
    }
    const BIGNUM* privateValue =
        EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(keyOf(key)));
    return privateValue && bignumToPadded(privateValue, size, d);
}

bool PlatformCrypto::importOkp(CryptoKeyKind kind, CryptoBytes publicKey,
                               CryptoBytes privateKey, AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    int type = okpKeyType(kind);
    if (type == NID_undef) {
        return false;
    }
    if (privateKey.size == 0) {
        KeyPointer key(EVP_PKEY_new_raw_public_key(
            type, nullptr, publicKey.data, publicKey.size));
        if (!key) {
            return false;
        }
        *out = wrapKey(key.release(), false);
        return true;
    }
    KeyPointer key(EVP_PKEY_new_raw_private_key(type, nullptr, privateKey.data,
                                                privateKey.size));
    if (!key) {
        return false;
    }
    AsymmetricKey result = wrapKey(key.release(), true);
    // The public key comes from the private key. A given public key that
    // does not match it is a data error.
    if (publicKey.size != 0) {
        std::vector<uint8_t> derived;
        std::vector<uint8_t> unused;
        if (!exportOkp(result, &derived, &unused) ||
            derived.size() != publicKey.size ||
            CRYPTO_memcmp(derived.data(), publicKey.data, derived.size()) !=
                0) {
            return false;
        }
    }
    *out = result;
    return true;
}

bool PlatformCrypto::exportOkp(const AsymmetricKey& key,
                               std::vector<uint8_t>* publicKey,
                               std::vector<uint8_t>* privateKey)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    size_t size = 0;
    if (!pkey || EVP_PKEY_get_raw_public_key(pkey, nullptr, &size) != 1) {
        return false;
    }
    publicKey->resize(size);
    if (EVP_PKEY_get_raw_public_key(pkey, publicKey->data(), &size) != 1) {
        return false;
    }
    if (!key.isPrivate()) {
        return true;
    }
    if (EVP_PKEY_get_raw_private_key(pkey, nullptr, &size) != 1) {
        return false;
    }
    privateKey->resize(size);
    return EVP_PKEY_get_raw_private_key(pkey, privateKey->data(), &size) == 1;
}

bool PlatformCrypto::importRsaComponents(const RsaKeyComponents& components,
                                         AsymmetricKey* out)
{
    ErrorQueueScope errorScope;
    bool isPrivate = !components.d.empty();
    RsaPointer rsa(RSA_new());
    BignumPointer n(bytesToBignum(components.n));
    BignumPointer e(bytesToBignum(components.e));
    if (!rsa || !n || !e) {
        return false;
    }
    BignumPointer d, p, q, dp, dq, qi;
    if (isPrivate) {
        d.reset(bytesToBignum(components.d));
        p.reset(bytesToBignum(components.p));
        q.reset(bytesToBignum(components.q));
        dp.reset(bytesToBignum(components.dp));
        dq.reset(bytesToBignum(components.dq));
        qi.reset(bytesToBignum(components.qi));
        if (!d || !p || !q || !dp || !dq || !qi) {
            return false;
        }
    }
    // The set0 functions take ownership when they succeed.
    if (RSA_set0_key(rsa.get(), n.get(), e.get(), d.get()) != 1) {
        return false;
    }
    n.release();
    e.release();
    d.release();
    if (isPrivate) {
        if (RSA_set0_factors(rsa.get(), p.get(), q.get()) != 1) {
            return false;
        }
        p.release();
        q.release();
        if (RSA_set0_crt_params(rsa.get(), dp.get(), dq.get(), qi.get()) != 1) {
            return false;
        }
        dp.release();
        dq.release();
        qi.release();
        if (!isConsistentRsaKey(rsa.get())) {
            return false;
        }
    }
    KeyPointer key(EVP_PKEY_new());
    if (!key || EVP_PKEY_set1_RSA(key.get(), rsa.get()) != 1) {
        return false;
    }
    *out = wrapKey(key.release(), isPrivate);
    return true;
}

bool PlatformCrypto::exportRsaComponents(const AsymmetricKey& key,
                                         RsaKeyComponents* out)
{
    EVP_PKEY* pkey = keyOf(key);
    const RSA* rsa = pkey ? EVP_PKEY_get0_RSA(pkey) : nullptr;
    if (!rsa) {
        return false;
    }
    const BIGNUM *n, *e, *d;
    RSA_get0_key(rsa, &n, &e, &d);
    out->n = bignumToBytes(n);
    out->e = bignumToBytes(e);
    if (!key.isPrivate()) {
        return true;
    }
    const BIGNUM *p, *q, *dp, *dq, *qi;
    RSA_get0_factors(rsa, &p, &q);
    RSA_get0_crt_params(rsa, &dp, &dq, &qi);
    if (!d || !p || !q || !dp || !dq || !qi) {
        return false;
    }
    out->d = bignumToBytes(d);
    out->p = bignumToBytes(p);
    out->q = bignumToBytes(q);
    out->dp = bignumToBytes(dp);
    out->dq = bignumToBytes(dq);
    out->qi = bignumToBytes(qi);
    return true;
}

bool PlatformCrypto::rsaSign(const AsymmetricKey& key,
                             RsaSignaturePadding padding, CryptoHash hash,
                             size_t saltLength, CryptoBytes data,
                             std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    if (!pkey || !isKind(pkey, CryptoKeyKind::RSA) || !key.isPrivate()) {
        return false;
    }
    const EVP_MD* md = hashFunction(hash);
    DigestContext context(EVP_MD_CTX_new());
    EVP_PKEY_CTX* keyContext = nullptr;
    return context &&
           initDigestContext(context.get(), pkey, md, true, &keyContext) &&
           setRsaSignaturePadding(keyContext, padding, md, saltLength) &&
           signWith(context.get(), data, out);
}

bool PlatformCrypto::rsaVerify(const AsymmetricKey& key,
                               RsaSignaturePadding padding, CryptoHash hash,
                               size_t saltLength, CryptoBytes data,
                               CryptoBytes signature)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    if (!pkey || !isKind(pkey, CryptoKeyKind::RSA)) {
        return false;
    }
    const EVP_MD* md = hashFunction(hash);
    DigestContext context(EVP_MD_CTX_new());
    EVP_PKEY_CTX* keyContext = nullptr;
    return context &&
           initDigestContext(context.get(), pkey, md, false, &keyContext) &&
           setRsaSignaturePadding(keyContext, padding, md, saltLength) &&
           verifyWith(context.get(), data, signature);
}

bool PlatformCrypto::rsaOaepEncrypt(const AsymmetricKey& key, CryptoHash hash,
                                    CryptoBytes label, CryptoBytes data,
                                    std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    return oaepCrypt(key, hash, label, data, true, out);
}

bool PlatformCrypto::rsaOaepDecrypt(const AsymmetricKey& key, CryptoHash hash,
                                    CryptoBytes label, CryptoBytes data,
                                    std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    return oaepCrypt(key, hash, label, data, false, out);
}

bool PlatformCrypto::ecdsaSign(const AsymmetricKey& key, CryptoHash hash,
                               CryptoBytes data, std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    CryptoCurve curve;
    if (!pkey || !key.isPrivate() || !curveOfKey(pkey, &curve)) {
        return false;
    }
    DigestContext context(EVP_MD_CTX_new());
    std::vector<uint8_t> der;
    if (!context ||
        !initDigestContext(context.get(), pkey, hashFunction(hash), true,
                           nullptr) ||
        !signWith(context.get(), data, &der)) {
        return false;
    }
    const unsigned char* cursor = der.data();
    EcdsaSignaturePointer signature(
        d2i_ECDSA_SIG(nullptr, &cursor, static_cast<long>(der.size())));
    if (!signature) {
        return false;
    }
    const BIGNUM *r, *s;
    ECDSA_SIG_get0(signature.get(), &r, &s);
    size_t size = curveSize(curve);
    std::vector<uint8_t> rBytes, sBytes;
    if (!bignumToPadded(r, size, &rBytes) ||
        !bignumToPadded(s, size, &sBytes)) {
        return false;
    }
    rBytes.insert(rBytes.end(), sBytes.begin(), sBytes.end());
    out->swap(rBytes);
    return true;
}

bool PlatformCrypto::ecdsaVerify(const AsymmetricKey& key, CryptoHash hash,
                                 CryptoBytes data, CryptoBytes signature)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    CryptoCurve curve;
    if (!pkey || !curveOfKey(pkey, &curve)) {
        return false;
    }
    // A signature of the wrong size does not verify. It is not an error.
    size_t size = curveSize(curve);
    if (signature.size != 2 * size) {
        return false;
    }
    EcdsaSignaturePointer der(ECDSA_SIG_new());
    BignumPointer r(BN_bin2bn(signature.data, static_cast<int>(size), nullptr));
    BignumPointer s(
        BN_bin2bn(signature.data + size, static_cast<int>(size), nullptr));
    // set0 takes ownership when it succeeds.
    if (!der || !r || !s || ECDSA_SIG_set0(der.get(), r.get(), s.get()) != 1) {
        return false;
    }
    r.release();
    s.release();
    std::vector<uint8_t> encoded;
    DigestContext context(EVP_MD_CTX_new());
    return encodeDer(der.get(), i2d_ECDSA_SIG, &encoded) && context &&
           initDigestContext(context.get(), pkey, hashFunction(hash), false,
                             nullptr) &&
           verifyWith(context.get(), data, encoded);
}

bool PlatformCrypto::ed25519Sign(const AsymmetricKey& key, CryptoBytes data,
                                 std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    if (!pkey || !isKind(pkey, CryptoKeyKind::Ed25519) || !key.isPrivate()) {
        return false;
    }
    DigestContext context(EVP_MD_CTX_new());
    return context &&
           initDigestContext(context.get(), pkey, nullptr, true, nullptr) &&
           signWith(context.get(), data, out);
}

bool PlatformCrypto::ed25519Verify(const AsymmetricKey& key, CryptoBytes data,
                                   CryptoBytes signature)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* pkey = keyOf(key);
    if (!pkey || !isKind(pkey, CryptoKeyKind::Ed25519)) {
        return false;
    }
    // The WebCrypto spec rejects a small-order public key A and a
    // small-order R (the first half of the signature). OpenSSL accepts
    // some of them, so check here.
    uint8_t publicKey[32];
    size_t publicKeySize = sizeof(publicKey);
    if (EVP_PKEY_get_raw_public_key(pkey, publicKey, &publicKeySize) != 1 ||
        publicKeySize != 32 || isSmallOrderEd25519Point(publicKey) ||
        (signature.size == 64 && isSmallOrderEd25519Point(signature.data))) {
        return false;
    }
    DigestContext context(EVP_MD_CTX_new());
    return context &&
           initDigestContext(context.get(), pkey, nullptr, false, nullptr) &&
           verifyWith(context.get(), data, signature);
}

bool PlatformCrypto::deriveSharedSecret(const AsymmetricKey& privateKey,
                                        const AsymmetricKey& publicKey,
                                        std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    EVP_PKEY* own = keyOf(privateKey);
    EVP_PKEY* peer = keyOf(publicKey);
    if (!own || !peer || !privateKey.isPrivate() ||
        EVP_PKEY_id(own) != EVP_PKEY_id(peer)) {
        return false;
    }
    int id = EVP_PKEY_id(own);
    if (id == EVP_PKEY_EC) {
        CryptoCurve ownCurve, peerCurve;
        if (!curveOfKey(own, &ownCurve) || !curveOfKey(peer, &peerCurve) ||
            ownCurve != peerCurve) {
            return false;
        }
    } else if (id != EVP_PKEY_X25519) {
        return false;
    }
    KeyContext context(EVP_PKEY_CTX_new(own, nullptr));
    size_t size = 0;
    if (!context || EVP_PKEY_derive_init(context.get()) != 1 ||
        EVP_PKEY_derive_set_peer(context.get(), peer) != 1 ||
        EVP_PKEY_derive(context.get(), nullptr, &size) != 1) {
        return false;
    }
    // OpenSSL fails here for an X25519 result of all zeros.
    std::vector<uint8_t> secret(size);
    if (EVP_PKEY_derive(context.get(), secret.data(), &size) != 1) {
        return false;
    }
    secret.resize(size);
    out->swap(secret);
    return true;
}

} // namespace Starfish
