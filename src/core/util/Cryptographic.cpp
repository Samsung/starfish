/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
#include "core/util/Cryptographic.h"
#include <openssl/sha.h>

namespace Starfish {

class CryptoAlgorithmSHA256 : public CryptoAlgorithm {
    SHA256_CTX m_context;

public:
    CryptoAlgorithmSHA256();
    void update(const std::string& str);
    std::string computeHash();
};

class CryptoAlgorithmSHA384 : public CryptoAlgorithm {
    SHA512_CTX m_context;

public:
    CryptoAlgorithmSHA384();
    void update(const std::string& str);
    std::string computeHash();
};

class CryptoAlgorithmSHA512 : public CryptoAlgorithm {
    SHA512_CTX m_context;

public:
    CryptoAlgorithmSHA512();
    void update(const std::string& str);
    std::string computeHash();
};

CryptoAlgorithmSHA256::CryptoAlgorithmSHA256()
    : CryptoAlgorithm()
{
    SHA256_Init(&m_context);
}

void CryptoAlgorithmSHA256::update(const std::string& str)
{
    SHA256_Update(&m_context, str.c_str(), str.size());
}

std::string CryptoAlgorithmSHA256::computeHash()
{
    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256_Final(hash.data(), &m_context);
    return std::string(reinterpret_cast<char const*>(hash.data()),
                       SHA256_DIGEST_LENGTH);
}

CryptoAlgorithmSHA384::CryptoAlgorithmSHA384()
    : CryptoAlgorithm()
{
    SHA384_Init(&m_context);
}

void CryptoAlgorithmSHA384::update(const std::string& str)
{
    SHA384_Update(&m_context, str.c_str(), str.size());
}

std::string CryptoAlgorithmSHA384::computeHash()
{
    std::vector<unsigned char> hash(SHA384_DIGEST_LENGTH);
    SHA384_Final(hash.data(), &m_context);
    return std::string(reinterpret_cast<char const*>(hash.data()),
                       SHA384_DIGEST_LENGTH);
}

CryptoAlgorithmSHA512::CryptoAlgorithmSHA512()
    : CryptoAlgorithm()
{
    SHA512_Init(&m_context);
}

void CryptoAlgorithmSHA512::update(const std::string& str)
{
    SHA512_Update(&m_context, str.c_str(), str.size());
}

std::string CryptoAlgorithmSHA512::computeHash()
{
    std::vector<unsigned char> hash(SHA512_DIGEST_LENGTH);
    SHA512_Final(hash.data(), &m_context);
    return std::string(reinterpret_cast<char const*>(hash.data()),
                       SHA512_DIGEST_LENGTH);
}

Crypto::Crypto(CryptoAlgorithmType hashType)
{
    switch (hashType) {
    case CryptoAlgorithmType::Sha256:
        m_algorithm = new CryptoAlgorithmSHA256();
        break;
    case CryptoAlgorithmType::Sha384:
        m_algorithm = new CryptoAlgorithmSHA384();
        break;
    case CryptoAlgorithmType::Sha512:
        m_algorithm = new CryptoAlgorithmSHA512();
        break;
    default:
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        break;
    }
}

Crypto::Crypto(CryptoAlgorithmType hashType, const std::string& str)
    : Crypto(hashType)
{
    this->update(str);
}

void Crypto::update(const std::string& str)
{
    m_algorithm->update(str);
}

std::string Crypto::digest(
    DigestEncodingType encoding /*= DigestEncodingType::None*/)
{
    std::string output;
    std::string hash = m_algorithm->computeHash();

    switch (encoding) {
    case DigestEncodingType::Base64:
        output = StringUtils::toBase64(hash);
        break;
    case DigestEncodingType::Hex:
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        break;
    default:
        output = hash;
        break;
    }

    return output;
}
}
