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

#ifndef __StarfishPlatformCryptoInternal__
#define __StarfishPlatformCryptoInternal__

// Helpers shared by the PlatformCrypto*.cpp files. Only those files include
// this header, because it includes OpenSSL.

#include "PlatformCrypto.h"

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include <climits>
#include <memory>

namespace Starfish {

// Owns the OpenSSL key of an AsymmetricKey. OpenSSL keys do not change after
// creation, so AsymmetricKey copies share one Impl.
struct AsymmetricKey::Impl {
    Impl(EVP_PKEY* key, bool isPrivate)
        : key(key)
        , isPrivate(isPrivate)
    {
    }
    ~Impl()
    {
        EVP_PKEY_free(key);
    }

    EVP_PKEY* key;
    bool isPrivate;
};

namespace PlatformCryptoInternal {

    // Clears the OpenSSL error queue when a function starts and when it
    // returns. OpenSSL keeps errors from a failed call in a per-thread
    // queue. Some OpenSSL functions check that queue, so an old error can
    // make a later, valid call fail.
    class ErrorQueueScope {
    public:
        ErrorQueueScope()
        {
            ERR_clear_error();
        }
        ~ErrorQueueScope()
        {
            ERR_clear_error();
        }
    };

    struct Deleter {
        void operator()(EVP_CIPHER_CTX* p) const
        {
            EVP_CIPHER_CTX_free(p);
        }
        void operator()(EVP_PKEY_CTX* p) const
        {
            EVP_PKEY_CTX_free(p);
        }
        void operator()(EVP_MD_CTX* p) const
        {
            EVP_MD_CTX_free(p);
        }
        void operator()(EVP_PKEY* p) const
        {
            EVP_PKEY_free(p);
        }
        void operator()(BIGNUM* p) const
        {
            BN_free(p);
        }
    };
    typedef std::unique_ptr<EVP_CIPHER_CTX, Deleter> CipherContext;
    typedef std::unique_ptr<EVP_PKEY_CTX, Deleter> KeyContext;
    typedef std::unique_ptr<EVP_MD_CTX, Deleter> DigestContext;
    typedef std::unique_ptr<EVP_PKEY, Deleter> KeyPointer;
    typedef std::unique_ptr<BIGNUM, Deleter> BignumPointer;

    inline const EVP_MD* hashFunction(CryptoHash hash)
    {
        switch (hash) {
        case CryptoHash::SHA1:
            return EVP_sha1();
        case CryptoHash::SHA256:
            return EVP_sha256();
        case CryptoHash::SHA384:
            return EVP_sha384();
        case CryptoHash::SHA512:
            return EVP_sha512();
        }
        return nullptr;
    }

    // OpenSSL treats a null pointer with a zero length as an error in some
    // functions. Empty input is valid in WebCrypto, so give OpenSSL a valid
    // pointer instead.
    inline const uint8_t* nonNullData(CryptoBytes bytes)
    {
        static const uint8_t kEmpty = 0;
        return bytes.data ? bytes.data : &kEmpty;
    }

    // OpenSSL takes lengths as int. Larger inputs are rejected, not
    // truncated.
    inline bool fitsInInt(size_t size)
    {
        return size <= static_cast<size_t>(INT_MAX);
    }

} // namespace PlatformCryptoInternal

} // namespace Starfish

#endif
