#include "StarFishConfig.h"
#include "TextConverter.h"

namespace StarFish {

TextConverter::TextConverter(String* charsetName)
    : m_converter(nullptr)
{
    UErrorCode err = U_ZERO_ERROR;
    m_converter = ucnv_open(charsetName->utf8Data(), &err);
    if (U_FAILURE(err)) {
        STARFISH_LOG_ERROR("TextConverter got unknown encoding -> %s\n", charsetName->utf8Data());
        m_converter = nullptr;
    }
    m_encoding = charsetName;
    registerFinalizer();
}

TextConverter::TextConverter(String* mimetype, String* preferredEncoding, const char* bytes, size_t len)
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

        m_converter = ucnv_open(type->utf8Data(), &err);
        if (!U_FAILURE(err)) {
            m_encoding = String::fromUTF8(ucnv_getName(m_converter, &err));
            registerFinalizer();
            return;
        } else {
            STARFISH_LOG_ERROR("TextConverter got unknown encoding -> %s\n", type->utf8Data());
            m_converter = nullptr;
        }
    }

    UCharsetDetector* det;
    const UCharsetMatch** match;

    err = U_ZERO_ERROR;
    det = ucsdet_open(&err);
    STARFISH_ASSERT(!U_FAILURE(err));
    ucsdet_setText(det, bytes, 1024 * 16 < len ? 1024 * 16 : len, &err);
    STARFISH_ASSERT(!U_FAILURE(err));

    int confidence, num;

    match = ucsdet_detectAll(det, &num, &err);
    if (U_FAILURE(err)) {
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
        STARFISH_LOG_INFO("encoding detector verbose info.. %s[%d]\n", charset, confidence);
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

        if (ucnv_compareNames(charset, preferredEncoding->utf8Data()) == 0) {
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
        STARFISH_LOG_ERROR("TextConverter got unknown encoding -> %s\n", bestCharset);
        m_converter = nullptr;
    }

    m_encoding = String::fromUTF8(bestCharset);
    registerFinalizer();
}

String* TextConverter::convert(const char* bytes, size_t len, bool isEndOfStream)
{
    if (m_converter) {
        UErrorCode err;
        err = U_ZERO_ERROR;

        UChar displayName[32];
        char displayNameASCII[32];
        ucnv_getDisplayName(m_converter, "en-US", displayName, 32, &err);
        STARFISH_ASSERT(!U_FAILURE(err));
        for (size_t i = 0; i < 32; i ++) {
            displayNameASCII[i] = displayName[i];
        }

        if (isEndOfStream && ucnv_compareNames(displayNameASCII, "utf-8") == 0) {
            return String::fromUTF8(bytes, len);
        }
        STARFISH_ASSERT(!U_FAILURE(err));
        m_bufferToConvert.assign(&bytes[0], &bytes[len]);
        UTF32String str;
        bool hasUTFChar = false;
        while (true) {
            UChar targetOrg[512];
            UChar* target = targetOrg;
            UChar* targetEnd = &target[512];
            const char* input = m_bufferToConvert.data();
            ucnv_toUnicode(m_converter, &target, targetEnd, &input, input + m_bufferToConvert.size(), NULL, isEndOfStream, &err);

            size_t length = (size_t)target - (size_t)targetOrg;
            length /= sizeof(UChar);
            UChar* targetStart = targetOrg;
            for (size_t i = 0; i < length; /* U16_NEXT post-increments */) {
                char32_t c;
                U16_NEXT(targetStart, i, length, c);
                if (c > 127) {
                    hasUTFChar = true;
                }
                str += c;
            }

            m_bufferToConvert.erase(m_bufferToConvert.begin(), m_bufferToConvert.begin() + ((size_t)input - (size_t)m_bufferToConvert.data()));
            if (err != U_BUFFER_OVERFLOW_ERROR) {
                break;
            }
            err = U_ZERO_ERROR;
        }
        if (hasUTFChar)
            return String::createUTF32String(str);
        return String::createASCIIStringFromUTF32Source(str);
    }
    return String::fromUTF8(bytes, len);
}

void TextConverter::registerFinalizer()
{
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        // STARFISH_LOG_INFO("TextConverter::~TextConverter\n");
        TextConverter* nr = (TextConverter*)obj;
        if (nr->m_converter) {
            ucnv_close(nr->m_converter);
        }
    }, NULL, NULL, NULL);
}

}
