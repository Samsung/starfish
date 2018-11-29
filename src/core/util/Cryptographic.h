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

#ifndef __StarfishCryptographic__
#define __StarfishCryptographic__

namespace Starfish {

enum class CryptoAlgorithmType : std::uint32_t {
    None = 0,
    Sha256 = 1 << 0,
    Sha384 = 1 << 1,
    Sha512 = 1 << 2,
};

#define NUM_SUPPORTED_CRYPTO_ALGORITHM_TYPE 3

enum class DigestEncodingType {
    None = 0,
    Base64,
    Hex,
};

class CryptoAlgorithm : public gc {
public:
    virtual void update(const std::string& str) = 0;
    virtual std::string computeHash() = 0;
};

class Crypto : public gc {
public:
    Crypto(CryptoAlgorithmType hashType);
    Crypto(CryptoAlgorithmType hashType, const std::string& str);

    void update(const std::string& str);
    std::string digest(DigestEncodingType encoding = DigestEncodingType::None);

private:
    CryptoAlgorithm* m_algorithm{ nullptr };
};

} // end of namespace Starfish

#endif
