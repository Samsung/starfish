#ifndef __StarFishDOMException__
#define __StarFishDOMException__

#include "dom/binding/ScriptWrappable.h"

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

    DOMException(ScriptBindingInstance* instance, Code code, const char* message = nullptr);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    String* message() { return m_message; }
    const char* name() { return s_names[m_code]; }
    int code() { return m_code; }

private:
    ScriptBindingInstance* m_instance;
    uint8_t m_code;
    String* m_message;
    static const char* s_names[];
    static const char* s_descriptions[];
};

}

#endif
