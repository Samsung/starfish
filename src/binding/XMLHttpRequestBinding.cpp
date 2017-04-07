/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue xhrElementFunction(ESVMInstance* instance)
{
    Window* wnd = ((Window*)ESVMInstance::currentInstance()
                       ->globalObject()
                       ->extraPointerData());
    auto xhr = new XMLHttpRequest(wnd->document());
    return xhr->scriptValue();
}

#define DEFINE_XHR_EVENT_HANDLER_FUNC(eventName)                          \
    static ESValue on##eventName##GetterFunction(ESVMInstance* instance)  \
    {                                                                     \
        GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);                     \
        auto eventType = originalObj->networkRequest()                    \
                             .starFish()                                  \
                             ->staticStrings()                            \
                             ->m_##eventName;                             \
        return originalObj->attributeEventListener(eventType);            \
    }                                                                     \
                                                                          \
    static ESValue on##eventName##SetterFunction(ESVMInstance* instance)  \
    {                                                                     \
        GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);                     \
        auto eventType = originalObj->networkRequest()                    \
                             .starFish()                                  \
                             ->staticStrings()                            \
                             ->m_##eventName;                             \
        ESValue v = instance->currentExecutionContext()->readArgument(0); \
        originalObj->setAttributeEventListener(eventType, v);             \
        return ESValue();                                                 \
    }

DEFINE_XHR_EVENT_HANDLER_FUNC(loadstart);
DEFINE_XHR_EVENT_HANDLER_FUNC(progress);
DEFINE_XHR_EVENT_HANDLER_FUNC(abort);
DEFINE_XHR_EVENT_HANDLER_FUNC(error);
DEFINE_XHR_EVENT_HANDLER_FUNC(load);
DEFINE_XHR_EVENT_HANDLER_FUNC(timeout);
DEFINE_XHR_EVENT_HANDLER_FUNC(loadend);
DEFINE_XHR_EVENT_HANDLER_FUNC(readystatechange);
#undef DEFINE_XHR_EVENT_HANDLER_FUNC

static ESValue timeoutGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    uint32_t c = originalObj->networkRequest().timeout();
    return ESValue(c);
}

static ESValue timeoutSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    try {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        originalObj->setTimeout(v.toUint32());
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue();
}

static ESValue readyStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    int c = originalObj->networkRequest().readyState();
    return ESValue(c);
}

static ESValue statusGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    int c = originalObj->networkRequest().status();
    return ESValue(c);
}

static ESValue responseTextGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    try {
        String* c = originalObj->responseText();

#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("&&&responseText\n");
#endif
        return toJSString(c);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue();
}

static ESValue responseGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    try {
        ESValue c = originalObj->response();
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("&&&response\n");
#endif
        return c;
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue();
}

static ESValue responseTypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    XMLHttpRequest::ResponseType type = originalObj->responseType();
    if (type == XMLHttpRequest::ResponseType::Unspecified) {
        return ESString::create("");
    } else if (type == XMLHttpRequest::ResponseType::ArrayBuffer) {
        return ESString::create("arraybuffer");
    } else if (type == XMLHttpRequest::ResponseType::BlobType) {
        return ESString::create("blob");
    } else if (type == XMLHttpRequest::ResponseType::DocumentType) {
        return ESString::create("document");
    } else if (type == XMLHttpRequest::ResponseType::Json) {
        return ESString::create("json");
    } else if (type == XMLHttpRequest::ResponseType::Text) {
        return ESString::create("text");
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue responseTypeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    try {
        ESString* str =
            instance->currentExecutionContext()->readArgument(0).toString();
        if (*str == "") {
            originalObj->setResponseType(
                XMLHttpRequest::ResponseType::Unspecified);
            return ESValue();
        } else if (*str == "arraybuffer") {
#ifdef USE_ES6_FEATURE
            originalObj->setResponseType(
                XMLHttpRequest::ResponseType::ArrayBuffer);
            return ESValue();
#endif
        } else if (*str == "blob") {
            originalObj->setResponseType(
                XMLHttpRequest::ResponseType::BlobType);
            return ESValue();
        } else if (*str == "document") {
        } else if (*str == "json") {
            originalObj->setResponseType(XMLHttpRequest::ResponseType::Json);
            return ESValue();
        } else if (*str == "text") {
            originalObj->setResponseType(XMLHttpRequest::ResponseType::Text);
            return ESValue();
        }
        STARFISH_LOG_ERROR(
            "The provided value '%s' is not a valid enum value of "
            "type "
            "XMLHttpRequestResponseType.",
            str->utf8Data());
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    return ESValue();
}

static ESValue setRequestHeaderFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    if (instance->currentExecutionContext()->argumentCount() < 2) {
        auto msg = ESString::create(
            "Failed to execute 'setRequestHeader' on "
            "'XMLHttpRequest': setRequestHeader needs 2 "
            "parameter.");
        instance->throwError(ESValue(TypeError::create(msg)));
    }
    try {
        String* s1 = toBrowserString(
                         instance->currentExecutionContext()->readArgument(0))
                         ->trim();
        String* s2 = toBrowserString(
                         instance->currentExecutionContext()->readArgument(1))
                         ->trim();
        originalObj->setRequestHeader(s1, s2);
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue openFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    try {
        if (instance->currentExecutionContext()->argumentCount() >= 2) {
            // https://xhr.spec.whatwg.org/#the-open()-method
            // TOOD If method is not a method, throw a
            // SyntaxError
            // exception.
            // TODO If method is a forbidden method, throw a
            // SecurityError exception.
            // Let parsedURL be the result of parsing url with
            // context object's relevant settings object's API
            // base
            // URL.
            // If parsedURL is failure, throw a SyntaxError
            // exception.
            std::string method = instance->currentExecutionContext()
                                     ->readArgument(0)
                                     .toString()
                                     ->utf8Data();
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            NetworkRequest::MethodType mt;
            if (method == "post") {
                mt = NetworkRequest::POST_METHOD;
            } else if (method == "get") {
                mt = NetworkRequest::GET_METHOD;
            } else {
                mt = NetworkRequest::UNKNOWN_METHOD;
                STARFISH_LOG_ERROR("Unsupported method : %s\n", method.c_str());
            }

            bool async = true;
            if (instance->currentExecutionContext()->argumentCount() >= 3) {
                async = instance->currentExecutionContext()
                            ->readArgument(2)
                            .toBoolean();
            }

            String* userName = String::emptyString;
            String* password = String::emptyString;

            if (instance->currentExecutionContext()->argumentCount() == 4) {
                userName = toBrowserString(
                    instance->currentExecutionContext()->readArgument(3));
            } else if (instance->currentExecutionContext()->argumentCount() >=
                       5) {
                userName = toBrowserString(
                    instance->currentExecutionContext()->readArgument(3));
                password = toBrowserString(
                    instance->currentExecutionContext()->readArgument(4));
            }
            originalObj->open(
                mt, toBrowserString(
                        instance->currentExecutionContext()->readArgument(1)),
                async, userName, password);
        } else {
            auto msg = ESString::create(
                "Failed to execute 'open' on 'XMLHttpRequest': "
                "2 "
                "arguments required.");
            instance->throwError(ESValue(TypeError::create(msg)));
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue sendFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    try {
        if (instance->currentExecutionContext()->argumentCount() == 0) {
            originalObj->send();
        } else {
            originalObj->send(toBrowserString(
                instance->currentExecutionContext()->readArgument(0)));
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue abortFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);
    try {
        originalObj->abort();
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

ESFunctionObject* bindingXMLHttpRequest(
    ScriptBindingInstance* scriptBindingInstance)
{
    /* XMLHttpRequestEventTarget */

    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        XMLHttpRequestEventTarget,
        fetchData(scriptBindingInstance)->m_fnEventTarget);
    fetchData(scriptBindingInstance)
        ->m_instance->globalObject()
        ->defineDataProperty(XMLHttpRequestEventTargetString, true, false, true,
                             XMLHttpRequestEventTargetFunction);

    /* XMLHttpRequest */
    ESFunctionObject* fnXhrElement = ESFunctionObject::create(
        NULL, xhrElementFunction, ESString::create("XMLHttpRequest"), 0, true,
        false);

    fnXhrElement->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);

    fnXhrElement->protoType().asESPointer()->asESObject()->set__proto__(
        XMLHttpRequestEventTargetFunction->protoType());

    fetchData(scriptBindingInstance)
        ->m_instance->globalObject()
        ->defineDataProperty(ESString::create("XMLHttpRequest"), false, false,
                             false, fnXhrElement);

#define DEFINE_XHR_EVENT_HANDLER(eventName)                               \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(              \
        XMLHttpRequestEventTargetFunction->protoType()                    \
            .asESPointer()                                                \
            ->asESObject(),                                               \
        ESString::create("on" #eventName), on##eventName##GetterFunction, \
        on##eventName##SetterFunction);

    DEFINE_XHR_EVENT_HANDLER(loadstart);
    DEFINE_XHR_EVENT_HANDLER(progress);
    DEFINE_XHR_EVENT_HANDLER(abort);
    DEFINE_XHR_EVENT_HANDLER(error);
    DEFINE_XHR_EVENT_HANDLER(load);
    DEFINE_XHR_EVENT_HANDLER(timeout);
    DEFINE_XHR_EVENT_HANDLER(loadend);
    DEFINE_XHR_EVENT_HANDLER(readystatechange);
#undef DEFINE_XHR_EVENT_HANDLER

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnXhrElement->protoType().asESPointer()->asESObject(),
        ESString::create("timeout"), timeoutGetterFunction,
        timeoutSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnXhrElement->protoType().asESPointer()->asESObject(),
        ESString::create("readyState"), readyStateGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnXhrElement->protoType().asESPointer()->asESObject(),
        ESString::create("status"), statusGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnXhrElement->protoType().asESPointer()->asESObject(),
        ESString::create("responseText"), responseTextGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnXhrElement->protoType().asESPointer()->asESObject(),
        ESString::create("response"), responseGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnXhrElement->protoType().asESPointer()->asESObject(),
        ESString::create("responseType"), responseTypeGetterFunction,
        responseTypeSetterFunction);

    fnXhrElement->asESObject()->defineDataProperty(
        ESString::create("UNSENT"), false, true, false, ESValue(0));

    fnXhrElement->asESObject()->defineDataProperty(
        ESString::create("OPENED"), false, true, false, ESValue(1));

    fnXhrElement->asESObject()->defineDataProperty(
        ESString::create("HEADERS_RECEIVED"), false, true, false, ESValue(2));

    fnXhrElement->asESObject()->defineDataProperty(
        ESString::create("LOADING"), false, true, false, ESValue(3));

    fnXhrElement->asESObject()->defineDataProperty(
        ESString::create("DONE"), false, true, false, ESValue(4));

    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("UNSENT"), false, true, false, ESValue(0));
    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("OPENED"), false, true, false, ESValue(1));
    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("HEADERS_RECEIVED"), false, true, false, ESValue(2));
    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("LOADING"), false, true, false, ESValue(3));
    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("DONE"), false, true, false, ESValue(4));

    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("setRequestHeader"), false, false, false,
        ESFunctionObject::create(NULL, setRequestHeaderFunction,
                                 ESString::create("setRequestHeader"), 2,
                                 false));

    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("open"), false, false, false,
        ESFunctionObject::create(NULL, openFunction, ESString::create("open"),
                                 1, false));

    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("send"), false, false, false,
        ESFunctionObject::create(NULL, sendFunction, ESString::create("send"),
                                 1, false));

    fnXhrElement->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("abort"), false, false, false,
        ESFunctionObject::create(NULL, abortFunction, ESString::create("abort"),
                                 1, false));

    return fnXhrElement;
}
}
