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
        } else if (token->equalsIgnoreCase("'unsafe-eval'")) {
            m_allowEval = true;
        } else if (token->equalsIgnoreCase("'unsafe-inline'")) {
            m_allowInline = true;
        } else if (isScheme(token)) {
            m_schemeList.push_back(token);
        } else if (parseHash(token) || parseNonce(token)) {
            continue;
        } else if (auto sourceURL = parseHost(token)) {
            m_sourceList.push_back(sourceURL);
        }
    }
}

bool ContentSecurityPolicySourceListDirective::parseNonce(String* source)
{
    if (!source || source->startsWith("'") == false) {
        return false;
    }

    auto prefix = String::createASCIIString("'nonce-");

    if (source->startsWith(prefix)) {
        // make a substring without the last single quote
        auto prefixLength = prefix->length();
        auto nonceValue = source->substring(prefixLength, source->length() -
                                                              prefixLength - 1);
        m_nonces.insert(nonceValue->toUTF8NonGCString());
        return true;
    }
    return false;
}

bool ContentSecurityPolicySourceListDirective::parseHash(String* source)
{
    if (!source || source->startsWith("'") == false) {
        return false;
    }

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
            auto prefixLength = prefix->length();
            hashAlgorithmType = supportedPrefixes[i].type;
            // make a substring without the last single quote
            base64Value = source->substring(prefixLength, source->length() -
                                                              prefixLength - 1);
            break;
        }
    }

    if (hashAlgorithmType == CryptoAlgorithmType::None) {
        return false;
    }

    // TODO: check the need to support base64url-encoded
    // TODO: validate base64 encoding if needed

    // add hash-algorithm
    m_hashAlgorithmsUsed |= static_cast<uint32_t>(hashAlgorithmType);
    // add base64-value
    m_hashes.insert(base64Value->toUTF8NonGCString());

    return true;
}

ContentSecurityPolicySource*
ContentSecurityPolicySourceListDirective::parseHost(String* sourceString)
{
    size_t length = sourceString->length();
    if (length == 0) {
        return nullptr;
    }

    ContentSecurityPolicySource* source = new ContentSecurityPolicySource();

    bool isStarProtocol = true;
    size_t protocolPos = sourceString->find("://");
    if (protocolPos == SIZE_MAX) {
        source->isStarProtocol = true;
        protocolPos = 0;
    } else {
        if (protocolPos + 1 >= length) {
            return nullptr;
        }
        source->protocol = sourceString->substring(0, protocolPos + 1);
        source->isStarProtocol = false;
        protocolPos += 3;
    }

    size_t start = protocolPos;
    size_t cur = start;
    while (cur < length) {
        auto c = sourceString->charAt(cur);

        if (c == '.' && source->serverName->isEmpty()) {
            if (cur - start <= 0) {
                return nullptr;
            }
            source->serverName = sourceString->substring(start, cur - start);
            source->isStarServer = source->serverName->equals("*");
            start = cur + 1;
        } else if (c == ':') {
            if (cur - start <= 0) {
                return nullptr;
            }
            source->domainName = sourceString->substring(start, cur - start);
            start = cur + 1;
        } else if (c == '/' || cur == length - 1) {
            size_t end = (c == '/') ? cur - start : cur - start + 1;
            if (end <= 0) {
                return nullptr;
            }
            if (source->domainName->isEmpty()) {
                source->domainName = sourceString->substring(start, end);
                source->isStarPort = false;
            } else {
                source->port = sourceString->substring(start, end);
                source->isStarPort = source->port->equals("*");
            }
            if (cur + 1 < length) {
                source->path = sourceString->substring(cur, length - cur);
            }
            break;
        }
        cur++;
    }
    return source;
}

bool ContentSecurityPolicySourceListDirective::isScheme(String* scheme)
{
    return scheme->charAt(scheme->length() - 1) == ':';
}

static std::string getCSPHash(CryptoAlgorithmType hashType,
                              const std::string& str)
{
    Crypto hash(hashType, str);
    return hash.digest(DigestEncodingType::Base64);
}

bool ContentSecurityPolicySourceListDirective::allowContent(String* content)
{
    if (content && !content->isEmpty()) {
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

bool ContentSecurityPolicySourceListDirective::allowNonce(String* content)
{
    if (content && !content->isEmpty()) {
        auto nonce = content->trim()->toUTF8NonGCString();
        if (m_nonces.find(nonce) != m_nonces.end()) {
            return true;
        }
    }
    return false;
}

bool ContentSecurityPolicySourceListDirective::matcheScheme(String* scheme,
                                                            ResourceURL* url)
{
    if (scheme->equalsIgnoreCase("http:")) {
        return url->isHTTPFamilyURL();
    }
    return scheme->equalsIgnoreCase(url->protocol());
}

bool ContentSecurityPolicySourceListDirective::allowScheme(ResourceURL* resUrl)
{
    for (auto scheme : m_schemeList) {
        if (matcheScheme(scheme, resUrl)) {
            return true;
        }
    }
    return false;
}

bool ContentSecurityPolicySourceListDirective::allowHost(ResourceURL* url,
                                                         bool ignoreScheme)
{
    if (url->origin()->equalsIgnoreCase("null")) {
        return false; // not URL scheme
    }

    String* host = url->hostname();
    size_t pos = host->find(".");
    if (pos == SIZE_MAX) {
        return false;
    }

    String* server = host->substring(0, pos);
    String* domain = host->substring(pos + 1, host->length() - pos - 1);
    String* port = url->port();
    String* path = url->pathname();

    for (auto source : m_sourceList) {
        if (!ignoreScheme && !source->isStarProtocol &&
            !matcheScheme(source->protocol, url)) {
            continue;
        } else if (!source->isStarServer &&
                   !source->serverName->equalsIgnoreCase(server)) {
            continue;
        } else if (!source->isStarPort &&
                   !source->port->equalsIgnoreCase(port)) {
            continue;
        } else if (source->domainName->equalsIgnoreCase(domain)) {
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
}
