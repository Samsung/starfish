/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "ScriptWrappable.h"

#include "platform/window/Window.h"
#include "platform/message_loop/MessageLoop.h"
#include "dom/DOM.h"
#include "dom/NodeList.h"
#include "style/CSSStyleLookupTrie.h"
#include "extra/Console.h"
#include "extra/Navigator.h"
#include "extra/Location.h"
#include "platform/location/Geolocation.h"

#include <Escargot.h>
#include <vm/ESVMInstance.h>
#ifdef USE_ES6_FEATURE
#include <runtime/JobQueue.h>
#endif

#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"
#endif

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "extra/TimeRanges.h"
#include "dom/TextTrack.h"
#include "extra/MediaSource.h"
#include "extra/SourceBuffer.h"
#endif

namespace StarFish {

#ifdef USE_ES6_FEATURE
class PromiseJobQueue : public escargot::JobQueue {
private:
    PromiseJobQueue() { }
public:
    static escargot::JobQueue* create()
    {
        return new PromiseJobQueue();
    }

    virtual size_t enqueueJob(escargot::Job* job)
    {
        StarFish* sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
        m_pendingJobs.push_back(sf->messageLoop()->addIdler([](size_t id, void* data, void* data2) {
            escargot::Job* j = (escargot::Job*)data;
            PromiseJobQueue* q = (PromiseJobQueue*)data2;
            j->run(escargot::ESVMInstance::currentInstance());
            q->m_pendingJobs.erase(std::find(q->m_pendingJobs.begin(), q->m_pendingJobs.end(), id));
        }, job, this));
        return 0;
    }

    void clearPendingJobs()
    {
        StarFish* sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
        for (size_t i = 0; i < m_pendingJobs.size(); i++) {
            sf->messageLoop()->removeIdler(m_pendingJobs[i]);
        }
    }
private:
    std::vector<size_t, gc_allocator<size_t>> m_pendingJobs;
};
#endif


static ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance)
{
    return (ScriptBindingInstanceDataEscargot*)instance->data();
}

ScriptBindingInstance::ScriptBindingInstance()
{
    m_enterCount = 0;
    m_data = new ScriptBindingInstanceDataEscargot(this);
    fetchData(this)->m_instance = new escargot::ESVMInstance();
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
        STARFISH_RELEASE_ASSERT(escargot::ESVMInstance::currentInstance() == nullptr);
        fetchData(this)->m_instance->enter();
    }
    m_enterCount++;
}

void ScriptBindingInstance::exit()
{
    if (m_enterCount == 1) {
        STARFISH_RELEASE_ASSERT(escargot::ESVMInstance::currentInstance() != nullptr);
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

#define INVALID_INDEX (escargot::ESValue::ESInvalidIndexValue)
#define TO_INDEX_UINT32(argValue, idx) \
    uint32_t idx; \
    idx = argValue.toIndex(); \
    if (idx == INVALID_INDEX) { \
        double __number = argValue.toNumber(); \
        idx = __number < 0 ? INVALID_INDEX : (std::isnan(__number) ? 0 : (uint32_t)__number); \
    }

#define DEFINE_FUNCTION(functionName, parentName)                                                                                                                                                                          \
    escargot::ESString* functionName##String = escargot::ESString::create(#functionName);                                                                                                                                  \
    escargot::ESFunctionObject* functionName##Function = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance*) -> escargot::ESValue      \
        {      \
            return escargot::ESValue();           \
        }, functionName##String, 0, true, true); \
    functionName##Function->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false); \
    functionName##Function->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);                                                                                                                     \
    functionName##Function->protoType().asESPointer()->asESObject()->set__proto__(parentName);

// fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(functionName##String, true, false, true, functionName##Function);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentName)                                                                                                                                                                          \
    escargot::ESString* functionName##String = escargot::ESString::create(#functionName);                                                                                                                                  \
    escargot::ESFunctionObject* functionName##Function = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance*) -> escargot::ESValue      \
        {      \
            escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Illegal constructor")))); \
            STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
            return escargot::ESValue();           \
        }, functionName##String, 0, true, true); \
    functionName##Function->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false); \
    functionName##Function->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);                                                                                                                     \
    functionName##Function->protoType().asESPointer()->asESObject()->set__proto__(parentName);

// fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(functionName##String, true, false, true, functionName##Function);

#define DEFINE_FUNCTION_WITH_PARENTFUNC(functionName, parentFunction) \
    DEFINE_FUNCTION(functionName, parentFunction->protoType()) \
    functionName##Function->set__proto__(parentFunction);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(functionName, parentFunction) \
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentFunction->protoType()) \
    functionName##Function->set__proto__(parentFunction);

// TypeError: Illegal invocation
#define THROW_ILLEGAL_INVOCATION() \
    { \
        escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Illegal invocation")))); \
        STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
    }

#define THROW_DOM_EXCEPTION(instance, errcode) \
    { \
        auto __sf = ((Window*)instance->globalObject()->extraPointerData()); \
        auto __err = new DOMException(__sf->scriptBindingInstance(), errcode, nullptr); \
        escargot::ESVMInstance::currentInstance()->throwError(__err->scriptValue()); \
    }

#define CHECK_TYPEOF(thisValue, btype)                                                 \
    {                                                                                 \
        escargot::ESValue v = thisValue;                                              \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() == kEscargotObjectCheckMagic) && (((ScriptWrappable*)v.asESPointer()->asESObject()->extraPointerData())->type() & btype))) { \
            THROW_ILLEGAL_INVOCATION()                                                \
        }                                                                             \
    }

#define CHECK_TYPEOF_WITH_ERRCODE(thisValue, btype, instance, errcode) \
    { \
        escargot::ESValue v = thisValue; \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() == kEscargotObjectCheckMagic) && (((ScriptWrappable*)v.asESPointer()->asESObject()->extraPointerData())->type() & btype))) { \
            THROW_DOM_EXCEPTION(instance, errcode); \
        } \
    }

#define GENERATE_THIS_AND_CHECK_TYPE(btype, destType)                                                \
    escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();        \
    {                                                                                               \
        escargot::ESValue v = thisValue;                                                            \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() == kEscargotObjectCheckMagic) && (((ScriptWrappable*)v.asESPointer()->asESObject()->extraPointerData())->type() & btype))) { \
            THROW_ILLEGAL_INVOCATION()                                                              \
        }                                                                                           \
    }                                                                                               \
    destType* originalObj = (destType*)(thisValue.asESPointer()->asESObject()->extraPointerData()); \
    escargot::ESValue v = instance->currentExecutionContext()->readArgument(0);                     \

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(escargot::ESObject* obj, escargot::ESString* propertyName,
    escargot::NativeFunctionType getter, escargot::NativeFunctionType setter, bool isEnumerable = true, bool isConfigurable = true)
{
    bool isWritable = setter;

    escargot::ESPropertyAccessorData* accData = new escargot::ESPropertyAccessorData();
    accData->setJSGetter(escargot::ESFunctionObject::create(nullptr, getter, escargot::ESVMInstance::currentInstance()->strings().emptyString, 0, false, false));
    if (setter) {
        accData->setJSSetter(escargot::ESFunctionObject::create(nullptr, setter, escargot::ESVMInstance::currentInstance()->strings().emptyString, 1, false, false));
    }
    obj->defineAccessorProperty(propertyName, accData, isWritable, isEnumerable, isConfigurable);
}


String* toBrowserString(const escargot::ESValue& v)
{
    escargot::NullableUTF8String s = v.toString()->toNullableUTF8String();
    String* newStr = String::fromUTF8(s.m_buffer, s.m_bufferSize);
    // NOTE: input string contains whitecharacters as is, i.e., "\n" is stored as '\','n'
    // The right way is, input string should already have '\n', and white spaces should be removed from here.
    // For time being, we simply remove "\n" and other whitespaces strings.
    // newStr = newStr->replaceAll(String::fromUTF8("\n"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\t"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\f"), String::spaceString);
    // newStr = newStr->replaceAll(String::fromUTF8("\r"), String::spaceString);
    return newStr;
}

escargot::ESValue toJSString(String* v)
{
    return createScriptString(v);
}

void ScriptBindingInstance::initBinding(StarFish* sf)
{
    fetchData(this)->m_instance->setlocale(sf->locale());
    fetchData(this)->m_instance->setTimezoneID(icu::UnicodeString::fromUTF8(icu::StringPiece(sf->timezoneID()->utf8Data())));

    escargot::ESValue v;

#if defined(STARFISH_ENABLE_TEST)
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("wptTestEnd"), false, false, false, escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        const char* hide = getenv("HIDE_WINDOW");
        if ((hide && strlen(hide)))
            ::exit(0);
        return escargot::ESValue();
    }, escargot::ESString::create("wptTestEnd"), 1, false));
#endif

    escargot::ESObject* console = escargot::ESObject::create();
    console->set(escargot::ESString::create("log"), escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        Window* wnd = (Window*)instance->globalObject()->extraPointerData();
        wnd->starFish()->console()->log(toBrowserString(instance->currentExecutionContext()->readArgument(0).toString()));
        return escargot::ESValue();
    }, escargot::ESString::create("log"), 1, false));
    console->set(escargot::ESString::create("error"), escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        Window* wnd = (Window*)instance->globalObject()->extraPointerData();
        wnd->starFish()->console()->error(toBrowserString(instance->currentExecutionContext()->readArgument(0).toString()));
        return escargot::ESValue();
    }, escargot::ESString::create("error"), 1, false));
    console->set(escargot::ESString::create("warn"), escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        Window* wnd = (Window*)instance->globalObject()->extraPointerData();
        wnd->starFish()->console()->warn(toBrowserString(instance->currentExecutionContext()->readArgument(0).toString()));
        return escargot::ESValue();
    }, escargot::ESString::create("warn"), 1, false));
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("console"), false, false, false, console);

#ifdef STARFISH_ENABLE_MULTI_PAGE
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(), escargot::ESString::create("location"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        return (((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->location()->scriptObject();
    }, nullptr, true, false);
#endif

    escargot::ESFunctionObject* toStringFunction = escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isESPointer() && thisValue.asESPointer()->isESObject()) {
            escargot::ESObject* obj = thisValue.asESPointer()->asESObject();
            if (obj->extraData() == kEscargotObjectCheckMagic) {
                escargot::ESValue constructor = thisValue.asESPointer()->asESObject()->get(escargot::ESString::create("constructor"));
                if (constructor.isObject()) {
                    escargot::ESValue constructor_name = constructor.asESPointer()->asESObject()->get(escargot::ESString::create("name"));
                    String* result = String::createASCIIString("[object ");
                    result = result->concat(toBrowserString(constructor_name.toString()));
                    result = result->concat(String::createASCIIString("]"));
                    return toJSString(result);
                }
            }
        }
        Window* wnd = (Window*)instance->globalObject()->extraPointerData();
        escargot::ESFunctionObject* function = fetchData(wnd->document()->scriptBindingInstance())->m_orgToString;
        return callScriptFunction(function, instance->currentExecutionContext()->arguments(), instance->currentExecutionContext()->argumentCount(), thisValue);

        }, escargot::ESString::create("toString"), 0, false);
    fetchData(this)->m_orgToString = fetchData(this)->m_instance->globalObject()->objectPrototype()->getOwnProperty(escargot::ESString::create("toString")).asESPointer()->asESFunctionObject();
    fetchData(this)->m_instance->globalObject()->objectPrototype()->defineDataProperty(escargot::ESString::create("toString"), true, false, true, toStringFunction);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(EventTarget, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_eventTarget = EventTargetFunction;
    fetchData(this)->m_instance->globalObject()->defineDataProperty(EventTargetString, true, false, true, EventTargetFunction);

    auto fnAddEventListener = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventTargetObject);
        if (instance->currentExecutionContext()->argumentCount() < 2) {
            auto msg = escargot::ESString::create("Failed to execute 'addEventListener' on 'EventTaraget': needs 2 parameter.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
        }
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
        escargot::ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
        if (firstArg.isESString() && secondArg.isESPointer() && secondArg.asESPointer()->isESFunctionObject()) {
            escargot::ESString* argStr = firstArg.asESString();
            auto eventTypeName = String::fromUTF8(argStr->utf8Data());
            auto listener = new EventListener(secondArg);
            bool capture = thirdArg.isBoolean() ? thirdArg.toBoolean() : false;
            ((EventTarget *)thisValue.asESPointer()->asESObject()->extraPointerData())->addEventListener(eventTypeName, listener, capture);
#ifdef STARFISH_TC_COVERAGE
            STARFISH_LOG_INFO("&&&%s\n", eventTypeName->utf8Data());
#endif
        }
        return escargot::ESValue();
    }, escargot::ESString::create("addEventListener"), 0, false);
    fnAddEventListener->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("addEventListener"), true, true, true, fnAddEventListener);

    auto fnRemoveEventListener = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventTargetObject);
        if (instance->currentExecutionContext()->argumentCount() < 2) {
            auto msg = escargot::ESString::create("Failed to execute 'removeEventListener' on 'EventTaraget': needs 2 parameter.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
        }
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
        escargot::ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
        if (firstArg.isESString() && secondArg.isESPointer() && secondArg.asESPointer()->isESFunctionObject()) {
            // TODO: Verify valid event type. (e.g. click)
            escargot::ESString* argStr = firstArg.asESString();
            auto eventTypeName = String::fromUTF8(argStr->utf8Data());
            auto listener = new EventListener(secondArg);
            bool capture = thirdArg.isBoolean() ? thirdArg.toBoolean() : false;
            ((EventTarget *)thisValue.asESPointer()->asESObject()->extraPointerData())->removeEventListener(eventTypeName, listener, capture);
        }
        return escargot::ESValue();
    }, escargot::ESString::create("removeEventListener"), 0, false);
    fnRemoveEventListener->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeEventListener"), true, true, true, fnRemoveEventListener);

    auto fnDispatchEvent = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventTargetObject);
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        if (firstArg.isUndefinedOrNull()) {
            escargot::ESString* msg = escargot::ESString::create("Failed to execute 'dispatchEvent' on 'EventTarget': parameter 1 is not of type 'Event'.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        bool ret = false;
        if (argCount == 1 && firstArg.isObject()) {
            if (!(firstArg.isObject() && (firstArg.asESPointer()->asESObject()->extraData() == kEscargotObjectCheckMagic) && ((ScriptWrappable*)firstArg.asESPointer()->asESObject()->extraPointerData())->type() == ScriptWrappable::Type::EventObject)) {
                auto msg = escargot::ESString::create("Failed to execute 'dispatchEvent' on 'EventTarget': parameter 1 is not of type 'Event'.");
                instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            }
            Event* event = (Event*)firstArg.asESPointer()->asESObject()->extraPointerData();
            ret = ((EventTarget*)thisValue.asESPointer()->asESObject()->extraPointerData())->dispatchEvent(event);
        }
        return escargot::ESValue(ret);
    }, escargot::ESString::create("dispatchEvent"), 1, false);
    fnDispatchEvent->codeBlock()->m_forceDenyStrictMode = true;
    EventTargetFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("dispatchEvent"), true, true, true, fnDispatchEvent);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(Window, EventTargetFunction);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(), escargot::ESString::create("window"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        return escargot::ESVMInstance::currentInstance()->globalObject();
    }, nullptr, true, false);
    fetchData(this)->m_instance->globalObject()->set__proto__(WindowFunction->protoType());
    fetchData(this)->m_instance->globalObject()->defineDataProperty(WindowString, true, false, true, WindowFunction);
    fetchData(this)->m_window = WindowFunction;

#ifdef STARFISH_ENABLE_TEST
    WindowFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getComputedStyle"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
                CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
                // Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
                Node* node = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
                escargot::ESValue pseudoElm = instance->currentExecutionContext()->readArgument(1);

                if (node->isNode() && pseudoElm.isNull()) {
                    CSSStyleDeclaration* s = node->asNode()->getComputedStyle();
                    if (s) {
                        return s->scriptValue();
                    }
                }
                return escargot::ESValue(escargot::ESValue::ESNull);
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("getComputedStyle"), 2, false));
#endif

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(), escargot::ESString::create("document"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("&&&document\n");
#endif
        return (((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document()->scriptObject();
    }, nullptr, true, false);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(), escargot::ESString::create("navigator"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        return (((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->navigator()->scriptObject();
    }, nullptr, true, false);

    // binding names first
    escargot::ESObject* globalObject = fetchData(this)->m_instance->globalObject();
#define DECLARE_NAME_FOR_BINDING(codeName, exportName) \
    globalObject->defineAccessorProperty(escargot::ESString::create(#exportName), [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue \
        { \
            return fetchData((((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document()->scriptBindingInstance())->codeName##Value(); \
        }, [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& value) \
        { \
            fetchData((((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document()->scriptBindingInstance())->codeName(); \
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData(); \
            fetchData(wnd->document()->scriptBindingInstance())->m_value##codeName = value; \
        }, true, false, true);
    STARFISH_ENUM_LAZY_BINDING_NAMES(DECLARE_NAME_FOR_BINDING)

    /* 4.5.1 Interface DOMImplementation */
#ifdef STARFISH_EXP
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMImplementation, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_domImplementation = DOMImplementationFunction;

    DOMImplementationFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("createHTMLDocument"), false, false, false,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMImplementationObject, DOMImplementation);
            DOMImplementation* impl = originalObj;
            if (impl) {
                Document* doc = impl->createHTMLDocument();
                if (doc) {
                    return doc->scriptValue();
                }
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        },
    escargot::ESString::create("createHTMLDocument"), 2, false));
#endif

#ifdef TIZEN_DEVICE_API
    DeviceAPI::initialize(fetchData(this)->m_instance);
#endif

}

#define IMPL_EMPTY_BINDING(codeName, exportName, fromCodeName) \
escargot::ESFunctionObject* binding##exportName(ScriptBindingInstance* scriptBindingInstance) \
{ \
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(exportName, fetchData(scriptBindingInstance)->fromCodeName()); \
    return exportName##Function; \
}

IMPL_EMPTY_BINDING(htmlDocument, HTMLDocument, document);
IMPL_EMPTY_BINDING(htmlHtmlElement, HTMLHtmlElement, htmlElement);
IMPL_EMPTY_BINDING(htmlHeadElement, HTMLHeadElement, htmlElement);
IMPL_EMPTY_BINDING(htmlDivElement, HTMLDivElement, htmlElement);
IMPL_EMPTY_BINDING(htmlParagraphElement, HTMLMetaElement, htmlElement);
IMPL_EMPTY_BINDING(htmlParagraphElement, HTMLParagraphElement, htmlElement);
IMPL_EMPTY_BINDING(htmlSpanElement, HTMLSpanElement, htmlElement);
IMPL_EMPTY_BINDING(htmlBrElement, HTMLBRElement, htmlElement);
IMPL_EMPTY_BINDING(htmlUnknownElement, HTMLUnknownElement, htmlElement);

escargot::ESFunctionObject* bindingNode(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(Node, fetchData(scriptBindingInstance)->m_eventTarget);

    /* 4.4 Interface Node */
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("ELEMENT_NODE"), false, true, false, escargot::ESValue(Node::ELEMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("ATTRIBUTE_NODE"), false, true, false, escargot::ESValue(Node::ATTRIBUTE_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("TEXT_NODE"), false, true, false, escargot::ESValue(Node::TEXT_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("CDATA_SECTION_NODE"), false, true, false, escargot::ESValue(Node::CDATA_SECTION_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("ENTITY_REFERENCE_NODE"), false, true, false, escargot::ESValue(Node::ENTITY_REFERENCE_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("ENTITY_NODE"), false, true, false, escargot::ESValue(Node::ENTITY_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("PROCESSING_INSTRUCTION_NODE"), false, true, false, escargot::ESValue(Node::PROCESSING_INSTRUCTION_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("COMMENT_NODE"), false, true, false, escargot::ESValue(Node::COMMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("DOCUMENT_NODE"), false, true, false, escargot::ESValue(Node::DOCUMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("DOCUMENT_TYPE_NODE"), false, true, false, escargot::ESValue(Node::DOCUMENT_TYPE_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("DOCUMENT_FRAGMENT_NODE"), false, true, false, escargot::ESValue(Node::DOCUMENT_FRAGMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(escargot::ESString::create("NOTATION_NODE"), false, true, false, escargot::ESValue(Node::NOTATION_NODE));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("ELEMENT_NODE"), false, true, false, escargot::ESValue(Node::ELEMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("ATTRIBUTE_NODE"), false, true, false, escargot::ESValue(Node::ATTRIBUTE_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("TEXT_NODE"), false, true, false, escargot::ESValue(Node::TEXT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("CDATA_SECTION_NODE"), false, true, false, escargot::ESValue(Node::CDATA_SECTION_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("ENTITY_REFERENCE_NODE"), false, true, false, escargot::ESValue(Node::ENTITY_REFERENCE_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("ENTITY_NODE"), false, true, false, escargot::ESValue(Node::ENTITY_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("PROCESSING_INSTRUCTION_NODE"), false, true, false, escargot::ESValue(Node::PROCESSING_INSTRUCTION_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("COMMENT_NODE"), false, true, false, escargot::ESValue(Node::COMMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("DOCUMENT_NODE"), false, true, false, escargot::ESValue(Node::DOCUMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("DOCUMENT_TYPE_NODE"), false, true, false, escargot::ESValue(Node::DOCUMENT_TYPE_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("DOCUMENT_FRAGMENT_NODE"), false, true, false, escargot::ESValue(Node::DOCUMENT_FRAGMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("NOTATION_NODE"), false, true, false, escargot::ESValue(Node::NOTATION_NODE));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nodeType"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        unsigned short nodeType = originalObj->nodeType();
        return escargot::ESValue(nodeType);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nodeName"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        String* nodeName = originalObj->nodeName();
        return toJSString(nodeName);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("ownerDocument"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Document* doc = originalObj->ownerDocument();
        if (doc == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return doc->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("parentNode"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* p = originalObj->parentNode();
        if (p == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return p->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("parentElement"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Element* p = originalObj->parentElement();
        if (p == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return p->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("childNodes"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        NodeList* list = originalObj->childNodes();
        STARFISH_ASSERT(list);
        return list->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nextSibling"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj->nextSibling();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("previousSibling"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj->previousSibling();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("firstChild"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj->firstChild();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("lastChild"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj->lastChild();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nodeValue"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (!nd->isCharacterData())
            return escargot::ESValue(escargot::ESValue::ESNull);
        String* s = nd->nodeValue();
        return toJSString(s);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* node = originalObj;
        if (v.isESString()) {
            node->setNodeValue(toBrowserString(v));
        } else if (v.isNull()) {
            node->setNodeValue(nullptr);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("textContent"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocumentType() || nd->isDocument()) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return toJSString(nd->textContent());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        String* arg_str = toBrowserString(v);
        if (v.isUndefinedOrNull() || !arg_str->length()) {
            originalObj->setTextContent(String::emptyString);
        } else
            originalObj->setTextContent(arg_str);
        return escargot::ESValue();
    });

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("cloneNode"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, (ScriptWrappable::Type::NodeObject));
            Node* obj = (Node*) thisValue.asESPointer()->asESObject()->extraPointerData();
            escargot::ESValue arg = instance->currentExecutionContext()->readArgument(0);
            bool deepClone = false;
            if (arg.isBoolean()) {
                deepClone = arg.asBoolean();
            }
            Node* node = obj->cloneNode(deepClone);
            return node->scriptValue();
        }, escargot::ESString::create("cloneNode"), 0, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("hasChildNodes"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            bool result = obj->hasChildNodes();
            return escargot::ESValue(result);
        }, escargot::ESString::create("hasChildNodes"), 0, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("isEqualNode"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);

            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);

            if (argValue.isUndefinedOrNull())
                return escargot::ESValue(false);

            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            Node* node = (Node*)argValue.asESPointer()->asESObject()->extraPointerData();
            bool found = obj->isEqualNode(node);
            return escargot::ESValue(found);
        }, escargot::ESString::create("isEqualNode"), 1, false));

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_DISCONNECTED"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_DISCONNECTED);
        },
        NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_PRECEDING"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_PRECEDING);
        },
        NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_FOLLOWING"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_FOLLOWING);
        },
        NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_CONTAINS"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_CONTAINS);
        },
        NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_CONTAINED_BY"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_CONTAINED_BY);
        },
        NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_DISCONNECTED"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_DISCONNECTED);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_PRECEDING"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_PRECEDING);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_FOLLOWING"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_FOLLOWING);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_CONTAINS"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_CONTAINS);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_CONTAINED_BY"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_CONTAINED_BY);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("compareDocumentPosition"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            Node* nodeRef = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
            unsigned short pos = obj->compareDocumentPosition(nodeRef);
            return escargot::ESValue(pos);
        }, escargot::ESString::create("compareDocumentPosition"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("contains"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            if (instance->currentExecutionContext()->readArgument(0).isNull())
                return escargot::ESValue(obj->contains(nullptr));
            CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
            Node* nodeRef = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
            bool found = obj->contains(nodeRef);
            return escargot::ESValue(found);
        }, escargot::ESString::create("contains"), 1, false));

    escargot::ESFunctionObject* appendChildFunction = escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF_WITH_ERRCODE(thisValue, ScriptWrappable::Type::NodeObject, instance, DOMException::HIERARCHY_REQUEST_ERR);
            CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            Node* child = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
            obj->appendChild(child);
            return child->scriptValue();
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("appendChild"), 1, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("appendChild"), true, true, true, appendChildFunction);

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeChild"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF_WITH_ERRCODE(thisValue, ScriptWrappable::Type::NodeObject, instance, DOMException::NOT_FOUND_ERR);
                CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
                Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
                Node* child = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
                Node* n = obj->removeChild(child);
                return n->scriptValue();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("removeChild"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("replaceChild"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
                CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
                CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(1), ScriptWrappable::Type::NodeObject);
                Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
                Node* node = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
                Node* child = (Node*)instance->currentExecutionContext()->readArgument(1).asESPointer()->asESObject()->extraPointerData();
                Node* n = obj->replaceChild(node, child);
                return n->scriptValue();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("replaceChild"), 2, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("insertBefore"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
                Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

                CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
                Node* node = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();

                escargot::ESValue arg2 = instance->currentExecutionContext()->readArgument(1);
                Node* child = nullptr;
                if (!arg2.isNull()) {
                    CHECK_TYPEOF(arg2, ScriptWrappable::Type::NodeObject);
                    child = (Node*)arg2.asESPointer()->asESObject()->extraPointerData();
                }

                Node* n = obj->insertBefore(node, child);
                return n->scriptValue();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                return escargot::ESValue(escargot::ESValue::ESNull);
            }
        }, escargot::ESString::create("insertBefore"), 2, false));
    return NodeFunction;
}

escargot::ESFunctionObject* bindingElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(Element, fetchData(scriptBindingInstance)->node());

    /* 4.8 Interface Element */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("firstElementChild"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->firstElementChild() ? originalObj->firstElementChild()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("lastElementChild"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->lastElementChild() ? originalObj->lastElementChild()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nextElementSibling"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->nextElementSibling() ? originalObj->nextElementSibling()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("previousElementSibling"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->previousElementSibling() ? originalObj->previousElementSibling()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("childElementCount"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return escargot::ESValue(originalObj->childElementCount());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("clientLeft"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (originalObj->isElement()) {
            return escargot::ESValue(originalObj->asElement()->clientLeft());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("clientTop"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (originalObj->isElement()) {
            return escargot::ESValue(originalObj->asElement()->clientTop());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("clientWidth"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (originalObj->isElement()) {
            return escargot::ESValue(originalObj->asElement()->clientWidth());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("clientHeight"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (originalObj->isElement()) {
            return escargot::ESValue(originalObj->asElement()->clientHeight());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("localName"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            return toJSString(nd->asElement()->localName());
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("tagName"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            return toJSString(nd->asElement()->localName()->toUpper());
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("namespaceURI"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            if (nd->asElement()->name().namespaceURIAtomic() == AtomicString::emptyAtomicString())
                return ScriptValueNull;
            return toJSString(nd->asElement()->name().namespaceURI());
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("id"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_id));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_id, toBrowserString(v));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("className"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_class));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_class, toBrowserString(v));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("children"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        HTMLCollection* nd = originalObj->children();
        return nd->scriptValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("classList"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        DOMTokenList* nd = originalObj->classList();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        return nd->scriptValue();
    }, nullptr);

#ifdef STARFISH_ENABLE_TEST
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("innerHTML"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            return toJSString(nd->asElement()->innerHTML());
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            nd->asElement()->setInnerHTML(toBrowserString(v));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });
#endif

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("attributes"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        NamedNodeMap* nd = originalObj->attributes();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        return nd->scriptValue();
    }, nullptr);

    escargot::ESFunctionObject* removeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        Node* p = obj->parentNode();
        if (p == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }
        obj->remove();
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("remove"), 0, false);
    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, removeFunction);

    escargot::ESFunctionObject* getAttributeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);

            auto sf = ((Window*)instance->globalObject()->extraPointerData())->starFish();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);

            if (argValue.isESString()) {
                String* keyStr = toBrowserString(argValue);
                Element* elem = ((Node*)thisValue.asESPointer()->asESObject()->extraPointerData())->asElement();
                size_t idx = elem->hasAttribute(elem->document()->createAttributeName(keyStr));
                if (idx == SIZE_MAX)
                    return escargot::ESValue(escargot::ESValue::ESNull);
                else
                    return toJSString(elem->getAttribute(idx));
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("getAttribute"), 0, false);
    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getAttribute"), false, false, false, getAttributeFunction);

    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("setAttribute"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue nd = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(nd, ScriptWrappable::Type::NodeObject);

                escargot::ESValue key = instance->currentExecutionContext()->readArgument(0);
                escargot::ESValue val = instance->currentExecutionContext()->readArgument(1);

                if (key.isESString()) {
                    auto sf = ((Window*)instance->globalObject()->extraPointerData())->starFish();
                    // Validate key string
                    String* keyStr = toBrowserString(key);
                    if (!QualifiedName::checkNameProductionRule(keyStr, keyStr->length()))
                        throw new DOMException(sf->window()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

                    String* attrVal = toBrowserString(val);
                    Element* elem = ((Node*)nd.asESPointer()->asESObject()->extraPointerData())->asElement();
                    elem->setAttribute(elem->document()->createAttributeName(keyStr), attrVal);
                }
                return escargot::ESValue(escargot::ESValue::ESNull);
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("setAttribute"), 0, false)
    );

    // spec of removeAttribute
    // https://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-6D6AC0F9
    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeAttribute"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue nd = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(nd, ScriptWrappable::Type::NodeObject);

                escargot::ESValue key = instance->currentExecutionContext()->readArgument(0);

                if (key.isESString()) {
                    auto sf = ((Window*)instance->globalObject()->extraPointerData())->starFish();
                    Element* elem = ((Node*)nd.asESPointer()->asESObject()->extraPointerData())->asElement();
                    String* keyStr = toBrowserString(key);
                    elem->removeAttribute(elem->document()->createAttributeName(keyStr));
                }
                return escargot::ESValue();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("removeAttribute"), 0, false)
    );

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("style"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (!originalObj->isElement()) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        CSSStyleDeclaration* s = originalObj->asElement()->inlineStyle();
        return s->scriptValue();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_style, toBrowserString(v));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getElementsByClassName"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

            if (obj->isElement()) {
                Element* elem = obj->asElement();
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
                if (argValue.isESString() || argValue.isUndefinedOrNull()) {
                    escargot::ESString* argStr;
                    if (argValue.isNull()) {
                        argStr = escargot::ESString::create("null");
                    } else if (argValue.isUndefined()) {
                        argStr = escargot::ESString::create("undefined");
                    } else {
                        argStr = argValue.asESString();
                    }
                    HTMLCollection* result = elem->getElementsByClassName(toBrowserString(argStr));
                    if (result != nullptr)
                        return result->scriptValue();
                } else if (argValue.isObject() && argValue.asESPointer()->isESArrayObject()) {
                    escargot::ESArrayObject* array = argValue.asESPointer()->asESArrayObject();
                    String* listSoFar = String::createASCIIString("");
                    for (unsigned i = 0; i < array->length(); i++) {
                        escargot::ESValue val = array->get(i);
                        if (val.isESString()) {
                            listSoFar = listSoFar->concat(toBrowserString(val.asESString()));
                            if (i < array->length()-1) {
                                listSoFar = listSoFar->concat(String::createASCIIString(","));
                            }
                        } else {
                            return escargot::ESValue(escargot::ESValue::ESNull);
                        }
                    }
                    HTMLCollection* result = elem->getElementsByClassName(listSoFar);
                    if (result) {
                        return result->scriptValue();
                    }
                }
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("Element&&&getElementsByClassName\n");
#endif
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, escargot::ESString::create("getElementsByClassName"), 1, false)
    );

    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getElementsByTagName"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

            if (obj->isElement()) {
                Element* elem = obj->asElement();
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
                if (argValue.isESString()) {
                    escargot::ESString* argStr = argValue.asESString();
                    QualifiedName name(elem->document()->window()->starFish()->staticStrings()->m_xhtmlNamespaceURI, AtomicString::createAttrAtomicString(elem->document()->window()->starFish(), toBrowserString(argStr)));
                    HTMLCollection* result = elem->getElementsByTagName(name);
                    if (result) {
                        return result->scriptValue();
                    }
                }
#ifdef STARFISH_TC_COVERAGE
                STARFISH_LOG_INFO("Element&&&getElementsByTagName\n");
#endif
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, escargot::ESString::create("getElementsByTagName"), 1, false)
    );

    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("hasAttribute"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);

            int count = instance->currentExecutionContext()->argumentCount();
            if (count == 1) {
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
                if (argValue.isESString()) {
                    auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
                    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(sf, argValue.asESString()->utf8Data()));
                    size_t res = ((Element*) thisValue.asESPointer()->asESObject()->extraPointerData())->hasAttribute(name);
                    return res != SIZE_MAX ? escargot::ESValue(true) : escargot::ESValue(false);
                } else {
                    return escargot::ESValue(false);
                }
            } else {
                auto msg = escargot::ESString::create("Failed to execute 'hasAttribute' on Element: 1 argument required, but only 0 present.");
                instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("hasAttribute"), 1, false)
    );

    return ElementFunction;
}

escargot::ESFunctionObject* bindingCharacterData(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(CharacterData, fetchData(scriptBindingInstance)->node());

    /* 4.9 Interface CharacterData */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("data"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isCharacterData()) {
            return toJSString(nd->asCharacterData()->data());
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isCharacterData()) {
            if (v.isNull()) {
                nd->asCharacterData()->setData(toBrowserString(::escargot::ESString::create("")));
                return escargot::ESValue();
            } else if (v.isUndefined()) {
                nd->asCharacterData()->setData(toBrowserString(::escargot::ESString::create("undefined")));
                return escargot::ESValue();
            }
            nd->asCharacterData()->setData(toBrowserString(v));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isCharacterData()) {
            if (nd->asCharacterData()->data()->isASCIIString())
                return escargot::ESValue(nd->asCharacterData()->length());
            else {
                // TODO: measure length without converting
                return escargot::ESValue(toJSString(nd->asCharacterData()->data()).toString()->length());
            }
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nextElementSibling"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->nextElementSibling() ? originalObj->nextElementSibling()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CharacterDataFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("previousElementSibling"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->previousElementSibling() ? originalObj->previousElementSibling()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    escargot::ESFunctionObject* removeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        Node* p = obj->parentNode();
        if (p == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }
        obj->remove();
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("remove"), 0, false);
    CharacterDataFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, removeFunction);

    return CharacterDataFunction;
}

escargot::ESFunctionObject* bindingText(ScriptBindingInstance* scriptBindingInstance)
{
    /* 4.10 Interface Text */
    auto TextFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        if (argCount > 0) {
            escargot::ESString* data = firstArg.toString();
            Text* text = new Text((((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document(), String::fromUTF8(data->utf8Data()));
            return text->scriptValue();
        }
        return escargot::ESValue();
    }, escargot::ESString::create("Text"), 0, true, true);
    TextFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    TextFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    TextFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->characterData()->protoType());
    TextFunction->set__proto__(fetchData(scriptBindingInstance)->characterData());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("wholeText"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (!nd->isText()) {
            THROW_ILLEGAL_INVOCATION();
        }
        String* text = (nd->asText())->wholeText();
        return toJSString(text);
    }, nullptr);

    return TextFunction;
}

escargot::ESFunctionObject* bindingComment(ScriptBindingInstance* scriptBindingInstance)
{
    /* 4.10 Interface Comment */
    auto CommentFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        if (argCount > 0) {
            escargot::ESString* data = firstArg.toString();
            Comment* comment = new Comment((((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document(), String::fromUTF8(data->utf8Data()));
            return comment->scriptValue();
        }
        return escargot::ESValue();
    }, escargot::ESString::create("Comment"), 0, true, true);
    CommentFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    CommentFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);                                                                                                                     \
    CommentFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->characterData()->protoType()); \
    CommentFunction->set__proto__(fetchData(scriptBindingInstance)->characterData());

    return CommentFunction;
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
escargot::ESFunctionObject* bindingTextTrack(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(TextTrack, fetchData(scriptBindingInstance)->m_eventTarget);

    TextTrackFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("addCue"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::TextTrackObject);
            CHECK_TYPEOF(firstArg, ScriptWrappable::Type::TextTrackCueObject);

            TextTrack* textTrack = (TextTrack*)thisValue.asESPointer()->asESObject()->extraPointerData();
            TextTrackCue* cue = (TextTrackCue*)firstArg.asESPointer()->asESObject()->extraPointerData();
            textTrack->addCue(cue);
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("addCue"), 1, false)
    );

    TextTrackFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeCue"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::TextTrackObject);
            CHECK_TYPEOF(firstArg, ScriptWrappable::Type::TextTrackCueObject);

            TextTrack* textTrack = (TextTrack*)thisValue.asESPointer()->asESObject()->extraPointerData();
            TextTrackCue* cue = (TextTrackCue*)firstArg.asESPointer()->asESObject()->extraPointerData();
            textTrack->removeCue(cue);
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("removeCue"), 0, false)
    );

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("oncuechange"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_cuechange;
        return originalObj->attributeEventListener(eventname);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_cuechange;
        if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            originalObj->setAttributeEventListener(eventname, v);
        } else {
            originalObj->clearAttributeEventListener(eventname);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("cues"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        TextTrackCueList* list = originalObj->cues();
        if (list)
            return list->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("activeCues"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        TextTrackCueList* list = originalObj->activeCues();
        if (list)
            return list->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("kind"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        return toJSString(TextTrack::kindToString(originalObj->kind()));
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("label"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        return toJSString(originalObj->label());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("language"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        return toJSString(originalObj->language());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("mode"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        return toJSString(TextTrack::modeToString(originalObj->mode()));
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        if (firstArg.isESString()) {
            originalObj->setMode(toBrowserString(firstArg.toString()));
        }
        return firstArg;
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("id"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackObject, TextTrack);
        return toJSString(originalObj->id());
    }, nullptr);

    return TextTrackFunction;
}

escargot::ESFunctionObject* bindingTextTrackList(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(TextTrackList, fetchData(scriptBindingInstance)->m_eventTarget);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackListObject, TextTrackList);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    TextTrackListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getTrackById"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackListObject, TextTrackList);
            TextTrack* track = originalObj->getTrackById(toBrowserString(v.toString()));
            if (track) {
                return track->scriptValue();
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, escargot::ESString::create("getTrackById"), 0, false)
    );

    return TextTrackListFunction;
}

escargot::ESFunctionObject* bindingTextTrackCue(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(TextTrackCue, fetchData(scriptBindingInstance)->m_eventTarget);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("startTime"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        return escargot::ESValue(originalObj->startTime());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        double startTime = firstArg.toNumber();
        if (std::isnan(startTime)) {
            THROW_ILLEGAL_INVOCATION();
        }
        originalObj->setStartTime(startTime);
        return firstArg;
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("endTime"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        return escargot::ESValue(originalObj->endTime());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        double endTime = firstArg.toNumber();
        if (std::isnan(endTime)) {
            THROW_ILLEGAL_INVOCATION();
        }
        originalObj->setEndTime(endTime);
        return firstArg;
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("track"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        TextTrack* track = originalObj->track();
        if (track)
            return track->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("id"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        return toJSString(originalObj->id());
    },  [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        String* id = toBrowserString(firstArg.toString());
        originalObj->setId(id);
        return firstArg;
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onenter"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_enter;
        return originalObj->attributeEventListener(eventname);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_enter;
        if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            originalObj->setAttributeEventListener(eventname, v);
        } else {
            originalObj->clearAttributeEventListener(eventname);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onexit"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_exit;
        return originalObj->attributeEventListener(eventname);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_exit;
        if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            originalObj->setAttributeEventListener(eventname, v);
        } else {
            originalObj->clearAttributeEventListener(eventname);
        }
        return escargot::ESValue();
    });

    return TextTrackCueFunction;
}

escargot::ESFunctionObject* bindingTextTrackCueList(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(TextTrackCueList, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TextTrackCueListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueListObject, TextTrackCueList);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    return TextTrackCueListFunction;
}

escargot::ESFunctionObject* bindingVTTCue(ScriptBindingInstance* scriptBindingInstance)
{
    auto VTTCueFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 3) {
            THROW_ILLEGAL_INVOCATION();
        }
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
        escargot::ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
        double startTime = firstArg.toNumber();
        double endTime = secondArg.toNumber();
        if (std::isnan(startTime) || std::isnan(endTime) || !thirdArg.isESString()) {
            THROW_ILLEGAL_INVOCATION();
        }
        VTTCue* cue = new VTTCue(startTime, endTime, String::fromUTF8(thirdArg.toString()->utf8Data()));
        return cue->scriptValue();

    }, escargot::ESString::create("VTTCue"), 3, true, true);
    VTTCueFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    VTTCueFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    VTTCueFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->characterData()->protoType());
    VTTCueFunction->set__proto__(fetchData(scriptBindingInstance)->textTrackCue());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        VTTCueFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("text"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        if (!originalObj->isVTTCue()) {
            THROW_ILLEGAL_INVOCATION();
        }
        VTTCue* cue = (VTTCue*)originalObj;
        return toJSString(cue->text());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        if (!originalObj->isVTTCue() || !firstArg.isESString()) {
            THROW_ILLEGAL_INVOCATION();
        }
        VTTCue* cue = (VTTCue*)originalObj;
        cue->setText(toBrowserString(firstArg.toString()));
        return firstArg;
    });

    VTTCueFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getCueAsHTML"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TextTrackCueObject, TextTrackCue);
            if (!originalObj->isVTTCue()) {
                THROW_ILLEGAL_INVOCATION();
            }
            VTTCue* cue = (VTTCue*)originalObj;
            Document* document = (((Window*)instance->globalObject()->extraPointerData()))->document();
            DocumentFragment* df = cue->getCueAsHTML(document);
            if (df) {
                return df->scriptValue();
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, escargot::ESString::create("getCueAsHTML"), 0, false)
    );

    return VTTCueFunction;
}

escargot::ESFunctionObject* bindingTimeRanges(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(TimeRanges, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    TimeRangesFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("start"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TimeRangesObject, TimeRanges);
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            TO_INDEX_UINT32(firstArg, idx);
            if (idx != INVALID_INDEX && idx < originalObj->length()) {
                return escargot::ESValue(originalObj->start(idx));
            }
            THROW_ILLEGAL_INVOCATION();
        }, escargot::ESString::create("start"), 0, false)
    );

    TimeRangesFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("end"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TimeRangesObject, TimeRanges);
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            TO_INDEX_UINT32(firstArg, idx);
            if (idx != INVALID_INDEX && idx < originalObj->length()) {
                return escargot::ESValue(originalObj->end(idx));
            }
            THROW_ILLEGAL_INVOCATION();
        }, escargot::ESString::create("end"), 0, false)
    );

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        TimeRangesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::TimeRangesObject, TimeRanges);
        return escargot::ESValue(originalObj->length());
    }, nullptr);


    return TimeRangesFunction;
}

escargot::ESFunctionObject* bindingMediaSource(ScriptBindingInstance* scriptBindingInstance)
{
    // DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(MediaSource, fetchData(scriptBindingInstance)->m_eventTarget);

    auto MediaSourceFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        Window* w = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData());
        MediaSource* b = new MediaSource(w->starFish());
        return b->scriptValue();
    }, escargot::ESString::create("MediaSource"), 0, true, true);

    MediaSourceFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    MediaSourceFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    MediaSourceFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->m_eventTarget->protoType());
    MediaSourceFunction->set__proto__(fetchData(scriptBindingInstance)->m_eventTarget);

    MediaSourceFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("addSourceBuffer"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::MediaSourceObject, MediaSource);
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

            try {
                MediaSource* mediaSource = originalObj;
                String* type = toBrowserString(firstArg);
                SourceBuffer* buffer = mediaSource->addSourceBuffer(type);
                return buffer->scriptValue();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("addSourceBuffer"), 1, false)
    );

    MediaSourceFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeSourceBuffer"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::MediaSourceObject, MediaSource);
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            CHECK_TYPEOF(firstArg, ScriptWrappable::Type::SourceBufferObject);

            try {
                MediaSource* mediaSource = (MediaSource*) originalObj;
                SourceBuffer* sourceBuffer = (SourceBuffer*) firstArg.asESPointer()->asESObject()->extraPointerData();
                mediaSource->removeSourceBuffer(sourceBuffer);
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("removeSourceBuffer"), 1, false)
    );

    MediaSourceFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("endOfStream"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::MediaSourceObject);
            MediaSource* mediaSource = (MediaSource*)thisValue.asESPointer()->asESObject()->extraPointerData();
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            if (!firstArg.isUndefinedOrNull() && !firstArg.isESString())
                THROW_ILLEGAL_INVOCATION();
            String* error = String::emptyString;
            if (!firstArg.isUndefinedOrNull())
                error = toBrowserString(firstArg.toString());
            try {
                mediaSource->endOfStream(error);
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("endOfStream"), 1, false)
    );

    // static bool isTypeSupported(type)
    MediaSourceFunction->defineDataProperty(escargot::ESString::create("isTypeSupported"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            String* type = toBrowserString(firstArg);
            bool res = MediaSource::isTypeSupported(type);
            if (res)
                return escargot::ESValue(escargot::ESValue::ESTrueTag::ESTrue);
            else
                return escargot::ESValue(escargot::ESValue::ESFalseTag::ESFalse);
        }, escargot::ESString::create("isTypeSupported"), 1, false)
    );

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourceFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("sourceBuffers"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::MediaSourceObject, MediaSource);
        return escargot::ESValue(originalObj->sourceBuffers()->scriptValue());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourceFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("activeSourceBuffers"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::MediaSourceObject, MediaSource);
        return escargot::ESValue(originalObj->activeSourceBuffers()->scriptValue());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourceFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("readyState"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::MediaSourceObject, MediaSource);
        MediaSource::ReadyState readyState = originalObj->readyState();

        if (readyState == MediaSource::Open)
            return toJSString(originalObj->starFish()->staticStrings()->m_open.localName());
        else if (readyState == MediaSource::Ended)
            return toJSString(originalObj->starFish()->staticStrings()->m_ended.localName());

        STARFISH_ASSERT(readyState == MediaSource::Closed);
        return toJSString(originalObj->starFish()->staticStrings()->m_closed.localName());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        MediaSourceFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("duration"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::MediaSourceObject, MediaSource);
        return escargot::ESValue(originalObj->duration());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::MediaSourceObject, MediaSource);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        double duration = firstArg.toNumber();
        if (std::isnan(duration)) {
            THROW_ILLEGAL_INVOCATION();
        }
        try {
            originalObj->setDuration(duration);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    });

    return MediaSourceFunction;
}

escargot::ESFunctionObject* bindingSourceBuffer(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(SourceBuffer, fetchData(scriptBindingInstance)->m_eventTarget);
    SourceBufferFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("appendBuffer"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::SourceBufferObject);
            SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()->asESObject()->extraPointerData();
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

            try {
                if (false) {

                }
#ifdef USE_ES6_FEATURE
                else if (firstArg.isESPointer() && firstArg.asESPointer()->isESArrayBufferObject()) {
                    escargot::ESArrayBufferObject* v = firstArg.asESPointer()->asESArrayBufferObject();
                    sourceBuffer->appendBuffer((uint8_t*)v->data(), v->bytelength());
                } else if (firstArg.isESPointer() && firstArg.asESPointer()->isESArrayBufferView()) {
                    escargot::ESArrayBufferView* v = firstArg.asESPointer()->asESArrayBufferView();
                    uint8_t* p = (uint8_t*)v->buffer()->data();
                    sourceBuffer->appendBuffer(p, v->bytelength());
                }
#endif
                else {
                    escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Failed to execute 'appendBuffer' on 'SourceBuffer': No function was found that matched the signature provided."))));
                }
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("appendBuffer"), 1, false)
    );

    SourceBufferFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::SourceBufferObject);
            SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()->asESObject()->extraPointerData();
            escargot::ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
            escargot::ESValue arg1 = instance->currentExecutionContext()->readArgument(1);

            try {
                sourceBuffer->remove(arg0.toNumber(), arg1.toNumber());
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("remove"), 2, false)
    );

    SourceBufferFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("abort"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::SourceBufferObject);
            SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()->asESObject()->extraPointerData();

            try {
                sourceBuffer->abort();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("abort"), 0, false)
    );

    // TODO: appendStream

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("mode"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        return escargot::ESValue(originalObj->mode());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        double mode = firstArg.toNumber();
        if (std::isnan(mode)) {
            THROW_ILLEGAL_INVOCATION();
        }
        try {
            if (mode == SourceBuffer::AppendMode::Segments) {
                originalObj->setMode(SourceBuffer::AppendMode::Segments);
            } else if (mode == SourceBuffer::AppendMode::Sequence) {
                originalObj->setMode(SourceBuffer::AppendMode::Sequence);
            }
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("updating"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        return escargot::ESValue(originalObj->updating());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("buffered"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
            TimeRanges* timeRanges = originalObj->buffered();
            if (timeRanges) {
                return timeRanges->scriptValue();
            }
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("timestampOffset"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        return escargot::ESValue(originalObj->timestampOffset());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        double timeStampOffset = firstArg.toNumber();
        if (std::isnan(timeStampOffset)) {
            THROW_ILLEGAL_INVOCATION();
        }
        try {
            originalObj->setTimestampOffset(timeStampOffset);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    });

    /*
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("audioTracks"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        AudioTrackList* audioTracks = originalObj->audioTracks();
        if (audioTracks) {
            return audioTracks->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("videoTracks"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        VideoTrackList* videoTracks = originalObj->videoTracks();
        if (videoTracks) {
            return videoTracks->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, nullptr);
    */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("textTracks"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        TextTrackList* textTracks = originalObj->textTracks();
        if (textTracks) {
            return textTracks->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("appendWindowStart"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        return escargot::ESValue(originalObj->appendWindowStart());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        double timeStamp = firstArg.toNumber();
        if (std::isnan(timeStamp)) {
            THROW_ILLEGAL_INVOCATION();
        }
        try {
            originalObj->setAppendWindowStart(timeStamp);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("appendWindowEnd"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        return escargot::ESValue(originalObj->appendWindowEnd());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        double timeStamp = firstArg.toNumber();
        if (std::isnan(timeStamp)) {
            THROW_ILLEGAL_INVOCATION();
        }
        try {
            originalObj->setAppendWindowEnd(timeStamp);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    });

    // event handler

#define DEFINE_SOURCEBUFFER_EVENT_HANDLER(eventName) \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction( \
        SourceBufferFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("on" #eventName), \
        [](escargot::ESVMInstance* instance) -> escargot::ESValue { \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer); \
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_##eventName; \
        return originalObj->attributeEventListener(eventname); \
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue { \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferObject, SourceBuffer); \
        auto eventname = (((Window*)instance->globalObject()->extraPointerData()))->starFish()->staticStrings()->m_##eventName; \
        if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) { \
            originalObj->setAttributeEventListener(eventname, v); \
        } else { \
            originalObj->clearAttributeEventListener(eventname); \
        } \
        return escargot::ESValue(); \
    });

    DEFINE_SOURCEBUFFER_EVENT_HANDLER(updatestart);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(update);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(updateend);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(error);
    DEFINE_SOURCEBUFFER_EVENT_HANDLER(abort);
#undef DEFINE_SOURCEBUFFER_EVENT_HANDLER

    return SourceBufferFunction;
}

escargot::ESFunctionObject* bindingSourceBufferList(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(SourceBufferList, fetchData(scriptBindingInstance)->m_eventTarget);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferListObject, SourceBufferList);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        SourceBufferListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("SourceBuffer"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::SourceBufferListObject, SourceBufferList);
        SourceBuffer* buffer = originalObj->at(v.toInt32());
        if (buffer) {
            return buffer->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, nullptr);

    return SourceBufferListFunction;
}

#endif

escargot::ESFunctionObject* bindingDocumentFragment(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(DocumentFragment, fetchData(scriptBindingInstance)->node());
    return DocumentFragmentFunction;
}

escargot::ESFunctionObject* bindingDocumentType(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(DocumentType, fetchData(scriptBindingInstance)->node());

    escargot::ESFunctionObject* removeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        Node* p = obj->parentNode();
        if (p == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }
        obj->remove();
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("remove"), 0, false);
    DocumentTypeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, removeFunction);

    /* 4.7 Interface DocumentType */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("name"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        String* s = originalObj->nodeName();
        return toJSString(s);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("publicId"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (!nd->isDocumentType()) {
            THROW_ILLEGAL_INVOCATION();
        }
        String* s = nd->asDocumentType()->publicId();
        return toJSString(s);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentTypeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("systemId"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (!nd->isDocumentType()) {
            THROW_ILLEGAL_INVOCATION();
        }
        String* s = nd->asDocumentType()->systemId();
        return toJSString(s);
    }, nullptr);

    return DocumentTypeFunction;
}

escargot::ESFunctionObject* bindingDocument(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(Document, fetchData(scriptBindingInstance)->node());

    /* 4.5 Interface Document */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("head"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            Document* document = nd->asDocument();
            HTMLHeadElement* head = document->headElement();
            if (head) {
                return head->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("body"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            Document* document = nd->asDocument();
            HTMLBodyElement* body = document->bodyElement();
            if (body) {
                return body->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
        if (!v.isUndefinedOrNull()) {
            Node* nd = originalObj;
            Node* node_v = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
            if (nd->isDocument()) {
                if (node_v->isElement() && node_v->asElement()->isHTMLElement() && node_v->asElement()->asHTMLElement()->isHTMLBodyElement()) {
                    HTMLBodyElement* body = nd->asDocument()->bodyElement();
                    HTMLHtmlElement* html_root = nd->asDocument()->rootElement();
                    if (body)
                        html_root->removeChild(body);
                    html_root->appendChild(node_v);
                } else {
                    THROW_DOM_EXCEPTION(instance, DOMException::HIERARCHY_REQUEST_ERR);
                }
                return escargot::ESValue();
            }
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("documentElement"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            Document* document = nd->asDocument();
            Element* docElem = document->documentElement();
            if (docElem) {
                return docElem->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

#ifdef STARFISH_EXP
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("implementation"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            Document* document = nd->asDocument();
            DOMImplementation* impl = document->domImplementation();
            if (impl) {
                return impl->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);
#endif

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("characterSet"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return toJSString(nd->asDocument()->charset());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("charset"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return toJSString(nd->asDocument()->charset());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("contentType"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return toJSString(nd->asDocument()->contentType());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("compatMode"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return toJSString(nd->asDocument()->compatMode());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("documentURI"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return nd->asDocument()->documentURI() ? toJSString(nd->asDocument()->documentURI()->urlString()) : ScriptValueNull;
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("URL"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return nd->asDocument()->documentURI() ? toJSString(nd->asDocument()->documentURI()->urlString()) : ScriptValueNull;
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("firstElementChild"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->firstElementChild() ? originalObj->firstElementChild()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("lastElementChild"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return originalObj->lastElementChild() ? originalObj->lastElementChild()->scriptValue() : escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("childElementCount"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        return escargot::ESValue(originalObj->childElementCount());
    }, nullptr);

    escargot::ESFunctionObject* getElementByIdFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);

            if (argValue.isESString()) {
                escargot::ESString* argStr = argValue.asESString();

                if (*argStr == *(escargot::strings->emptyString.string()))
                    return escargot::ESValue(escargot::ESValue::ESNull);

                Element* elem = doc->getElementById(toBrowserString(argStr));
                if (elem != nullptr)
                    return elem->scriptValue();
            } else if (argValue.isNull()) {
                Element* elem = doc->getElementById(toBrowserString(escargot::ESString::create("null")));
                if (elem != nullptr)
                    return elem->scriptValue();
            } else if (argValue.isUndefined()) {
                Element* elem = doc->getElementById(toBrowserString(escargot::ESString::create("undefined")));
                if (elem != nullptr)
                    return elem->scriptValue();
            }
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("getElementById"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getElementById"), false, false, false, getElementByIdFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("doctype"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            DocumentType* docType = nd->asDocument()->docType();
            if (docType != nullptr) {
                return docType->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    escargot::ESFunctionObject* createDocumentFragmentFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                return doc->createDocumentFragment()->scriptValue();
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("DocumentFragment"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("createDocumentFragment"), false, false, false, createDocumentFragmentFunction);

    escargot::ESFunctionObject* createElementFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
                if (argValue.isUndefined()) {
                    AtomicString name = AtomicString::createAttrAtomicString(doc->window()->starFish(), "undefined");
                    Element* elem = doc->createElement(name, true);
                    if (elem != nullptr)
                        return elem->scriptValue();
                } else if (argValue.isNull()) {
                    AtomicString name = AtomicString::createAttrAtomicString(doc->window()->starFish(), "null");
                    Element* elem = doc->createElement(name, true);
                    if (elem != nullptr)
                        return elem->scriptValue();
                } else if (argValue.isESString()) {
                    escargot::ESString* argStr = argValue.asESString();
                    auto bStr = toBrowserString(argStr);
                    if (!QualifiedName::checkNameProductionRule(bStr, bStr->length()))
                        throw new DOMException(doc->window()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);
                    AtomicString name = AtomicString::createAttrAtomicString(doc->window()->starFish(), argStr->utf8Data());
                    Element* elem = doc->createElement(name, true);
                    if (elem != nullptr)
                        return elem->scriptValue();
                }
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("createElement"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("createElement"), false, false, false, createElementFunction);

    escargot::ESFunctionObject* createTextNodeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            if (argValue.isESString()) {
                escargot::ESString* argStr = argValue.asESString();
                Text* elem = doc->createTextNode(toBrowserString(argStr));
                if (elem != nullptr)
                    return elem->scriptValue();
            } else if (argValue.isNull()) {
                Text* elem = doc->createTextNode(toBrowserString(escargot::ESString::create("null")));
                if (elem != nullptr)
                    return elem->scriptValue();
            } else if (argValue.isUndefined()) {
                Text* elem = doc->createTextNode(toBrowserString(escargot::ESString::create("undefined")));
                if (elem != nullptr)
                    return elem->scriptValue();
            } else {
                THROW_ILLEGAL_INVOCATION();
            }

        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("createTextNode"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("createTextNode"), false, false, false, createTextNodeFunction);

    escargot::ESFunctionObject* createCommentNodeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            Comment* elem = nullptr;
            if (argValue.isUndefined()) {
                elem = doc->createComment(String::fromUTF8("undefined"));
            } else if (argValue.isNull()) {
                elem = doc->createComment(String::fromUTF8("null"));
            } else if (argValue.isESString()) {
                escargot::ESString* argStr = argValue.asESString();
                elem = doc->createComment(toBrowserString(argStr));
            }
            if (elem != nullptr)
                return elem->scriptValue();
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("createComment"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("createComment"), false, false, false, createCommentNodeFunction);

    escargot::ESFunctionObject* getElementsByTagNameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            if (argValue.isESString()) {
                escargot::ESString* argStr = argValue.asESString();
                QualifiedName name(doc->window()->starFish()->staticStrings()->m_xhtmlNamespaceURI, AtomicString::createAttrAtomicString(doc->window()->starFish(), toBrowserString(argStr)));
                HTMLCollection* result = doc->getElementsByTagName(name);
                if (result != nullptr)
                    return result->scriptValue();
            }
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("getElementsByTagName"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getElementsByTagName"), false, false, false, getElementsByTagNameFunction);

    escargot::ESFunctionObject* getElementsByClassNameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            if (argValue.isESString() || argValue.isUndefinedOrNull()) {
                escargot::ESString* argStr;
                if (argValue.isNull()) {
                    argStr = escargot::ESString::create("null");
                } else if (argValue.isUndefined()) {
                    argStr = escargot::ESString::create("undefined");
                } else {
                    argStr = argValue.asESString();
                }

                HTMLCollection* result = doc->getElementsByClassName(toBrowserString(argStr));
                if (result != nullptr)
                    return result->scriptValue();
            } else if (argValue.isObject() && argValue.asESPointer()->isESArrayObject()) {
                escargot::ESArrayObject* array = argValue.asESPointer()->asESArrayObject();
                String* listSoFar = String::createASCIIString("");
                for (unsigned i = 0; i < array->length(); i++) {
                    escargot::ESValue val = array->get(i);
                    if (val.isESString()) {
                        listSoFar = listSoFar->concat(toBrowserString(val.asESString()));
                        if (i < array->length()-1) {
                            listSoFar = listSoFar->concat(String::createASCIIString(","));
                        }
                    } else {
                        return escargot::ESValue(escargot::ESValue::ESNull);
                    }
                }
                HTMLCollection* result = doc->getElementsByClassName(listSoFar);
                if (result) {
                    return result->scriptValue();
                }
            }
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("getElementsByClassName"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getElementsByClassName"), false, false, false, getElementsByClassNameFunction);

    escargot::ESFunctionObject* createAttributeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
                if (argValue.isUndefined()) {
                    Attr* result = doc->createAttribute(QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(doc->window()->starFish(), "undefined")));
                    if (result != nullptr)
                        return result->scriptValue();
                } else if (argValue.isNull()) {
                    Attr* result = doc->createAttribute(QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(doc->window()->starFish(), "null")));
                    if (result != nullptr)
                        return result->scriptValue();
                } else if (argValue.isESString()) {
                    escargot::ESString* argStr = argValue.asESString();
                    Attr* result = doc->createAttribute(QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(doc->window()->starFish(), argStr->utf8Data())));
                    if (result != nullptr)
                        return result->scriptValue();
                }
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("createAttribute"), 1, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("createAttribute"), false, false, false, createAttributeFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("children"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (originalObj->isDocument()) {
            return originalObj->children()->scriptValue();
        }
        THROW_ILLEGAL_INVOCATION()
    }, nullptr);

    /* Page Visibility */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("hidden"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (originalObj->isDocument()) {
            bool hidden = originalObj->asDocument()->hidden();
            return escargot::ESValue(hidden);
        }
        THROW_ILLEGAL_INVOCATION()
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("visibilityState"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        if (originalObj->isDocument()) {
            String* visibilityState = originalObj->asDocument()->visibilityState();
            return toJSString(visibilityState);
        }
        THROW_ILLEGAL_INVOCATION()
    }, nullptr);

    escargot::ESFunctionObject* elementFromPointFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            escargot::ESValue argValue0 = instance->currentExecutionContext()->readArgument(0);
            escargot::ESValue argValue1 = instance->currentExecutionContext()->readArgument(0);
            Element* element = doc->elementFromPoint(argValue0.toNumber(), argValue1.toNumber());
            if (element) {
                return element->scriptValue();
            }
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("elementFromPoint"), 2, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("elementFromPoint"), false, false, false, elementFromPointFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onclick"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return nd->attributeEventListener(nd->document()->window()->starFish()->staticStrings()->m_click);
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            auto eventType = nd->document()->window()->starFish()->staticStrings()->m_click;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                nd->setAttributeEventListener(eventType, v);
            } else {
                nd->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onmouseover"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return nd->attributeEventListener(nd->document()->window()->starFish()->staticStrings()->m_mouseover);
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            auto eventType = nd->document()->window()->starFish()->staticStrings()->m_mouseover;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                nd->setAttributeEventListener(eventType, v);
            } else {
                nd->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onfocus"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return nd->attributeEventListener(nd->document()->window()->starFish()->staticStrings()->m_focus);
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            auto eventType = nd->document()->window()->starFish()->staticStrings()->m_focus;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                nd->setAttributeEventListener(eventType, v);
            } else {
                nd->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });



    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("defaultView"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            Document* document = nd->asDocument();
            Window* window = document->window();
            return window->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("location"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            LocationObj* location = nd->asDocument()->location();
            return location->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    return DocumentFunction;
}

escargot::ESFunctionObject* bindingHTMLElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLElement, fetchData(scriptBindingInstance)->element());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("dir"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_dir));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_dir, toBrowserString(v));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onclick"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            return element->attributeEventListener(element->document()->window()->starFish()->staticStrings()->m_click);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_click;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->setAttributeEventListener(eventType, v);
            } else {
                element->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    escargot::ESFunctionObject* clickFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* obj = originalObj;
        String* eventType = obj->document()->window()->starFish()->staticStrings()->m_click.localName();
        Event* e = new Event(eventType, EventInit(true, true));
        obj->dispatchEvent(e);
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("click"), 1, false);
    HTMLElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("click"), true, true, true, clickFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onmouseover"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            return element->attributeEventListener(element->document()->window()->starFish()->staticStrings()->m_mouseover);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_mouseover;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->setAttributeEventListener(eventType, v);
            } else {
                element->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    escargot::ESFunctionObject* mouseoverFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* obj = originalObj;
        String* eventType = obj->document()->window()->starFish()->staticStrings()->m_mouseover.localName();
        Event* e = new Event(eventType, EventInit(true, true));
        obj->dispatchEvent(e);
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("mouseover"), 1, false);
    HTMLElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("mouseover"), true, true, true, mouseoverFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onload"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_load;
            return element->attributeEventListener(eventType);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_load;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->setAttributeEventListener(eventType, v);
            } else {
                element->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onunload"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_unload;
            return element->attributeEventListener(eventType);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_unload;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->setAttributeEventListener(eventType, v);
            } else {
                element->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onkeydown"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            return element->attributeEventListener(element->document()->window()->starFish()->staticStrings()->m_keydown);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_keydown;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->setAttributeEventListener(eventType, v);
            } else {
                element->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onkeyup"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            return element->attributeEventListener(element->document()->window()->starFish()->staticStrings()->m_keyup);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_keyup;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->setAttributeEventListener(eventType, v);
            } else {
                element->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onfocus"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            return element->attributeEventListener(element->document()->window()->starFish()->staticStrings()->m_focus);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_focus;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->setAttributeEventListener(eventType, v);
            } else {
                element->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    escargot::ESFunctionObject* focusFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* obj = originalObj;
        String* eventType = obj->document()->window()->starFish()->staticStrings()->m_focus.localName();
        Event* e = new Event(eventType, EventInit(false, false));
        obj->dispatchEvent(e);
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("focus"), 1, false);
    HTMLElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("focus"), true, true, true, focusFunction);

    return HTMLElementFunction;
}

escargot::ESFunctionObject* bindingHTMLBodyElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLBodyElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLBodyElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onload"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_load;
            return element->document()->window()->attributeEventListener(eventType);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_load;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->document()->window()->setAttributeEventListener(eventType, v);
            } else {
                element->document()->window()->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLBodyElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onunload"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_unload;
            return element->document()->window()->attributeEventListener(eventType);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
            auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
            auto eventType = element->document()->window()->starFish()->staticStrings()->m_unload;
            if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
                element->document()->window()->setAttributeEventListener(eventType, v);
            } else {
                element->document()->window()->clearAttributeEventListener(eventType);
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    return HTMLBodyElementFunction;
}

escargot::ESFunctionObject* bindingHTMLScriptElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLScriptElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("src"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            size_t idx = nd->asElement()->hasAttribute(nd->document()->window()->starFish()->staticStrings()->m_src);
            if (idx != SIZE_MAX) {
                return toJSString(URL::getURLString(nd->document()->documentURI()->urlString(), nd->asElement()->getAttribute(idx)));
            }
            return toJSString(String::emptyString);
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_src, toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("type"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_type));
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_type, toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("charset"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_charset));
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_charset, toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("text"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            return toJSString(nd->asElement()->asHTMLElement()->asHTMLScriptElement()->text());
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            nd->asElement()->asHTMLElement()->asHTMLScriptElement()->setText(toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    // TODO : Implement setter
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("charset"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(nd->document()->window()->starFish(), "charset"));
            size_t idx = nd->asElement()->hasAttribute(name);
            if (idx == SIZE_MAX)
                return escargot::ESString::create("");
            else {
                String* value = nd->asElement()->getAttribute(idx);
                // STARFISH_ASSERT(value.equals("UTF-8"));
                return toJSString(value);
            }
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    return HTMLScriptElementFunction;
}

escargot::ESFunctionObject* bindingHTMLStyleElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLStyleElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLStyleElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("type"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLStyleElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_type));
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLStyleElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_type, toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });
    return HTMLStyleElementFunction;
}

escargot::ESFunctionObject* bindingHTMLLinkElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLLinkElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("href"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
            size_t idx = nd->asElement()->hasAttribute(nd->document()->window()->starFish()->staticStrings()->m_href);
            if (idx != SIZE_MAX) {
                return toJSString(URL::getURLString(nd->document()->documentURI()->urlString(), nd->asElement()->getAttribute(idx)));
            }
            return toJSString(String::emptyString);
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_href, toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("rel"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_rel));
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_rel, toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("type"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_type));
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
            nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_type, toBrowserString(v.toString()));
            return escargot::ESValue();
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    return HTMLLinkElementFunction;
}

escargot::ESFunctionObject* bindingHTMLImageElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLImageElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("src"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    return toJSString(nd->asElement()->asHTMLElement()->asHTMLImageElement()->src());
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLImageElement()->setSrc(toBrowserString(v));
                    return escargot::ESValue();
                }
            }
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("width"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    String* width = nd->asElement()->asHTMLElement()->asHTMLImageElement()->width();
                    return escargot::ESValue(String::parseInt(width));
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;

        if (!(v.isESString() && v.asESString()->hasOnlyDigit())) {
            return escargot::ESValue();
        }

        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLImageElement()->setWidth(v.toInt32());
                    return escargot::ESValue();
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("height"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    String* height = nd->asElement()->asHTMLElement()->asHTMLImageElement()->height();
                    return escargot::ESValue(String::parseInt(height));
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;

        if (!(v.isESString() && v.asESString()->hasOnlyDigit())) {
            return escargot::ESValue();
        }

        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLImageElement()->setHeight(v.toInt32());
                    return escargot::ESValue();
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
        return escargot::ESValue();
    });
    return HTMLImageElementFunction;
}

#ifdef STARFISH_ENABLE_MULTI_PAGE
escargot::ESFunctionObject* bindingHTMLAnchorElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLAnchorElement, fetchData(scriptBindingInstance)->htmlElement());
    return HTMLAnchorElementFunction;
}
#endif

#ifdef STARFISH_ENABLE_MULTIMEDIA

#define DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F) \
    [](escargot::ESVMInstance* instance) -> escargot::ESValue { \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node); \
        Node* nd = originalObj; \
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTML##ElementName##Element()) { \
            HTML##ElementName##Element* __element = nd->asElement()->asHTMLElement()->asHTML##ElementName##Element(); \
            RETURN_##TYPE_F(getter) \
        } \
        return escargot::ESValue(); \
    }
#define DEFINE_HTMLELEMENT_PROPERTY_SETTER(ElementName, setter, TYPE_F) \
    [](escargot::ESVMInstance* instance) -> escargot::ESValue { \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node); \
        Node* nd = originalObj; \
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLMediaElement()) { \
            HTML##ElementName##Element* __element = nd->asElement()->asHTMLElement()->asHTML##ElementName##Element(); \
            ARG_##TYPE_F(setter) \
            return v; \
        } \
        return escargot::ESValue(); \
    }
#define DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(ElementName, getter, setter, TYPE_F) \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction( \
        HTML##ElementName##ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create(#getter), \
        DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F) \
    , \
        DEFINE_HTMLELEMENT_PROPERTY_SETTER(ElementName, setter, TYPE_F) \
    );
#define DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(ElementName, getter, TYPE_F) \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction( \
        HTML##ElementName##ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create(#getter), \
        DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F) \
    , nullptr);

#define DEFINE_HTMLELEMENT_EVENT_PROPERTY(ElementName, getter, setter, TYPE_F) \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction( \
        HTML##ElementName##ElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("on"#getter), \
        DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F) \
    , \
        DEFINE_HTMLELEMENT_PROPERTY_SETTER(ElementName, getter, TYPE_F) \
    );

#define RETURN_TYPE_STRING(getter) \
    return toJSString(__element->getter());
#define RETURN_TYPE_PRIMITIVE(getter) \
    return escargot::ESValue(__element->getter());
#define RETURN_TYPE_NUMBER(getter) \
    RETURN_TYPE_PRIMITIVE(getter)
#define RETURN_TYPE_BOOLEAN(getter) \
    RETURN_TYPE_PRIMITIVE(getter)
#define RETURN_TYPE_SCRIPTVALUE(getter) \
    auto __result = __element->getter(); \
    if (__result) { \
        return __result->scriptValue(); \
    }
#define RETURN_TYPE_EVENT(getter) \
    return __element->attributeEventListener(__element->document()->window()->starFish()->staticStrings()->m_##getter);

#define ARG_TYPE_STRING(setter) \
    __element->setter(toBrowserString(v.toString()));
#define ARG_TYPE_NUMBER(setter) \
    double __doubleValue = v.toNumber(); \
    if (std::isnan(__doubleValue)) { \
        THROW_ILLEGAL_INVOCATION(); \
    } \
    __element->setter(__doubleValue);
#define ARG_TYPE_BOOLEAN(setter) \
    if (v.isBoolean()) { \
        __element->setter(v.asBoolean()); \
    }
#define ARG_TYPE_EVENT(setter) \
    auto eventType = __element->document()->window()->starFish()->staticStrings()->m_##setter; \
    if (v.isObject() || (v.isESPointer() && v.asESPointer()->isESFunctionObject())) { \
        __element->setAttributeEventListener(eventType, v); \
    } else { \
        __element->clearAttributeEventListener(eventType); \
    }

escargot::ESFunctionObject* bindingHTMLMediaElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLMediaElement, fetchData(scriptBindingInstance)->htmlElement());

    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("NETWORK_EMPTY"), false, true, false, escargot::ESValue(HTMLMediaElement::NETWORK_EMPTY));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("NETWORK_IDLE"), false, true, false, escargot::ESValue(HTMLMediaElement::NETWORK_IDLE));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("NETWORK_LOADING"), false, true, false, escargot::ESValue(HTMLMediaElement::NETWORK_LOADING));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("NETWORK_NO_SOURCE"), false, true, false, escargot::ESValue(HTMLMediaElement::NETWORK_NO_SOURCE));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("HAVE_NOTHING"), false, true, false, escargot::ESValue(HTMLMediaElement::HAVE_NOTHING));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("HAVE_METADATA"), false, true, false, escargot::ESValue(HTMLMediaElement::HAVE_METADATA));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("HAVE_CURRENT_DATA"), false, true, false, escargot::ESValue(HTMLMediaElement::HAVE_CURRENT_DATA));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("HAVE_FUTURE_DATA"), false, true, false, escargot::ESValue(HTMLMediaElement::HAVE_FUTURE_DATA));
    HTMLMediaElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("HAVE_ENOUGH_DATA"), false, true, false, escargot::ESValue(HTMLMediaElement::HAVE_ENOUGH_DATA));

    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, src, setSrc, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, currentSrc, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, networkState, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, preload, setPreload, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, buffered, TYPE_SCRIPTVALUE);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, readyState, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, seeking, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, currentTime, setCurrentTime, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, duration, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, paused, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, defaultPlaybackRate, setDefaultPlaybackRate, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, playbackRate, setPlaybackRate, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, played, TYPE_SCRIPTVALUE);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, seekable, TYPE_SCRIPTVALUE);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, ended, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, autoplay, setAutoplay, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, loop, setLoop, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, controls, setControls, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, volume, setVolume, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Media, muted, setMuted, TYPE_BOOLEAN);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Media, textTracks, TYPE_SCRIPTVALUE);

    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, progress, Progress, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, suspend, Suspend, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, abort, Abort, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, error, Error, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, emptied, Emptied, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, stalled, Stalled, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, loadedmetadata, Loadedmetadata, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, loadeddata, Loadeddata, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, loadstart, Loadstart, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, canplay, Canplay, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, canplaythrough, Canplaythrough, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, playing, Playing, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, waiting, Waiting, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, seeking, Seeking, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, seeked, Seeked, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, ended, Ended, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, durationchange, Durationchange, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, timeupdate, Timeupdate, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, play, Play, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, pause, Pause, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, ratechange, Ratechange, TYPE_EVENT);
    DEFINE_HTMLELEMENT_EVENT_PROPERTY(Media, volumechange, Volumechange, TYPE_EVENT);

    HTMLMediaElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("addTextTrack"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
            Node* nd = originalObj;
            if (!(nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLMediaElement()))
                THROW_ILLEGAL_INVOCATION();

            escargot::ESValue arg1 = instance->currentExecutionContext()->readArgument(0);
            escargot::ESValue arg2 = instance->currentExecutionContext()->readArgument(1);
            escargot::ESValue arg3 = instance->currentExecutionContext()->readArgument(2);
            String* kind = String::emptyString;
            String* label = String::emptyString;
            String* language = String::emptyString;

            // First Arg : kind
            if (arg1.isUndefinedOrNull() || !arg1.isESString())
                THROW_ILLEGAL_INVOCATION();
            kind = toBrowserString(arg1.toString());
            // Second Arg : label (can be omitted)
            if (!arg2.isUndefinedOrNull() && !arg2.isESString())
                THROW_ILLEGAL_INVOCATION();
            if (!arg2.isUndefinedOrNull())
                label = toBrowserString(arg2.toString());
            // Third Arg : language (can be omitted)
            if (!arg3.isUndefinedOrNull() && !arg3.isESString())
                THROW_ILLEGAL_INVOCATION();
            if (!arg3.isUndefinedOrNull())
                language = toBrowserString(arg3.toString());

            HTMLMediaElement* element = originalObj->asElement()->asHTMLElement()->asHTMLMediaElement();
            TextTrack* track = element->addTextTrack(kind, label, language);
            if (!track)
                THROW_ILLEGAL_INVOCATION();
            return track->scriptValue();
        }, escargot::ESString::create("addTextTrack"), 3, false)
    );

    HTMLMediaElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("load"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
            Node* nd = originalObj;
            if (!(nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLMediaElement()))
                THROW_ILLEGAL_INVOCATION();
            originalObj->asElement()->asHTMLElement()->asHTMLMediaElement()->load();
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("load"), 0, false)
    );

    HTMLMediaElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("canPlayType"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
            Node* nd = originalObj;
            if (!(nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLMediaElement()))
                THROW_ILLEGAL_INVOCATION();
            String* result = originalObj->asElement()->asHTMLElement()->asHTMLMediaElement()->canPlayType(toBrowserString(v.toString()));
            if (!result)
                THROW_ILLEGAL_INVOCATION();
            return toJSString(result);
        }, escargot::ESString::create("canPlayType"), 1, false)
    );

    HTMLMediaElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("pause"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
            Node* nd = originalObj;
            if (!(nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLMediaElement()))
                THROW_ILLEGAL_INVOCATION();
            originalObj->asElement()->asHTMLElement()->asHTMLMediaElement()->pause();
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        }, escargot::ESString::create("pause"), 0, false)
    );

    HTMLMediaElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("play"), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
            Node* nd = originalObj;
            if (!(nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLMediaElement()))
                THROW_ILLEGAL_INVOCATION();
#ifdef USE_ES6_FEATURE
            return originalObj->asElement()->asHTMLElement()->asHTMLMediaElement()->play()->scriptValue();
#else
            originalObj->asElement()->asHTMLElement()->asHTMLMediaElement()->play();
            return escargot::ESValue();
#endif
        }, escargot::ESString::create("play"), 0, false)
    );


    return HTMLMediaElementFunction;
}

escargot::ESFunctionObject* bindingHTMLVideoElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLVideoElement, fetchData(scriptBindingInstance)->htmlMediaElement());

    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Video, width, setWidth, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Video, height, setHeight, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Video, videoWidth, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Video, videoHeight, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Video, poster, setPoster, TYPE_STRING);

    return HTMLVideoElementFunction;
}

escargot::ESFunctionObject* bindingHTMLAudioElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLAudioElement, fetchData(scriptBindingInstance)->htmlMediaElement());
    // TODO
    return HTMLAudioElementFunction;
}

escargot::ESFunctionObject* bindingHTMLTrackElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLTrackElement, fetchData(scriptBindingInstance)->htmlElement());

    HTMLTrackElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("NONE"), false, true, false, escargot::ESValue(HTMLTrackElement::NONE));
    HTMLTrackElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("LOADING"), false, true, false, escargot::ESValue(HTMLTrackElement::LOADING));
    HTMLTrackElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("LOADED"), false, true, false, escargot::ESValue(HTMLTrackElement::LOADED));
    HTMLTrackElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("ERROR"), false, true, false, escargot::ESValue(HTMLTrackElement::ERROR));

    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Track, readyState, TYPE_NUMBER);
    DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(Track, src, setSrc, TYPE_STRING);
    DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(Track, track, TYPE_SCRIPTVALUE);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLTrackElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("default"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLTrackElement()) {
            return escargot::ESValue(nd->asElement()->asHTMLElement()->asHTMLTrackElement()->defaultValue());
        }
        return escargot::ESValue();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLTrackElement()) {
            escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
            if (!firstArg.isBoolean())
                return escargot::ESValue();
            nd->asElement()->asHTMLElement()->asHTMLTrackElement()->setDefaultValue(firstArg.asBoolean());
            return firstArg;
        }
        return escargot::ESValue();
    });

    return HTMLTrackElementFunction;
}

#undef DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY
#undef DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY
#undef DEFINE_HTMLELEMENT_PROPERTY_GETTER
#undef DEFINE_HTMLELEMENT_PROPERTY_SETTER
#undef RETURN_TYPE_STRING
#undef RETURN_TYPE_PRIMITIVE
#undef RETURN_TYPE_NUMBER
#undef RETURN_TYPE_BOOLEAN
#undef RETURN_TYPE_SCRIPTVALUE
#undef ARG_TYPE_STRING
#undef ARG_TYPE_NUMBER
#undef ARG_TYPE_BOOLEAN

#endif

escargot::ESFunctionObject* bindingHTMLCollection(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLCollection, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    /* 4.2.7.2 Interface HTMLCollection */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLCollectionFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::HTMLCollectionObject, HTMLCollection);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    escargot::ESFunctionObject* itemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
    {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::HTMLCollectionObject);
        HTMLCollection* self = (HTMLCollection*)(thisValue.asESPointer()->asESObject()->extraPointerData());

        size_t count = instance->currentExecutionContext()->argumentCount();
        if (count > 0) {
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            TO_INDEX_UINT32(argValue, idx);
            if (idx != INVALID_INDEX && idx < self->length()) {
                Element* elem = self->item(idx);
                STARFISH_ASSERT(elem != nullptr);
                return elem->scriptValue();
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } else {
            auto msg = escargot::ESString::create("Failed to execute 'hasAttribute' on Element: 1 argument required, but only 0 present.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("item"), 1, false);
    HTMLCollectionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), true, true, true, itemFunction);

    escargot::ESFunctionObject* namedItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::HTMLCollectionObject);

        size_t count = instance->currentExecutionContext()->argumentCount();
        if (count > 0) {
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            escargot::ESString* argStr = argValue.toString();
            Element* elem = ((HTMLCollection*) thisValue.asESPointer()->asESObject()->extraPointerData())->namedItem(toBrowserString(argStr));
            if (elem != nullptr)
                return elem->scriptValue();
            return escargot::ESValue(escargot::ESValue::ESNull);
        } else {
            auto msg = escargot::ESString::create("Failed to execute 'namedItem' on 'HTMLCollection': 1 argument required, but only 0 present.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("namedItem"), 1, false);
    HTMLCollectionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("namedItem"), true, true, true, namedItemFunction);

    return HTMLCollectionFunction;
}

escargot::ESFunctionObject* bindingEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* 3.2 Interface Event */
    auto eventFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);

        if (argCount == 0) {
            auto msg = escargot::ESString::create("Failed to construct 'Event': 1 argument required, but only 0 present.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else if (argCount == 1) {
            escargot::ESString* type = firstArg.toString();
            auto event = new Event(String::fromUTF8(type->utf8Data()));
            return event->scriptValue();
        } else {
            if (secondArg.isObject() || secondArg.isUndefinedOrNull()) {
                escargot::ESString* type = firstArg.toString();
                bool canBubbles = false;
                bool canCancelable = false;
                if (!secondArg.isUndefinedOrNull()) {
                    escargot::ESValue bubbles = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("bubbles"));
                    escargot::ESValue cancelable = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("cancelable"));
                    canBubbles = bubbles.isBoolean() ? bubbles.asBoolean() : canBubbles;
                    canCancelable = cancelable.isBoolean() ? cancelable.asBoolean() : canCancelable;
                }
#ifdef STARFISH_TC_COVERAGE
                if (canBubbles) {
                    STARFISH_LOG_INFO("&&&EventInit::bubbles\n");
                }
                if (canCancelable) {
                    STARFISH_LOG_INFO("&&&EventInit::cancelable\n");
                }
#endif
                auto event = new Event(String::fromUTF8(type->utf8Data()), EventInit(canBubbles, canCancelable));
                return event->scriptValue();
            } else {
                escargot::ESString* msg = escargot::ESString::create("Failed to construct 'Event': parameter 2 ('eventInitDict') is not an object.");
                instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }, escargot::ESString::create("Event"), 1, true, true);
    eventFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    eventFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    eventFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    // fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("Event"), true, false, true, eventFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("type"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        String* type = const_cast<String*>(originalObj->eventType());
        return toJSString(type);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("target"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        EventTarget* target = originalObj->target();
        if (target) {
            return target->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("currentTarget"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        EventTarget* currentTarget = originalObj->currentTarget();
        if (currentTarget) {
            return currentTarget->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    eventFunction->asESObject()->defineDataProperty(escargot::ESString::create("NONE"), false, true, false, escargot::ESValue(Event::NONE));
    eventFunction->asESObject()->defineDataProperty(escargot::ESString::create("CAPTURING_PHASE"), false, true, false, escargot::ESValue(Event::CAPTURING_PHASE));
    eventFunction->asESObject()->defineDataProperty(escargot::ESString::create("AT_TARGET"), false, true, false, escargot::ESValue(Event::AT_TARGET));
    eventFunction->asESObject()->defineDataProperty(escargot::ESString::create("BUBBLING_PHASE"), false, true, false, escargot::ESValue(Event::BUBBLING_PHASE));

    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("NONE"), false, true, false, escargot::ESValue(Event::NONE));
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("CAPTURING_PHASE"), false, true, false, escargot::ESValue(Event::CAPTURING_PHASE));
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("AT_TARGET"), false, true, false, escargot::ESValue(Event::AT_TARGET));
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("BUBBLING_PHASE"), false, true, false, escargot::ESValue(Event::BUBBLING_PHASE));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("eventPhase"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        unsigned short eventPhase = originalObj->eventPhase();
        return escargot::ESValue(eventPhase);
    }, nullptr);

    auto stopPropagationFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventObject);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount == 0) {
            ((Event*) thisValue.asESPointer()->asESObject()->extraPointerData())->setStopPropagation();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("stopPropagation"), 0, false);
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("stopPropagation"), false, false, false, stopPropagationFunction);

    auto stopImmediatePropagationFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventObject);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount == 0) {
            ((Event*) thisValue.asESPointer()->asESObject()->extraPointerData())->setStopImmediatePropagation();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("stopImmediatePropagation"), 0, false);
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("stopImmediatePropagation"), false, false, false, stopImmediatePropagationFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("bubbles"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        bool bubbles = originalObj->bubbles();
        return escargot::ESValue(bubbles);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("cancelable"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        bool cancelable = originalObj->cancelable();
        return escargot::ESValue(cancelable);
    }, nullptr);

    auto preventDefaultFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventObject);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount == 0) {
            ((Event*) thisValue.asESPointer()->asESObject()->extraPointerData())->preventDefault();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("preventDefault"), 0, false);
    eventFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("preventDefault"), false, false, false, preventDefaultFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("defaultPrevented"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        bool defaultPrevented = originalObj->defaultPrevented();
        return escargot::ESValue(defaultPrevented);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("timeStamp"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        DOMTimeStamp timeStamp = originalObj->timeStamp();
        return escargot::ESValue(timeStamp);
    }, nullptr);

    return eventFunction;
}

escargot::ESFunctionObject* bindingUIEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* UI Events */
    DEFINE_FUNCTION_WITH_PARENTFUNC(UIEvent, fetchData(scriptBindingInstance)->event());
    return UIEventFunction;
}

escargot::ESFunctionObject* bindingMouseEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* Mouse Events */
    DEFINE_FUNCTION_WITH_PARENTFUNC(MouseEvent, fetchData(scriptBindingInstance)->uiEvent());
    return MouseEventFunction;
}

escargot::ESFunctionObject* bindingTouchEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* Touch Events */
    DEFINE_FUNCTION_WITH_PARENTFUNC(TouchEvent, fetchData(scriptBindingInstance)->uiEvent());
    return TouchEventFunction;
}

escargot::ESFunctionObject* bindingKeyboardEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* Keyboard Events */
    DEFINE_FUNCTION_WITH_PARENTFUNC(KeyboardEvent, fetchData(scriptBindingInstance)->uiEvent());
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("keyCode"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        if (originalObj->isUIEvent() && originalObj->asUIEvent()->isKeyboardEvent()) {
            return escargot::ESValue(originalObj->asUIEvent()->asKeyboardEvent()->keyCode());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("ctrlKey"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        if (originalObj->isUIEvent() && originalObj->asUIEvent()->isKeyboardEvent()) {
            return escargot::ESValue(originalObj->asUIEvent()->asKeyboardEvent()->ctrlKey());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("altKey"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        if (originalObj->isUIEvent() && originalObj->asUIEvent()->isKeyboardEvent()) {
            return escargot::ESValue(originalObj->asUIEvent()->asKeyboardEvent()->altKey());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("shiftKey"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        if (originalObj->isUIEvent() && originalObj->asUIEvent()->isKeyboardEvent()) {
            return escargot::ESValue(originalObj->asUIEvent()->asKeyboardEvent()->shiftKey());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("metaKey"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        if (originalObj->isUIEvent() && originalObj->asUIEvent()->isKeyboardEvent()) {
            return escargot::ESValue(originalObj->asUIEvent()->asKeyboardEvent()->metaKey());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);
    return KeyboardEventFunction;
}

escargot::ESFunctionObject* bindingFocusEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* Focus Events */
    DEFINE_FUNCTION_WITH_PARENTFUNC(FocusEvent, fetchData(scriptBindingInstance)->uiEvent());
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        FocusEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("relatedTarget"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        if (originalObj->isUIEvent() && originalObj->asUIEvent()->isFocusEvent()) {
            return escargot::ESValue(originalObj->asUIEvent()->asFocusEvent()->relatedTarget());
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);
    return FocusEventFunction;
}

escargot::ESFunctionObject* bindingProgressEvent(ScriptBindingInstance* scriptBindingInstance)
{
    /* Progress Events */
    auto progressEventFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);

        if (argCount == 0) {
            auto msg = escargot::ESString::create("Failed to construct 'ProgressEvent': 1 argument required, but only 0 present.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else if (argCount == 1) {
            escargot::ESString* type = firstArg.toString();
            auto event = new ProgressEvent(String::fromUTF8(type->utf8Data()));
            return event->scriptValue();
        } else {
            if (secondArg.isObject()) {
                escargot::ESString* type = firstArg.toString();
                escargot::ESObject* obj = secondArg.asESPointer()->asESObject();
                escargot::ESValue bubbles = obj->get(escargot::ESString::create("bubbles"));
                escargot::ESValue cancelable = obj->get(escargot::ESString::create("cancelable"));
                escargot::ESValue lengthComputable = obj->get(escargot::ESString::create("lengthComputable"));
                escargot::ESValue loaded = obj->get(escargot::ESString::create("loaded"));
                escargot::ESValue total = obj->get(escargot::ESString::create("total"));
                bool canBubbles = bubbles.isBoolean() ? bubbles.asBoolean() : false;
                bool canCancelable = cancelable.isBoolean() ? cancelable.asBoolean() : false;
                bool canLengthComputable = false;
                if (lengthComputable.isBoolean()) {
                    canLengthComputable = lengthComputable.asBoolean();
                } else if (lengthComputable.isESString()) {
                    canLengthComputable = lengthComputable.asESString()->length() != 0 ? true : false;
                }
                unsigned long long loadedValue = 0;
                if (loaded.isNumber()) {
                    loadedValue = loaded.asNumber();
                } else if (loaded.isESString()) {
                    loadedValue = loaded.toUint32();
                }
                unsigned long long totalValue = 0;
                if (total.isNumber()) {
                    totalValue = total.asNumber();
                } else if (loaded.isESString()) {
                    totalValue = total.toUint32();
                }
#ifdef STARFISH_TC_COVERAGE
                if (type->isESString()) {
                    STARFISH_LOG_INFO("ProgressEventInit&&&type\n");
                }
                if (total.isESString()) {
                    STARFISH_LOG_INFO("ProgressEventInit&&&total\n");
                }
                if (canLengthComputable) {
                    STARFISH_LOG_INFO("ProgressEventInit&&&lengthComputable\n");
                }
                if (loadedValue != 0) {
                    STARFISH_LOG_INFO("ProgressEventInit&&&loaded\n");
                }
#endif
                auto event = new ProgressEvent(String::fromUTF8(type->utf8Data()), ProgressEventInit(canBubbles, canCancelable, canLengthComputable, loadedValue, totalValue));
                return event->scriptValue();
            } else {
                escargot::ESString* msg = escargot::ESString::create("Failed to construct 'ProgressEvent': parameter 2 ('eventInitDict') is not an object.");
                instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }, escargot::ESString::create("ProgressEvent"), 1, true, true);
    progressEventFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    progressEventFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->event()->protoType());
    progressEventFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    // fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("ProgressEvent"), true, false, true, progressEventFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        progressEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("lengthComputable"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        Event* e = originalObj;
        if (e->isProgressEvent()) {
            bool lengthComputable = e->asProgressEvent()->lengthComputable();
            return escargot::ESValue(lengthComputable);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        progressEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("loaded"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        Event* e = originalObj;
        if (e->isProgressEvent()) {
            unsigned long long loaded = e->asProgressEvent()->loaded();
            return escargot::ESValue(loaded);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        progressEventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("total"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        Event* e = originalObj;
        if (e->isProgressEvent()) {
            unsigned long long total = e->asProgressEvent()->total();
            return escargot::ESValue(total);
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, nullptr);

    return progressEventFunction;
}

escargot::ESFunctionObject* bindingNodeList(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NodeList, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    NodeListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeListObject);
            NodeList* nodeList = (NodeList*)thisValue.asESPointer()->asESObject()->extraPointerData();

            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            TO_INDEX_UINT32(argValue, idx);
            if (idx != INVALID_INDEX && idx < nodeList->length()) {
                Node* nd = nodeList->item(idx);
                return nd->scriptValue();
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, escargot::ESString::create("item"), 1, false)
    );

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeListObject, NodeList);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    return NodeListFunction;
}

// https://dom.spec.whatwg.org/#interface-domtokenlist
escargot::ESFunctionObject* bindingDOMTokenList(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMTokenList, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    /* 7.1 Interface DOMTokenList */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMTokenListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMTokenListObject, DOMTokenList);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    escargot::ESFunctionObject* domTokenListItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);

        DOMTokenList* self = (DOMTokenList*)(thisValue.asESPointer()->asESObject()->extraPointerData());
        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        TO_INDEX_UINT32(argValue, idx);
        if (idx != INVALID_INDEX && idx < self->length()) {
            String* elem = self->item(idx);
            return toJSString(elem);
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("item"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false, domTokenListItemFunction);

    escargot::ESFunctionObject* domTokenListContainsFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);
        try {
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            escargot::ESString* argStr = argValue.toString();
            bool res = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->contains(toBrowserString(argStr));
            return escargot::ESValue(res);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("contains"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("contains"), false, false, false, domTokenListContainsFunction);

    escargot::ESFunctionObject* domTokenListAddFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);
        try {
            std::vector<String*, gc_allocator<String*>> tokens;
            int argCount = instance->currentExecutionContext()->argumentCount();
            for (int i = 0; i < argCount; i++) {
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(i);
                escargot::ESString* argStr = argValue.toString();
                String* aa = toBrowserString(argStr);
                tokens.push_back(aa);
            }
            if (argCount > 0)
                ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->add(&tokens);
            return escargot::ESValue();
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("add"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("add"), false, false, false, domTokenListAddFunction);

    escargot::ESFunctionObject* domTokenListRemoveFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);
        try {
            std::vector<String*, gc_allocator<String*>> tokens;
            int argCount = instance->currentExecutionContext()->argumentCount();
            for (int i = 0; i < argCount; i++) {
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(i);
                escargot::ESString* argStr = argValue.toString();
                String* aa = toBrowserString(argStr);
                tokens.push_back(aa);
            }
            if (argCount > 0)
                ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->remove(&tokens);
            return escargot::ESValue();
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("remove"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, domTokenListRemoveFunction);

    escargot::ESFunctionObject* domTokenListToggleFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);
        try {
            int argCount = instance->currentExecutionContext()->argumentCount();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            escargot::ESValue forceValue;

            if (argCount == 0)
                instance->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Not enough arguments"))));
            if (argCount >= 2)
                forceValue = instance->currentExecutionContext()->readArgument(1);
            if (argCount > 0) {
                escargot::ESString* argStr = argValue.toString();
                bool didAdd;
                if (argCount == 1) {
                    didAdd = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toggle(toBrowserString(argStr), false, false);
                } else {
                    if (forceValue.isUndefined()) {
                        didAdd = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toggle(toBrowserString(argStr), false, false);
                    } else {
                        ASSERT(forceValue.isBoolean());
                        didAdd = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toggle(toBrowserString(argStr), true, forceValue.asBoolean());
                    }
                }
                return escargot::ESValue(didAdd);
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("toggle"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("toggle"), false, false, false, domTokenListToggleFunction);

    escargot::ESFunctionObject* domTokenListToStringFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);
        String* str = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toString();
        return toJSString(str);
    }, escargot::ESString::create("toString"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("toString"), true, false, true, domTokenListToStringFunction);

    return DOMTokenListFunction;
}

escargot::ESFunctionObject* bindingDOMSettableTokenList(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMSettableTokenList, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    /* 7.2 Interface DOMSettableTokenList */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMSettableTokenListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMSettableTokenListObject, DOMSettableTokenList);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    escargot::ESFunctionObject* domSettableTokenListItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);
        DOMTokenList* self = (DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData();
        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        TO_INDEX_UINT32(argValue, idx);
        if (idx != INVALID_INDEX && idx < self->length()) {
            String* elem = self->item(idx);
            return toJSString(elem);
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("item"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false, domSettableTokenListItemFunction);

    escargot::ESFunctionObject* domSettableTokenListContainsFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);
        try {
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            if (argValue.isESString()) {
                bool res = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->contains(toBrowserString(argValue.asESString()));
                return escargot::ESValue(res);
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

    }, escargot::ESString::create("contains"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("contains"), false, false, false, domSettableTokenListContainsFunction);

    escargot::ESFunctionObject* domSettableTokenListAddFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);
        try {
            std::vector<String*, gc_allocator<String*>> tokens;
            int argCount = instance->currentExecutionContext()->argumentCount();
            for (int i = 0; i < argCount; i++) {
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(i);
                if (argValue.isESString()) {
                    String* aa = toBrowserString(argValue.asESString());
                    tokens.push_back(aa);
                } else {
                    THROW_ILLEGAL_INVOCATION()
                }
            }
            if (argCount > 0)
                ((DOMTokenList*) thisValue.asESPointer()->asESObject())->add(&tokens);
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("add"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("add"), false, false, false, domSettableTokenListAddFunction);

    escargot::ESFunctionObject* domSettableTokenListRemoveFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);
        try {
            std::vector<String*, gc_allocator<String*>> tokens;
            int argCount = instance->currentExecutionContext()->argumentCount();
            for (int i = 0; i < argCount; i++) {
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(i);
                if (argValue.isESString()) {
                    String* aa = toBrowserString(argValue.asESString());
                    tokens.push_back(aa);
                } else {
                    THROW_ILLEGAL_INVOCATION()
                }
            }
            if (argCount > 0)
                ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->remove(&tokens);
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("remove"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, domSettableTokenListRemoveFunction);

    escargot::ESFunctionObject* domSettableTokenListToggleFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);
        try {
            int argCount = instance->currentExecutionContext()->argumentCount();
            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            escargot::ESValue forceValue;
            if (argCount >= 2)
                forceValue = instance->currentExecutionContext()->readArgument(1);
            if (argCount > 0 && argValue.isESString()) {
                bool didAdd;
                if (argCount == 1) {
                    didAdd = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toggle(toBrowserString(argValue.asESString()), false, false);
                } else {
                    ASSERT(forceValue.isBoolean());
                    didAdd = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toggle(toBrowserString(argValue.asESString()), true, forceValue.asBoolean());
                }
                return escargot::ESValue(didAdd);
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("toggle"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("toggle"), false, false, false, domSettableTokenListToggleFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMSettableTokenListFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("value"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMSettableTokenListObject, DOMSettableTokenList);
        String* value = originalObj->value();
        return toJSString(value);
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMSettableTokenListObject, DOMSettableTokenList);
        originalObj->setValue(toBrowserString(v));
        return escargot::ESValue();
    });

    return DOMSettableTokenListFunction;
}

escargot::ESFunctionObject* bindingNamedNodeMap(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NamedNodeMap, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    /* 4.8.1 Interface NamedNodeMap */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NamedNodeMapFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NamedNodeMapObject, NamedNodeMap);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    escargot::ESFunctionObject* NamedNodeMapItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NamedNodeMapObject);
        NamedNodeMap* self = (NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData();
        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        TO_INDEX_UINT32(argValue, idx);
        if (idx != INVALID_INDEX && idx < self->length()) {
            Attr* elem = self->item(idx);
            if (elem != nullptr)
                return elem->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("item"), 1, false);
    NamedNodeMapFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false, NamedNodeMapItemFunction);

    escargot::ESFunctionObject* NamedNodeMapGetNamedItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NamedNodeMapObject);

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            NamedNodeMap* t = ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData());
            String* key = toBrowserString(argValue);
            auto attrName = t->element()->document()->createAttributeName(key);
            Attr* elem = t->getNamedItem(attrName);
            if (elem != nullptr)
                return elem->scriptValue();
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("getNamedItem"), 1, false);
    NamedNodeMapFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getNamedItem"), false, false, false, NamedNodeMapGetNamedItemFunction);

    escargot::ESFunctionObject* NamedNodeMapSetNamedItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NamedNodeMapObject);
        NamedNodeMap* namedNodeMap = (NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData();
        STARFISH_ASSERT(namedNodeMap->element());

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        CHECK_TYPEOF(argValue, ScriptWrappable::Type::NodeObject);
        if (!((Node*) argValue.asESPointer()->asESObject()->extraPointerData())->isAttr()) {
            THROW_ILLEGAL_INVOCATION()
        }
        Attr* passedAttr = (Attr*) argValue.asESPointer()->asESObject()->extraPointerData();
        Attr* toReturn = namedNodeMap->setNamedItem(passedAttr);
        if (toReturn)
            return toReturn->scriptValue();
        else
            return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("setNamedItem"), 1, false);
    NamedNodeMapFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("setNamedItem"), false, false, false, NamedNodeMapSetNamedItemFunction);

    escargot::ESFunctionObject* NamedNodeMapRemoveNamedItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NamedNodeMapObject);

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            try {
                QualifiedName name(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(((Window*)instance->globalObject()->extraPointerData())->starFish(), argValue.asESString()->utf8Data()));
                Attr* old = ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->getNamedItem(name);
                if (old == nullptr)
                    throw new DOMException(((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->striptBindingInstance(), DOMException::Code::NOT_FOUND_ERR, nullptr);
                Attr* toReturn = new Attr(old->document(), ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->striptBindingInstance(), name, old->value());
                ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->removeNamedItem(name);
                if (toReturn != nullptr)
                    return toReturn->scriptValue();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

        } else
            THROW_ILLEGAL_INVOCATION()
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("removeNamedItem"), 1, false);
    NamedNodeMapFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeNamedItem"), false, false, false, NamedNodeMapRemoveNamedItemFunction);

    return NamedNodeMapFunction;
}

escargot::ESFunctionObject* bindingAttr(ScriptBindingInstance* scriptBindingInstance)
{
    /* 4.8.2 Interface Attr */
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(Attr, fetchData(scriptBindingInstance)->node());

    auto attrNameValueGetter = [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        String* n = ((Attr*)originalObj)->name().localName();
        return toJSString(n);
    };

    auto attrValueGetter = [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        String* value = ((Attr*)originalObj)->value();
        return toJSString(value);
    };

    auto attrValueSetter = [](escargot::ESVMInstance* instance) -> escargot::ESValue
    {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        ((Attr*)originalObj)->setValue(toBrowserString(v));
        // FIXME(JMP): Actually this function have to return old Attr's value but we have to modify 'typedef void (*ESNativeSetter)(...)' in escargot/src/runtime/ESValue.h
        // Because this need to many changes, we do the modification latter
        return escargot::ESValue();
    };

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("name"), attrNameValueGetter, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("localName"), attrNameValueGetter, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("value"), attrValueGetter, attrValueSetter);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nodeValue"), attrValueGetter, attrValueSetter);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("textContent"), attrValueGetter, attrValueSetter);

    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ownerElement"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        if (!((Node*) originalObj->extraPointerData())->isAttr()) {
            THROW_ILLEGAL_INVOCATION()
        }

        Element* elem = ((Attr*) originalObj->extraPointerData())->ownerElement();
        if (elem != nullptr)
            return elem->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        NULL, false, false, false);

    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("specified"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        if (!((Node*) originalObj->extraPointerData())->isAttr()) {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(true);
        },
        NULL, false, false, false);

    return AttrFunction;
}

escargot::ESFunctionObject* bindingCSSStyleDeclaration(ScriptBindingInstance* scriptBindingInstance)
{
    /* style-related getter/setter start here */
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleDeclaration, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CSSStyleDeclarationFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("length"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CSSStyleDeclarationObject, CSSStyleDeclaration);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

#ifdef STARFISH_ENABLE_TEST
    CSSStyleDeclarationFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getPropertyValue"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::CSSStyleDeclarationObject);
                CSSStyleDeclaration* decl = (CSSStyleDeclaration*)thisValue.asESPointer()->asESObject()->extraPointerData();
                escargot::ESValue prop = instance->currentExecutionContext()->readArgument(0);

                if (prop.isESString()) {
                    String* name = toBrowserString(prop);
                    const char* c = name->utf8Data();
                    CSSStyleKind kind = lookupCSSStyle(c, strlen(c));
                    String* val = String::emptyString;
                    switch (kind) {
#define MATCH_KEY(Name, ...) \
                    case CSSStyleKind::Name: \
                        val = decl->Name(); \
                        break;
                        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(MATCH_KEY)
#undef MATCH_KEY
                    default:
                        break;
                    }
                    return toJSString(val);
                } else {
                    return escargot::ESString::create("");
                }
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("getPropertyValue"), 2, false));

    CSSStyleDeclarationFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("setProperty"), true, true, true,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::CSSStyleDeclarationObject);
                CSSStyleDeclaration* decl = (CSSStyleDeclaration*)thisValue.asESPointer()->asESObject()->extraPointerData();
                escargot::ESValue prop = instance->currentExecutionContext()->readArgument(0);
                escargot::ESValue val = instance->currentExecutionContext()->readArgument(1);

                String* name = toBrowserString(prop.toString());
                const char* c = name->utf8Data();
                CSSStyleKind kind = lookupCSSStyle(c, strlen(c));

                if (kind == CSSStyleKind::Unknown) {
                } else {
                    if (false) {

                    }
#define SET_ATTR(name, nameLower, nameCSSCase) \
                    else if (kind == CSSStyleKind::name) { \
                        decl->set##name(toBrowserString(val)); \
                    }
                    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
                }
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            return escargot::ESValue();
        }, escargot::ESString::create("setProperty"), 3, false));
#endif

    return CSSStyleDeclarationFunction;
}

escargot::ESFunctionObject* bindingCSSStyleRule(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleRule, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    return CSSStyleRuleFunction;
}

escargot::ESFunctionObject* bindingXMLHttpRequest(ScriptBindingInstance* scriptBindingInstance)
{
    /* XMLHttpRequestEventTarget */

    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(XMLHttpRequestEventTarget, fetchData(scriptBindingInstance)->m_eventTarget);
    fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(XMLHttpRequestEventTargetString, true, false, true, XMLHttpRequestEventTargetFunction);

    /* XMLHttpRequest */
    escargot::ESFunctionObject* xhrElementFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        Window* wnd = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData());
        auto xhr = new XMLHttpRequest(wnd->document());
        return xhr->scriptValue();
    }, escargot::ESString::create("XMLHttpRequest"), 0, true, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->set__proto__(XMLHttpRequestEventTargetFunction->protoType());
    fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("XMLHttpRequest"), false, false, false, xhrElementFunction);

#define DEFINE_XHR_EVENT_HANDLER(eventName) \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction( \
        XMLHttpRequestEventTargetFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("on" #eventName), \
        [](escargot::ESVMInstance* instance) -> escargot::ESValue { \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest); \
        auto eventType = originalObj->networkRequest().starFish()->staticStrings()->m_##eventName; \
        return originalObj->attributeEventListener(eventType); \
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue { \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest); \
        auto eventType = originalObj->networkRequest().starFish()->staticStrings()->m_##eventName; \
        originalObj->setAttributeEventListener(eventType, v); \
        return escargot::ESValue(); \
    });

    DEFINE_XHR_EVENT_HANDLER(loadstart);
    DEFINE_XHR_EVENT_HANDLER(progress);
    DEFINE_XHR_EVENT_HANDLER(abort);
    DEFINE_XHR_EVENT_HANDLER(error);
    DEFINE_XHR_EVENT_HANDLER(load);
    DEFINE_XHR_EVENT_HANDLER(timeout);
    DEFINE_XHR_EVENT_HANDLER(loadend);
    DEFINE_XHR_EVENT_HANDLER(readystatechange);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("timeout"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        uint32_t c = originalObj->networkRequest().timeout();
        return escargot::ESValue(c);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        try {
            originalObj->setTimeout(v.toUint32());
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("readyState"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        int c = originalObj->networkRequest().readyState();
        return escargot::ESValue(c);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("status"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        int c = originalObj->networkRequest().status();
        return escargot::ESValue(c);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("responseText"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        try {
            String* c = originalObj->responseText();

#ifdef STARFISH_TC_COVERAGE
            STARFISH_LOG_INFO("&&&responseText\n");
#endif
            return toJSString(c);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("response"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        try {
            escargot::ESValue c = originalObj->response();
#ifdef STARFISH_TC_COVERAGE
            STARFISH_LOG_INFO("&&&response\n");
#endif
            return c;
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("responseType"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        XMLHttpRequest::ResponseType type = originalObj->responseType();
        if (type == XMLHttpRequest::ResponseType::Unspecified) {
            return escargot::ESString::create("");
        } else if (type == XMLHttpRequest::ResponseType::ArrayBuffer) {
            return escargot::ESString::create("arraybuffer");
        } else if (type == XMLHttpRequest::ResponseType::BlobType) {
            return escargot::ESString::create("blob");
        } else if (type == XMLHttpRequest::ResponseType::DocumentType) {
            return escargot::ESString::create("document");
        } else if (type == XMLHttpRequest::ResponseType::Json) {
            return escargot::ESString::create("json");
        } else if (type == XMLHttpRequest::ResponseType::Text) {
            return escargot::ESString::create("text");
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        try {
            escargot::ESString* str = instance->currentExecutionContext()->readArgument(0).toString();
            if (*str == "") {
                originalObj->setResponseType(XMLHttpRequest::ResponseType::Unspecified);
                return escargot::ESValue();
            } else if (*str == "arraybuffer") {
#ifdef USE_ES6_FEATURE
                originalObj->setResponseType(XMLHttpRequest::ResponseType::ArrayBuffer);
                return escargot::ESValue();
#endif
            } else if (*str == "blob") {
                originalObj->setResponseType(XMLHttpRequest::ResponseType::BlobType);
                return escargot::ESValue();
            } else if (*str == "document") {
            } else if (*str == "json") {
                originalObj->setResponseType(XMLHttpRequest::ResponseType::Json);
                return escargot::ESValue();
            } else if (*str == "text") {
                originalObj->setResponseType(XMLHttpRequest::ResponseType::Text);
                return escargot::ESValue();
            }
            STARFISH_LOG_ERROR("The provided value '%s' is not a valid enum value of type XMLHttpRequestResponseType.", str->utf8Data());
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        return escargot::ESValue();
    });

    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("UNSENT"), false, true, false, escargot::ESValue(0));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("OPENED"), false, true, false, escargot::ESValue(1));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("HEADERS_RECEIVED"), false, true, false, escargot::ESValue(2));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("LOADING"), false, true, false, escargot::ESValue(3));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("DONE"), false, true, false, escargot::ESValue(4));

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("UNSENT"), false, true, false, escargot::ESValue(0));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("OPENED"), false, true, false, escargot::ESValue(1));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("HEADERS_RECEIVED"), false, true, false, escargot::ESValue(2));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("LOADING"), false, true, false, escargot::ESValue(3));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("DONE"), false, true, false, escargot::ESValue(4));

    escargot::ESFunctionObject* xhrSetRequestHeaderFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (instance->currentExecutionContext()->argumentCount() < 2) {
            auto msg = escargot::ESString::create("Failed to execute 'setRequestHeader' on 'XMLHttpRequest': setRequestHeader needs 2 parameter.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
        }
        try {
            String* s1 = toBrowserString(instance->currentExecutionContext()->readArgument(0))->trim();
            String* s2 = toBrowserString(instance->currentExecutionContext()->readArgument(1))->trim();
            originalObj->setRequestHeader(s1, s2);
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("setRequestHeader"), 2, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("setRequestHeader"), false, false, false, xhrSetRequestHeaderFunction);

    escargot::ESFunctionObject* xhrOpenFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        try {
            if (instance->currentExecutionContext()->argumentCount() >= 2) {
                // https://xhr.spec.whatwg.org/#the-open()-method
                // TOOD If method is not a method, throw a SyntaxError exception.
                // TODO If method is a forbidden method, throw a SecurityError exception.
                // Let parsedURL be the result of parsing url with context object's relevant settings object's API base URL.
                // If parsedURL is failure, throw a SyntaxError exception.
                std::string method = instance->currentExecutionContext()->readArgument(0).toString()->utf8Data();
                std::transform(method.begin(), method.end(), method.begin(), ::tolower);
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
                    async = instance->currentExecutionContext()->readArgument(2).toBoolean();
                }

                String* userName = String::emptyString;
                String* password = String::emptyString;

                if (instance->currentExecutionContext()->argumentCount() == 4) {
                    userName = toBrowserString(instance->currentExecutionContext()->readArgument(3));
                } else if (instance->currentExecutionContext()->argumentCount() >= 5) {
                    userName = toBrowserString(instance->currentExecutionContext()->readArgument(3));
                    password = toBrowserString(instance->currentExecutionContext()->readArgument(4));
                }
                originalObj->open(mt, toBrowserString(instance->currentExecutionContext()->readArgument(1)), async, userName, password);
            } else {
                auto msg = escargot::ESString::create("Failed to execute 'open' on 'XMLHttpRequest': 2 arguments required.");
                instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("open"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("open"), false, false, false, xhrOpenFunction);

    escargot::ESFunctionObject* xhrSendFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        try {
            if (instance->currentExecutionContext()->argumentCount() == 0) {
                originalObj->send();
            } else {
                originalObj->send(toBrowserString(instance->currentExecutionContext()->readArgument(0)));
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("send"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("send"), false, false, false, xhrSendFunction);

    escargot::ESFunctionObject* xhrAbortFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        try {
            originalObj->abort();
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("abort"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("abort"), false, false, false, xhrAbortFunction);

    return xhrElementFunction;
}

escargot::ESFunctionObject* bindingBlob(ScriptBindingInstance* scriptBindingInstance)
{
    /* Blob */
    auto BlobFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        // https://www.w3.org/TR/FileAPI/#blob-constructor-steps
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount == 0) {
            Window* w = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData());
            Blob* b = new Blob(w->starFish(), 0, String::emptyString, nullptr, false, false);
            return b->scriptValue();
        }
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

        void* bytes = 0;
        escargot::ESValue lengthString = escargot::currentInstance->strings().length.string();
        escargot::ESObject* obj = nullptr;
        escargot::ESValue lengthValue;

        if (!firstArg.isObject() || (lengthValue = (obj = firstArg.toObject())->get(lengthString)).isUndefinedOrNull()) {
            auto msg = escargot::ESString::create("Failed to construct 'Blob': The 1st argument is neither an array, nor does it have indexed properties.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
        }

        size_t length = (size_t)lengthValue.toNumber();

        std::vector<std::pair<void* , size_t>, gc_allocator<std::pair<void*, size_t>>> bufferInfo;
        size_t totalByteLength = 0;
        for (size_t i = 0; i < length; i ++) {
            escargot::ESValue element = obj->get(escargot::ESValue(i));

#ifdef USE_ES6_FEATURE
            // ESArrayBufferView
            if (element.isESPointer() && element.asESPointer()->isESArrayBufferView()) {
                escargot::ESArrayBufferView* v = element.asESPointer()->asESArrayBufferView();
                const char* p = (const char*)v->buffer()->data();
                p += v->byteoffset();
                bufferInfo.push_back(std::make_pair((void*)p, v->bytelength()));
                totalByteLength += v->bytelength();
            }

            // ESArrayBufferObject
            if (element.isESPointer() && element.asESPointer()->isESArrayBufferObject()) {
                escargot::ESArrayBufferObject* v = element.asESPointer()->asESArrayBufferObject();
                bufferInfo.push_back(std::make_pair((void*)v->data(), v->bytelength()));
                totalByteLength += v->bytelength();
            }
#endif
            // Blob
            if (element.isObject()) {
                escargot::ESObject* o = element.toObject();
                if (o->extraData() == kEscargotObjectCheckMagic && ((ScriptWrappable*)o->extraPointerData())->type() == ScriptWrappable::Type::BlobObject) {
                    Blob* bb = (Blob*)o->extraPointerData();
                    bufferInfo.push_back(std::make_pair(bb->data(), bb->size()));
                    totalByteLength += bb->size();
                    continue;
                }
            }

            // otherwise, toString()
            NullableUTF8String s = toBrowserString(element.toString())->toNullableUTF8String();
            bufferInfo.push_back(std::make_pair((void*)s.m_buffer, s.m_bufferSize));
            totalByteLength += s.m_bufferSize;
        }

        size_t offset = 0;
        char* buffer = (char*)GC_MALLOC_ATOMIC(totalByteLength);
        for (size_t i = 0; i < bufferInfo.size(); i ++) {
            memcpy(buffer + offset, bufferInfo[i].first, bufferInfo[i].second);
            offset += bufferInfo[i].second;
        }

        STARFISH_ASSERT(offset == totalByteLength);

        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
        String* type = String::emptyString;
        if (!secondArg.isUndefinedOrNull()) {
            type = toBrowserString(secondArg.toString())->toLower();
        }

        Window* w = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData());
        Blob* newBlob = new Blob(w->starFish(), totalByteLength, type, buffer, false, false);
        return newBlob->scriptValue();
    }, escargot::ESString::create("Blob"), 0, true, true);
    BlobFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    BlobFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    BlobFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    BlobFunction->set__proto__(fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        BlobFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("size"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::BlobObject, Blob);
        return escargot::ESValue(originalObj->size());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        BlobFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("type"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::BlobObject, Blob);
        return escargot::ESValue(toJSString(originalObj->mimeType()));
    }, nullptr);


    escargot::ESFunctionObject* BlobSliceFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::BlobObject, Blob);
        escargot::ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
        escargot::ESValue arg2 = instance->currentExecutionContext()->readArgument(2);

        // FIXME range of size_t and int64_t is not match!
        int64_t start = 0;
        int64_t end = (int64_t)originalObj->size();
        if (!arg0.isUndefinedOrNull()) {
            start = arg0.toNumber();
        }
        if (!arg1.isUndefinedOrNull()) {
            end = arg1.toNumber();
        }
        String* type = String::emptyString;
        if (!arg2.isUndefinedOrNull()) {
            type = toBrowserString(arg2.toString());
        }
        Blob* b = originalObj->slice(start, end, type);
        return b->scriptValue();
    }, escargot::ESString::create("slice"), 0, false);
    BlobFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("slice"), false, false, false, BlobSliceFunction);

    return BlobFunction;
}

escargot::ESFunctionObject* bindingURL(ScriptBindingInstance* scriptBindingInstance)
{
#ifdef STARFISH_ENABLE_TEST
    escargot::ESFunctionObject* URLFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 1) {
        // throw error
        } else if (argCount == 1) {
            escargot::ESValue urlString = instance->currentExecutionContext()->readArgument(0);
            auto url = URL::createURL(String::emptyString, String::fromUTF8(urlString.asESString()->utf8Data()));
            return url->scriptValue();
        } else { // ignore redundant arguments
            escargot::ESValue urlString = instance->currentExecutionContext()->readArgument(0);
            escargot::ESValue baseURLString = instance->currentExecutionContext()->readArgument(1);
            // FIXME second argument can be not only string but also object
            STARFISH_ASSERT(baseURLString.isESString());

            auto url = URL::createURL(String::fromUTF8(baseURLString.asESString()->utf8Data()), String::fromUTF8(urlString.asESString()->utf8Data()));
            return url->scriptValue();
        }
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }, escargot::ESString::create("URL"), 2, true, true);

    URLFunction->defineAccessorProperty(escargot::ESVMInstance::currentInstance()->strings().prototype.string(), escargot::ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    URLFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    URLFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    URLFunction->set__proto__(fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
#else
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(URL, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
#endif
    escargot::ESFunctionObject* URLCreateObjectURLFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
        if (arg0.isObject() && (arg0.asObject()->extraData() == kEscargotObjectCheckMagic) && ((ScriptWrappable*)arg0.asObject()->extraPointerData())->type() == ScriptWrappable::Type::BlobObject) {
            Blob* b = (Blob*)arg0.toObject()->extraPointerData();
            String* url = URL::createObjectURL(b);
            return toJSString(url);
#ifdef STARFISH_ENABLE_MULTIMEDIA
        } else if (arg0.isObject() && (arg0.asObject()->extraData() == kEscargotObjectCheckMagic) && ((ScriptWrappable*)arg0.asObject()->extraPointerData())->type() == ScriptWrappable::Type::MediaSourceObject) {
            MediaSource* m = (MediaSource*)arg0.toObject()->extraPointerData();
            String* url = URL::createObjectURL(m);
            return toJSString(url);
#endif
        } else {
            escargot::ESString* msg = escargot::ESString::create("Failed to execute 'createObjectURL' on 'URL': No function was found that matched the signature provided.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("createObjectURL"), 1, false);
    URLFunction->defineDataProperty(escargot::ESString::create("createObjectURL"), false, false, false, URLCreateObjectURLFunction);

    escargot::ESFunctionObject* URLRevokeObjectURLFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        String* arg0 = toBrowserString(instance->currentExecutionContext()->readArgument(0).toString());
        StarFish* sf = ((Window*)instance->globalObject()->extraPointerData())->starFish();
        URL::revokeObjectURL(sf, arg0);
        return escargot::ESValue();
    }, escargot::ESString::create("revokeObjectURL"), 1, false);
    URLFunction->defineDataProperty(escargot::ESString::create("revokeObjectURL"), false, false, false, URLRevokeObjectURLFunction);

#ifdef STARFISH_ENABLE_TEST
    // FIXME setters below should not be null
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("href"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getHref());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("origin"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->origin());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("protocol"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getProtocol());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("username"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getUsername());
    },  [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                escargot::ESValue arg = instance->currentExecutionContext()->readArgument(0);
                escargot::ESString* argString = arg.toString();
                originalObj->setUsername(String::fromUTF8(argString->utf8Data()));
            }
            return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("password"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getPassword());
    },  [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                escargot::ESValue arg = instance->currentExecutionContext()->readArgument(0);
                escargot::ESString* argString = arg.toString();
                originalObj->setPassword(String::fromUTF8(argString->utf8Data()));
            }
            return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("host"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getHost());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("hostname"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getHostname());
    }, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("port"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getPort());
    }, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("pathname"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getPathname());
    },  [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 1) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else {
            escargot::ESValue arg = instance->currentExecutionContext()->readArgument(0);
            escargot::ESString* argString = arg.toString();
            originalObj->setPathname(String::fromUTF8(argString->utf8Data()));
        }
        return escargot::ESValue();
    });
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("search"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getSearch());
    }, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("hash"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
        return toJSString(originalObj->getHash());
    }, nullptr);
/*
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("searchParams"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    }, nullptr);
*/
#endif
    return URLFunction;
}

escargot::ESFunctionObject* bindingDOMException(ScriptBindingInstance* scriptBindingInstance)
{
    /* DOM Exception */
    DEFINE_FUNCTION(DOMException, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->m_instance->globalObject()->errorPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(), fetchData(scriptBindingInstance)->m_instance->strings().name,
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMExceptionObject, DOMException);
        return escargot::ESString::create(originalObj->name());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(), fetchData(scriptBindingInstance)->m_instance->strings().message,
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMExceptionObject, DOMException);
        return toJSString(originalObj->message());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("code"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMExceptionObject, DOMException);
        return escargot::ESValue(originalObj->code());
    }, nullptr);

    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("INDEX_SIZE_ERR"), false, false, false, escargot::ESValue(1));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("HIERARCHY_REQUEST_ERR"), false, false, false, escargot::ESValue(3));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("WRONG_DOCUMENT_ERR"), false, false, false, escargot::ESValue(4));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_CHARACTER_ERR"), false, false, false, escargot::ESValue(5));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("NO_MODIFICATION_ALLOWED_ERR"), false, false, false, escargot::ESValue(7));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("NOT_FOUND_ERR"), false, false, false, escargot::ESValue(8));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("NOT_SUPPORTED_ERR"), false, false, false, escargot::ESValue(9));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("INUSE_ATTRIBUTE_ERR"), false, false, false, escargot::ESValue(10));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_STATE_ERR"), false, false, false, escargot::ESValue(11));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("SYNTAX_ERR"), false, false, false, escargot::ESValue(12));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_MODIFICATION_ERR"), false, false, false, escargot::ESValue(13));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("NAMESPACE_ERR"), false, false, false, escargot::ESValue(14));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_ACCESS_ERR"), false, false, false, escargot::ESValue(15));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("SECURITY_ERR"), false, false, false, escargot::ESValue(18));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("NETWORK_ERR"), false, false, false, escargot::ESValue(19));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("ABORT_ERR"), false, false, false, escargot::ESValue(20));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("URL_MISMATCH_ERR"), false, false, false, escargot::ESValue(21));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("QUOTA_EXCEEDED_ERR"), false, false, false, escargot::ESValue(22));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("TIMEOUT_ERR"), false, false, false, escargot::ESValue(23));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_NODE_TYPE_ERR"), false, false, false, escargot::ESValue(24));
    DOMExceptionFunction->asESObject()->defineDataProperty(escargot::ESString::create("DATA_CLONE_ERR"), false, false, false, escargot::ESValue(25));

    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("INDEX_SIZE_ERR"), false, false, false, escargot::ESValue(1));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("HIERARCHY_REQUEST_ERR"), false, false, false, escargot::ESValue(3));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("WRONG_DOCUMENT_ERR"), false, false, false, escargot::ESValue(4));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_CHARACTER_ERR"), false, false, false, escargot::ESValue(5));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("NO_MODIFICATION_ALLOWED_ERR"), false, false, false, escargot::ESValue(7));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("NOT_FOUND_ERR"), false, false, false, escargot::ESValue(8));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("NOT_SUPPORTED_ERR"), false, false, false, escargot::ESValue(9));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("INUSE_ATTRIBUTE_ERR"), false, false, false, escargot::ESValue(10));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_STATE_ERR"), false, false, false, escargot::ESValue(11));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("SYNTAX_ERR"), false, false, false, escargot::ESValue(12));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_MODIFICATION_ERR"), false, false, false, escargot::ESValue(13));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("NAMESPACE_ERR"), false, false, false, escargot::ESValue(14));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_ACCESS_ERR"), false, false, false, escargot::ESValue(15));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("SECURITY_ERR"), false, false, false, escargot::ESValue(18));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("NETWORK_ERR"), false, false, false, escargot::ESValue(19));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("ABORT_ERR"), false, false, false, escargot::ESValue(20));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("URL_MISMATCH_ERR"), false, false, false, escargot::ESValue(21));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("QUOTA_EXCEEDED_ERR"), false, false, false, escargot::ESValue(22));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("TIMEOUT_ERR"), false, false, false, escargot::ESValue(23));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("INVALID_NODE_TYPE_ERR"), false, false, false, escargot::ESValue(24));
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("DATA_CLONE_ERR"), false, false, false, escargot::ESValue(25));

    return DOMExceptionFunction;
}
escargot::ESFunctionObject* bindingLocation(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Location, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("href"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        return toJSString(originalObj->getHref());
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 1) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else {
            originalObj->setHref(String::fromUTF8(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data()));
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("pathname"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        return toJSString(originalObj->getPathname());
    },  [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 1) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else {
            originalObj->setPathname(String::fromUTF8(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data()));
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("search"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        return toJSString(originalObj->getSearch());
    },  [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 1) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else {
            originalObj->setSearch(String::fromUTF8(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data()));
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("hash"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        return toJSString(originalObj->getHash());
    },  [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject, LocationObj);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 1) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        } else {
            originalObj->setHash(String::fromUTF8(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data()));
        }
        return escargot::ESValue();
    });

    return LocationFunction;
}

escargot::ESFunctionObject* bindingNavigator(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Navigator, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("appCodeName"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject, Navigator);
        return toJSString(originalObj->appCodeName());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("appName"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject, Navigator);
        return toJSString(originalObj->appName());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("appVersion"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject, Navigator);
        return toJSString(originalObj->appVersion());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("vendor"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject, Navigator);
        return toJSString(originalObj->vendor());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("userAgent"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject, Navigator);
        return toJSString(originalObj->userAgent());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NavigatorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("geolocation"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NavigatorObject, Navigator);
        return originalObj->geoLocation()->scriptObject();
    }, nullptr);

    return NavigatorFunction;
}

escargot::ESFunctionObject* bindingGeolocation(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(Geolocation, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    escargot::ESFunctionObject* getCurrentPositionFunction = escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::GeolocationObject, Geolocation);

        escargot::ESValue opt = instance->currentExecutionContext()->readArgument(2);
        int32_t maximumAgeNumber = 0;
        int32_t timeoutNumber = std::numeric_limits<int32_t>::max();
        bool enableHighAccuracy = false;
        if (opt.isObject()) {
            escargot::ESValue maximumAge = opt.asObject()->get(escargot::ESString::create("maximumAge"));
            double maximumAgeNumberDouble = maximumAge.toNumber();
            if (std::isnan(maximumAgeNumberDouble) || maximumAgeNumberDouble < 0) {
                maximumAgeNumber = 0;
            } else {
                maximumAgeNumber = maximumAgeNumberDouble;
            }

            escargot::ESValue timeout = opt.asObject()->get(escargot::ESString::create("timeout"));
            double timeoutNumberDouble = timeout.toNumber();
            if (std::isnan(timeoutNumberDouble)) {
                timeoutNumber = std::numeric_limits<int32_t>::max();
            } else if (timeoutNumberDouble < 0) {
                timeoutNumber = 0;
            } else {
                timeoutNumber = timeoutNumberDouble;
            }

            enableHighAccuracy = opt.asObject()->get(escargot::ESString::create("enableHighAccuracy")).toBoolean();
        }

        escargot::ESValue cb0 = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue cb1 = instance->currentExecutionContext()->readArgument(1);
        originalObj->getCurrentPosition([](StarFish*, Geoposition* pos, void* data) {
            if (data) {
                escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
                escargot::ESValue a = pos->scriptValue();
                callScriptFunction(fn, &a, 1, ScriptValueUndefined);
            }
        }, cb0.isFunction() ? cb0.asFunction() : nullptr, [](StarFish*, PositionError* error, void* data)
        {
            if (data) {
                escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
                escargot::ESValue a = error->scriptValue();
                callScriptFunction(fn, &a, 1, ScriptValueUndefined);
            }
        }, cb1.isFunction() ? cb1.asFunction() : nullptr,
        enableHighAccuracy, timeoutNumber, maximumAgeNumber);

        return escargot::ESValue();
    }, escargot::ESString::create("getCurrentPosition"), 1);

    GeolocationFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getCurrentPosition"), true, true, true, getCurrentPositionFunction);
    return GeolocationFunction;
}

escargot::ESFunctionObject* bindingGeoposition(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(Geoposition, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        GeopositionFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("coords"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::GeopositionObject, Geoposition);
        return escargot::ESValue(originalObj->coords()->scriptObject());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        GeopositionFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("timestamp"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::GeopositionObject, Geoposition);
        return escargot::ESValue(originalObj->timestamp());
    }, nullptr);

    return GeopositionFunction;
}

escargot::ESFunctionObject* bindingCoordinates(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(Coordinates, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("latitude"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CoordinatesObject, Coordinates);
        return escargot::ESValue(originalObj->latitude());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("longitude"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CoordinatesObject, Coordinates);
        return escargot::ESValue(originalObj->longitude());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("altitude"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CoordinatesObject, Coordinates);
        if (originalObj->altitude())
            return escargot::ESValue(*originalObj->altitude());
        else
            return ScriptValueNull;
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("accuracy"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CoordinatesObject, Coordinates);
        return escargot::ESValue(originalObj->accuracy());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("altitudeAccuracy"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CoordinatesObject, Coordinates);
        if (originalObj->altitudeAccuracy())
            return escargot::ESValue(*originalObj->altitudeAccuracy());
        else
            return ScriptValueNull;
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("heading"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CoordinatesObject, Coordinates);
        if (originalObj->heading())
            return escargot::ESValue(*originalObj->heading());
        else
            return escargot::ESValue(std::numeric_limits<double>::quiet_NaN());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        CoordinatesFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("speed"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CoordinatesObject, Coordinates);
        if (originalObj->speed())
            return escargot::ESValue(*originalObj->speed());
        else
            return ScriptValueNull;
    }, nullptr);

    return CoordinatesFunction;
}

escargot::ESFunctionObject* bindingPositionError(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION(PositionError, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        PositionErrorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("code"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::PositionErrorObject, PositionError);
        return escargot::ESValue(originalObj->code());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        PositionErrorFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("message"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::PositionErrorObject, PositionError);
        return escargot::ESString::create(originalObj->message());
    }, nullptr);

    PositionErrorFunction->asESObject()->defineDataProperty(escargot::ESString::create("PERMISSION_DENIED"), false, false, false, escargot::ESValue(1));
    PositionErrorFunction->asESObject()->defineDataProperty(escargot::ESString::create("POSITION_UNAVAILABLE"), false, false, false, escargot::ESValue(2));
    PositionErrorFunction->asESObject()->defineDataProperty(escargot::ESString::create("TIMEOUT"), false, false, false, escargot::ESValue(3));

    PositionErrorFunction->protoType().toObject()->defineDataProperty(escargot::ESString::create("PERMISSION_DENIED"), false, false, false, escargot::ESValue(1));
    PositionErrorFunction->protoType().toObject()->defineDataProperty(escargot::ESString::create("POSITION_UNAVAILABLE"), false, false, false, escargot::ESValue(2));
    PositionErrorFunction->protoType().toObject()->defineDataProperty(escargot::ESString::create("TIMEOUT"), false, false, false, escargot::ESValue(3));

    return PositionErrorFunction;
}

#ifdef STARFISH_ENABLE_DOMPARSER
escargot::ESFunctionObject* bindingDOMParser(ScriptBindingInstance* scriptBindingInstance)
{
    /* XMLHttpRequest */
    escargot::ESFunctionObject* DOMParserFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        Window* wnd = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData());
        auto v = new DOMParser(wnd->starFish());
        return v->scriptValue();
    }, escargot::ESString::create("DOMParser"), 0, true, false);

    DOMParserFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    DOMParserFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("DOMParser"), false, false, false, DOMParserFunction);

    escargot::ESFunctionObject* parseFromStringFunction = escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMParserObject, DOMParser);

        if (instance->currentInstance()->currentExecutionContext()->argumentCount() < 2) {
            auto msg = escargot::ESString::create("Failed to execute 'parseFromString' on 'DOMParser': needs 2 parameter.");
            instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
        }

        try {
            Document* doc = originalObj->parseFromString(toBrowserString(instance->currentInstance()->currentExecutionContext()->readArgument(0)), toBrowserString(instance->currentInstance()->currentExecutionContext()->readArgument(1)));
            return doc->scriptValue();
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("parseFromString"), 2);

    DOMParserFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("parseFromString"), true, true, true, parseFromStringFunction);

    return DOMParserFunction;
}
#endif

String* ScriptBindingInstance::evaluate(String* str)
{
    std::jmp_buf tryPosition;
    if (setjmp(fetchData(this)->m_instance->registerTryPos(&tryPosition)) == 0) {
        auto result = fetchData(this)->m_instance->evaluate(toJSString(str).asESString());
        String* s = toBrowserString(result);
        fetchData(this)->m_instance->unregisterTryPos(&tryPosition);
        fetchData(this)->m_instance->unregisterCheckedObjectAll();
        return s;
    } else {
        escargot::ESValue err = fetchData(this)->m_instance->getCatchedError();
        ((Window*)fetchData(this)->m_instance->globalObject()->extraPointerData())->starFish()->console()->error(toBrowserString(err));
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
