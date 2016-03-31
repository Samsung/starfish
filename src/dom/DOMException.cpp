#include "StarFishConfig.h"
#include "DOMException.h"

namespace StarFish {

const char* DOMException::s_names[] = {
    "DOMException",
    "IndexSizeError",
    "",
    "HierarchyRequestError",
    "WrongDocumentError",
    "InvalidCharacterError",
    "",
    "NoModificationAllowedError",
    "NotFoundError",
    "NotSupportedError",
    "InUseAttributeError",
    "InvalidStateError",
    "SyntaxError",
    "InvalidModificationError",
    "NamespaceError",
    "InvalidAccessError",
    "",
    "",
    "SecurityError",
    "NetworkError",
    "AbortError",
    "URLMismatchError",
    "QuotaExceededError",
    "TimeoutError",
    "InvalidNodeTypeError",
    "DataCloneError",
};

const char* DOMException::s_descriptions[] = {
    "Unspecified DOM Exception.",
    "The index is not in the allowed range.",
    "",
    "The operation would yield an incorrect node tree.",
    "The object is in the wrong document.",
    "The string contains invalid characters.",
    "",
    "The object can not be modified.",
    "The object can not be found here.",
    "The operation is not supported.",
    "The attribute is in use.",
    "The object is in an invalid state.",
    "The string did not match the expected pattern.",
    "The object can not be modified in this way.",
    "The operation is not allowed by Namespaces in XML.",
    "The object does not support the operation or argument.",
    "",
    "",
    "The operation is insecure.",
    "A network error occurred.",
    "The operation was aborted.",
    "The given URL does not match another URL.",
    "The quota has been exceeded.",
    "The operation timed out.",
    "The supplied node is incorrect or has an incorrect ancestor for this operation.",
    "The object can not be cloned.",
};

DOMException::DOMException(ScriptBindingInstance* instance, Code code, const char* message)
    : ScriptWrappable(this)
    , m_instance(instance)
    , m_code(code)
{
    if (!message)
        message = s_descriptions[code];
    m_message = String::fromUTF8(message);

    initScriptWrappable(this, instance);
    scriptObject()->defineDataProperty(escargot::ESString::create("code"), false, false, false, escargot::ESValue(m_code));
}

}
