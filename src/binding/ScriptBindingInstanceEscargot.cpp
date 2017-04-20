/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "binding/Binding.h"
#include "platform/window/Window.h"
#include "platform/message_loop/MessageLoop.h"

#include <Escargot.h>
#include <vm/ESVMInstance.h>
#ifdef USE_ES6_FEATURE
#include <runtime/JobQueue.h>
#endif

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"
#endif

namespace StarFish {

using namespace escargot;

#ifdef USE_ES6_FEATURE
class PromiseJobQueue : public JobQueue {
private:
    PromiseJobQueue()
    {
    }

public:
    static JobQueue* create()
    {
        return new PromiseJobQueue();
    }

    virtual size_t enqueueJob(Job* job)
    {
        StarFish* sf = ((Window*)ESVMInstance::currentInstance()
                            ->globalObject()
                            ->extraPointerData())
                           ->starFish();
        m_pendingJobs.push_back(sf->messageLoop()->addIdler(
            [](size_t id, void* data, void* data2) {
                Job* j = (Job*)data;
                PromiseJobQueue* q = (PromiseJobQueue*)data2;
                j->run(ESVMInstance::currentInstance());
                q->m_pendingJobs.erase(std::find(q->m_pendingJobs.begin(),
                                                 q->m_pendingJobs.end(), id));
            },
            job, this));
        return 0;
    }

    void clearPendingJobs()
    {
        StarFish* sf = ((Window*)ESVMInstance::currentInstance()
                            ->globalObject()
                            ->extraPointerData())
                           ->starFish();
        for (size_t i = 0; i < m_pendingJobs.size(); i++) {
            sf->messageLoop()->removeIdler(m_pendingJobs[i]);
        }
    }

private:
    GCVector<size_t> m_pendingJobs;
};
#endif

ScriptBindingInstance::ScriptBindingInstance()
{
    m_enterCount = 0;
    m_data = new ScriptBindingInstanceDataEscargot(this);
    fetchData(this)->m_instance = new ESVMInstance();
#ifdef USE_ES6_FEATURE
    m_promiseJobQueue = fetchData(this)->m_instance->jobQueue();
#endif
}

ScriptBindingInstance::~ScriptBindingInstance()
{
    delete fetchData(this)->m_instance;
}

void ScriptBindingInstance::enter()
{
    if (m_enterCount == 0) {
        STARFISH_RELEASE_ASSERT(ESVMInstance::currentInstance() == nullptr);
        fetchData(this)->m_instance->enter();
    }
    m_enterCount++;
}

void ScriptBindingInstance::exit()
{
    if (m_enterCount == 1) {
        STARFISH_RELEASE_ASSERT(ESVMInstance::currentInstance() != nullptr);
        fetchData(this)->m_instance->exit();
    }
    m_enterCount--;
}

void ScriptBindingInstance::close()
{
#ifdef TIZEN_DEVICE_API
    DeviceAPI::close(fetchData(this)->m_instance);
#endif
#ifdef USE_ES6_FEATURE
    ((PromiseJobQueue*)m_promiseJobQueue)->clearPendingJobs();
#endif
}

#if defined(STARFISH_ENABLE_TEST)
static ESValue wptTextEndFunction(ESVMInstance* instance)
{
    const char* hide = getenv("HIDE_WINDOW");
    if ((hide && strlen(hide))) {
        ::exit(0);
    }
    return ESValue();
}
#endif

static ESValue logFunction(ESVMInstance* instance)
{
    Window* wnd = (Window*)instance->globalObject()->extraPointerData();
    wnd->starFish()->console()->log(toBrowserString(
        instance->currentExecutionContext()->readArgument(0).toString()));
    return ESValue();
}

static ESValue infoFunction(ESVMInstance* instance)
{
    Window* wnd = (Window*)instance->globalObject()->extraPointerData();
    wnd->starFish()->console()->info(toBrowserString(
        instance->currentExecutionContext()->readArgument(0).toString()));
    return ESValue();
}

static ESValue errorFunction(ESVMInstance* instance)
{
    Window* wnd = (Window*)instance->globalObject()->extraPointerData();
    wnd->starFish()->console()->error(toBrowserString(
        instance->currentExecutionContext()->readArgument(0).toString()));
    return ESValue();
}

static ESValue warnFunction(ESVMInstance* instance)
{
    Window* wnd = (Window*)instance->globalObject()->extraPointerData();
    wnd->starFish()->console()->warn(toBrowserString(
        instance->currentExecutionContext()->readArgument(0).toString()));
    return ESValue();
}

#ifdef STARFISH_ENABLE_MULTI_PAGE
static ESValue locationGetterFunction(ESVMInstance* instance)
{
    return (((Window*)ESVMInstance::currentInstance()
                 ->globalObject()
                 ->extraPointerData()))
        ->location()
        ->scriptObject();
}

static ESValue locationSetterFunction(ESVMInstance* instance)
{
    Window* wnd = (Window*)instance->globalObject()->extraPointerData();
    wnd->location()->setHref(
        String::fromUTF8(instance->currentExecutionContext()
                             ->readArgument(0)
                             .toString()
                             ->utf8Data()));
    return ESValue();
}
#endif

static ESValue toStringFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    if (thisValue.isESPointer() && thisValue.asESPointer()->isESObject()) {
        ESObject* obj = thisValue.asESPointer()->asESObject();
        if (obj->extraData() == kEscargotObjectCheckMagic) {
            ESValue constructor = thisValue.asESPointer()->asESObject()->get(
                ESString::create("constructor"));
            if (constructor.isObject()) {
                ESValue constructor_name =
                    constructor.asESPointer()->asESObject()->get(
                        ESString::create("name"));
                String* result = String::createASCIIString("[object ");
                result = result->concat(
                    toBrowserString(constructor_name.toString()));
                result = result->concat(String::createASCIIString("]"));
                return toJSString(result);
            }
        }
    }
    Window* wnd = (Window*)instance->globalObject()->extraPointerData();
    ESFunctionObject* function =
        fetchData(wnd->document()->scriptBindingInstance())->m_orgToString;
    return callScriptFunction(
        function, instance->currentExecutionContext()->arguments(),
        instance->currentExecutionContext()->argumentCount(), thisValue);
}

static ESValue windowGetterFunction(ESVMInstance* instance)
{
    return ESVMInstance::currentInstance()->globalObject();
}

static ESValue documentGetterFunction(ESVMInstance* instance)
{
#ifdef STARFISH_TC_COVERAGE
    STARFISH_LOG_INFO("&&&document\n");
#endif
    return (((Window*)ESVMInstance::currentInstance()
                 ->globalObject()
                 ->extraPointerData()))
        ->document()
        ->scriptObject();
}

static ESValue historyGetterFunction(ESVMInstance* instance)
{
    return (((Window*)ESVMInstance::currentInstance()
                 ->globalObject()
                 ->extraPointerData()))
        ->history()
        ->scriptObject();
}

static ESValue navigatorGetterFunction(ESVMInstance* instance)
{
    return (((Window*)ESVMInstance::currentInstance()
                 ->globalObject()
                 ->extraPointerData()))
        ->navigator()
        ->scriptObject();
}

#define DECLARE_FUNC_FOR_BINDING(exportName)                          \
    static ESValue exportName##GetterFunction(                        \
        ESObject* obj, ESObject* originalObj, ESString* propertyName) \
    {                                                                 \
        return fetchData((((Window*)ESVMInstance::currentInstance()   \
                               ->globalObject()                       \
                               ->extraPointerData()))                 \
                             ->document()                             \
                             ->scriptBindingInstance())               \
            ->value##exportName();                                    \
    }                                                                 \
                                                                      \
    static void exportName##SetterFunction(                           \
        ESObject* obj, ESObject* originalObj, ESString* propertyName, \
        const ESValue& value)                                         \
    {                                                                 \
        fetchData((((Window*)ESVMInstance::currentInstance()          \
                        ->globalObject()                              \
                        ->extraPointerData()))                        \
                      ->document()                                    \
                      ->scriptBindingInstance())                      \
            ->fn##exportName();                                       \
        Window* wnd = (Window*)ESVMInstance::currentInstance()        \
                          ->globalObject()                            \
                          ->extraPointerData();                       \
        fetchData(wnd->document()->scriptBindingInstance())           \
            ->m_value##exportName = value;                            \
    }
STARFISH_ENUM_LAZY_BINDING_NAMES(DECLARE_FUNC_FOR_BINDING)
#undef DECLARE_FUNC_FOR_BINDING

#ifdef STARFISH_EXP
static ESValue createHTMLDocumentFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMImplementation);
    DOMImplementation* impl = originalObj;
    if (impl) {
        Document* doc = impl->createHTMLDocument();
        if (doc) {
            return doc->scriptValue();
        }
    }
    return ESValue(ESValue::ESNull);
}
#endif

void ScriptBindingInstance::initBinding(StarFish* sf)
{
    fetchData(this)->m_instance->setlocale(sf->locale());
    fetchData(this)->m_instance->setTimezoneID(icu::UnicodeString::fromUTF8(
        icu::StringPiece(sf->timezoneID()->utf8Data())));

    // binding names first
    ESObject* globalObject = fetchData(this)->m_instance->globalObject();
#define DECLARE_NAME_FOR_BINDING(exportName)                       \
    globalObject->defineAccessorProperty(                          \
        ESString::create(#exportName), exportName##GetterFunction, \
        exportName##SetterFunction, true, false, true);
    STARFISH_ENUM_LAZY_BINDING_NAMES(DECLARE_NAME_FOR_BINDING)
#undef DECLARE_NAME_FOR_BINDING

    fetchData(this)->valueEventTarget();
    fetchData(this)->valueWindow();

#if defined(STARFISH_ENABLE_TEST)
    fetchData(this)->m_instance->globalObject()->defineDataProperty(
        ESString::create("wptTestEnd"), false, false, false,
        ESFunctionObject::create(nullptr, wptTextEndFunction,
                                 ESString::create("wptTestEnd"), 1, false));
#endif

    ESObject* console = ESObject::create();
    console->set(ESString::create("log"),
                 ESFunctionObject::create(nullptr, logFunction,
                                          ESString::create("log"), 1, false));
    console->set(ESString::create("info"),
                 ESFunctionObject::create(nullptr, infoFunction,
                                          ESString::create("info"), 1, false));
    console->set(ESString::create("error"),
                 ESFunctionObject::create(nullptr, errorFunction,
                                          ESString::create("error"), 1, false));
    console->set(ESString::create("warn"),
                 ESFunctionObject::create(nullptr, warnFunction,
                                          ESString::create("warn"), 1, false));
    fetchData(this)->m_instance->globalObject()->defineDataProperty(
        ESString::create("console"), false, false, false, console);

#ifdef STARFISH_ENABLE_MULTI_PAGE
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(),
        ESString::create("location"), locationGetterFunction,
        locationSetterFunction, true, false);
#endif

    fetchData(this)->m_orgToString =
        fetchData(this)
            ->m_instance->globalObject()
            ->objectPrototype()
            ->getOwnProperty(ESString::create("toString"))
            .asESPointer()
            ->asESFunctionObject();
    auto fnToString = ESFunctionObject::create(
        nullptr, toStringFunction, ESString::create("toString"), 0, false);
    fetchData(this)
        ->m_instance->globalObject()
        ->objectPrototype()
        ->defineDataProperty(ESString::create("toString"), true, false, true,
                             fnToString);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(), ESString::create("window"),
        windowGetterFunction, nullptr, true, false);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(),
        ESString::create("document"), documentGetterFunction, nullptr, true,
        false);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(),
        ESString::create("history"), historyGetterFunction, nullptr, true,
        true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(),
        ESString::create("navigator"), navigatorGetterFunction, nullptr, true,
        false);

/* 4.5.1 Interface DOMImplementation */
#ifdef STARFISH_EXP
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(
        DOMImplementation,
        fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_fnDOMImplementation = DOMImplementationFunction;

    DOMImplementationFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("createHTMLDocument"), false, false, false,
            ESFunctionObject::create(nullptr, createHTMLDocumentFunction,
                                     ESString::create("createHTMLDocument"), 2,
                                     false));
#endif

#ifdef TIZEN_DEVICE_API
    DeviceAPI::initialize(fetchData(this)->m_instance);
#endif
}

#define IMPL_EMPTY_BINDING(exportName, parentName)                           \
    ESFunctionObject* binding##exportName(                                   \
        ScriptBindingInstance* scriptBindingInstance)                        \
    {                                                                        \
        DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(                     \
            exportName, fetchData(scriptBindingInstance)->fn##parentName()); \
        return exportName##Function;                                         \
    }

// TODO PseudoElement may not be a binding target
IMPL_EMPTY_BINDING(PseudoElement, Element);
#undef IMPL_EMPTY_BINDING

String* ScriptBindingInstance::evaluate(String* str)
{
    std::jmp_buf tryPosition;
    if (setjmp(fetchData(this)->m_instance->registerTryPos(&tryPosition)) ==
        0) {
        auto result =
            fetchData(this)->m_instance->evaluate(toJSString(str).asESString());
        String* s = toBrowserString(result);
        fetchData(this)->m_instance->unregisterTryPos(&tryPosition);
        fetchData(this)->m_instance->unregisterCheckedObjectAll();
        return s;
    } else {
        ESValue err = fetchData(this)->m_instance->getCatchedError();
        ((Window*)fetchData(this)
             ->m_instance->globalObject()
             ->extraPointerData())
            ->starFish()
            ->console()
            ->error(toBrowserString(err));
    }
    return String::emptyString;
}
}
#ifdef USE_ES6_FEATURE
escargot::JobQueue* escargot::JobQueue::create()
{
    return StarFish::PromiseJobQueue::create();
}
#endif
