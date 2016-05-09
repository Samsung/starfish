#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"

#include "ScriptWrappable.h"

#include "platform/window/Window.h"
#include "dom/DOM.h"
#include "dom/NodeList.h"

#include "Escargot.h"
#include "vm/ESVMInstance.h"

#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"
#endif

namespace StarFish {

static ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance)
{
    return (ScriptBindingInstanceDataEscargot*)instance->data();
}

ScriptBindingInstance::ScriptBindingInstance()
{
    m_enterCount = 0;
    m_data = new ScriptBindingInstanceDataEscargot(this);
    fetchData(this)->m_instance = new escargot::ESVMInstance();
    enter();
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
#define THROW_ILLEGAL_INVOCATION()                                                                                                                           \
    escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Illegal invocation")))); \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

#define THROW_DOM_EXCEPTION(instance, errcode) \
    { \
        auto __sf = ((Window*)instance->globalObject()->extraPointerData())->starFish(); \
        auto __err = new DOMException(__sf->scriptBindingInstance(), errcode, nullptr); \
        escargot::ESVMInstance::currentInstance()->throwError(__err->scriptValue()); \
    }

#define CHECK_TYPEOF(thisValue, type)                                                 \
    {                                                                                 \
        escargot::ESValue v = thisValue;                                              \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() & type))) { \
            THROW_ILLEGAL_INVOCATION()                                                \
        }                                                                             \
    }

#define CHECK_TYPEOF_WITH_ERRCODE(thisValue, type, instance, errcode) \
    { \
        escargot::ESValue v = thisValue; \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() & type))) { \
            THROW_DOM_EXCEPTION(instance, errcode); \
        } \
    }

#define GENERATE_THIS_AND_CHECK_TYPE(type, destType)                                                \
    escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();        \
    {                                                                                               \
        escargot::ESValue v = thisValue;                                                            \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() & type))) {               \
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
    escargot::ESValue v;

#if defined(STARFISH_ENABLE_PIXEL_TEST) || defined(STARFISH_EMULATOR_RELEASE)
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("wptTestEnd"), false, false, false, escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        const char* hide = getenv("HIDE_WINDOW");
        if ((hide && strlen(hide)))
            ::exit(0);
        return escargot::ESValue();
    }, escargot::ESString::create("wptTestEnd"), 1, false));
#endif

    escargot::ESObject* console = escargot::ESObject::create();
    console->set(escargot::ESString::create("log"), escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESString* msg = instance->currentExecutionContext()->readArgument(0).toString();
#ifndef STARFISH_TIZEN_WEARABLE
        STARFISH_LOG_INFO("%s\n", msg->utf8Data());
#else
        STARFISH_LOG_INFO("console.log: %s\n", msg->utf8Data());
#endif

        return escargot::ESValue();
    }, escargot::ESString::create("log"), 1, false));
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("console"), false, false, false, console);

    // Navigator
    // TODO implement Navigator function
    escargot::ESObject* navigator = escargot::ESObject::create();
    navigator->defineDataProperty(escargot::ESString::create("appCodeName"), true, true, true, escargot::ESString::create(APP_CODE_NAME));
    navigator->defineDataProperty(escargot::ESString::create("appName"), true, true, true, escargot::ESString::create(APP_CODE_NAME));
    navigator->defineDataProperty(escargot::ESString::create("appVersion"), true, true, true, escargot::ESString::create(APP_VERSION(APP_NAME, VERSION)));
    navigator->defineDataProperty(escargot::ESString::create("vendor"), true, true, true, escargot::ESString::create(VENDOR_NAME));
    navigator->defineDataProperty(escargot::ESString::create("userAgent"), true, true, true, escargot::ESString::create(USER_AGENT(APP_CODE_NAME, VERSION)));
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("navigator"), true, true, true, navigator);

    escargot::ESFunctionObject* toStringFunction = escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isESPointer() && thisValue.asESPointer()->isESObject()) {
            escargot::ESObject* obj = thisValue.asESPointer()->asESObject();
            if (obj->extraData()) {
                escargot::ESValue constructor = thisValue.asESPointer()->asESObject()->get(escargot::ESString::create("constructor"));
                escargot::ESValue constructor_name = constructor.asESPointer()->asESObject()->get(escargot::ESString::create("name"));
                String* result = String::createASCIIString("[object ");
                result = result->concat(toBrowserString(constructor_name.toString()));
                result = result->concat(String::createASCIIString("]"));
                return toJSString(result);
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
            THROW_ILLEGAL_INVOCATION()
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
        }
        return escargot::ESValue();
    }, escargot::ESString::create("addEventListener"), 0, false);
    EventTargetFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("addEventListener"), true, true, true, fnAddEventListener);

    auto fnRemoveEventListener = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventTargetObject);
        if (instance->currentExecutionContext()->argumentCount() < 2) {
            THROW_ILLEGAL_INVOCATION()
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
            if (!(firstArg.isObject() && (firstArg.asESPointer()->asESObject()->extraData() & ScriptWrappable::Type::EventObject))) {
                auto msg = escargot::ESString::create("Failed to execute 'dispatchEvent' on 'EventTarget': parameter 1 is not of type 'Event'.");
                instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
            }
            Event* event = (Event*)firstArg.asESPointer()->asESObject()->extraPointerData();
            ret = ((EventTarget*)thisValue.asESPointer()->asESObject()->extraPointerData())->dispatchEvent(event);
        }
        return escargot::ESValue(ret);
    }, escargot::ESString::create("dispatchEvent"), 1, false);
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

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fetchData(this)->m_instance->globalObject(), escargot::ESString::create("document"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        return (((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document()->scriptObject();
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
/*
    globalObject->defineAccessorProperty(escargot::ESString::create("Node"), [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            return fetchData((((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document()->scriptBindingInstance())->node();
        }, [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& value)
        {
            fetchData((((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document()->scriptBindingInstance())->node();
            escargot::ESVMInstance::currentInstance()->globalObject()->defineDataProperty(name, true, false, true, value);
        }, true, false, true);
*/


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

#ifdef STARFISH_ENABLE_AUDIO
    // TODO convert into defineNativeAccessorPropertyButNeedToGenerateJSFunction way
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLAudioElement, fetchData(scriptBindingInstance)->htmlElement());
    fetchData(this)->m_htmlAudioElement = HTMLAudioElementFunction;

    escargot::ESFunctionObject* audioPlayFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
    {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* nd = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLAudioElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLAudioElement()->play();
                    return escargot::ESValue(escargot::ESValue::ESNull);
                }
            }
        }
        THROW_ILLEGAL_INVOCATION()
    }, escargot::ESString::create("play"), 1, false);
    HTMLAudioElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("play"), false, false, false, audioPlayFunction);

    escargot::ESFunctionObject* audioPauseFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
    {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* nd = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLAudioElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLAudioElement()->pause();
                    return escargot::ESValue(escargot::ESValue::ESNull);
                }
            }
        }
        THROW_ILLEGAL_INVOCATION()
    }, escargot::ESString::create("pause"), 1, false);
    HTMLAudioElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("pause"), false, false, false, audioPauseFunction);

    HTMLAudioElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("paused"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement()) {
                if (nd->asElement()->isHTMLElement()) {
                    if (nd->asElement()->asHTMLElement()->isHTMLAudioElement()) {
                        return escargot::ESValue(nd->asElement()->asHTMLElement()->asHTMLAudioElement()->paused());
                    }
                }
            }
            THROW_ILLEGAL_INVOCATION();
            RELEASE_ASSERT_NOT_REACHED();
        },
        NULL, true, true, false);
#endif


#ifdef TIZEN_DEVICE_API
    fetchData(this)->m_deviceAPIObject = new DeviceAPI::NativePluginManager(fetchData(this)->m_instance);
#endif

#ifdef TIZEN_DEVICE_API
    {
        escargot::ESValue loadFunction = fetchData(this)->m_instance->globalObject()->get(escargot::ESString::create("load"));
        escargot::ESValue pathValue = escargot::ESString::create("/usr/lib/webapi-plugins/include.js");

        ScriptValue result;
        escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();

        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            result = escargot::ESFunctionObject::call(instance, loadFunction, escargot::ESValue(), &pathValue, 1, false);
            instance->unregisterTryPos(&tryPosition);
            STARFISH_LOG_INFO("Tizen device api load ok");
        } else {
            result = instance->getCatchedError();
            STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
        }
    }

#endif
}

#define IMPL_EMPTY_BINDING(codeName, exportName, fromCodeName) \
escargot::ESFunctionObject* binding##exportName(ScriptBindingInstance* scriptBindingInstance) \
{ \
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(exportName, fetchData(scriptBindingInstance)->fromCodeName()); \
    return exportName##Function; \
}

IMPL_EMPTY_BINDING(htmlDocument, HTMLDocument, document);
IMPL_EMPTY_BINDING(comment, Comment, characterData);
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

    NodeFunction->defineAccessorProperty(escargot::ESString::create("ELEMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::ELEMENT_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("ATTRIBUTE_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::ATTRIBUTE_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("TEXT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::TEXT_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("CDATA_SECTION_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::CDATA_SECTION_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("ENTITY_REFERENCE_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::ENTITY_REFERENCE_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("ENTITY_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::ENTITY_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("PROCESSING_INSTRUCTION_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::PROCESSING_INSTRUCTION_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("COMMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::COMMENT_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::DOCUMENT_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_TYPE_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::DOCUMENT_TYPE_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("DOCUMENT_FRAGMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::DOCUMENT_FRAGMENT_NODE);
            },
            NULL, false, false, false);

    NodeFunction->defineAccessorProperty(escargot::ESString::create("NOTATION_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            escargot::ESValue v = originalObj;
            return escargot::ESValue(Node::NOTATION_NODE);
            },
            NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ELEMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::ELEMENT_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ATTRIBUTE_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::ATTRIBUTE_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("TEXT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::TEXT_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("CDATA_SECTION_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::CDATA_SECTION_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ENTITY_REFERENCE_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::ENTITY_REFERENCE_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ENTITY_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::ENTITY_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("PROCESSING_INSTRUCTION_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::PROCESSING_INSTRUCTION_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("COMMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::COMMENT_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_TYPE_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_TYPE_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("DOCUMENT_FRAGMENT_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::DOCUMENT_FRAGMENT_NODE);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("NOTATION_NODE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Node::NOTATION_NODE);
        },
        NULL, false, false, false);

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

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("cloneNode"), false, false, false,
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
        }, escargot::ESString::create("cloneNode"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("hasChildNodes"), false, false, false,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            bool result = obj->hasChildNodes();
            return escargot::ESValue(result);
        }, escargot::ESString::create("hasChildNodes"), 0, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("isEqualNode"), false, false, false,
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

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("compareDocumentPosition"), false, false, false,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            Node* nodeRef = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
            unsigned short pos = obj->compareDocumentPosition(nodeRef);
            return escargot::ESValue(pos);
        }, escargot::ESString::create("compareDocumentPosition"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("contains"), false, false, false,
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

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("appendChild"), false, false, false, appendChildFunction);

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeChild"), false, false, false,
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

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("replaceChild"), false, false, false,
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

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("insertBefore"), false, false, false,
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
            // FIXME(JMP): We have to fix this to follow DOM spec after implementing Namespace
            return toJSString(nd->asElement()->localName()->toUpper());
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
                QualifiedName name = QualifiedName::fromString(sf, argValue.asESString()->utf8Data());
                Element* elem = ((Node*)thisValue.asESPointer()->asESObject()->extraPointerData())->asElement();
                size_t idx = elem->hasAttribute(name);
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

                if (key.isESString() && val.isESString()) {
                    auto sf = ((Window*)instance->globalObject()->extraPointerData())->starFish();
                    // Validate key string
                    QualifiedName attrKey = QualifiedName::fromString(sf, key.asESString()->utf8Data());
                    if (!QualifiedName::checkNameProductionRule(attrKey.string(), attrKey.string()->length()))
                        throw new DOMException(sf->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);

                    String* attrVal = toBrowserString(val.asESString());
                    Element* elem = ((Node*)nd.asESPointer()->asESObject()->extraPointerData())->asElement();
                    if (elem) {
                        elem->setAttribute(attrKey, attrVal);
                    }
                }
                return escargot::ESValue(escargot::ESValue::ESNull);
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("setAttribute"), 0, false)
    );

    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeAttribute"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            try {
                escargot::ESValue nd = instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(nd, ScriptWrappable::Type::NodeObject);

                escargot::ESValue key = instance->currentExecutionContext()->readArgument(0);

                if (key.isESString()) {
                    auto sf = ((Window*)instance->globalObject()->extraPointerData())->starFish();
                    QualifiedName attrKey = QualifiedName::fromString(sf, key.asESString()->utf8Data());
                    Element* elem = ((Node*)nd.asESPointer()->asESObject()->extraPointerData())->asElement();
                    if (elem) {
                        elem->removeAttribute(attrKey);
                    }
                }
                return escargot::ESValue(escargot::ESValue::ESNull);
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
                    HTMLCollection* result = elem->getElementsByTagName(toBrowserString(argStr));
                    if (result) {
                        return result->scriptValue();
                    }
                }
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
                    QualifiedName name = QualifiedName::fromString(sf, argValue.asESString()->utf8Data());
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
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(Text, fetchData(scriptBindingInstance)->characterData());

    /* 4.10 Interface Text */

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
            return escargot::ESString::create("UTF-8");
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("charset"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return escargot::ESString::create("UTF-8");
        }
        THROW_ILLEGAL_INVOCATION();
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("compatMode"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isDocument()) {
            return escargot::ESString::create("CSS1Compat");
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
                    QualifiedName name = QualifiedName::fromString(doc->window()->starFish(), "undefined");
                    Element* elem = doc->createElement(name);
                    if (elem != nullptr)
                        return elem->scriptValue();
                } else if (argValue.isNull()) {
                    QualifiedName name = QualifiedName::fromString(doc->window()->starFish(), "null");
                    Element* elem = doc->createElement(name);
                    if (elem != nullptr)
                        return elem->scriptValue();
                } else if (argValue.isESString()) {
                    escargot::ESString* argStr = argValue.asESString();
                    auto bStr = toBrowserString(argStr);
                    if (!QualifiedName::checkNameProductionRule(bStr, bStr->length()))
                        throw new DOMException(doc->window()->starFish()->scriptBindingInstance(), DOMException::Code::INVALID_CHARACTER_ERR, nullptr);
                    QualifiedName name = QualifiedName::fromString(doc->window()->starFish(), argStr->utf8Data());
                    Element* elem = doc->createElement(name);
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
                HTMLCollection* result = doc->getElementsByTagName(toBrowserString(argStr));
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
                    Attr* result = doc->createAttribute(QualifiedName::fromString(doc->window()->starFish(), "undefined"));
                    if (result != nullptr)
                        return result->scriptValue();
                } else if (argValue.isNull()) {
                    Attr* result = doc->createAttribute(QualifiedName::fromString(doc->window()->starFish(), "null"));
                    if (result != nullptr)
                        return result->scriptValue();
                } else if (argValue.isESString()) {
                    escargot::ESString* argStr = argValue.asESString();
                    Attr* result = doc->createAttribute(QualifiedName::fromString(doc->window()->starFish(), argStr->utf8Data()));
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
        if (nd->isElement() && nd->asElement()->asHTMLElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_dir));
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->asHTMLElement()) {
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
            return element->onclick();
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            if (v.isESPointer() && v.asESPointer()->isESFunctionObject()) {
                element->setOnclick(v);
            } else {
                element->clearOnClick();
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    escargot::ESFunctionObject* clickFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* obj = originalObj;
        String* eventType = obj->document()->window()->starFish()->staticStrings()->m_click.string();
        Event* e = new Event(eventType, EventInit(true, true));
        obj->dispatchEvent(e);
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("click"), 1, false);
    HTMLElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("click"), true, true, true, clickFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onload"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            return element->onload();
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            if (v.isESPointer() && v.asESPointer()->isESFunctionObject()) {
                element->setOnload(v);
            } else {
                element->clearOnload();
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
            return element->onunload();
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement()) {
            auto element = nd->asElement()->asHTMLElement();
            if (v.isESPointer() && v.asESPointer()->isESFunctionObject()) {
                element->setOnunload(v);
            } else {
                element->clearOnunload();
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    return HTMLElementFunction;
}

escargot::ESFunctionObject* bindingHTMLBodyElement(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(HTMLBodyElement, fetchData(scriptBindingInstance)->htmlElement());
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
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_src));
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

    // TODO : Implement setter
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("charset"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            QualifiedName name = QualifiedName::fromString(nd->document()->window()->starFish(), "charset");
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
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_href));
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
                    int width = nd->asElement()->asHTMLElement()->asHTMLImageElement()->width();
                    if (width >= 0) {
                        return escargot::ESValue(width);
                    } else {
                        return escargot::ESValue(escargot::ESValue::ESNull);
                    }
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
                    int height = nd->asElement()->asHTMLElement()->asHTMLImageElement()->height();
                    if (height >= 0) {
                        return escargot::ESValue(height);
                    } else {
                        return escargot::ESValue(escargot::ESValue::ESNull);
                    }
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

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        escargot::ESString* argStr = argValue.toString();
        Element* elem = ((HTMLCollection*) thisValue.asESPointer()->asESObject()->extraPointerData())->namedItem(toBrowserString(argStr));
        if (elem != nullptr)
            return elem->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESNull);
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
            escargot::ESString* type;
            if (firstArg.isESString()) {
                type = firstArg.asESString();
            } else if (firstArg.isNumber()) {
                type = escargot::ESString::create(firstArg.asNumber());
            } else if (firstArg.isBoolean()) {
                type = firstArg.asBoolean() ? escargot::ESString::create("true") : escargot::ESString::create("false");
            } else if (firstArg.isObject()) {
                type = escargot::ESString::create("[object Object]");
            } else if (firstArg.isNull()) {
                type = escargot::ESString::create("null");
            } else if (firstArg.isUndefined()) {
                type = escargot::ESString::create("undefined");
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            auto event = new Event(String::fromUTF8(type->utf8Data()));
            return event->scriptValue();
        } else {
            if (secondArg.isObject() || secondArg.isUndefinedOrNull()) {
                escargot::ESString* type;
                if (firstArg.isESString()) {
                    type = firstArg.asESString();
                } else if (firstArg.isNumber()) {
                    type = escargot::ESString::create(firstArg.asNumber());
                } else if (firstArg.isBoolean()) {
                    type = firstArg.asBoolean() ? escargot::ESString::create("true") : escargot::ESString::create("false");
                } else if (firstArg.isObject()) {
                    type = escargot::ESString::create("[object Object]");
                } else if (firstArg.isNull()) {
                    type = escargot::ESString::create("null");
                } else if (firstArg.isUndefined()) {
                    type = escargot::ESString::create("undefined");
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                bool canBubbles = false;
                bool canCancelable = false;
                if (!secondArg.isUndefinedOrNull()) {
                    escargot::ESValue bubbles = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("bubbles"));
                    escargot::ESValue cancelable = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("cancelable"));
                    canBubbles = bubbles.isBoolean() ? bubbles.asBoolean() : canBubbles;
                    canCancelable = cancelable.isBoolean() ? cancelable.asBoolean() : canCancelable;
                }

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
        String* type = const_cast<String*>(originalObj->type());
        return toJSString(type);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("target"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        EventTarget* target = originalObj->target();
        if (target && (target->isNode() || target->isWindow())) {
            return target->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("currentTarget"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        EventTarget* currentTarget = originalObj->currentTarget();
        if (currentTarget && (currentTarget->isNode() || currentTarget->isWindow())) {
            return currentTarget->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

    eventFunction->defineAccessorProperty(escargot::ESString::create("NONE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            return escargot::ESValue(Event::NONE);
            },
            NULL, false, false, false);

    eventFunction->defineAccessorProperty(escargot::ESString::create("CAPTURING_PHASE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            return escargot::ESValue(Event::CAPTURING_PHASE);
            },
            NULL, false, false, false);

    eventFunction->defineAccessorProperty(escargot::ESString::create("AT_TARGET"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            return escargot::ESValue(Event::AT_TARGET);
            },
            NULL, false, false, false);

    eventFunction->defineAccessorProperty(escargot::ESString::create("BUBBLING_PHASE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            return escargot::ESValue(Event::BUBBLING_PHASE);
            },
            NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("NONE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Event::NONE);
        },
        NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("CAPTURING_PHASE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Event::CAPTURING_PHASE);
        },
        NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("AT_TARGET"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Event::AT_TARGET);
        },
        NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("BUBBLING_PHASE"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return escargot::ESValue(Event::BUBBLING_PHASE);
        },
        NULL, false, false, false);

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
            escargot::ESString* type;
            if (firstArg.isESString()) {
                type = firstArg.asESString();
            } else if (firstArg.isNumber()) {
                type = escargot::ESString::create(firstArg.asNumber());
            } else if (firstArg.isBoolean()) {
                type = firstArg.asBoolean() ? escargot::ESString::create("true") : escargot::ESString::create("false");
            } else if (firstArg.isObject()) {
                type = escargot::ESString::create("[object Object]");
            } else if (firstArg.isNull()) {
                type = escargot::ESString::create("null");
            } else if (firstArg.isUndefined()) {
                type = escargot::ESString::create("undefined");
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
            auto event = new ProgressEvent(String::fromUTF8(type->utf8Data()));
            return event->scriptValue();
        } else {
            if (secondArg.isObject()) {
                escargot::ESString* type;
                if (firstArg.isESString()) {
                    type = firstArg.asESString();
                } else if (firstArg.isNumber()) {
                    type = escargot::ESString::create(firstArg.asNumber());
                } else if (firstArg.isBoolean()) {
                    type = firstArg.asBoolean() ? escargot::ESString::create("true") : escargot::ESString::create("false");
                } else if (firstArg.isObject()) {
                    type = escargot::ESString::create("[object Object]");
                } else if (firstArg.isNull()) {
                    type = escargot::ESString::create("null");
                } else if (firstArg.isUndefined()) {
                    type = escargot::ESString::create("undefined");
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }

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
            return escargot::ESValue(escargot::ESValue::ESNull);
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
            return escargot::ESValue(escargot::ESValue::ESNull);
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

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            bool res = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->contains(toBrowserString(argValue.asESString()));
            return escargot::ESValue(res);
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
    }, escargot::ESString::create("contains"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("contains"), false, false, false, domSettableTokenListContainsFunction);

    escargot::ESFunctionObject* domSettableTokenListAddFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);

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
    }, escargot::ESString::create("add"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("add"), false, false, false, domSettableTokenListAddFunction);

    escargot::ESFunctionObject* domSettableTokenListRemoveFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);

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
    }, escargot::ESString::create("remove"), 1, false);
    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, domSettableTokenListRemoveFunction);

    escargot::ESFunctionObject* domSettableTokenListToggleFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMSettableTokenListObject);

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
            Attr* elem = ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->getNamedItem(
            QualifiedName::fromString(((Window*)instance->globalObject()->extraPointerData())->starFish(), argValue.asESString()->utf8Data()));
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
                QualifiedName name = QualifiedName::fromString(((Window*)instance->globalObject()->extraPointerData())->starFish(),
                    argValue.asESString()->utf8Data());
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
        String* n = ((Attr*)originalObj)->name();
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

#define DEFINE_ACCESSOR_PROPERTY(name, nameLower, lowerCaseName)                                                                 \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(                                                                     \
        CSSStyleDeclarationFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create(#nameLower),            \
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {                                                              \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CSSStyleDeclarationObject, CSSStyleDeclaration);                     \
        String* c = originalObj->name();                                                                                         \
        STARFISH_ASSERT(c);                                                                                                      \
        return toJSString(c);                                                                                                    \
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {                                                               \
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::CSSStyleDeclarationObject, CSSStyleDeclaration);                     \
        if (v.isESString()) {                                                                                                    \
            originalObj->set##name(v.asESString()->utf8Data());                                                                  \
        } else if (v.isNumber()) {                                                                                               \
            originalObj->set##name(std::to_string(v.toNumber()).c_str());                                                        \
        }                                                                                                                        \
        return escargot::ESValue();                                                                                              \
    });
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(DEFINE_ACCESSOR_PROPERTY)

    return CSSStyleDeclarationFunction;
}

escargot::ESFunctionObject* bindingCSSStyleRule(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleRule, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    return CSSStyleRuleFunction;
}

escargot::ESFunctionObject* bindingXMLHttpRequest(ScriptBindingInstance* scriptBindingInstance)
{
    /* XMLHttpRequest */
    escargot::ESFunctionObject* xhrElementFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        auto xhr = new XMLHttpRequest();
        xhr->initScriptWrappable(xhr);
        return xhr->scriptValue();
    }, escargot::ESString::create("XMLHttpRequest"), 0, true, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(scriptBindingInstance)->m_eventTarget->protoType());
    fetchData(scriptBindingInstance)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("XMLHttpRequest"), false, false, false, xhrElementFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("responseType"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        String* c = originalObj->getResponseTypeStr();
        return toJSString(c);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("loadstart");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onloadstart"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("loadstart"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("loadstart");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onprogress"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("progress"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("progress");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onabort"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("abort"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("abort");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onerror"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("error"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("error");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onload"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("load"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("load");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("ontimeout"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("timeout"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("timeout");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onloadend"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("loadend"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("loadend");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onreadystatechange"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        escargot::ESValue c = originalObj->getHandler(String::fromUTF8("readystatechange"), originalObj->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            String* eventType = String::fromUTF8("readystatechange");
            originalObj->setHandler(eventType, v);
        }
        return escargot::ESValue();
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("timeout"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        uint32_t c = originalObj->getTimeout();
        return escargot::ESValue(c);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);

            if (v.isNumber()) {
                originalObj->setTimeout(v.toInt32());
            }
            return escargot::ESValue();
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("readyState"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        int c = originalObj->getReadyState();
        return escargot::ESValue(c);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("status"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        int c = originalObj->getStatus();
        return escargot::ESValue(c);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("statusText"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        String* c = originalObj->getStatusText();
        return toJSString(c);
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("response"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
            return originalObj->getResponse();
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("responseText"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
            String* c = originalObj->getResponseText();
            return toJSString(c);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("responseXML"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        String* c = originalObj->getResponseXML();
        return toJSString(c);
    }, nullptr);

    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("UNSENT"), false, false, false, escargot::ESValue(0));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("OPENED"), false, false, false, escargot::ESValue(1));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("HEADERS_RECEIVED"), false, false, false, escargot::ESValue(2));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("LOADING"), false, false, false, escargot::ESValue(3));
    xhrElementFunction->asESObject()->defineDataProperty(escargot::ESString::create("DONE"), false, false, false, escargot::ESValue(4));

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("UNSENT"), false, false, false, escargot::ESValue(0));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("OPENED"), false, false, false, escargot::ESValue(1));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("HEADERS_RECEIVED"), false, false, false, escargot::ESValue(2));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("LOADING"), false, false, false, escargot::ESValue(3));
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("DONE"), false, false, false, escargot::ESValue(4));

    escargot::ESFunctionObject* xhrGetResponseHeaderFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isObject()) {
            if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject && instance->currentExecutionContext()->argumentCount() == 1) {
                char* rh = ((XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData())->getResponseHeader(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data());
                if (rh)
                    toJSString(String::fromUTF8(rh));
                else
                    toJSString(String::emptyString);
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("getResponseHeader"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getResponseHeader"), false, false, false, xhrGetResponseHeaderFunction);

    escargot::ESFunctionObject* xhrGetAllResponseHeadersFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isObject()) {
            if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject) {
                XMLHttpRequest* xhr = (XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData();
                char* arh = xhr->getAllResponseHeadersStr();
                if (arh)
                    return toJSString(String::fromUTF8(xhr->getAllResponseHeadersStr()));
                else
                    return toJSString(String::emptyString);
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("getAllResponseHeaders"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getAllResponseHeaders"), false, false, false, xhrGetAllResponseHeadersFunction);

    escargot::ESFunctionObject* xhrOpenFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
            if (v.isObject()) {
                if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject) {
                    XMLHttpRequest* xhr = (XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData();
                    if (instance->currentExecutionContext()->argumentCount() == 2)
                        xhr->setOpen(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data(), String::createASCIIString(instance->currentExecutionContext()->readArgument(1).toString()->utf8Data()), true);
                    else if (instance->currentExecutionContext()->argumentCount() >= 3)
                        if (instance->currentExecutionContext()->readArgument(2).isBoolean())
                            xhr->setOpen(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data(), String::createASCIIString(instance->currentExecutionContext()->readArgument(1).toString()->utf8Data()), instance->currentExecutionContext()->readArgument(2).toBoolean());
                }
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("open"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("open"), false, false, false, xhrOpenFunction);

    escargot::ESFunctionObject* xhrSendFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
            if (v.isObject()) {
                if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject) {
                    XMLHttpRequest* xhr = (XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData();
                    escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
                    if (argValue.isESString()) {
                        escargot::ESString* argStr = argValue.asESString();
                        xhr->send(toBrowserString(argStr));
                    } else {
                        xhr->send(String::emptyString);
                    }
                }
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        } catch(DOMException* e) {
            escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }, escargot::ESString::create("send"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("send"), false, false, false, xhrSendFunction);

    escargot::ESFunctionObject* xhrAbortFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isObject()) {
            if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject) {
                ((XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData())->abort();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("abort"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("abort"), false, false, false, xhrAbortFunction);

    escargot::ESFunctionObject* xhrSetRequestHeaderFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
                if (v.isObject()) {
                    if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject && instance->currentExecutionContext()->argumentCount() == 2) {
                        ((XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData())->setRequestHeader(
                            instance->currentExecutionContext()->readArgument(0).toString()->utf8Data(),
                            instance->currentExecutionContext()->readArgument(1).toString()->utf8Data());
                    }
                }
                return escargot::ESValue(escargot::ESValue::ESNull);
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
    }, escargot::ESString::create("setRequestHeader"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("setRequestHeader"), false, false, false, xhrSetRequestHeaderFunction);

    return xhrElementFunction;
}

escargot::ESFunctionObject* bindingBlob(ScriptBindingInstance* scriptBindingInstance)
{
    /* Blob */
    DEFINE_FUNCTION(Blob, fetchData(scriptBindingInstance)->m_instance->globalObject()->objectPrototype());
    return BlobFunction;
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

    return DOMExceptionFunction;
}

void ScriptBindingInstance::evaluate(String* str)
{
    std::jmp_buf tryPosition;
    if (setjmp(fetchData(this)->m_instance->registerTryPos(&tryPosition)) == 0) {
        escargot::ESValue ret = fetchData(this)->m_instance->evaluate(toJSString(str).asESString());
        fetchData(this)->m_instance->printValue(ret);
        fetchData(this)->m_instance->unregisterTryPos(&tryPosition);
    } else {
        escargot::ESValue err = fetchData(this)->m_instance->getCatchedError();
        STARFISH_LOG_ERROR("Uncaught %s\n", err.toString()->utf8Data());
    }
}
}
