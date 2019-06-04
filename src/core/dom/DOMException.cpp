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

#include "StarfishConfig.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

static const char* s_names[] = {
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

const size_t s_domExceptionNameCount = sizeof(s_names) / sizeof(size_t);
static_assert(s_domExceptionNameCount == 26, "");

static const char* s_descriptions[] = {
    "Unspecified DOM Exception.", "The index is not in the allowed range.", "",
    "The operation would yield an incorrect node tree.",
    "The object is in the wrong document.",
    "The string contains invalid characters.", "",
    "The object can not be modified.", "The object can not be found here.",
    "The operation is not supported.", "The attribute is in use.",
    "The object is in an invalid state.",
    "The string did not match the expected pattern.",
    "The object can not be modified in this way.",
    "The operation is not allowed by Namespaces in XML.",
    "The object does not support the operation or argument.", "", "",
    "The operation is insecure.", "A network error occurred.",
    "The operation was aborted.", "The given URL does not match another URL.",
    "The quota has been exceeded.", "The operation timed out.",
    "The supplied node is incorrect or has an incorrect ancestor for this "
    "operation.",
    "The object can not be cloned.",
};

const size_t s_domExceptionDescriptionsCount =
    sizeof(s_descriptions) / sizeof(size_t);
static_assert(s_domExceptionDescriptionsCount == 26, "");

DOMException::DOMException(ExecutionContext* executionContext, Code code,
                           const char* message)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_code(code)
    , m_name(String::emptyString)
{
    if (code < SCRIPT_ERROR) {
        if (message == nullptr) {
            if (m_code < s_domExceptionDescriptionsCount) {
                message = s_descriptions[m_code];
            } else {
                message = "";
            }
        }
    } else {
        if (message == nullptr) {
            message = "";
        }

        if (code == SCRIPT_ERROR) {
            overrideScriptObject(
                scriptTypeError(scriptBindingInstance(),
                                String::fromUTF8(message, strlen(message))));
        } else if (code == SCRIPT_EVAL_ERR) {
            overrideScriptObject(
                scriptEvalError(scriptBindingInstance(),
                                String::fromUTF8(message, strlen(message))));
        } else if (code == SCRIPT_RANGE_ERR) {
            overrideScriptObject(
                scriptRangeError(scriptBindingInstance(),
                                 String::fromUTF8(message, strlen(message))));
        } else if (code == SCRIPT_REFERENCE_ERR) {
            overrideScriptObject(scriptReferenceError(
                scriptBindingInstance(),
                String::fromUTF8(message, strlen(message))));
        } else if (code == SCRIPT_TYPE_ERR) {
            overrideScriptObject(
                scriptTypeError(scriptBindingInstance(),
                                String::fromUTF8(message, strlen(message))));
        } else if (code == SCRIPT_URI_ERR) {
            overrideScriptObject(
                scriptURIError(scriptBindingInstance(),
                               String::fromUTF8(message, strlen(message))));
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    }
    m_message = String::fromUTF8(message, strlen(message));
}

DOMException::DOMException(ExecutionContext* executionContext, String* message,
                           String* name)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_code(Code::DOM_EXCEPTION)
    , m_message(message)
    , m_name(name)
{
}

String* DOMException::name()
{
    if (m_code == DOM_EXCEPTION && m_name->length() > 0) {
        return m_name;
    }
    if (m_code < s_domExceptionNameCount) {
        return String::fromUTF8(s_names[m_code], strlen(s_names[m_code]));
    } else {
        return m_name;
    }
}

ScriptBindingInstance* DOMException::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}
}
