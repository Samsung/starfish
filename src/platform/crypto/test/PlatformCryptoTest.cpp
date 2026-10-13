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

// Known-answer tests from the RFCs and NIST documents that define each
// algorithm. The vectors are copied as written in those documents.

#include "PlatformCrypto.h"

#include <gtest/gtest.h>
#include <openssl/err.h>

#include <string>

using namespace Starfish;

namespace {

std::vector<uint8_t> fromHex(const std::string& hex)
{
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        bytes.push_back(
            static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

std::string toHex(const std::vector<uint8_t>& bytes)
{
    static const char kDigits[] = "0123456789abcdef";
    std::string hex;
    for (uint8_t byte : bytes) {
        hex.push_back(kDigits[byte >> 4]);
        hex.push_back(kDigits[byte & 0xf]);
    }
    return hex;
}

std::vector<uint8_t> fromText(const std::string& text)
{
    return std::vector<uint8_t>(text.begin(), text.end());
}

std::vector<uint8_t> concat(const std::vector<uint8_t>& a,
                            const std::vector<uint8_t>& b)
{
    std::vector<uint8_t> result(a);
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

} // namespace

// FIPS 180-2, appendix B.1 and C.1, and the empty message.
TEST(PlatformCrypto, Digest)
{
    std::vector<uint8_t> out;
    ASSERT_TRUE(
        PlatformCrypto::digest(CryptoHash::SHA1, fromText("abc"), &out));
    EXPECT_EQ("a9993e364706816aba3e25717850c26c9cd0d89d", toHex(out));
    ASSERT_TRUE(
        PlatformCrypto::digest(CryptoHash::SHA256, fromText("abc"), &out));
    EXPECT_EQ(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        toHex(out));
    ASSERT_TRUE(
        PlatformCrypto::digest(CryptoHash::SHA384, fromText("abc"), &out));
    EXPECT_EQ(
        "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5b"
        "ed8086072ba1e7cc2358baeca134c825a7",
        toHex(out));
    ASSERT_TRUE(
        PlatformCrypto::digest(CryptoHash::SHA512, fromText("abc"), &out));
    EXPECT_EQ(
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d3"
        "9a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54c"
        "a49f",
        toHex(out));
    ASSERT_TRUE(
        PlatformCrypto::digest(CryptoHash::SHA256, CryptoBytes(), &out));
    EXPECT_EQ(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        toHex(out));
}

// RFC 4231, test cases 1 and 2.
TEST(PlatformCrypto, Hmac)
{
    std::vector<uint8_t> key(20, 0x0b);
    std::vector<uint8_t> data = fromText("Hi There");
    std::vector<uint8_t> out;
    ASSERT_TRUE(PlatformCrypto::hmacSign(CryptoHash::SHA256, key, data, &out));
    EXPECT_EQ(
        "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7",
        toHex(out));
    EXPECT_TRUE(PlatformCrypto::hmacVerify(CryptoHash::SHA256, key, data, out));

    std::vector<uint8_t> wrong(out);
    wrong[0] ^= 1;
    EXPECT_FALSE(
        PlatformCrypto::hmacVerify(CryptoHash::SHA256, key, data, wrong));
    wrong.assign(out.begin(), out.end() - 1);
    EXPECT_FALSE(
        PlatformCrypto::hmacVerify(CryptoHash::SHA256, key, data, wrong));

    ASSERT_TRUE(PlatformCrypto::hmacSign(
        CryptoHash::SHA512, fromText("Jefe"),
        fromText("what do ya want for nothing?"), &out));
    EXPECT_EQ(
        "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505"
        "549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bc"
        "e737",
        toHex(out));
}

// NIST SP 800-38A, F.5.1 (CTR-AES128.Encrypt), first two blocks.
TEST(PlatformCrypto, AesCtr)
{
    std::vector<uint8_t> key = fromHex("2b7e151628aed2a6abf7158809cf4f3c");
    std::vector<uint8_t> counter = fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    std::vector<uint8_t> plaintext = fromHex(
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51");
    std::vector<uint8_t> out;
    ASSERT_TRUE(PlatformCrypto::aesCtr(key, counter, 128, plaintext, &out));
    EXPECT_EQ(
        "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff",
        toHex(out));

    // CTR is its own inverse.
    std::vector<uint8_t> back;
    ASSERT_TRUE(PlatformCrypto::aesCtr(key, counter, 128, out, &back));
    EXPECT_EQ(plaintext, back);

    ASSERT_TRUE(PlatformCrypto::aesCtr(key, counter, 128, CryptoBytes(), &out));
    EXPECT_TRUE(out.empty());
}

// With an 8-bit counter, the counter after ...feff is ...fe00. OpenSSL alone
// would use ...ff00. The second block must match a separate call that
// starts at ...fe00.
TEST(PlatformCrypto, AesCtrCounterWraps)
{
    std::vector<uint8_t> key = fromHex("2b7e151628aed2a6abf7158809cf4f3c");
    std::vector<uint8_t> counter = fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    std::vector<uint8_t> wrapped = fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfe00");
    std::vector<uint8_t> input(32, 0);

    std::vector<uint8_t> out;
    ASSERT_TRUE(PlatformCrypto::aesCtr(key, counter, 8, input, &out));
    std::vector<uint8_t> secondBlock;
    ASSERT_TRUE(PlatformCrypto::aesCtr(
        key, wrapped, 8, std::vector<uint8_t>(16, 0), &secondBlock));
    EXPECT_EQ(secondBlock, std::vector<uint8_t>(out.begin() + 16, out.end()));

    // A 1-bit counter counts two blocks. A third block would reuse the
    // key stream.
    EXPECT_FALSE(PlatformCrypto::aesCtr(key, counter, 1,
                                        std::vector<uint8_t>(48, 0), &out));
    EXPECT_FALSE(PlatformCrypto::aesCtr(key, counter, 0, input, &out));
    EXPECT_FALSE(PlatformCrypto::aesCtr(key, counter, 129, input, &out));
}

// NIST SP 800-38A, F.2.1 (CBC-AES128.Encrypt), first block. With padding,
// 16 bytes of input give 32 bytes of output.
TEST(PlatformCrypto, AesCbc)
{
    std::vector<uint8_t> key = fromHex("2b7e151628aed2a6abf7158809cf4f3c");
    std::vector<uint8_t> iv = fromHex("000102030405060708090a0b0c0d0e0f");
    std::vector<uint8_t> plaintext =
        fromHex("6bc1bee22e409f96e93d7e117393172a");
    std::vector<uint8_t> out;
    ASSERT_TRUE(PlatformCrypto::aesCbcEncrypt(key, iv, plaintext, &out));
    ASSERT_EQ(32u, out.size());
    EXPECT_EQ("7649abac8119b246cee98e9b12e9197d",
              toHex(std::vector<uint8_t>(out.begin(), out.begin() + 16)));
    std::vector<uint8_t> back;
    ASSERT_TRUE(PlatformCrypto::aesCbcDecrypt(key, iv, out, &back));
    EXPECT_EQ(plaintext, back);

    // An empty input gives one block of padding.
    ASSERT_TRUE(PlatformCrypto::aesCbcEncrypt(key, iv, CryptoBytes(), &out));
    EXPECT_EQ(16u, out.size());
    out.back() ^= 1;
    EXPECT_FALSE(PlatformCrypto::aesCbcDecrypt(key, iv, out, &back));
    EXPECT_FALSE(PlatformCrypto::aesCbcEncrypt(key, std::vector<uint8_t>(12, 0),
                                               plaintext, &out));
}

// "The Galois/Counter Mode of Operation (GCM)", McGrew and Viega, test
// cases 4 (96-bit IV) and 6 (480-bit IV).
TEST(PlatformCrypto, AesGcm)
{
    std::vector<uint8_t> key = fromHex("feffe9928665731c6d6a8f9467308308");
    std::vector<uint8_t> plaintext = fromHex(
        "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a31"
        "8a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39");
    std::vector<uint8_t> aad =
        fromHex("feedfacedeadbeeffeedfacedeadbeefabaddad2");

    struct {
        const char* iv;
        const char* ciphertext;
        const char* tag;
    } cases[] = {
        { "cafebabefacedbaddecaf888",
          "42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e"
          "21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091",
          "5bc94fbc3221a5db94fae95ae7121a47" },
        { "9313225df88406e555909c5aff5269aa6a7a9538534f7da1e4c303d2a318a728"
          "c3c0c95156809539fcf0e2429a6b525416aedbf5a0de6a57a637b39b",
          "8ce24998625615b603a033aca13fb894be9112a5c3a211a8ba262a3cca7e2ca7"
          "01e4a9a4fba43c90ccdcb281d48c7c6fd62875d2aca417034c34aee5",
          "619cc5aefffe0bfa462af43c1699d050" },
    };
    for (const auto& c : cases) {
        std::vector<uint8_t> iv = fromHex(c.iv);
        std::vector<uint8_t> expected =
            concat(fromHex(c.ciphertext), fromHex(c.tag));
        std::vector<uint8_t> out;
        ASSERT_TRUE(
            PlatformCrypto::aesGcmEncrypt(key, iv, aad, 128, plaintext, &out));
        EXPECT_EQ(toHex(expected), toHex(out));

        std::vector<uint8_t> back;
        ASSERT_TRUE(
            PlatformCrypto::aesGcmDecrypt(key, iv, aad, 128, out, &back));
        EXPECT_EQ(plaintext, back);
    }
}

// A shorter tag is the first bytes of the full tag (NIST SP 800-38D,
// 7.1). A wrong tag or wrong additional data must fail.
TEST(PlatformCrypto, AesGcmTagLength)
{
    std::vector<uint8_t> key = fromHex("feffe9928665731c6d6a8f9467308308");
    std::vector<uint8_t> iv = fromHex("cafebabefacedbaddecaf888");
    std::vector<uint8_t> plaintext = fromText("tag length test");
    std::vector<uint8_t> aad = fromText("aad");

    std::vector<uint8_t> full;
    ASSERT_TRUE(
        PlatformCrypto::aesGcmEncrypt(key, iv, aad, 128, plaintext, &full));
    std::vector<uint8_t> shortTag;
    ASSERT_TRUE(
        PlatformCrypto::aesGcmEncrypt(key, iv, aad, 96, plaintext, &shortTag));
    ASSERT_EQ(plaintext.size() + 12, shortTag.size());
    EXPECT_EQ(std::vector<uint8_t>(full.begin(), full.end() - 4), shortTag);

    std::vector<uint8_t> back;
    ASSERT_TRUE(
        PlatformCrypto::aesGcmDecrypt(key, iv, aad, 96, shortTag, &back));
    EXPECT_EQ(plaintext, back);

    std::vector<uint8_t> tampered(shortTag);
    tampered.back() ^= 1;
    EXPECT_FALSE(
        PlatformCrypto::aesGcmDecrypt(key, iv, aad, 96, tampered, &back));
    EXPECT_FALSE(PlatformCrypto::aesGcmDecrypt(key, iv, fromText("other"), 96,
                                               shortTag, &back));
    EXPECT_FALSE(
        PlatformCrypto::aesGcmEncrypt(key, iv, aad, 100, plaintext, &back));
    EXPECT_FALSE(PlatformCrypto::aesGcmEncrypt(key, CryptoBytes(), aad, 128,
                                               plaintext, &back));

    // An empty plaintext gives only the tag.
    ASSERT_TRUE(PlatformCrypto::aesGcmEncrypt(key, iv, CryptoBytes(), 128,
                                              CryptoBytes(), &full));
    EXPECT_EQ(16u, full.size());
    ASSERT_TRUE(PlatformCrypto::aesGcmDecrypt(key, iv, CryptoBytes(), 128, full,
                                              &back));
    EXPECT_TRUE(back.empty());
}

// RFC 3394, 4.1 (128-bit KEK, 128-bit key data).
TEST(PlatformCrypto, AesKw)
{
    std::vector<uint8_t> kek = fromHex("000102030405060708090a0b0c0d0e0f");
    std::vector<uint8_t> keyData = fromHex("00112233445566778899aabbccddeeff");
    std::vector<uint8_t> out;
    ASSERT_TRUE(PlatformCrypto::aesKwWrap(kek, keyData, &out));
    EXPECT_EQ("1fa68b0a8112b447aef34bd8fb5a7b829d3e862371d2cfe5", toHex(out));

    std::vector<uint8_t> back;
    ASSERT_TRUE(PlatformCrypto::aesKwUnwrap(kek, out, &back));
    EXPECT_EQ(keyData, back);

    out[0] ^= 1;
    EXPECT_FALSE(PlatformCrypto::aesKwUnwrap(kek, out, &back));
    EXPECT_FALSE(
        PlatformCrypto::aesKwWrap(kek, std::vector<uint8_t>(20, 0), &out));
}

// RFC 5869, test cases 1 and 3.
TEST(PlatformCrypto, Hkdf)
{
    std::vector<uint8_t> ikm(22, 0x0b);
    std::vector<uint8_t> out;
    ASSERT_TRUE(PlatformCrypto::hkdf(
        CryptoHash::SHA256, ikm, fromHex("000102030405060708090a0b0c"),
        fromHex("f0f1f2f3f4f5f6f7f8f9"), 42, &out));
    EXPECT_EQ(
        "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5"
        "bf34007208d5b887185865",
        toHex(out));

    // Empty salt and info.
    ASSERT_TRUE(PlatformCrypto::hkdf(CryptoHash::SHA256, ikm, CryptoBytes(),
                                     CryptoBytes(), 42, &out));
    EXPECT_EQ(
        "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d"
        "2d9d201395faa4b61a96c8",
        toHex(out));

    ASSERT_TRUE(PlatformCrypto::hkdf(CryptoHash::SHA256, ikm, CryptoBytes(),
                                     CryptoBytes(), 0, &out));
    EXPECT_TRUE(out.empty());
    // The output is at most 255 times the hash size.
    EXPECT_FALSE(PlatformCrypto::hkdf(CryptoHash::SHA256, ikm, CryptoBytes(),
                                      CryptoBytes(), 255 * 32 + 1, &out));
}

// RFC 6070 (PBKDF2-HMAC-SHA1), and the SHA-256 value for the same input.
TEST(PlatformCrypto, Pbkdf2)
{
    std::vector<uint8_t> password = fromText("password");
    std::vector<uint8_t> salt = fromText("salt");
    std::vector<uint8_t> out;
    ASSERT_TRUE(
        PlatformCrypto::pbkdf2(CryptoHash::SHA1, password, salt, 1, 20, &out));
    EXPECT_EQ("0c60c80f961f0e71f3a9b524af6012062fe037a6", toHex(out));
    ASSERT_TRUE(PlatformCrypto::pbkdf2(CryptoHash::SHA1, password, salt, 4096,
                                       20, &out));
    EXPECT_EQ("4b007901b765489abead49d926f721d065a429c1", toHex(out));
    ASSERT_TRUE(PlatformCrypto::pbkdf2(CryptoHash::SHA256, password, salt, 1,
                                       32, &out));
    EXPECT_EQ(
        "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b",
        toHex(out));

    EXPECT_FALSE(
        PlatformCrypto::pbkdf2(CryptoHash::SHA1, password, salt, 0, 20, &out));
}

// A failed call must not leave errors that make a later call fail.
TEST(PlatformCrypto, FailureLeavesNoError)
{
    std::vector<uint8_t> kek(16, 1);
    std::vector<uint8_t> out;
    EXPECT_FALSE(
        PlatformCrypto::aesKwUnwrap(kek, std::vector<uint8_t>(24, 0), &out));
    EXPECT_EQ(0u, ERR_peek_error());
    EXPECT_FALSE(PlatformCrypto::aesGcmDecrypt(
        kek, std::vector<uint8_t>(12, 0), CryptoBytes(), 128,
        std::vector<uint8_t>(16, 0), &out));
    EXPECT_EQ(0u, ERR_peek_error());
}
