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
#include "platform/loader/ResourceURL.h"
#include "core/csp/ContentSecurityPolicySourceListDirective.h"
#include "core/util/Cryptographic.h"

namespace Starfish {

void ContentSecurityPolicySourceListDirective::parseSource(
    const GCVector<StringView>& sources)
{
    auto sourcesSize = sources.size();
    if (sourcesSize < 2) {
        return;
    }

    for (size_t i = 1; i < sourcesSize; i++) {
        auto token = sources[i].substring();

        if (token->equalsIgnoreCase("'self'")) {
            m_allowSelf = true;
        } else if (token->equalsIgnoreCase("*")) {
            m_allowStar = true;
        } else if (token->equalsIgnoreCase("'unsafe-eval'")) {
            m_allowEval = true;
        } else if (token->equalsIgnoreCase("'unsafe-inline'")) {
            m_allowInline = true;
        } else if (token->equalsIgnoreCase("'none'")) {
            continue;
        } else if (isScheme(token)) {
            m_schemeList.push_back(token);
        } else if (parseHash(token) || parseNonce(token)) {
            continue;
        } else if (parseHostSource(token)) {
            continue;
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
        auto nonceLength = source->length() - prefixLength - 1;
        if (!nonceLength) {
            return false;
        }
        auto nonceValue = source->substring(prefixLength, nonceLength);
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
        STARFISH_ASSERT(supportedPrefixes[i].prefix != nullptr);
        auto prefix = String::createASCIIString(
            supportedPrefixes[i].prefix, strlen(supportedPrefixes[i].prefix));

        if (source->startsWith(prefix)) {
            auto prefixLength = prefix->length();
            hashAlgorithmType = supportedPrefixes[i].type;
            auto base64Length = source->length() - prefixLength - 1;
            if (!base64Length) {
                return false;
            }
            // make a substring without the last single quote
            base64Value = source->substring(prefixLength, base64Length);
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

bool ContentSecurityPolicySourceListDirective::parseHostSource(
    String* sourceString)
{
    size_t length = sourceString->length();
    if (length == 0) {
        return false;
    }

    ContentSecurityPolicySource* source = new ContentSecurityPolicySource();
    size_t position = 0;

    if (!parseScheme(source, sourceString, position) ||
        !parseHost(source, sourceString, position) ||
        !parsePort(source, sourceString, position) ||
        !parsePath(source, sourceString, position)) {
        return false;
    }

    m_sourceList.push_back(source);
    return true;
}

bool ContentSecurityPolicySourceListDirective::parseScheme(
    ContentSecurityPolicySource* source, String* sourceString, size_t& position)
{
    // scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

    size_t end = sourceString->length();
    if (position >= end) {
        return false;
    }
    size_t begin = position;
    char32_t c = sourceString->charAt(position);

    if (!isASCIIAlpha(c)) {
        source->isStarProtocol = true;
    }
    position++;

    while (position < end) {
        c = sourceString->charAt(position);

        if (c == ':') {
            if ((position + 2 < end) &&
                sourceString->charAt(position + 1) == '/' &&
                sourceString->charAt(position + 2) == '/') {
                if (source->isStarProtocol) {
                    return false;
                }
                source->protocol =
                    sourceString->substring(begin, position - begin + 1);
                source->isStarProtocol = false;
                position += 3;
                return true;
            }
            break;
        } else if (!isSchemeCharacter(c)) {
            break;
        }

        position++;
    }
    position = begin;
    source->isStarProtocol = true;
    return true;
}

bool ContentSecurityPolicySourceListDirective::parseHost(
    ContentSecurityPolicySource* source, String* sourceString, size_t& position)
{
    // host-part = "*" / [ "*." ] 1*host-char *( "." 1*host-char )

    size_t end = sourceString->length();
    if (position >= end) {
        return false;
    }
    size_t begin = position;
    char32_t c = sourceString->charAt(position);

    if (c == '*') {
        position++;
        if (position == end ||
            (position < end && (sourceString->charAt(position) == '/' ||
                                sourceString->charAt(position) == ':'))) {
            source->isStarServer = true;
            source->isStarDomain = true;
            return true;
        }
    }

    while (position < end) {
        c = sourceString->charAt(position);

        if (c == '.') {
            if (source->serverName->isEmpty()) {
                if (position == begin ||
                    (source->isStarPort && position - begin > 1)) {
                    return false;
                }
                source->serverName =
                    sourceString->substring(begin, position - begin);
                source->isStarServer = source->serverName->equals("*");
                begin = position + 1;
            }
        } else if (c == ':' || c == '/') {
            break;
        } else if (!isHostCharacter(c)) {
            return false;
        }
        position++;
    }

    if (position == begin || sourceString->charAt(position - 1) == '.') {
        return false;
    }

    String* host = sourceString->substring(begin, position - begin);
    if (source->serverName->isEmpty()) {
        source->serverName = host;
        source->isStarServer = source->serverName->equals("*");
        source->isStarDomain = true;
    } else {
        source->domainName = host;
    }
    return true;
}

bool ContentSecurityPolicySourceListDirective::parsePort(
    ContentSecurityPolicySource* source, String* sourceString, size_t& position)
{
    // port-part = 1*DIGIT / "*"
    size_t end = sourceString->length();
    if (position >= end || sourceString->charAt(position) != ':') {
        return true; // port does not exist
    }

    position++;
    size_t begin = position;

    char32_t c = sourceString->charAt(position);
    if (c == '*') {
        source->isStarPort = true;
        position++;
    }

    while (position < end) {
        c = sourceString->charAt(position);
        if (c == '/') {
            break;
        } else if (!isASCIIDigit(c)) {
            return false;
        }
        position++;
    }

    if (source->isStarPort) {
        if (position - begin == 1) {
            return true;
        }
        return false;
    } else if (begin == position) {
        return false;
    }

    source->port = sourceString->substring(begin, position - begin);
    return true;
}

bool ContentSecurityPolicySourceListDirective::parsePath(
    ContentSecurityPolicySource* source, String* sourceString, size_t& position)
{
    size_t end = sourceString->length();
    if (position >= end || sourceString->charAt(position) != '/') {
        return true; // path does not exist
    }
    size_t begin = position;
    position++;

    char32_t c;
    while (position < end) {
        c = sourceString->charAt(position);
        if (c == '?' || c == '#') {
            return false;
        }
        position++;
    }

    source->path = sourceString->substring(begin, position - begin);
    return true;
}

bool ContentSecurityPolicySourceListDirective::isScheme(String* scheme)
{
    size_t end = scheme->length();
    if (end == 0 || scheme->charAt(end - 1) != ':') {
        return false;
    }

    size_t position = 0;
    char32_t c;
    while (position < end - 1) {
        c = scheme->charAt(position);
        if (!isSchemeCharacter(c)) {
            return false;
        }
        position++;
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

bool ContentSecurityPolicySourceListDirective::allowScheme(ResourceURL* resUrl)
{
    for (auto scheme : m_schemeList) {
        if (matcheScheme(scheme, resUrl)) {
            return true;
        }
    }
    return false;
}

bool ContentSecurityPolicySourceListDirective::allowHost(ResourceURL* url)
{
    if (url->origin()->equalsIgnoreCase("null")) {
        return false; // not URL scheme
    }

    for (auto source : m_sourceList) {
        if (!source->isStarProtocol && !matcheScheme(source->protocol, url)) {
            continue;
        } else if (!source->isStarPort &&
                   !matchePort(source->port, source->protocol, url)) {
            continue;
        } else if (!matchePath(source->path, url->pathname())) {
            continue;
        }

        String* host = url->hostname();
        String* server = nullptr;
        String* domain = nullptr;
        size_t pos = host->find(".");
        if (pos == SIZE_MAX) {
            server = host;
        } else {
            server = host->substring(0, pos);
            domain = host->substring(pos + 1, host->length() - pos - 1);
        }

        if (!source->isStarServer &&
            !source->serverName->equalsIgnoreCase(server)) {
            continue;
        } else if (!source->isStarDomain &&
                   (domain && !source->domainName->equalsIgnoreCase(domain))) {
            continue;
        }
        return true;
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

bool ContentSecurityPolicySourceListDirective::matchePort(
    String* sourcePort, String* sourceProtocol, ResourceURL* url)
{
    String* port = url->port();
    if (sourcePort->equals(port)) {
        return true;
    } else if (sourcePort->isEmpty()) {
        return ResourceURL::isDefaultPortForProtocol(port, url->protocol());
    } else if (port->isEmpty()) {
        return ResourceURL::isDefaultPortForProtocol(sourcePort,
                                                     url->protocol());
    }
    return false;
}

bool ContentSecurityPolicySourceListDirective::matchePath(String* sourcePath,
                                                          String* urlPath)
{
    if (sourcePath->isEmpty() ||
        (sourcePath->equals("/") && urlPath->isEmpty())) {
        return true;
    }
    if (sourcePath->endsWith("/")) {
        return urlPath->startsWith(sourcePath);
    }

    // TODO: decode escape sequences
    return sourcePath->equalsIgnoreCase(urlPath);
}
}
