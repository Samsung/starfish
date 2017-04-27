/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

        SIMPLE_EXCEPTION = 100,
        ERROR = 101,
        EVAL_ERR = 102,
        RANGE_ERR = 103,
        REFERENCE_ERR = 104,
        TYPE_ERR = 105,
        URI_ERR = 106,
    };

    DOMException(ScriptBindingInstance* instance, Code code,
                 const char* message = nullptr);

    // Constructor exposed to script.
    DOMException(Document* document, String* message, String* name);

    virtual void init(ScriptBindingInstance* instance) override
    {
        scriptObject()->set__proto__(
            fetchData(instance)->fnDOMException()->protoType());
        scriptObject()->defineDataProperty(ESString::create("code"), false,
                                           false, false, ESValue(m_code));
    }

    virtual bool isDOMException() const override
    {
        return true;
    }

    String* message()
    {
        return m_message;
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
    ScriptBindingInstance* m_instance;
    uint16_t m_code;
    String* m_message;
    String* m_name;
    static const char* s_names[];
    static const char* s_descriptions[];
};
}

#endif
