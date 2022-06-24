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

#ifndef __StarfishTextDecoder__
#define __StarfishTextDecoder__

#include "binding/ScriptWrappable.h"
#include "binding/generated/ArrayBufferViewOrArrayBufferUnion.h"

namespace Starfish {

struct TextDecoderOptions {
public:
    TextDecoderOptions()
        : m_fatal(false)
        , m_ignoreBOM(false)
    {
    }

    void setFatal(bool fatal)
    {
        m_fatal = fatal;
    }

    bool fatal() const
    {
        return m_fatal;
    }

    void setIgnoreBOM(bool ignoreBOM)
    {
        m_ignoreBOM = ignoreBOM;
    }

    bool ignoreBOM() const
    {
        return m_ignoreBOM;
    }

private:
    bool m_fatal;
    bool m_ignoreBOM;
};

struct TextDecodeOptions {
public:
    TextDecodeOptions()
        : m_stream(false)
    {
    }

    void setStream(bool stream)
    {
        m_stream = stream;
    }

    bool stream() const
    {
        return m_stream;
    }

private:
    bool m_stream;
};

class TextDecoder : public ScriptWrappable {
public:
    TextDecoder(ExecutionContext* executionContext, String* label);
    TextDecoder(ExecutionContext* executionContext, String* label,
                TextDecoderOptions options);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(TextDecoder)

    String* decode(Nullable<ArrayBufferViewOrArrayBuffer> input = nullptr,
                   Nullable<TextDecodeOptions> options = nullptr);
    String* decode(const uint8_t* data, size_t length,
                   TextDecodeOptions options);

    String* encoding() const
    {
        return m_encoding;
    }

    bool fatal() const
    {
        return m_fatal;
    }

    bool ignoreBOM() const
    {
        return m_ignoreBOM;
    }

protected:
    ExecutionContext* m_executionContext;
    String* m_encoding;
    UConverter* m_converter;
    bool m_fatal;
    bool m_ignoreBOM;
};
} // namespace Starfish

#endif
