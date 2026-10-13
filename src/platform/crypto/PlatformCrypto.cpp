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

// This file must not include Starfish headers. See PlatformCrypto.h.
#include "PlatformCryptoInternal.h"

#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

namespace Starfish {

using namespace PlatformCryptoInternal;

namespace {

    const size_t kAesBlockSize = 16;

    const EVP_CIPHER* aesCtrCipher(size_t keySize)
    {
        switch (keySize) {
        case 16:
            return EVP_aes_128_ctr();
        case 24:
            return EVP_aes_192_ctr();
        case 32:
            return EVP_aes_256_ctr();
        }
        return nullptr;
    }

    const EVP_CIPHER* aesCbcCipher(size_t keySize)
    {
        switch (keySize) {
        case 16:
            return EVP_aes_128_cbc();
        case 24:
            return EVP_aes_192_cbc();
        case 32:
            return EVP_aes_256_cbc();
        }
        return nullptr;
    }

    // EVP_CipherInit_ex uses PKCS #7 padding by default, as WebCrypto does.
    bool aesCbcCrypt(CryptoBytes key, CryptoBytes iv, CryptoBytes input,
                     bool isEncrypt, std::vector<uint8_t>* out)
    {
        ErrorQueueScope errorScope;
        const EVP_CIPHER* cipher = aesCbcCipher(key.size);
        if (!cipher || iv.size != kAesBlockSize || !fitsInInt(input.size) ||
            input.size > static_cast<size_t>(INT_MAX) - kAesBlockSize) {
            return false;
        }
        CipherContext context(EVP_CIPHER_CTX_new());
        if (!context ||
            EVP_CipherInit_ex(context.get(), cipher, nullptr, key.data, iv.data,
                              isEncrypt ? 1 : 0) != 1) {
            return false;
        }
        std::vector<uint8_t> result(input.size + kAesBlockSize);
        int size = 0;
        if (input.size > 0 &&
            EVP_CipherUpdate(context.get(), result.data(), &size, input.data,
                             static_cast<int>(input.size)) != 1) {
            return false;
        }
        int finalSize = 0;
        if (EVP_CipherFinal_ex(context.get(), result.data() + size,
                               &finalSize) != 1) {
            return false;
        }
        result.resize(static_cast<size_t>(size + finalSize));
        out->swap(result);
        return true;
    }

    const EVP_CIPHER* aesGcmCipher(size_t keySize)
    {
        switch (keySize) {
        case 16:
            return EVP_aes_128_gcm();
        case 24:
            return EVP_aes_192_gcm();
        case 32:
            return EVP_aes_256_gcm();
        }
        return nullptr;
    }

    const EVP_CIPHER* aesKwCipher(size_t keySize)
    {
        switch (keySize) {
        case 16:
            return EVP_aes_128_wrap();
        case 24:
            return EVP_aes_192_wrap();
        case 32:
            return EVP_aes_256_wrap();
        }
        return nullptr;
    }

    bool isValidGcmTagLength(size_t tagLengthBits)
    {
        switch (tagLengthBits) {
        case 32:
        case 64:
        case 96:
        case 104:
        case 112:
        case 120:
        case 128:
            return true;
        }
        return false;
    }

    // Runs AES-CTR over `size` bytes with the full 128-bit counter block `iv`.
    // OpenSSL increments the whole block, so the caller splits the input where
    // the WebCrypto counter would wrap.
    bool ctrCrypt(const EVP_CIPHER* cipher, const uint8_t* key,
                  const uint8_t* iv, const uint8_t* input, size_t size,
                  uint8_t* out)
    {
        if (size == 0) {
            return true;
        }
        CipherContext context(EVP_CIPHER_CTX_new());
        if (!context ||
            EVP_EncryptInit_ex(context.get(), cipher, nullptr, key, iv) != 1) {
            return false;
        }
        int outSize = 0;
        if (EVP_EncryptUpdate(context.get(), out, &outSize, input,
                              static_cast<int>(size)) != 1) {
            return false;
        }
        return static_cast<size_t>(outSize) == size;
    }

} // namespace

bool PlatformCrypto::randomBytes(uint8_t* out, size_t size)
{
    ErrorQueueScope errorScope;
    while (size > 0) {
        int chunk = fitsInInt(size) ? static_cast<int>(size) : INT_MAX;
        if (RAND_bytes(out, chunk) != 1) {
            return false;
        }
        out += chunk;
        size -= static_cast<size_t>(chunk);
    }
    return true;
}

void PlatformCrypto::cleanse(void* data, size_t size)
{
    OPENSSL_cleanse(data, size);
}

bool PlatformCrypto::aesCbcEncrypt(CryptoBytes key, CryptoBytes iv,
                                   CryptoBytes plaintext,
                                   std::vector<uint8_t>* out)
{
    return aesCbcCrypt(key, iv, plaintext, true, out);
}

bool PlatformCrypto::aesCbcDecrypt(CryptoBytes key, CryptoBytes iv,
                                   CryptoBytes input, std::vector<uint8_t>* out)
{
    return aesCbcCrypt(key, iv, input, false, out);
}

size_t PlatformCrypto::hashSize(CryptoHash hash)
{
    return static_cast<size_t>(EVP_MD_size(hashFunction(hash)));
}

bool PlatformCrypto::digest(CryptoHash hash, CryptoBytes data,
                            std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int resultSize = 0;
    if (EVP_Digest(nonNullData(data), data.size, result, &resultSize,
                   hashFunction(hash), nullptr) != 1) {
        return false;
    }
    out->assign(result, result + resultSize);
    return true;
}

bool PlatformCrypto::hmacSign(CryptoHash hash, CryptoBytes key,
                              CryptoBytes data, std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    if (!fitsInInt(key.size)) {
        return false;
    }
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int resultSize = 0;
    // HMAC() is the one-shot API. It is in OpenSSL 1.1.1, and OpenSSL 3
    // keeps it without deprecation (unlike HMAC_CTX).
    if (!HMAC(hashFunction(hash), nonNullData(key), static_cast<int>(key.size),
              nonNullData(data), data.size, result, &resultSize)) {
        return false;
    }
    out->assign(result, result + resultSize);
    return true;
}

bool PlatformCrypto::hmacVerify(CryptoHash hash, CryptoBytes key,
                                CryptoBytes data, CryptoBytes signature)
{
    std::vector<uint8_t> expected;
    if (!hmacSign(hash, key, data, &expected)) {
        return false;
    }
    // The length is public, so comparing it first leaks nothing.
    if (expected.size() != signature.size) {
        return false;
    }
    return CRYPTO_memcmp(expected.data(), signature.data, expected.size()) == 0;
}

bool PlatformCrypto::aesCtr(CryptoBytes key, CryptoBytes counter,
                            size_t counterLengthBits, CryptoBytes input,
                            std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    const EVP_CIPHER* cipher = aesCtrCipher(key.size);
    if (!cipher || counter.size != kAesBlockSize || counterLengthBits == 0 ||
        counterLengthBits > kAesBlockSize * 8 || !fitsInInt(input.size)) {
        return false;
    }

    // Find how many blocks fit before the counter wraps. That is
    // (all ones in the low bits) - (counter in the low bits) + 1.
    // The value can be up to 2^128, so it is kept as big-endian bytes.
    uint8_t untilWrap[kAesBlockSize];
    uint8_t wrappedCounter[kAesBlockSize];
    for (size_t i = 0; i < kAesBlockSize; i++) {
        size_t bitsBelowByteEnd = (kAesBlockSize - 1 - i) * 8;
        uint8_t mask;
        if (bitsBelowByteEnd >= counterLengthBits) {
            mask = 0;
        } else if (counterLengthBits - bitsBelowByteEnd >= 8) {
            mask = 0xff;
        } else {
            mask = static_cast<uint8_t>(
                (1 << (counterLengthBits - bitsBelowByteEnd)) - 1);
        }
        untilWrap[i] = static_cast<uint8_t>(~counter.data[i] & mask);
        // The counter after a wrap: the low bits become zero.
        wrappedCounter[i] = static_cast<uint8_t>(counter.data[i] & ~mask);
    }

    size_t blocks = (input.size + kAesBlockSize - 1) / kAesBlockSize;
    // untilWrap holds (blocks before wrap - 1). When it does not fit in 64
    // bits, it is larger than any input, so there is no wrap.
    bool isLarge = false;
    uint64_t blocksBeforeWrapMinusOne = 0;
    for (size_t i = 0; i < kAesBlockSize; i++) {
        if (i < kAesBlockSize - 8) {
            isLarge = isLarge || untilWrap[i] != 0;
        } else {
            blocksBeforeWrapMinusOne =
                (blocksBeforeWrapMinusOne << 8) | untilWrap[i];
        }
    }

    // A counter of n bits counts 2^n blocks. Using a counter value twice
    // would reuse the key stream.
    if (counterLengthBits < 64 &&
        static_cast<uint64_t>(blocks) >
            (static_cast<uint64_t>(1) << counterLengthBits)) {
        return false;
    }

    out->resize(input.size);
    if (isLarge || blocks <= blocksBeforeWrapMinusOne + 1) {
        return ctrCrypt(cipher, key.data, counter.data, input.data, input.size,
                        out->data());
    }

    size_t firstSize =
        static_cast<size_t>(blocksBeforeWrapMinusOne + 1) * kAesBlockSize;
    return ctrCrypt(cipher, key.data, counter.data, input.data, firstSize,
                    out->data()) &&
           ctrCrypt(cipher, key.data, wrappedCounter, input.data + firstSize,
                    input.size - firstSize, out->data() + firstSize);
}

bool PlatformCrypto::aesGcmEncrypt(CryptoBytes key, CryptoBytes iv,
                                   CryptoBytes additionalData,
                                   size_t tagLengthBits, CryptoBytes plaintext,
                                   std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    const EVP_CIPHER* cipher = aesGcmCipher(key.size);
    if (!cipher || iv.size == 0 || !isValidGcmTagLength(tagLengthBits) ||
        !fitsInInt(iv.size) || !fitsInInt(additionalData.size) ||
        !fitsInInt(plaintext.size)) {
        return false;
    }
    size_t tagSize = tagLengthBits / 8;

    // The IV length must be set before the IV. So select the cipher first,
    // then set the length, then set the key and the IV.
    CipherContext context(EVP_CIPHER_CTX_new());
    if (!context ||
        EVP_EncryptInit_ex(context.get(), cipher, nullptr, nullptr, nullptr) !=
            1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(iv.size), nullptr) != 1 ||
        EVP_EncryptInit_ex(context.get(), nullptr, nullptr, key.data,
                           iv.data) != 1) {
        return false;
    }

    int outSize = 0;
    if (additionalData.size > 0 &&
        EVP_EncryptUpdate(context.get(), nullptr, &outSize, additionalData.data,
                          static_cast<int>(additionalData.size)) != 1) {
        return false;
    }

    out->resize(plaintext.size + tagSize);
    size_t written = 0;
    if (plaintext.size > 0) {
        if (EVP_EncryptUpdate(context.get(), out->data(), &outSize,
                              plaintext.data,
                              static_cast<int>(plaintext.size)) != 1) {
            return false;
        }
        written = static_cast<size_t>(outSize);
    }
    // GCM is a stream mode, so final writes nothing. The buffer is still
    // one block in case a cipher does.
    uint8_t finalBlock[kAesBlockSize];
    if (EVP_EncryptFinal_ex(context.get(), finalBlock, &outSize) != 1 ||
        written + static_cast<size_t>(outSize) != plaintext.size) {
        return false;
    }
    return EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG,
                               static_cast<int>(tagSize),
                               out->data() + plaintext.size) == 1;
}

bool PlatformCrypto::aesGcmDecrypt(CryptoBytes key, CryptoBytes iv,
                                   CryptoBytes additionalData,
                                   size_t tagLengthBits, CryptoBytes input,
                                   std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    const EVP_CIPHER* cipher = aesGcmCipher(key.size);
    if (!cipher || iv.size == 0 || !isValidGcmTagLength(tagLengthBits) ||
        !fitsInInt(iv.size) || !fitsInInt(additionalData.size) ||
        !fitsInInt(input.size)) {
        return false;
    }
    size_t tagSize = tagLengthBits / 8;
    if (input.size < tagSize) {
        return false;
    }
    size_t ciphertextSize = input.size - tagSize;

    CipherContext context(EVP_CIPHER_CTX_new());
    if (!context ||
        EVP_DecryptInit_ex(context.get(), cipher, nullptr, nullptr, nullptr) !=
            1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(iv.size), nullptr) != 1 ||
        EVP_DecryptInit_ex(context.get(), nullptr, nullptr, key.data,
                           iv.data) != 1) {
        return false;
    }

    int outSize = 0;
    if (additionalData.size > 0 &&
        EVP_DecryptUpdate(context.get(), nullptr, &outSize, additionalData.data,
                          static_cast<int>(additionalData.size)) != 1) {
        return false;
    }

    std::vector<uint8_t> plaintext(ciphertextSize);
    size_t written = 0;
    if (ciphertextSize > 0) {
        if (EVP_DecryptUpdate(context.get(), plaintext.data(), &outSize,
                              input.data,
                              static_cast<int>(ciphertextSize)) != 1) {
            return false;
        }
        written = static_cast<size_t>(outSize);
    }
    // SET_TAG takes a non-const pointer but only reads the tag.
    if (EVP_CIPHER_CTX_ctrl(
            context.get(), EVP_CTRL_GCM_SET_TAG, static_cast<int>(tagSize),
            const_cast<uint8_t*>(input.data + ciphertextSize)) != 1) {
        return false;
    }
    // Final checks the tag. The plaintext is returned only after that.
    uint8_t finalBlock[kAesBlockSize];
    if (EVP_DecryptFinal_ex(context.get(), finalBlock, &outSize) != 1 ||
        written + static_cast<size_t>(outSize) != ciphertextSize) {
        return false;
    }
    out->swap(plaintext);
    return true;
}

namespace {

    bool aesKwCrypt(CryptoBytes key, CryptoBytes data, bool isWrap,
                    std::vector<uint8_t>* out)
    {
        ErrorQueueScope errorScope;
        const EVP_CIPHER* cipher = aesKwCipher(key.size);
        // Wrap needs at least two 64-bit blocks. Unwrap input has one more.
        size_t minimumSize = isWrap ? 16 : 24;
        if (!cipher || data.size < minimumSize || data.size % 8 != 0 ||
            !fitsInInt(data.size + 8)) {
            return false;
        }

        CipherContext context(EVP_CIPHER_CTX_new());
        if (!context) {
            return false;
        }
        // OpenSSL disables the wrap modes in EVP by default.
        EVP_CIPHER_CTX_set_flags(context.get(), EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);
        // A null IV selects the default IV of RFC 3394.
        if (EVP_CipherInit_ex(context.get(), cipher, nullptr, key.data, nullptr,
                              isWrap ? 1 : 0) != 1) {
            return false;
        }

        std::vector<uint8_t> result(isWrap ? data.size + 8 : data.size - 8);
        int outSize = 0;
        // Unwrap fails here when the integrity check fails.
        if (EVP_CipherUpdate(context.get(), result.data(), &outSize, data.data,
                             static_cast<int>(data.size)) <= 0 ||
            static_cast<size_t>(outSize) != result.size()) {
            return false;
        }
        out->swap(result);
        return true;
    }

} // namespace

bool PlatformCrypto::aesKwWrap(CryptoBytes key, CryptoBytes data,
                               std::vector<uint8_t>* out)
{
    return aesKwCrypt(key, data, true, out);
}

bool PlatformCrypto::aesKwUnwrap(CryptoBytes key, CryptoBytes data,
                                 std::vector<uint8_t>* out)
{
    return aesKwCrypt(key, data, false, out);
}

bool PlatformCrypto::hkdf(CryptoHash hash, CryptoBytes key, CryptoBytes salt,
                          CryptoBytes info, size_t length,
                          std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    if (!fitsInInt(key.size) || !fitsInInt(salt.size) ||
        !fitsInInt(info.size)) {
        return false;
    }
    if (length == 0) {
        out->clear();
        return true;
    }
    // EVP_PKEY_HKDF and these setters are in OpenSSL 1.1.1 (as macros) and
    // in OpenSSL 3 (as functions) with the same names.
    // An empty salt means a salt of zeros, as RFC 5869 defines.
    KeyContext context(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr));
    if (!context || EVP_PKEY_derive_init(context.get()) != 1 ||
        EVP_PKEY_CTX_set_hkdf_md(context.get(), hashFunction(hash)) != 1 ||
        EVP_PKEY_CTX_set1_hkdf_key(context.get(), nonNullData(key),
                                   static_cast<int>(key.size)) != 1 ||
        EVP_PKEY_CTX_set1_hkdf_salt(context.get(), nonNullData(salt),
                                    static_cast<int>(salt.size)) != 1 ||
        EVP_PKEY_CTX_add1_hkdf_info(context.get(), nonNullData(info),
                                    static_cast<int>(info.size)) != 1) {
        return false;
    }
    // Fails when length is more than 255 times the hash size.
    std::vector<uint8_t> result(length);
    size_t resultSize = length;
    if (EVP_PKEY_derive(context.get(), result.data(), &resultSize) != 1 ||
        resultSize != length) {
        return false;
    }
    out->swap(result);
    return true;
}

bool PlatformCrypto::pbkdf2(CryptoHash hash, CryptoBytes password,
                            CryptoBytes salt, uint32_t iterations,
                            size_t length, std::vector<uint8_t>* out)
{
    ErrorQueueScope errorScope;
    if (iterations == 0 || iterations > static_cast<uint32_t>(INT_MAX) ||
        !fitsInInt(password.size) || !fitsInInt(salt.size) ||
        !fitsInInt(length)) {
        return false;
    }
    if (length == 0) {
        out->clear();
        return true;
    }
    std::vector<uint8_t> result(length);
    if (PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(nonNullData(password)),
                          static_cast<int>(password.size), nonNullData(salt),
                          static_cast<int>(salt.size),
                          static_cast<int>(iterations), hashFunction(hash),
                          static_cast<int>(length), result.data()) != 1) {
        return false;
    }
    out->swap(result);
    return true;
}

} // namespace Starfish
