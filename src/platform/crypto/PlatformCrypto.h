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

#ifndef __StarfishPlatformCrypto__
#define __StarfishPlatformCrypto__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

// Crypto operations for WebCrypto, on top of OpenSSL.
//
// This layer works on bytes only. It does not use the JS engine or other
// Starfish code, so it can be tested alone and moved to another host.
// It builds with C++11 and with OpenSSL 1.1.1 and 3. It uses only EVP APIs
// that both versions have.
//
// Every function returns false when the operation fails. The caller maps a
// failure to the WebCrypto error. The OpenSSL error queue is always empty
// when a function returns, so a failure does not affect the next call.
namespace Starfish {

// A read-only byte range. It does not own the bytes.
struct CryptoBytes {
    CryptoBytes()
        : data(nullptr)
        , size(0)
    {
    }
    CryptoBytes(const uint8_t* data, size_t size)
        : data(data)
        , size(size)
    {
    }
    CryptoBytes(const std::vector<uint8_t>& bytes)
        : data(bytes.data())
        , size(bytes.size())
    {
    }

    const uint8_t* data;
    size_t size;
};

enum class CryptoHash {
    SHA1,
    SHA256,
    SHA384,
    SHA512,
};

enum class CryptoKeyKind {
    RSA,
    EC,
    Ed25519,
    X25519,
};

enum class CryptoCurve {
    P256,
    P384,
    P521,
};

enum class RsaSignaturePadding {
    PKCS1v15,
    PSS,
};

// The big-endian integers of an RSA key, as JWK uses them (RFC 7518, 6.3).
// A public key has only `n` and `e`. A private key has all of them.
struct RsaKeyComponents {
    std::vector<uint8_t> n, e, d, p, q, dp, dq, qi;
};

// An RSA, EC, Ed25519 or X25519 key. Copies share the same key, which does
// not change after it is created. A default AsymmetricKey is null.
class AsymmetricKey {
public:
    struct Impl;

    AsymmetricKey()
    {
    }
    explicit AsymmetricKey(std::shared_ptr<Impl> impl)
        : m_impl(std::move(impl))
    {
    }

    bool isNull() const
    {
        return !m_impl;
    }
    bool isPrivate() const;
    CryptoKeyKind kind() const;
    // EC keys only.
    CryptoCurve curve() const;
    // RSA keys only.
    size_t rsaModulusBits() const;
    std::vector<uint8_t> rsaPublicExponent() const;

    // The public half. For a public key, returns the same key.
    AsymmetricKey publicKey() const;

    const std::shared_ptr<Impl>& impl() const
    {
        return m_impl;
    }

private:
    std::shared_ptr<Impl> m_impl;
};

class PlatformCrypto {
public:
    // Fills `out` with random bytes from the OpenSSL CSPRNG.
    static bool randomBytes(uint8_t* out, size_t size);
    // Overwrites memory so that a compiler cannot remove the write. Use it
    // for key material before the memory is freed.
    static void cleanse(void* data, size_t size);

    // Output size of the hash in bytes.
    static size_t hashSize(CryptoHash hash);

    static bool digest(CryptoHash hash, CryptoBytes data,
                       std::vector<uint8_t>* out);

    static bool hmacSign(CryptoHash hash, CryptoBytes key, CryptoBytes data,
                         std::vector<uint8_t>* out);
    // Compares in constant time. Returns false for a wrong signature, and
    // also when the operation fails.
    static bool hmacVerify(CryptoHash hash, CryptoBytes key, CryptoBytes data,
                           CryptoBytes signature);

    // AES-CTR. The rightmost `counterLengthBits` bits of `counter` are the
    // block counter. They wrap to zero, and the other bits do not change.
    // Fails when the input needs more blocks than the counter can count.
    static bool aesCtr(CryptoBytes key, CryptoBytes counter,
                       size_t counterLengthBits, CryptoBytes input,
                       std::vector<uint8_t>* out);

    // AES-CBC with PKCS #7 padding. `iv` is 16 bytes. Decrypt fails when
    // the padding is wrong.
    static bool aesCbcEncrypt(CryptoBytes key, CryptoBytes iv,
                              CryptoBytes plaintext, std::vector<uint8_t>* out);
    static bool aesCbcDecrypt(CryptoBytes key, CryptoBytes iv,
                              CryptoBytes input, std::vector<uint8_t>* out);

    // AES-GCM. The output of encrypt is the ciphertext and then the tag.
    // Decrypt takes the same layout and fails when the tag does not match.
    // `tagLengthBits` is one of 32, 64, 96, 104, 112, 120, 128.
    static bool aesGcmEncrypt(CryptoBytes key, CryptoBytes iv,
                              CryptoBytes additionalData, size_t tagLengthBits,
                              CryptoBytes plaintext, std::vector<uint8_t>* out);
    static bool aesGcmDecrypt(CryptoBytes key, CryptoBytes iv,
                              CryptoBytes additionalData, size_t tagLengthBits,
                              CryptoBytes input, std::vector<uint8_t>* out);

    // AES Key Wrap (RFC 3394). The data to wrap is a multiple of 8 bytes and
    // at least 16 bytes.
    static bool aesKwWrap(CryptoBytes key, CryptoBytes data,
                          std::vector<uint8_t>* out);
    static bool aesKwUnwrap(CryptoBytes key, CryptoBytes data,
                            std::vector<uint8_t>* out);

    // HKDF (RFC 5869) and PBKDF2 (RFC 8018) with HMAC. `length` is in bytes.
    // A length of 0 gives an empty result.
    static bool hkdf(CryptoHash hash, CryptoBytes key, CryptoBytes salt,
                     CryptoBytes info, size_t length,
                     std::vector<uint8_t>* out);
    static bool pbkdf2(CryptoHash hash, CryptoBytes password, CryptoBytes salt,
                       uint32_t iterations, size_t length,
                       std::vector<uint8_t>* out);

    // Key generation. Each returns the private key. Use publicKey() for the
    // public half. RSA fails when the exponent is not odd and above 1.
    static bool generateRsaKey(size_t modulusBits, CryptoBytes publicExponent,
                               AsymmetricKey* out);
    static bool generateEcKey(CryptoCurve curve, AsymmetricKey* out);
    static bool generateOkpKey(CryptoKeyKind kind, AsymmetricKey* out);

    // DER SubjectPublicKeyInfo and PKCS #8 PrivateKeyInfo. Import fails when
    // the key is not of `kind`, or when there are extra bytes after it. An
    // RSA key must use rsaEncryption, which WebCrypto uses for all RSA
    // algorithms.
    static bool importSpki(CryptoKeyKind kind, CryptoBytes der,
                           AsymmetricKey* out);
    static bool importPkcs8(CryptoKeyKind kind, CryptoBytes der,
                            AsymmetricKey* out);
    static bool exportSpki(const AsymmetricKey& key, std::vector<uint8_t>* out);
    static bool exportPkcs8(const AsymmetricKey& key,
                            std::vector<uint8_t>* out);

    // EC public point in SEC 1 form. Import takes the uncompressed or the
    // compressed form. Export writes the uncompressed form.
    static bool importEcPoint(CryptoCurve curve, CryptoBytes point,
                              AsymmetricKey* out);
    static bool exportEcPoint(const AsymmetricKey& key,
                              std::vector<uint8_t>* out);
    // EC key from JWK coordinates. Each value has the size of the curve
    // field. An empty `d` makes a public key. Fails when `d` does not match
    // the point, or the point is not on the curve.
    static bool importEcCoordinates(CryptoCurve curve, CryptoBytes x,
                                    CryptoBytes y, CryptoBytes d,
                                    AsymmetricKey* out);
    // `d` is written only for a private key.
    static bool exportEcCoordinates(const AsymmetricKey& key,
                                    std::vector<uint8_t>* x,
                                    std::vector<uint8_t>* y,
                                    std::vector<uint8_t>* d);

    // Ed25519 and X25519 raw keys (RFC 8032, RFC 7748). An empty
    // `privateKey` makes a public key. With both, the public key must match.
    static bool importOkp(CryptoKeyKind kind, CryptoBytes publicKey,
                          CryptoBytes privateKey, AsymmetricKey* out);
    // `privateKey` is written only for a private key.
    static bool exportOkp(const AsymmetricKey& key,
                          std::vector<uint8_t>* publicKey,
                          std::vector<uint8_t>* privateKey);

    // RSA key from JWK integers. A private key needs all of the values.
    static bool importRsaComponents(const RsaKeyComponents& components,
                                    AsymmetricKey* out);
    static bool exportRsaComponents(const AsymmetricKey& key,
                                    RsaKeyComponents* out);

    // RSASSA-PKCS1-v1_5 and RSA-PSS. `saltLength` is in bytes and is used
    // only for PSS. Verify returns false for a wrong signature, and also
    // when the operation fails.
    static bool rsaSign(const AsymmetricKey& key, RsaSignaturePadding padding,
                        CryptoHash hash, size_t saltLength, CryptoBytes data,
                        std::vector<uint8_t>* out);
    static bool rsaVerify(const AsymmetricKey& key, RsaSignaturePadding padding,
                          CryptoHash hash, size_t saltLength, CryptoBytes data,
                          CryptoBytes signature);

    // RSA-OAEP with MGF1 using the same hash. An empty label is no label.
    static bool rsaOaepEncrypt(const AsymmetricKey& key, CryptoHash hash,
                               CryptoBytes label, CryptoBytes data,
                               std::vector<uint8_t>* out);
    static bool rsaOaepDecrypt(const AsymmetricKey& key, CryptoHash hash,
                               CryptoBytes label, CryptoBytes data,
                               std::vector<uint8_t>* out);

    // ECDSA. The signature is r || s, each padded to the size of the curve
    // order (the WebCrypto form). OpenSSL uses DER, so this converts.
    static bool ecdsaSign(const AsymmetricKey& key, CryptoHash hash,
                          CryptoBytes data, std::vector<uint8_t>* out);
    static bool ecdsaVerify(const AsymmetricKey& key, CryptoHash hash,
                            CryptoBytes data, CryptoBytes signature);

    static bool ed25519Sign(const AsymmetricKey& key, CryptoBytes data,
                            std::vector<uint8_t>* out);
    // Returns false when the public key or R is a point of small order, as
    // the WebCrypto spec requires.
    static bool ed25519Verify(const AsymmetricKey& key, CryptoBytes data,
                              CryptoBytes signature);

    // ECDH and X25519. Both keys must be of the same kind and curve. The
    // result is the full shared secret. X25519 fails when the secret is all
    // zeros, which happens with a small-order public key.
    static bool deriveSharedSecret(const AsymmetricKey& privateKey,
                                   const AsymmetricKey& publicKey,
                                   std::vector<uint8_t>* out);
};

} // namespace Starfish

#endif
