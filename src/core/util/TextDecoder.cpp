/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"
#include "core/util/TextDecoder.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

namespace Starfish {

TextDecoder::TextDecoder(ExecutionContext* executionContext, String* label)
    : TextDecoder(executionContext, label, TextDecoderOptions())
{
}

TextDecoder::TextDecoder(ExecutionContext* executionContext, String* label,
                         TextDecoderOptions options)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_encoding(label)
    , m_fatal(options.fatal())
    , m_ignoreBOM(options.ignoreBOM())
{
    UErrorCode err = U_ZERO_ERROR;
    m_converter = ucnv_open(m_encoding->toUTF8NonGCString().data(), &err);
    if (U_FAILURE(err)) {
        m_converter = nullptr;
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_RANGE_ERR,
                               "The encoding is you provided is not exists");
    } else {
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                TextDecoder* self = (TextDecoder*)obj;
                if (self->m_converter) {
                    ucnv_close(self->m_converter);
                }
            },
            NULL, NULL, NULL);
    }
}

ScriptBindingInstance* TextDecoder::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

String* TextDecoder::decode(Optional<ArrayBufferViewOrArrayBuffer> input,
                            Optional<TextDecodeOptions> options)
{
    if (!input) {
        return String::emptyString;
    }

    TextDecodeOptions o =
        options.hasValue() ? options.value() : TextDecodeOptions();
    if (input.value().isArrayBufferViewValue()) {
        ScriptArrayBufferView unwrap = input.value().getArrayBufferViewValue();
        return decode(arrayBufferViewRawData(unwrap),
                      arrayBufferViewByteSize(unwrap), o);
    } else {
        STARFISH_ASSERT(input.value().isArrayBufferValue());
        ScriptArrayBuffer unwrap = input.value().getArrayBufferValue();
        return decode(arrayBufferRawData(unwrap), arrayBufferByteSize(unwrap),
                      o);
    }
}

String* TextDecoder::decode(const uint8_t* data, size_t length,
                            TextDecodeOptions options)
{
    if (m_ignoreBOM) {
        uint8_t c, c2, c3, c4;

        bool found = false;
        if (length > 1) {
            c = data[0] & 0xff;
            c2 = data[1] & 0xff;
            if (c == 0xff && c2 == 0xfe &&
                m_encoding->startsWith("utf-16", false)) {
                data += 2;
                length -= 2;
                found = true;
            } else if (c == 0xfe && c2 == 0xff &&
                       m_encoding->startsWith("utf-16", false)) {
                data += 2;
                length -= 2;
                found = true;
            }
        }
        if (!found && length > 2) {
            c3 = data[2] & 0xff;
            if (c == 0xef && c2 == 0xbb && c3 == 0xbf &&
                m_encoding->equalsIgnoreCase("utf-8")) {
                data += 3;
                length -= 3;
                found = true;
            }
        }
        if (!found && length > 3) {
            c4 = data[3] & 0xff;
            if (c == 0x00 && c2 == 0x00 && c3 == 0xfe && c4 == 0xff &&
                m_encoding->startsWith("utf-32", false)) {
                data += 4;
                length -= 4;
                found = true;
            } else if (c == 0xff && c2 == 0xfe && c3 == 0x00 && c4 == 0x00 &&
                       m_encoding->startsWith("utf-32", false)) {
                data += 4;
                length -= 4;
                found = true;
            }
        }
    }
    UTF32StringDataNonGCStd str;
    bool hasUTFChar = false;
    bool hasNonBMPChar = false;
    UErrorCode err = U_ZERO_ERROR;

    const char* input = (const char*)(data);
    const char* inputEnd = (const char*)(data + length);
    while (true) {
        UChar targetOrg[512];
        UChar* target = targetOrg;
        UChar* targetEnd = &target[512];
        ucnv_toUnicode(m_converter, &target, targetEnd, &input, inputEnd,
                       nullptr, !options.stream(), &err);

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
            if (UNLIKELY(m_fatal && c == 0xFFFD)) {
                throw new DOMException(m_executionContext,
                                       DOMException::Code::SCRIPT_TYPE_ERR,
                                       "failed to decode buffer");
            }
            str += c;
        }

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

} // namespace Starfish
