/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "TextConverter.h"

namespace StarFish {

TextConverter::TextConverter(String* charsetName)
    : m_converter(nullptr)
{
    UErrorCode err = U_ZERO_ERROR;
    auto utf8Data = charsetName->toUTF8NonGCString();
    m_converter = ucnv_open(utf8Data.data(), &err);
    if (U_FAILURE(err)) {
        STARFISH_LOG_ERROR("TextConverter: Unknown encoding: %s\n",
                           utf8Data.data());
        m_converter = nullptr;
    }
    m_encoding = charsetName;
    registerFinalizer();
}

TextConverter::TextConverter(String* mimetype, String* preferredEncoding,
                             const char* bytes, size_t len)
    : m_converter(nullptr)
{
    UErrorCode err = U_ZERO_ERROR;
    size_t charset = mimetype->find("charset=");
    if (charset != SIZE_MAX) {
        charset += 8;
        size_t semi = mimetype->find(";", charset);
        String* type;
        if (semi != SIZE_MAX) {
            type = mimetype->substring(charset, semi - charset);
        } else {
            type = mimetype->substring(charset, mimetype->length() - charset);
        }
        auto utf8Data = type->toUTF8NonGCString();
        m_converter = ucnv_open(utf8Data.data(), &err);
        if (!U_FAILURE(err)) {
            m_encoding = String::fromUTF8(ucnv_getName(m_converter, &err));
            registerFinalizer();
            return;
        } else {
            STARFISH_LOG_ERROR("TextConverter: Unknown encoding: %s\n",
                               utf8Data.data());
            m_converter = nullptr;
        }
    }

    UCharsetDetector* det;
    const UCharsetMatch** match;

    err = U_ZERO_ERROR;
    det = ucsdet_open(&err);
    STARFISH_ASSERT(!U_FAILURE(err));
    const int maxDetectBytes = 1024;
    ucsdet_setText(det, bytes, maxDetectBytes < len ? maxDetectBytes : len,
                   &err);
    STARFISH_ASSERT(!U_FAILURE(err));

    int confidence, num;

    match = ucsdet_detectAll(det, &num, &err);
    if (U_FAILURE(err)) {
        ucsdet_close(det);
        return;
    }

    const UCharsetMatch* m1 = ucsdet_detect(det, &err);
    STARFISH_ASSERT(!U_FAILURE(err));
    const char* bestCharset = ucsdet_getName(m1, &err);
    STARFISH_ASSERT(!U_FAILURE(err));

    /*
    #ifndef NDEBUG
        STARFISH_LOG_INFO("encoding detector verbose info start\n");
        for (int i = 0; i < num; i++) {
            const char* charset = nullptr;
            confidence = ucsdet_getConfidence(match[i], &err);
            charset = ucsdet_getName(match[i], &err);
            STARFISH_LOG_INFO("encoding detector verbose info.. %s[%d]\n",
                charset, confidence);
        }
        STARFISH_LOG_INFO("encoding detector verbose info end\n");
    #endif
    */
    for (int i = 0; i < num; i++) {
        const char* charset = nullptr;
        confidence = ucsdet_getConfidence(match[i], &err);
        STARFISH_ASSERT(!U_FAILURE(err));
        charset = ucsdet_getName(match[i], &err);
        STARFISH_ASSERT(!U_FAILURE(err));

        if (confidence == 100) {
            bestCharset = charset;
            break;
        }

        auto utf8Data = preferredEncoding->toUTF8NonGCString();
        if (ucnv_compareNames(charset, utf8Data.data()) == 0) {
            bestCharset = charset;
            break;
        }

        if (confidence < 10) {
            continue;
        }
        STARFISH_ASSERT(!U_FAILURE(err));
    }

    ucsdet_close(det);

    m_converter = ucnv_open(bestCharset, &err);
    if (U_FAILURE(err)) {
        STARFISH_LOG_ERROR("TextConverter: Unknown encoding: %s\n",
                           bestCharset);
        m_converter = nullptr;
    }

    m_encoding = String::fromUTF8(bestCharset);
    registerFinalizer();
}

TextConverter::~TextConverter()
{
    if (m_converter) {
        ucnv_close(m_converter);
        m_converter = nullptr;
    }
}

String* TextConverter::convert(const char* bytes, size_t len,
                               bool isEndOfStream)
{
    if (m_converter) {
        UErrorCode err;
        err = U_ZERO_ERROR;

        UChar displayName[32];
        char displayNameASCII[32];
        ucnv_getDisplayName(m_converter, "en-US", displayName, 32, &err);
        STARFISH_ASSERT(!U_FAILURE(err));
        for (size_t i = 0; i < 32; i++) {
            displayNameASCII[i] = displayName[i];
        }

        if (isEndOfStream &&
            ucnv_compareNames(displayNameASCII, "utf-8") == 0) {
            return String::fromUTF8(bytes, len);
        }
        STARFISH_ASSERT(!U_FAILURE(err));
        m_bufferToConvert.assign(&bytes[0], &bytes[len]);
        UTF32StringDataNonGCStd str;
        bool hasUTFChar = false;
        bool hasNonBMPChar = false;
        while (true) {
            UChar targetOrg[512];
            UChar* target = targetOrg;
            UChar* targetEnd = &target[512];
            const char* input = m_bufferToConvert.data();
            ucnv_toUnicode(m_converter, &target, targetEnd, &input,
                           input + m_bufferToConvert.size(), NULL,
                           isEndOfStream, &err);

            size_t length = (size_t)target - (size_t)targetOrg;
            length /= sizeof(UChar);
            UChar* targetStart = targetOrg;
            for (size_t i = 0; i < length; /* U16_NEXT post-increments */) {
                char32_t c;
                U16_NEXT(targetStart, i, length, c);
                if (c > 127) {
                    hasUTFChar = true;
                }
                if (c > 0xffff) {
                    hasNonBMPChar = true;
                }
                str += c;
            }

            m_bufferToConvert.erase(
                m_bufferToConvert.begin(),
                m_bufferToConvert.begin() +
                    ((size_t)input - (size_t)m_bufferToConvert.data()));
            if (err != U_BUFFER_OVERFLOW_ERROR) {
                break;
            }
            err = U_ZERO_ERROR;
        }
        if (hasNonBMPChar) {
            return String::createUTF32String(str);
        }
        if (hasUTFChar) {
            return String::createBMPStringFromUTF32Source(str);
        }
        return String::createASCIIStringFromUTF32Source(str);
    }
    return String::fromUTF8(bytes, len);
}

void TextConverter::registerFinalizer()
{
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       // STARFISH_LOG_INFO(
                                       //    "TextConverter::~TextConverter\n");
                                       TextConverter* nr = (TextConverter*)obj;
                                       if (nr->m_converter) {
                                           ucnv_close(nr->m_converter);
                                           nr->m_converter = nullptr;
                                       }
                                   },
                                   NULL, NULL, NULL);
}
}
