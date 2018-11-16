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
        } else if (isScheme(token)) {
            m_schemeList.push_back(token);
        } else if (token->startsWith("'") && parseHash(token)) {
            continue;
        } else if (auto sourceURL = parseHost(token)) {
            m_URLSourceList.push_back(sourceURL);
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

ContentSecurityPolicySourceURL*
ContentSecurityPolicySourceListDirective::parseHost(String* source)
{
    size_t length = source->length();

    size_t serverNamePos = source->find(".", 0);
    if (serverNamePos == SIZE_MAX) {
        return nullptr;
    }
    if (source->find(".", serverNamePos + 1) == SIZE_MAX) {
        return nullptr;
    }

    ContentSecurityPolicySourceURL* sourceURL =
        new ContentSecurityPolicySourceURL();

    bool isStarProtocol = true;
    size_t protocolPos = source->find(":/");
    if (protocolPos == SIZE_MAX) {
        sourceURL->isStarProtocol = true;
        protocolPos = 0;
    } else {
        if (protocolPos + 1 >= length) {
            return nullptr;
        }
        sourceURL->protocol = source->substring(0, protocolPos + 1);
        sourceURL->isStarProtocol = false;
        while (protocolPos < length) {
            auto c = source->charAt(protocolPos);
            if (c != ':' && c != '/')
                break;
            protocolPos++;
        }
    }

    if (serverNamePos - protocolPos <= 0) {
        return nullptr;
    }

    sourceURL->serverName =
        source->substring(protocolPos, serverNamePos - protocolPos);
    sourceURL->isStarServer = sourceURL->serverName->equals("*");

    size_t pathPos = source->find("/", protocolPos);
    if (pathPos != SIZE_MAX && length > pathPos) {
        sourceURL->path = source->substring(pathPos, length - pathPos);
    } else {
        pathPos = length;
    }
    if (pathPos - serverNamePos - 1 <= 0 || pathPos - protocolPos <= 0) {
        return nullptr;
    }

    sourceURL->domainName =
        source->substring(serverNamePos + 1, pathPos - serverNamePos - 1);
    sourceURL->host = source->substring(protocolPos, pathPos - protocolPos);

    return sourceURL;
}

bool ContentSecurityPolicySourceListDirective::isScheme(String* scheme)
{
    size_t length = scheme->length();
    if (scheme->charAt(length - 1) != ':') {
        return false;
    }

    return true;
}

static std::string getCSPHash(CryptoAlgorithmType hashType,
                              const std::string& str)
{
    Crypto hash(hashType, str);
    return hash.digest(DigestEncodingType::Base64);
}

bool ContentSecurityPolicySourceListDirective::allowContent(String* content)
{
    if (content) {
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
    }

    return false;
}

bool ContentSecurityPolicySourceListDirective::allowURL(ResourceURL* url,
                                                        bool ignoreScheme)
{
    size_t pos = url->host()->find(".");
    String* serverName = url->host()->substring(0, pos);
    String* domainName =
        url->host()->substring(pos + 1, url->host()->length() - pos - 1);

    return allowURL(url->protocol(), serverName, domainName, url->pathname(),
                    ignoreScheme);
}

bool ContentSecurityPolicySourceListDirective::allowURL(
    ContentSecurityPolicySourceURL* url, bool ignoreScheme)
{
    return allowURL(url->protocol, url->serverName, url->domainName, url->path,
                    ignoreScheme);
}

bool ContentSecurityPolicySourceListDirective::allowURL(String* scheme,
                                                        String* serverName,
                                                        String* domainName,
                                                        String* path,
                                                        bool ignoreScheme)
{
    for (auto source : m_URLSourceList) {
        if (!ignoreScheme && !source->isStarProtocol &&
            !source->protocol->equalsIgnoreCase(scheme)) {
            continue;
        }
        if (!source->isStarServer &&
            !source->serverName->equalsIgnoreCase(serverName)) {
            continue;
        }
        if (source->domainName->equalsIgnoreCase(domainName)) {
            if (source->path->length() > 1) {
                if (!source->path->equalsIgnoreCase(path)) {
                    continue;
                }
            }
            return true;
        }
    }
    return false;
}

bool ContentSecurityPolicySourceListDirective::allowScheme(String* str)
{
    for (auto scheme : m_schemeList) {
        if (str->equalsIgnoreCase(scheme)) {
            return true;
        }
    }
    return false;
}
}
