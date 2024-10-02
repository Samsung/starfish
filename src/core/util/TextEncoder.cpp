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
#include "core/util/TextEncoder.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"

namespace Starfish {

TextEncoder::TextEncoder(ExecutionContext* executionContext, String* label)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_encoding(label)
{
    UErrorCode err = U_ZERO_ERROR;
    m_converter = ucnv_open(m_encoding->toUTF8NonGCString().data(), &err);
    if (U_FAILURE(err)) {
        m_converter = nullptr;
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_RANGE_ERR,
                               "The encoding is you provided is not exists");
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            TextEncoder* self = (TextEncoder*)obj;
            if (self->m_converter) {
                ucnv_close(self->m_converter);
            }
        },
        this, NULL, NULL);
}

ScriptBindingInstance* TextEncoder::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ScriptUint8Array TextEncoder::encode(Optional<String*> input)
{
    if (!input) {
        return createEmptyUint8Array(
            m_executionContext->scriptBindingInstance());
    }

    String* v = input.value();

    struct Data {
        TextEncoder* self;
        Vector<char, std::allocator<char>> buffer;
    } d;

    d.self = this;

    v->peekUTF16Buffer(
        [](const char16_t* buf, size_t len, void* data) -> size_t {
            Data* d = (Data*)data;

            UErrorCode err = U_ZERO_ERROR;

            const char16_t* input = buf;
            const char16_t* inputEnd = buf + len;
            while (true) {
                char targetOrg[512];
                char* target = targetOrg;
                char* targetEnd = &target[512];
                ucnv_fromUnicode(d->self->m_converter, &target, targetEnd,
                                 &input, inputEnd, nullptr, true, &err);

                size_t length = (size_t)target - (size_t)targetOrg;
                d->buffer.insert(d->buffer.end(), targetOrg,
                                 &targetOrg[length]);
                if (err != U_BUFFER_OVERFLOW_ERROR) {
                    break;
                }
                err = U_ZERO_ERROR;
            }

            return 0;
        },
        &d);

    size_t len = d.buffer.size();
    char* buf = d.buffer.takeBuffer();
    return createScriptUint8Array(m_executionContext->scriptBindingInstance(),
                                  buf, len);
}

} // namespace Starfish
