/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/util/Cryptographic.h"

#if defined(OS_WINDOWS)
#include <Windows.h>
#include <bcrypt.h>
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#pragma comment(lib, "Bcrypt.lib")
#else
#include <openssl/sha.h>
#endif

namespace Starfish {

#if defined(OS_WINDOWS)
static void initHashHandle(BCRYPT_ALG_HANDLE& algHandle,
                           BCRYPT_HASH_HANDLE& hashHandle, LPCWSTR algKind)
{
    NTSTATUS status;

    PBYTE Hash = NULL;

    //
    // Open an algorithm handle
    // This sample passes BCRYPT_HASH_REUSABLE_FLAG with
    // BCryptAlgorithmProvider(...) to load a provider which supports reusable
    // hash
    //

    status = BCryptOpenAlgorithmProvider(
        &algHandle, // Alg Handle pointer
        algKind,    // Cryptographic Algorithm name (null
                    // terminated unicode string)
        NULL,       // Provider name; if null, the default provider is loaded
        0);         // Flags;

    if (!NT_SUCCESS(status)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    //
    // Create a hash handle
    //
    status = BCryptCreateHash(
        algHandle,   // Handle to an algorithm provider
        &hashHandle, // A pointer to a hash handle - can be a hash or hmac
                     // object
        NULL,        // Pointer to the buffer that recieves the hash/hmac object
        0,           // Size of the buffer in bytes
        NULL,        // A pointer to a key to use for the hash or MAC
        0,           // Size of the key in bytes
        0);          // Flags

    if (!NT_SUCCESS(status)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

static void updateHashData(BCRYPT_ALG_HANDLE algHandle,
                           BCRYPT_HASH_HANDLE hashHandle,
                           const std::string& data)
{
    NTSTATUS status;

    //
    // Hash the message(s)
    // More than one message can be hashed by calling BCryptHashData
    //
    status = BCryptHashData(
        hashHandle, // Handle to the hash or MAC object
        (PBYTE)
            data.data(), // A pointer to a buffer that contains the data to hash
        data.size(),     // Size of the buffer in bytes
        0);              // Flags

    if (!NT_SUCCESS(status)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

static std::string doComputeHash(BCRYPT_ALG_HANDLE algHandle,
                                 BCRYPT_HASH_HANDLE hashHandle)
{
    NTSTATUS status;

    DWORD resultLength = 0;
    DWORD hashLength = 0;
    //
    // Obtain the length of the hash
    //

    status = BCryptGetProperty(
        algHandle,          // Handle to a CNG object
        BCRYPT_HASH_LENGTH, // Property name (null terminated unicode string)
        (PBYTE)&hashLength, // Address of the output buffer which recieves the
                            // property value
        sizeof(hashLength), // Size of the buffer in bytes
        &resultLength,      // Number of bytes that were copied into the buffer
        0);                 // Flags

    if (!NT_SUCCESS(status)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    //
    // Obtain the hash of the message(s) into the hash buffer
    //

    std::string ret;
    ret.resize(hashLength);

    status = BCryptFinishHash(hashHandle, // Handle to the hash or MAC object
                              (PUCHAR)ret.data(), // A pointer to a buffer that
                              // receives the hash or MAC value
                              hashLength, // Size of the buffer in bytes
                              0);         // Flags

    if (!NT_SUCCESS(status)) {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return ret;
}

static void releaseHash(BCRYPT_ALG_HANDLE algHandle,
                        BCRYPT_HASH_HANDLE hashHandle)
{
    BCryptDestroyHash(hashHandle); // Handle to hash/MAC object which needs
                                   // to be destroyed
    BCryptCloseAlgorithmProvider(algHandle, // Handle to the algorithm provider
                                            // which needs to be closed
                                 0);        // Flags
}
#endif

class CryptoAlgorithmSHA256 : public CryptoAlgorithm {
#if defined(OS_WINDOWS)
    BCRYPT_ALG_HANDLE m_algHandle;
    BCRYPT_HASH_HANDLE m_hashHandle;
#else
    SHA256_CTX m_context;
#endif

public:
    CryptoAlgorithmSHA256();
    ~CryptoAlgorithmSHA256()
    {
#if defined(OS_WINDOWS)
        releaseHash(m_algHandle, m_hashHandle);
#endif
    }
    void update(const std::string& str);
    std::string computeHash();
};

class CryptoAlgorithmSHA384 : public CryptoAlgorithm {
#if defined(OS_WINDOWS)
    BCRYPT_ALG_HANDLE m_algHandle;
    BCRYPT_HASH_HANDLE m_hashHandle;
#else
    // Some old package doesn't have SHA384_CTX
    // Using SHA512_CTX shows same result
    SHA512_CTX m_context;
#endif

public:
    CryptoAlgorithmSHA384();
    ~CryptoAlgorithmSHA384()
    {
#if defined(OS_WINDOWS)
        releaseHash(m_algHandle, m_hashHandle);
#endif
    }
    void update(const std::string& str);
    std::string computeHash();
};

class CryptoAlgorithmSHA512 : public CryptoAlgorithm {
#if defined(OS_WINDOWS)
    BCRYPT_ALG_HANDLE m_algHandle;
    BCRYPT_HASH_HANDLE m_hashHandle;
#else
    SHA512_CTX m_context;
#endif

public:
    CryptoAlgorithmSHA512();
    ~CryptoAlgorithmSHA512()
    {
#if defined(OS_WINDOWS)
        releaseHash(m_algHandle, m_hashHandle);
#endif
    }
    void update(const std::string& str);
    std::string computeHash();
};

CryptoAlgorithmSHA256::CryptoAlgorithmSHA256()
    : CryptoAlgorithm()
{
#if defined(OS_WINDOWS)
    initHashHandle(m_algHandle, m_hashHandle, BCRYPT_SHA256_ALGORITHM);
#else
    SHA256_Init(&m_context);
#endif
}

void CryptoAlgorithmSHA256::update(const std::string& str)
{
#if defined(OS_WINDOWS)
    updateHashData(m_algHandle, m_hashHandle, str);
#else
    SHA256_Update(&m_context, str.c_str(), str.size());
#endif
}

std::string CryptoAlgorithmSHA256::computeHash()
{
#if defined(OS_WINDOWS)
    return doComputeHash(m_algHandle, m_hashHandle);
#else
    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256_Final(hash.data(), &m_context);
    return std::string(reinterpret_cast<char const*>(hash.data()),
                       SHA256_DIGEST_LENGTH);

#endif
}

CryptoAlgorithmSHA384::CryptoAlgorithmSHA384()
    : CryptoAlgorithm()
{
#if defined(OS_WINDOWS)
    initHashHandle(m_algHandle, m_hashHandle, BCRYPT_SHA384_ALGORITHM);
#else
    SHA384_Init(&m_context);
#endif
}

void CryptoAlgorithmSHA384::update(const std::string& str)
{
#if defined(OS_WINDOWS)
    updateHashData(m_algHandle, m_hashHandle, str);
#else
    SHA384_Update(&m_context, str.c_str(), str.size());
#endif
}

std::string CryptoAlgorithmSHA384::computeHash()
{
#if defined(OS_WINDOWS)
    return doComputeHash(m_algHandle, m_hashHandle);
#else
    std::vector<unsigned char> hash(SHA384_DIGEST_LENGTH);
    SHA384_Final(hash.data(), &m_context);
    return std::string(reinterpret_cast<char const*>(hash.data()),
                       SHA384_DIGEST_LENGTH);
#endif
}

CryptoAlgorithmSHA512::CryptoAlgorithmSHA512()
    : CryptoAlgorithm()
{
#if defined(OS_WINDOWS)
    initHashHandle(m_algHandle, m_hashHandle, BCRYPT_SHA512_ALGORITHM);
#else
    SHA512_Init(&m_context);
#endif
}

void CryptoAlgorithmSHA512::update(const std::string& str)
{
#if defined(OS_WINDOWS)
    updateHashData(m_algHandle, m_hashHandle, str);
#else
    SHA512_Update(&m_context, str.c_str(), str.size());
#endif
}

std::string CryptoAlgorithmSHA512::computeHash()
{
#if defined(OS_WINDOWS)
    return doComputeHash(m_algHandle, m_hashHandle);
#else
    std::vector<unsigned char> hash(SHA512_DIGEST_LENGTH);
    SHA512_Final(hash.data(), &m_context);
    return std::string(reinterpret_cast<char const*>(hash.data()),
                       SHA512_DIGEST_LENGTH);
#endif
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
        STARFISH_UNIMPLEMENTED();
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
        output = Base64Utils::encodeBase64(hash);
        break;
    case DigestEncodingType::Hex:
        STARFISH_UNIMPLEMENTED();
        break;
    default:
        output = hash;
        break;
    }

    return output;
}

} // namespace Starfish
