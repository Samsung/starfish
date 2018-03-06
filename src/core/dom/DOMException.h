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

#ifndef __StarFishDOMException__
#define __StarFishDOMException__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class DOMException : public ScriptWrappable {
public:
    enum Code {
        // https://heycam.github.io/webidl/#idl-exceptions
        DOM_EXCEPTION = 0,
        INDEX_SIZE_ERR = 1,
        HIERARCHY_REQUEST_ERR = 3,
        WRONG_DOCUMENT_ERR = 4,
        INVALID_CHARACTER_ERR = 5,
        NO_MODIFICATION_ALLOWED_ERR = 7,
        NOT_FOUND_ERR = 8,
        NOT_SUPPORTED_ERR = 9,
        INUSE_ATTRIBUTE_ERR = 10,
        INVALID_STATE_ERR = 11,
        SYNTAX_ERR = 12,
        INVALID_MODIFICATION_ERR = 13,
        NAMESPACE_ERR = 14,
        INVALID_ACCESS_ERR = 15,
        SECURITY_ERR = 18,
        NETWORK_ERR = 19,
        ABORT_ERR = 20,
        URL_MISMATCH_ERR = 21,
        QUOTA_EXCEEDED_ERR = 22,
        TIMEOUT_ERR = 23,
        INVALID_NODE_TYPE_ERR = 24,
        DATA_CLONE_ERR = 25,

        SCRIPT_ERROR = 101,
        SCRIPT_EVAL_ERR = 102,
        SCRIPT_RANGE_ERR = 103,
        SCRIPT_REFERENCE_ERR = 104,
        SCRIPT_TYPE_ERR = 105,
        SCRIPT_URI_ERR = 106,
    };

    DOMException(Document* document, Code code, const char* message = nullptr);

    // Constructor exposed to script.
    DOMException(Document* document, String* message, String* name);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;
    virtual bool isDOMException() const override;

    String* message()
    {
        return m_message;
    }

    void setMessage(String* message)
    {
        m_message = message;
    }

    String* name()
    {
        if (m_code == DOM_EXCEPTION && m_name->length() > 0) {
            return m_name;
        }
        return String::fromUTF8(s_names[m_code]);
    }

    uint16_t code()
    {
        return m_code;
    }

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    uint16_t m_code;
    String* m_message;
    String* m_name;
    static const char* s_names[];
    static const char* s_descriptions[];
};
}

#endif
