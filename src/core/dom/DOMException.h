/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishDOMException__
#define __StarfishDOMException__

#include "binding/ScriptWrappable.h"

namespace Starfish {

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
        ENCODING_ERR, // No legacy code from here
        NOT_ALLOWED_ERROR,

        SCRIPT_ERROR = 101,
        SCRIPT_EVAL_ERR = 102,
        SCRIPT_RANGE_ERR = 103,
        SCRIPT_REFERENCE_ERR = 104,
        SCRIPT_TYPE_ERR = 105,
        SCRIPT_URI_ERR = 106,
    };

    DOMException(ExecutionContext* executionContext, Code code,
                 const char* message = nullptr);

    // Constructor exposed to script.
    DOMException(ExecutionContext* executionContext, String* message,
                 String* name);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMException)

    String* message()
    {
        return m_message;
    }

    void setMessage(String* message)
    {
        m_message = message;
    }

    String* name();
    uint16_t code()
    {
        return m_code;
    }

private:
    ExecutionContext* m_executionContext;
    uint16_t m_code;
    String* m_message;
    String* m_name;
};
} // namespace Starfish

#endif
