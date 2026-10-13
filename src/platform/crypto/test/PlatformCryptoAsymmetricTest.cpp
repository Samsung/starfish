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

// Known-answer tests for the asymmetric keys.
//
// Ed25519 and X25519 use the vectors of RFC 8032 (7.1, test 1) and RFC 7748
// (6.1). The ECDSA key is the P-256 key of RFC 6979 (A.2.5). The ECDSA
// signature, the ECDH secret, the DER encodings and the RSA values were made
// with the Python "cryptography" package, which is a separate
// implementation. RSA-PSS uses a salt length of 0 here, so its signature is
// fixed and can be compared.

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

const char kEcD[] =
    "c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721";
const char kEcX[] =
    "60fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb6";
const char kEcY[] =
    "7903fe1008b8bc99a41ae9e95628bc64f2f1b20c2d7e9f5177a3c294d4462299";
// ECDSA P-256, SHA-256, message "sample".
const char kEcSignature[] =
    "021fbf6f176a0910188256a2692cb4077753673be6fd44f62c0819b56d310366"
    "4a2fdaa716be05c680391a7db88b318011bac1fe28227250567195b9aa78b3dc";
const char kEcSpki[] =
    "3059301306072a8648ce3d020106082a8648ce3d0301070342000460fed4ba25"
    "5a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb67903fe1008"
    "b8bc99a41ae9e95628bc64f2f1b20c2d7e9f5177a3c294d4462299";
const char kEcPkcs8[] =
    "308187020100301306072a8648ce3d020106082a8648ce3d030107046d306b02"
    "01010420c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b"
    "120f6721a1440342000460fed4ba255a9d31c961eb74c6356d68c049b8923b61"
    "fa6ce669622e60f29fb67903fe1008b8bc99a41ae9e95628bc64f2f1b20c2d7e"
    "9f5177a3c294d4462299";
const char kEc2D[] =
    "c6ef9c5d78ae012a011164acb397ce2088685d8f06bf9be0b283ab46476bee53";
const char kEc2X[] =
    "d12dfb5289c8d4f81208b70270398c342296970a0bccb74c736fc7554494bf63";
const char kEc2Y[] =
    "56fbf3ca366cc23e8157854c13c58d6aac23f046ada30f8353e74f33039872ab";
const char kEcdhSecret[] =
    "2e37ea8d4af773501b91541be97e4a2a92ca57991470f2d3b541d56db412ac37";

const char kRsaPkcs8[] =
    "30820278020100300d06092a864886f70d0101010500048202623082025e0201"
    "0002818100e8eee0adfbc56214e263fae5e065aa767acb565b8aca0298d6bf03"
    "a193e587b57a35f12f057ca6217beba37642086b405bb45d031b780a69a35d90"
    "55f8b9fb6b2f3264fe05ec410955725999c90d383ba77a6e1ab6bb34fe1af36d"
    "046bfa5361a4db0d8ba9fca6f1c91d9d9e6a5899d6004d941eed540155d4a46c"
    "2afff3242902030100010281810083dbadce6f86b5d612adfc3aa3038a3295a2"
    "83994080c86592b300b037d3a836001f870f5230d4ae81c2cb7ec73492da14cf"
    "8e13933a29d2d10ac70913cadc5a251e0e6503678a8eda5263780eae4a289e3c"
    "5805e2d76170f4d609687ac8d8b02d8b56d8a4993418fa8869bcd5010ae31a4e"
    "c52346c223d92bb736d9e4ea01ed024100f4bca40497691537eaf2cf3cde5d9f"
    "a70a0b9bb1e7955737422ffd55f0575f7c5060d26133094c075be56a61841d2d"
    "7c1ff5c9c054f6a91fd4c061f8b2b582a3024100f3a72bdca6ba00a017a650ca"
    "14b6d5f0a4c4b2406f7fb552ee53d013ced3804588d76984302e3b5f5b7f17f3"
    "a8e18baa1274757586aecb19f3c55b4325b8f6c3024100eab013210c090afdea"
    "621975274fe48f8c44817362be853a4a5036c1b88e30d4c74771b71ff7e45e92"
    "33378677e87afbb2b2425c1b9dc5a4eea1e5d1559e55bd02403c71a23df0d67f"
    "c53758b8fe08b97ac561fab1c5ae635e161ead881701de121a2de25278ca830d"
    "93905d2d11f33950ae763991d2000168bd1d6e3e3fa0220c85024100a8ec20d8"
    "c0f346d17a792672300e3ed43e1cf93e3747b75fadbe0dabd3a6fcee8207e176"
    "778e9c39e02889ba54e1fd38511a32f29d9d5d013587bf06b94e079b";
const char kRsaSpki[] =
    "30819f300d06092a864886f70d010101050003818d0030818902818100e8eee0"
    "adfbc56214e263fae5e065aa767acb565b8aca0298d6bf03a193e587b57a35f1"
    "2f057ca6217beba37642086b405bb45d031b780a69a35d9055f8b9fb6b2f3264"
    "fe05ec410955725999c90d383ba77a6e1ab6bb34fe1af36d046bfa5361a4db0d"
    "8ba9fca6f1c91d9d9e6a5899d6004d941eed540155d4a46c2afff32429020301"
    "0001";
const char kRsaPkcs1Sha256[] =
    "31c97a211c0ac38f6d3161426514428444714c1f8afad8cb2e6a5c271eb28e14"
    "2d6a54bc705293fcc174a52e7baf5b380c56b88bb3f8637ec31567107664e98d"
    "17e6c6bebf6f2571afe3cbf8dd6a46fb5ab318cda5e5f80e28853742e914ecbf"
    "722250bdb43f93f7af4e8a81b5e95efce3a3ee176e1b1680eec625694dfe0feb";
const char kRsaPss0Sha256[] =
    "41ded91e706876a2d11ec6d451ebf649a980d7978ca757c116628d5cba99e472"
    "9467a93e013435d0b3fd86aacae757b2de7c1077350f57ce5190b02490df60cc"
    "fee2aa6416f9e950bb13b44ef4e73ccdbf5076e1b12f8123f5cea7fb8d878251"
    "fdbcd286aa4ca483631088c1f54cf26422c0762935d91f7977aab4aac5697907";
const char kRsaOaepSha256Label[] =
    "18d3067f4c0e74fd12e2e87e81bb68a2603f3d6ddb81046fa72ddb0b01e29a72"
    "75e8cd990e6dedc98d0c6162297303605928943b16b3d859c1c41c3667ba07bb"
    "7b530d9d8dfdd000501085b3666f923a4c6938cee739dbfdab94ffeb8d456072"
    "ce0f6b7838397d654831a12022761033a0f4497f91419ad8d39e5268f3b3d43c";

AsymmetricKey ecKey()
{
    AsymmetricKey key;
    EXPECT_TRUE(PlatformCrypto::importEcCoordinates(
        CryptoCurve::P256, fromHex(kEcX), fromHex(kEcY), fromHex(kEcD), &key));
    return key;
}

AsymmetricKey rsaKey()
{
    AsymmetricKey key;
    EXPECT_TRUE(PlatformCrypto::importPkcs8(CryptoKeyKind::RSA,
                                            fromHex(kRsaPkcs8), &key));
    return key;
}

} // namespace

TEST(PlatformCryptoAsymmetric, EcImportExport)
{
    AsymmetricKey key = ecKey();
    ASSERT_FALSE(key.isNull());
    EXPECT_TRUE(key.isPrivate());
    EXPECT_EQ(CryptoKeyKind::EC, key.kind());
    EXPECT_EQ(CryptoCurve::P256, key.curve());

    std::vector<uint8_t> der;
    ASSERT_TRUE(PlatformCrypto::exportSpki(key, &der));
    EXPECT_EQ(kEcSpki, toHex(der));
    ASSERT_TRUE(PlatformCrypto::exportPkcs8(key, &der));
    EXPECT_EQ(kEcPkcs8, toHex(der));

    AsymmetricKey fromPkcs8;
    ASSERT_TRUE(PlatformCrypto::importPkcs8(CryptoKeyKind::EC,
                                            fromHex(kEcPkcs8), &fromPkcs8));
    std::vector<uint8_t> x, y, d;
    ASSERT_TRUE(PlatformCrypto::exportEcCoordinates(fromPkcs8, &x, &y, &d));
    EXPECT_EQ(kEcX, toHex(x));
    EXPECT_EQ(kEcY, toHex(y));
    EXPECT_EQ(kEcD, toHex(d));

    // Raw point, uncompressed and compressed.
    AsymmetricKey publicKey = key.publicKey();
    EXPECT_FALSE(publicKey.isPrivate());
    std::vector<uint8_t> point;
    ASSERT_TRUE(PlatformCrypto::exportEcPoint(publicKey, &point));
    EXPECT_EQ(std::string("04") + kEcX + kEcY, toHex(point));
    AsymmetricKey fromCompressed;
    // y ends in 0x99, which is odd, so the prefix is 03.
    ASSERT_TRUE(PlatformCrypto::importEcPoint(
        CryptoCurve::P256, fromHex(std::string("03") + kEcX), &fromCompressed));
    ASSERT_TRUE(PlatformCrypto::exportSpki(fromCompressed, &der));
    EXPECT_EQ(kEcSpki, toHex(der));
    // An spki with a compressed point exports uncompressed, as WebCrypto
    // requires. This is the spki of the key above, with the point 03 || x.
    std::string compressedSpki =
        std::string("3039301306072a8648ce3d020106082a8648ce3d03010703220003") +
        kEcX;
    AsymmetricKey fromCompressedSpki;
    ASSERT_TRUE(PlatformCrypto::importSpki(
        CryptoKeyKind::EC, fromHex(compressedSpki), &fromCompressedSpki));
    ASSERT_TRUE(PlatformCrypto::exportSpki(fromCompressedSpki, &der));
    EXPECT_EQ(kEcSpki, toHex(der));

    AsymmetricKey rejected;
    // Wrong kind, extra bytes, a point on another curve, a wrong d.
    EXPECT_FALSE(PlatformCrypto::importSpki(CryptoKeyKind::RSA,
                                            fromHex(kEcSpki), &rejected));
    EXPECT_FALSE(PlatformCrypto::importSpki(
        CryptoKeyKind::EC, fromHex(std::string(kEcSpki) + "00"), &rejected));
    EXPECT_FALSE(PlatformCrypto::importEcPoint(
        CryptoCurve::P384, fromHex(std::string("04") + kEcX + kEcY),
        &rejected));
    EXPECT_FALSE(PlatformCrypto::importEcCoordinates(
        CryptoCurve::P256, fromHex(kEcX), fromHex(kEcY), fromHex(kEc2D),
        &rejected));
    EXPECT_FALSE(PlatformCrypto::exportPkcs8(publicKey, &der));
}

TEST(PlatformCryptoAsymmetric, Ecdsa)
{
    AsymmetricKey key = ecKey();
    AsymmetricKey publicKey = key.publicKey();
    std::vector<uint8_t> message = fromText("sample");

    EXPECT_TRUE(PlatformCrypto::ecdsaVerify(publicKey, CryptoHash::SHA256,
                                            message, fromHex(kEcSignature)));

    std::vector<uint8_t> signature;
    ASSERT_TRUE(PlatformCrypto::ecdsaSign(key, CryptoHash::SHA256, message,
                                          &signature));
    EXPECT_EQ(64u, signature.size());
    EXPECT_TRUE(PlatformCrypto::ecdsaVerify(publicKey, CryptoHash::SHA256,
                                            message, signature));

    signature[10] ^= 1;
    EXPECT_FALSE(PlatformCrypto::ecdsaVerify(publicKey, CryptoHash::SHA256,
                                             message, signature));
    signature.pop_back();
    EXPECT_FALSE(PlatformCrypto::ecdsaVerify(publicKey, CryptoHash::SHA256,
                                             message, signature));
    EXPECT_FALSE(PlatformCrypto::ecdsaSign(publicKey, CryptoHash::SHA256,
                                           message, &signature));

    // P-521 has 66-byte values.
    AsymmetricKey p521;
    ASSERT_TRUE(PlatformCrypto::generateEcKey(CryptoCurve::P521, &p521));
    ASSERT_TRUE(PlatformCrypto::ecdsaSign(p521, CryptoHash::SHA512, message,
                                          &signature));
    EXPECT_EQ(132u, signature.size());
    EXPECT_TRUE(PlatformCrypto::ecdsaVerify(
        p521.publicKey(), CryptoHash::SHA512, message, signature));
}

TEST(PlatformCryptoAsymmetric, Ecdh)
{
    AsymmetricKey key = ecKey();
    AsymmetricKey peer;
    ASSERT_TRUE(PlatformCrypto::importEcCoordinates(
        CryptoCurve::P256, fromHex(kEc2X), fromHex(kEc2Y), CryptoBytes(),
        &peer));
    std::vector<uint8_t> secret;
    ASSERT_TRUE(PlatformCrypto::deriveSharedSecret(key, peer, &secret));
    EXPECT_EQ(kEcdhSecret, toHex(secret));

    AsymmetricKey p384;
    ASSERT_TRUE(PlatformCrypto::generateEcKey(CryptoCurve::P384, &p384));
    EXPECT_FALSE(
        PlatformCrypto::deriveSharedSecret(key, p384.publicKey(), &secret));
    EXPECT_FALSE(PlatformCrypto::deriveSharedSecret(peer, key, &secret));
}

TEST(PlatformCryptoAsymmetric, Ed25519)
{
    std::vector<uint8_t> privateKey = fromHex(
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60");
    std::vector<uint8_t> publicKey = fromHex(
        "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
    AsymmetricKey key;
    ASSERT_TRUE(PlatformCrypto::importOkp(CryptoKeyKind::Ed25519, publicKey,
                                          privateKey, &key));
    EXPECT_EQ(CryptoKeyKind::Ed25519, key.kind());

    std::vector<uint8_t> signature;
    ASSERT_TRUE(PlatformCrypto::ed25519Sign(key, CryptoBytes(), &signature));
    EXPECT_EQ(
        "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901"
        "555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a"
        "100b",
        toHex(signature));
    EXPECT_TRUE(PlatformCrypto::ed25519Verify(key.publicKey(), CryptoBytes(),
                                              signature));
    signature[0] ^= 1;
    EXPECT_FALSE(PlatformCrypto::ed25519Verify(key.publicKey(), CryptoBytes(),
                                               signature));

    std::vector<uint8_t> der;
    ASSERT_TRUE(PlatformCrypto::exportSpki(key, &der));
    EXPECT_EQ(
        "302a300506032b6570032100d75a980182b10ab7d54bfed3c964073a0ee172"
        "f3daa62325af021a68f707511a",
        toHex(der));
    ASSERT_TRUE(PlatformCrypto::exportPkcs8(key, &der));
    EXPECT_EQ(
        "302e020100300506032b6570042204209d61b19deffd5a60ba844af492ec2c"
        "c44449c5697b326919703bac031cae7f60",
        toHex(der));

    // A small-order public key fails, even with a signature that OpenSSL
    // alone would accept. The neutral point (y = 1) has order 1.
    std::vector<uint8_t> neutral(32, 0);
    neutral[0] = 1;
    AsymmetricKey smallOrder;
    ASSERT_TRUE(PlatformCrypto::importOkp(CryptoKeyKind::Ed25519, neutral,
                                          CryptoBytes(), &smallOrder));
    std::vector<uint8_t> neutralSignature(neutral);
    neutralSignature.resize(64, 0);
    EXPECT_FALSE(PlatformCrypto::ed25519Verify(smallOrder, CryptoBytes(),
                                               neutralSignature));

    // A public key that does not match the private key.
    publicKey[0] ^= 1;
    AsymmetricKey rejected;
    EXPECT_FALSE(PlatformCrypto::importOkp(CryptoKeyKind::Ed25519, publicKey,
                                           privateKey, &rejected));
}

TEST(PlatformCryptoAsymmetric, X25519)
{
    AsymmetricKey alice;
    ASSERT_TRUE(PlatformCrypto::importOkp(
        CryptoKeyKind::X25519, CryptoBytes(),
        fromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db9"
                "2c2a"),
        &alice));
    AsymmetricKey bob;
    ASSERT_TRUE(PlatformCrypto::importOkp(
        CryptoKeyKind::X25519,
        fromHex("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f88"
                "2b4f"),
        CryptoBytes(), &bob));
    std::vector<uint8_t> secret;
    ASSERT_TRUE(PlatformCrypto::deriveSharedSecret(alice, bob, &secret));
    EXPECT_EQ(
        "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742",
        toHex(secret));

    // u = 0 has small order, so the secret is all zeros.
    AsymmetricKey smallOrder;
    ASSERT_TRUE(PlatformCrypto::importOkp(CryptoKeyKind::X25519,
                                          std::vector<uint8_t>(32, 0),
                                          CryptoBytes(), &smallOrder));
    EXPECT_FALSE(
        PlatformCrypto::deriveSharedSecret(alice, smallOrder, &secret));

    AsymmetricKey generated;
    ASSERT_TRUE(
        PlatformCrypto::generateOkpKey(CryptoKeyKind::X25519, &generated));
    EXPECT_TRUE(PlatformCrypto::deriveSharedSecret(generated, alice.publicKey(),
                                                   &secret));
}

TEST(PlatformCryptoAsymmetric, RsaSignatures)
{
    AsymmetricKey key = rsaKey();
    ASSERT_FALSE(key.isNull());
    EXPECT_EQ(1024u, key.rsaModulusBits());
    EXPECT_EQ("010001", toHex(key.rsaPublicExponent()));
    AsymmetricKey publicKey;
    ASSERT_TRUE(PlatformCrypto::importSpki(CryptoKeyKind::RSA,
                                           fromHex(kRsaSpki), &publicKey));
    std::vector<uint8_t> message = fromText("sample");

    std::vector<uint8_t> signature;
    ASSERT_TRUE(PlatformCrypto::rsaSign(key, RsaSignaturePadding::PKCS1v15,
                                        CryptoHash::SHA256, 0, message,
                                        &signature));
    EXPECT_EQ(kRsaPkcs1Sha256, toHex(signature));
    EXPECT_TRUE(
        PlatformCrypto::rsaVerify(publicKey, RsaSignaturePadding::PKCS1v15,
                                  CryptoHash::SHA256, 0, message, signature));

    ASSERT_TRUE(PlatformCrypto::rsaSign(key, RsaSignaturePadding::PSS,
                                        CryptoHash::SHA256, 0, message,
                                        &signature));
    EXPECT_EQ(kRsaPss0Sha256, toHex(signature));
    EXPECT_TRUE(PlatformCrypto::rsaVerify(publicKey, RsaSignaturePadding::PSS,
                                          CryptoHash::SHA256, 0, message,
                                          signature));
    // The salt length is part of the check.
    EXPECT_FALSE(PlatformCrypto::rsaVerify(publicKey, RsaSignaturePadding::PSS,
                                           CryptoHash::SHA256, 32, message,
                                           signature));

    // A salted signature is random, so check only that it verifies.
    ASSERT_TRUE(PlatformCrypto::rsaSign(key, RsaSignaturePadding::PSS,
                                        CryptoHash::SHA384, 20, message,
                                        &signature));
    EXPECT_TRUE(PlatformCrypto::rsaVerify(publicKey, RsaSignaturePadding::PSS,
                                          CryptoHash::SHA384, 20, message,
                                          signature));
    EXPECT_FALSE(
        PlatformCrypto::rsaSign(publicKey, RsaSignaturePadding::PKCS1v15,
                                CryptoHash::SHA256, 0, message, &signature));
}

TEST(PlatformCryptoAsymmetric, RsaOaep)
{
    AsymmetricKey key = rsaKey();
    std::vector<uint8_t> plaintext;
    ASSERT_TRUE(PlatformCrypto::rsaOaepDecrypt(
        key, CryptoHash::SHA256, fromText("label"),
        fromHex(kRsaOaepSha256Label), &plaintext));
    EXPECT_EQ(fromText("sample"), plaintext);
    EXPECT_FALSE(PlatformCrypto::rsaOaepDecrypt(
        key, CryptoHash::SHA256, fromText("other"),
        fromHex(kRsaOaepSha256Label), &plaintext));

    // An empty plaintext is valid and decrypts to an empty result.
    std::vector<uint8_t> ciphertext;
    ASSERT_TRUE(PlatformCrypto::rsaOaepEncrypt(key.publicKey(),
                                               CryptoHash::SHA1, CryptoBytes(),
                                               CryptoBytes(), &ciphertext));
    EXPECT_EQ(128u, ciphertext.size());
    ASSERT_TRUE(PlatformCrypto::rsaOaepDecrypt(
        key, CryptoHash::SHA1, CryptoBytes(), ciphertext, &plaintext));
    EXPECT_TRUE(plaintext.empty());
    EXPECT_FALSE(PlatformCrypto::rsaOaepDecrypt(key.publicKey(),
                                                CryptoHash::SHA1, CryptoBytes(),
                                                ciphertext, &plaintext));
}

TEST(PlatformCryptoAsymmetric, RsaComponents)
{
    AsymmetricKey key = rsaKey();
    RsaKeyComponents components;
    ASSERT_TRUE(PlatformCrypto::exportRsaComponents(key, &components));
    EXPECT_EQ("010001", toHex(components.e));
    EXPECT_EQ(128u, components.n.size());

    AsymmetricKey rebuilt;
    ASSERT_TRUE(PlatformCrypto::importRsaComponents(components, &rebuilt));
    std::vector<uint8_t> der;
    ASSERT_TRUE(PlatformCrypto::exportPkcs8(rebuilt, &der));
    AsymmetricKey reparsed;
    ASSERT_TRUE(
        PlatformCrypto::importPkcs8(CryptoKeyKind::RSA, der, &reparsed));
    ASSERT_TRUE(PlatformCrypto::exportSpki(rebuilt, &der));
    EXPECT_EQ(kRsaSpki, toHex(der));

    // A private key whose primes do not match n.
    components.p[components.p.size() - 1] ^= 2;
    EXPECT_FALSE(PlatformCrypto::importRsaComponents(components, &rebuilt));

    RsaKeyComponents publicOnly;
    ASSERT_TRUE(
        PlatformCrypto::exportRsaComponents(key.publicKey(), &publicOnly));
    EXPECT_TRUE(publicOnly.d.empty());
    ASSERT_TRUE(PlatformCrypto::importRsaComponents(publicOnly, &rebuilt));
    EXPECT_FALSE(rebuilt.isPrivate());
}

TEST(PlatformCryptoAsymmetric, RsaGenerate)
{
    AsymmetricKey key;
    ASSERT_TRUE(PlatformCrypto::generateRsaKey(1024, fromHex("010001"), &key));
    EXPECT_EQ(1024u, key.rsaModulusBits());
    EXPECT_EQ("010001", toHex(key.rsaPublicExponent()));
    // Even and too small exponents.
    EXPECT_FALSE(PlatformCrypto::generateRsaKey(1024, fromHex("01"), &key));
    EXPECT_FALSE(PlatformCrypto::generateRsaKey(1024, fromHex("010000"), &key));
}

// The bug this layer prevents: a failed verify left an error in the
// OpenSSL queue, and the next key parse failed because of it.
TEST(PlatformCryptoAsymmetric, FailedVerifyDoesNotBreakSign)
{
    AsymmetricKey publicKey;
    ASSERT_TRUE(PlatformCrypto::importSpki(CryptoKeyKind::RSA,
                                           fromHex(kRsaSpki), &publicKey));
    std::vector<uint8_t> message = fromText("sample");
    EXPECT_FALSE(PlatformCrypto::rsaVerify(publicKey, RsaSignaturePadding::PSS,
                                           CryptoHash::SHA256, 0, message,
                                           std::vector<uint8_t>(128, 0)));
    EXPECT_EQ(0u, ERR_peek_error());

    AsymmetricKey key = rsaKey();
    std::vector<uint8_t> signature;
    EXPECT_TRUE(PlatformCrypto::rsaSign(key, RsaSignaturePadding::PKCS1v15,
                                        CryptoHash::SHA256, 0, message,
                                        &signature));
}
