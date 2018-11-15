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
#include "core/csp/ContentSecurityPolicySourceListDirective.h"
#include "core/util/Cryptographic.h"

namespace Starfish {

void ContentSecurityPolicySourceListDirective::parseSource(String* value)
{
    GCVector<StringView> tokens;
    StringUtils::wordTokenizer(value, tokens);

    for (size_t i = 0; i < tokens.size(); i++) {
        auto token = tokens[i].substring();

        if (token->equalsIgnoreCase("'self'")) {
            m_allowSelf = true;
        } else if (token->equalsIgnoreCase("*")) {
            m_allowStar = true;
        } else if (token->equalsIgnoreCase("'unsafe-inline'")) {
            m_allowInline = true;
        } else if (token->startsWith("'") && parseHash(token)) {
            continue;
        } else {
            parseHost(token);
        }
    }
}

bool ContentSecurityPolicySourceListDirective::parseHash(String* source)
{
    // extract algorithm type and digest
    struct SupportedPrefixesStruct {
        const char* prefix;
        CryptoAlgorithmType type;
    };

    static const SupportedPrefixesStruct supportedPrefixes[] = {
        { "'sha256-", CryptoAlgorithmType::Sha256 },
        { "'sha384-", CryptoAlgorithmType::Sha384 },
        { "'sha512-", CryptoAlgorithmType::Sha512 },
        { "'sha-256-", CryptoAlgorithmType::Sha256 },
        { "'sha-384-", CryptoAlgorithmType::Sha384 },
        { "'sha-512-", CryptoAlgorithmType::Sha512 },
    };

    const size_t supportedPrefixesLength =
        sizeof(supportedPrefixes) / sizeof(supportedPrefixes[0]);

    CryptoAlgorithmType hashAlgorithmType = CryptoAlgorithmType::None;
    String* base64Value;

    for (size_t i = 0; i < supportedPrefixesLength; i++) {
        auto prefix = String::createASCIIString(supportedPrefixes[i].prefix);

        if (source->startsWith(prefix)) {
            auto prefix_length = prefix->length();
            hashAlgorithmType = supportedPrefixes[i].type;
            // make a substring without the last single quote
            base64Value = source->substring(
                prefix_length, source->length() - prefix_length - 1);
            break;
        }
    }

    if (hashAlgorithmType == CryptoAlgorithmType::None) {
        return false;
    }

    // TODO: check the need to support base64url-encoded
    if (base64Value->length() > kMaxDigestSize) {
        // TODO: validate base64 encoding more strictly
        return false;
    }

    // add hash-algorithm
    m_hashAlgorithmsUsed |= static_cast<uint32_t>(hashAlgorithmType);
    // add base64-value
    m_hashes.insert(base64Value->toUTF8NonGCString());

    return true;
}

void ContentSecurityPolicySourceListDirective::parseHost(String* source)
{
    size_t length = source->length();

    size_t serverNamePos = source->find(".", 0);
    if (serverNamePos == SIZE_MAX) {
        return;
    }
    size_t domainNamePos = source->find(".", serverNamePos + 1);
    if (domainNamePos == SIZE_MAX) {
        return;
    }

    ContentSecurityPolicySourceURL* sourceURL =
        new ContentSecurityPolicySourceURL();

    bool isStarProtocol = true;
    size_t protocolPos = source->find(":");
    if (protocolPos == SIZE_MAX) {
        sourceURL->m_isStarProtocol = true;
        protocolPos = 0;
    } else {
        if (protocolPos + 1 >= length) {
            return;
        }
        sourceURL->m_protocol = source->substring(0, protocolPos + 1);
        sourceURL->m_isStarProtocol = false;
        while (protocolPos < length) {
            auto c = source->charAt(protocolPos);
            if (c != ':' && c != '/')
                break;
            protocolPos++;
        }
    }

    STARFISH_ASSERT(serverNamePos - protocolPos >= 0);
    sourceURL->m_serverName =
        source->substring(protocolPos, serverNamePos - protocolPos);
    sourceURL->m_isStarServer = sourceURL->m_serverName->equals("*");

    size_t pathPos = source->find("/", domainNamePos);
    if (pathPos != SIZE_MAX && length > pathPos) {
        sourceURL->m_path = source->substring(pathPos, length - pathPos);
    } else {
        pathPos = length;
    }

    STARFISH_ASSERT(length - serverNamePos - 1 >= 0);
    sourceURL->m_domainName =
        source->substring(serverNamePos + 1, pathPos - serverNamePos - 1);

    m_URLSourceList.push_back(sourceURL);
}

static std::string getCSPHash(CryptoAlgorithmType hashType,
                              const std::string& str)
{
    Crypto hash(hashType, str);
    return hash.digest(DigestEncodingType::Base64);
}

bool ContentSecurityPolicySourceListDirective::allowContent(String* content)
{
    uint32_t type = 0;

    for (uint32_t i = 0; i < NUM_SUPPORTED_CRYPTO_ALGORITHM_TYPE; i++) {
        type = (1 << i);
        if (m_hashAlgorithmsUsed & type) {
            auto hash = getCSPHash(static_cast<CryptoAlgorithmType>(type),
                                   content->toUTF8NonGCString());
            if (m_hashes.find(hash) != m_hashes.end()) {
                return true;
            }
        }
    }
    return false;
}

bool ContentSecurityPolicySourceListDirective::allowURL(ResourceURL* url)
{
    size_t pos = url->host()->find(".");
    String* serverName = url->host()->substring(0, pos);
    String* domainName =
        url->host()->substring(pos + 1, url->host()->length() - pos - 1);

    for (auto source : m_URLSourceList) {
        if (!source->m_isStarProtocol &&
            !source->m_protocol->equalsIgnoreCase(url->protocol())) {
            continue;
        }
        if (!source->m_isStarServer &&
            !source->m_serverName->equalsIgnoreCase(serverName)) {
            continue;
        }
        if (source->m_domainName->equalsIgnoreCase(domainName)) {
            if (source->m_path->length() > 1) {
                if (!source->m_path->equalsIgnoreCase(url->pathname())) {
                    continue;
                }
            }
            return true;
        }
    }

    return false;
}
}
