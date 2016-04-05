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
    m_data = new ScriptBindingInstanceDataEscargot();
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

#define DEFINE_FUNCTION(functionName, parentName)                                                                                                                                                                          \
    escargot::ESString* functionName##String = escargot::ESString::create(#functionName);                                                                                                                                  \
    escargot::ESFunctionObject* functionName##Function = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance*) -> escargot::ESValue      \
        {      \
            return escargot::ESValue();           \
        }, functionName##String, 0, true, false); \
    functionName##Function->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);                                                                                                                     \
    fetchData(this)->m_instance->globalObject()->defineDataProperty(functionName##String, true, false, true, functionName##Function);                                                                                    \
    functionName##Function->protoType().asESPointer()->asESObject()->set__proto__(parentName);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentName)                                                                                                                                                                          \
    escargot::ESString* functionName##String = escargot::ESString::create(#functionName);                                                                                                                                  \
    escargot::ESFunctionObject* functionName##Function = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance*) -> escargot::ESValue      \
        {      \
            escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Illegal constructor")))); \
            STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
            return escargot::ESValue();           \
        }, functionName##String, 0, true, false); \
    functionName##Function->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);                                                                                                                     \
    fetchData(this)->m_instance->globalObject()->defineDataProperty(functionName##String, true, false, true, functionName##Function);                                                                                    \
    functionName##Function->protoType().asESPointer()->asESObject()->set__proto__(parentName);
// TypeError: Illegal invocation
#define THROW_ILLEGAL_INVOCATION()                                                                                                                           \
    escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Illegal invocation")))); \
    STARFISH_RELEASE_ASSERT_NOT_REACHED();

#define CHECK_TYPEOF(thisValue, type)                                                 \
    {                                                                                 \
        escargot::ESValue v = thisValue;                                              \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() & type))) { \
            THROW_ILLEGAL_INVOCATION()                                                \
        }                                                                             \
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
    escargot::NativeFunctionType getter, escargot::NativeFunctionType setter)
{
    bool isWritable = setter;
    bool isEnumerable = true;
    bool isConfigurable = true;

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

#ifdef STARFISH_ENABLE_PIXEL_TEST
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(EventTarget, fetchData(this)->m_instance->globalObject()->objectPrototype());

    auto fnAddEventListener = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventTargetObject);

        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
        escargot::ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
        if (firstArg.isESString() && secondArg.isESPointer() && secondArg.asESPointer()->isESFunctionObject()) {
            // TODO: Verify valid event types (e.g. click)
            escargot::ESString* argStr = firstArg.asESString();
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, argStr->utf8Data());
            auto listener = new EventListener(secondArg);
            bool capture = thirdArg.isBoolean() ? thirdArg.toBoolean() : false;
            ((EventTarget *)thisValue.asESPointer()->asESObject()->extraPointerData())->addEventListener(eventTypeName, listener, capture);
        }
        return escargot::ESValue();
    }, escargot::ESString::create("addEventListener"), 0, false);
    EventTargetFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("addEventListener"), false, false, false, fnAddEventListener);

    auto fnRemoveEventListener = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventTargetObject);
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
        escargot::ESValue thirdArg = instance->currentExecutionContext()->readArgument(2);
        if (firstArg.isESString() && secondArg.isESPointer() && secondArg.asESPointer()->isESFunctionObject()) {
            // TODO: Verify valid event type. (e.g. click)
            escargot::ESString* argStr = firstArg.asESString();
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, argStr->utf8Data());
            auto listener = new EventListener(secondArg);
            bool capture = thirdArg.isBoolean() ? thirdArg.toBoolean() : false;
            ((EventTarget *)thisValue.asESPointer()->asESObject()->extraPointerData())->removeEventListener(eventTypeName, listener, capture);
        }
        return escargot::ESValue();
    }, escargot::ESString::create("removeEventListener"), 0, false);
    EventTargetFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeEventListener"), false, false, false, fnRemoveEventListener);

    auto fnDispatchEvent = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::EventTargetObject);
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        bool ret = false;
        if (argCount == 1 && firstArg.isObject()) {
            Event* event = (Event*)firstArg.asESPointer()->asESObject()->extraPointerData();
            ret = ((EventTarget*)thisValue.asESPointer()->asESObject()->extraPointerData())->dispatchEvent(event);
        }
        return escargot::ESValue(ret);
    }, escargot::ESString::create("dispatchEvent"), 1, false);
    EventTargetFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("dispatchEvent"), false, false, false, fnDispatchEvent);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Window, EventTargetFunction->protoType());
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("window"), false, false, false, fetchData(this)->m_instance->globalObject());
    fetchData(this)->m_instance->globalObject()->set__proto__(WindowFunction->protoType());
    fetchData(this)->m_window = WindowFunction;

    fetchData(this)->m_instance->globalObject()->defineAccessorProperty(escargot::ESString::create("document"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        return (((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData()))->document()->scriptObject();
        },
        NULL, false, false, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Node, EventTargetFunction->protoType());
    fetchData(this)->m_node = NodeFunction;

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
        NodeFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("baseURI"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        String* nodeName = originalObj->baseURI();
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
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
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
            CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            Node* nodeRef = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
            bool found = obj->contains(nodeRef);
            return escargot::ESValue(found);
        }, escargot::ESString::create("contains"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("lookupPrefix"), false, false, false,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            String* namespaceUri = toBrowserString(instance->currentExecutionContext()->readArgument(0).toString());
            String* ns = obj->lookupPrefix(namespaceUri);
            if (ns == nullptr) {
                return escargot::ESValue(escargot::ESValue::ESNull);
            }
            return toJSString(ns);
        }, escargot::ESString::create("lookupPrefix"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("lookupNamespaceURI"), false, false, false,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            String* prefix = toBrowserString(instance->currentExecutionContext()->readArgument(0).toString());
            String* ns = obj->lookupNamespaceURI(prefix);
            if (ns == nullptr) {
                return escargot::ESValue(escargot::ESValue::ESNull);
            }
            return toJSString(ns);
        }, escargot::ESString::create("lookupNamespaceURI"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("isDefaultNamespace"), false, false, false,
        escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            String* namespaceUri = toBrowserString(instance->currentExecutionContext()->readArgument(0).toString());
            bool ns = obj->isDefaultNamespace(namespaceUri);
            return escargot::ESValue(ns);
        }, escargot::ESString::create("isDefaultNamespace"), 1, false));

    escargot::ESFunctionObject* appendChildFunction = escargot::ESFunctionObject::create(nullptr, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        try {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
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
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Element, NodeFunction->protoType());
    fetchData(this)->m_element = ElementFunction;

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
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESUndefined);
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
                if (elem != nullptr)
                    return toJSString(elem->getAttribute(name));
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
                    QualifiedName attrKey = QualifiedName::fromString(sf, key.asESString()->utf8Data());
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
    }, nullptr);

    ElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("getElementsByClassName"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

            if (obj->isElement()) {
                Element* elem = obj->asElement();
                escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
                if (argValue.isESString()) {
                    escargot::ESString* argStr = argValue.asESString();
                    HTMLCollection* result = elem->getElementsByClassName(toBrowserString(argStr));
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DocumentType, NodeFunction->protoType());
    fetchData(this)->m_documentType = DocumentTypeFunction;

    DocumentTypeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, removeFunction);

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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Document, NodeFunction->protoType());
    fetchData(this)->m_document = DocumentFunction;

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
            HTMLBodyElement* head = document->bodyElement();
            if (head) {
                return head->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, nullptr);

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

/*
    // TODO implement this functions
    // TODO convert into defineNativeAccessorPropertyButNeedToGenerateJSFunction way
    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("URL"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESString::create("NOT supported yet!");
        },
        NULL, false, false, false);

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("documentURI"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESString::create("NOT supported yet!");
        },
        NULL, false, false, false);

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("origin"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESString::create("NOT supported yet!");
        },
        NULL, false, false, false);

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("compatMode"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESString::create("NOT supported yet!");
        },
        NULL, false, false, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("inputEncoding"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESString::create("NOT supported yet!");
        },
        NULL, false, false, false);

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("contentType"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESString::create("NOT supported yet!");
        },
        NULL, false, false, false);
*/
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
            if (argValue.isESString()) {
                escargot::ESString* argStr = argValue.asESString();
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLDocument, DocumentFunction->protoType());
    fetchData(this)->m_htmlDocument = HTMLDocumentFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CharacterData, NodeFunction->protoType());
    fetchData(this)->m_characterData = CharacterDataFunction;

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

    CharacterDataFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, removeFunction);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Text, CharacterDataFunction->protoType());
    fetchData(this)->m_text = TextFunction;

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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Comment, CharacterDataFunction->protoType());
    fetchData(this)->m_comment = CommentFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLElement, ElementFunction->protoType());
    fetchData(this)->m_htmlElement = HTMLElementFunction;

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

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("onload"),
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
                element->setOnload(v);
            } else {
                element->clearOnunload();
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        return escargot::ESValue();
    });

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLHtmlElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlHtmlElement = HTMLHtmlElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLHeadElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlHeadElement = HTMLHeadElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLScriptElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlScriptElement = HTMLScriptElementFunction;


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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLStyleElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlStyleElement = HTMLStyleElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLLinkElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlLinkElement = HTMLLinkElementFunction;

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("href"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_href));
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
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
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
            return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_rel));
        }
        THROW_ILLEGAL_INVOCATION();
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
        Node* nd = originalObj;
        if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLBodyElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlBodyElement = HTMLBodyElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLDivElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlDivElement = HTMLDivElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLImageElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlImageElement = HTMLImageElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLMetaElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlMetaElement = HTMLMetaElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLParagraphElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlParagraphElement = HTMLParagraphElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLSpanElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlSpanElement = HTMLSpanElementFunction;

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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLBRElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlBrElement = HTMLBRElementFunction;

#ifdef STARFISH_ENABLE_AUDIO
    // TODO convert into defineNativeAccessorPropertyButNeedToGenerateJSFunction way
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLAudioElement, HTMLElementFunction->protoType());
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLCollection, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_htmlCollection = HTMLCollectionFunction;

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
            uint32_t idx = argValue.toIndex();
            idx = (idx != escargot::ESValue::ESInvalidIndexValue) ? idx : 0;
            if (idx < self->length()) {
                Element* elem = self->item(idx);
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
        if (argValue.isESString()) {
            Element* elem = ((HTMLCollection*) thisValue.asESPointer()->asESObject()->extraPointerData())->namedItem(toBrowserString(argValue.asESString()));
            if (elem != nullptr)
                return elem->scriptValue();
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("namedItem"), 1, false);
    HTMLCollectionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("namedItem"), true, true, true, namedItemFunction);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NodeList, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_nodeList = NodeListFunction;

    NodeListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeListObject);
            NodeList* nodeList = (NodeList*)thisValue.asESPointer()->asESObject()->extraPointerData();

            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            uint32_t idx = argValue.toIndex();
            if (idx < nodeList->length()) {
                Node* nd = nodeList->item(argValue.asUInt32());
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMTokenList, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_domTokenList = DOMTokenListFunction;

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
        uint32_t idx = argValue.toIndex();
        if (idx < self->length()) {
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
            escargot::ESString* argStr = nullptr;
            if (argValue.isESString()) {
                argStr = argValue.asESString();
            } else {
                argStr = argValue.toString();
            }
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
                escargot::ESString* argStr = nullptr;
                if (argValue.isESString()) {
                    argStr = argValue.asESString();
                } else {
                    argStr = argValue.toString();
                }
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
                escargot::ESString* argStr = nullptr;
                if (argValue.isESString()) {
                    argStr = argValue.asESString();
                } else {
                    argStr = argValue.toString();
                }
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
            if (argCount >= 2)
                forceValue = instance->currentExecutionContext()->readArgument(1);
            if (argCount > 0) {
                escargot::ESString* argStr = nullptr;
                if (argValue.isESString()) {
                    argStr = argValue.asESString();
                } else {
                    argStr = argValue.toString();
                }
                bool didAdd;
                if (argCount == 1) {
                    didAdd = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toggle(toBrowserString(argStr), false, false);
                } else {
                    ASSERT(forceValue.isBoolean());
                    didAdd = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->toggle(toBrowserString(argStr), true, forceValue.asBoolean());
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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMSettableTokenList, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_domSettableTokenList = DOMSettableTokenListFunction;

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

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isUInt32()) {
            String* elem = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->item(argValue.asUInt32());
            if (elem != nullptr)
                return toJSString(elem);
        } else {
            THROW_ILLEGAL_INVOCATION()
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


    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NamedNodeMap, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_namedNodeMap = NamedNodeMapFunction;

    /* 4.8.1 Interface NamedNodeMap */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NamedNodeMapFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("NamedNodeMapFunction"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NamedNodeMapObject, NamedNodeMap);
        uint32_t len = originalObj->length();
        return escargot::ESValue(len);
    }, nullptr);

    escargot::ESFunctionObject* NamedNodeMapItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NamedNodeMapObject);

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isUInt32()) {
            Attr* elem = ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->item(argValue.asUInt32());
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
            QualifiedName::fromString(((NamedNodeMap*) thisValue.asESPointer()->asESObject())->element()->document()->window()->starFish(), argValue.asESString()->utf8Data()));
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

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        CHECK_TYPEOF(argValue, ScriptWrappable::Type::AttrObject);

        Attr* old = ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->getNamedItem(((Attr*) argValue.asESPointer()->asESObject())->name());
        Attr* toReturn = new Attr(old->document(), ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->striptBindingInstance(), old->name(), old->value());
        ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->setNamedItem((Attr*) argValue.asESPointer()->asESObject());
        if (toReturn != nullptr)
            return toReturn->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("setNamedItem"), 1, false);
    NamedNodeMapFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("setNamedItem"), false, false, false, NamedNodeMapSetNamedItemFunction);

    escargot::ESFunctionObject* NamedNodeMapRemoveNamedItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NamedNodeMapObject);

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            QualifiedName name = QualifiedName::fromString(((NamedNodeMap*) thisValue.asESPointer()->asESObject())->element()->document()->window()->starFish(),
                argValue.asESString()->utf8Data());
            Attr* old = ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->getNamedItem(name);
            Attr* toReturn = new Attr(old->document(), ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->striptBindingInstance(), name, old->value());
            ((NamedNodeMap*) thisValue.asESPointer()->asESObject()->extraPointerData())->removeNamedItem(name);
            if (toReturn != nullptr)
                return toReturn->scriptValue();
        } else
            THROW_ILLEGAL_INVOCATION()
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("removeNamedItem"), 1, false);
    NamedNodeMapFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("removeNamedItem"), false, false, false, NamedNodeMapRemoveNamedItemFunction);

    /* 4.8.2 Interface Attr */
    // FIXME Attr should inherit interface Node
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Attr, NodeFunction->protoType());
    fetchData(this)->m_attr = AttrFunction;

    auto attrNameValueGetter = [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::AttrObject, Attr);
        String* n = originalObj->name();
        return toJSString(n);
    };

    auto attrValueGetter = [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::AttrObject, Attr);
        String* value = originalObj->value();
        return toJSString(value);
    };

    auto attrValueSetter = [](escargot::ESVMInstance* instance) -> escargot::ESValue
    {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::AttrObject, Attr);
        originalObj->setValue(toBrowserString(v));
        // FIXME(JMP): Actually this function have to return old Attr's value but we have to modify 'typedef void (*ESNativeSetter)(...)' in escargot/src/runtime/ESValue.h
        // Because this need to many changes, we do the modification latter
        return escargot::ESValue();
    };

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("name"), attrNameValueGetter, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nodeName"), attrNameValueGetter, nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("value"), attrValueGetter, attrValueSetter);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("nodeValue"), attrValueGetter, attrValueSetter);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(AttrFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("textContent"), attrValueGetter, attrValueSetter);

    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ownerElement"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::AttrObject);

        Element* elem = ((Attr*) originalObj->extraPointerData())->ownerElement();
        if (elem != nullptr)
            return elem->scriptValue();
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        NULL, false, false, false);

    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("specified"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::AttrObject);
        return escargot::ESValue(true);
        },
        NULL, false, false, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLUnknownElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlUnknownElement = HTMLUnknownElementFunction;

    /* 3.2 Interface Event */
    auto eventFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESValue firstArg = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue secondArg = instance->currentExecutionContext()->readArgument(1);
        auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();

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
            }
            auto event = new Event(QualifiedName::fromString(sf, type->utf8Data()));
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
                }
                escargot::ESValue bubbles = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("bubbles"));
                escargot::ESValue cancelable = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("cancelable"));
                bool canBubbles = bubbles.isBoolean() ? bubbles.asBoolean() : false;
                bool canCancelable = cancelable.isBoolean() ? cancelable.asBoolean() : false;
                auto event = new Event(QualifiedName::fromString(sf, type->utf8Data()), EventInit(canBubbles, canCancelable));
                return event->scriptValue();
            } else {
                escargot::ESString* msg = escargot::ESString::create("Failed to construct 'Event': parameter 2 ('eventInitDict') is not an object.");
                instance->throwError(escargot::ESValue(escargot::TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }, escargot::ESString::create("Event"), 1, true, false);
    eventFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    eventFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("Event"), true, false, true, eventFunction);
    fetchData(this)->m_event = eventFunction;

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        eventFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("type"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::EventObject, Event);
        String* type = originalObj->type().string();
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

    /* UI Events */
    DEFINE_FUNCTION(UIEvent, eventFunction->protoType());
    fetchData(this)->m_uiEvent = UIEventFunction;

    /* Mouse Events */
    DEFINE_FUNCTION(MouseEvent, UIEventFunction->protoType());
    fetchData(this)->m_mouseEvent = MouseEventFunction;

    /* Progress Events */
    DEFINE_FUNCTION(ProgressEvent, eventFunction->protoType());
    fetchData(this)->m_progressEvent = ProgressEventFunction;

    /* style-related getter/setter start here */

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleDeclaration, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_cssStyleDeclaration = CSSStyleDeclarationFunction;

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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleRule, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_cssStyleRule = CSSStyleRuleFunction;

    /* XMLHttpRequest */
    escargot::ESFunctionObject* xhrElementFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        auto xhr = new XMLHttpRequest();
        xhr->initScriptWrappable(xhr);
        return xhr->scriptValue();
    }, escargot::ESString::create("XMLHttpRequest"), 0, true, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->set__proto__(EventTargetFunction->protoType());
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("XMLHttpRequest"), false, false, false, xhrElementFunction);
    fetchData(this)->m_xhrElement = xhrElementFunction;

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        xhrElementFunction->protoType().asESPointer()->asESObject(), escargot::ESString::create("responseType"),
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        String* c = originalObj->getResponseTypeStr();
        return toJSString(c);
    }, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "loadstart");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "loadstart");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "progress");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "abort");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "error");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "load");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "timeout");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "loadend");
            originalObj->setHandler(eventTypeName, v);
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
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "readystatechange");
            originalObj->setHandler(eventTypeName, v);
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
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::XMLHttpRequestObject, XMLHttpRequest);
        if (v.isNumber()) {
            originalObj->setTimeout(v.toInt32());
        }
        return escargot::ESValue();
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
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isObject()) {
            if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject) {
                XMLHttpRequest* xhr = (XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData();
                xhr->setOpen(instance->currentExecutionContext()->readArgument(0).toString()->utf8Data(), String::createASCIIString(instance->currentExecutionContext()->readArgument(1).toString()->utf8Data()));
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
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


    /* Blob */
    DEFINE_FUNCTION(Blob, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_blobElement = BlobFunction;

    /* DOM Exception */
    DEFINE_FUNCTION(DOMException, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_domException = DOMExceptionFunction;
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(this)->m_instance->globalObject()->errorPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(), fetchData(this)->m_instance->strings().name,
        [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::DOMExceptionObject, DOMException);
        return escargot::ESString::create(originalObj->name());
    }, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMExceptionFunction->protoType().asESPointer()->asESObject(), fetchData(this)->m_instance->strings().message,
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

#ifdef TIZEN_DEVICE_API
    fetchData(this)->m_deviceAPIObject = new DeviceAPI::NativePluginManager(fetchData(this)->m_instance);
#endif

#ifdef TIZEN_DEVICE_API
    {
        escargot::ESValue loadFunction = fetchData(this)->m_instance->globalObject()->get(escargot::ESString::create("load"));
        std::string path = "/opt/usr/apps/";
        path += getenv("PACKAGE_NAME");
        path += "/shared/test/deviceapi/";

        std::string prefix = path;
        path += "include.js";
        fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("prefix"), false, false, false, escargot::ESString::create(prefix.data()));
        escargot::ESValue pathValue = escargot::ESString::create(path.data());

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

void ScriptBindingInstance::evaluate(String* str)
{
    std::jmp_buf tryPosition;
    if (setjmp(fetchData(this)->m_instance->registerTryPos(&tryPosition)) == 0) {
        escargot::ESValue ret = fetchData(this)->m_instance->evaluate(toJSString(str).asESString());
        fetchData(this)->m_instance->printValue(ret);
        fetchData(this)->m_instance->unregisterTryPos(&tryPosition);
    } else {
        escargot::ESValue err = fetchData(this)->m_instance->getCatchedError();
        printf("Uncaught %s\n", err.toString()->utf8Data());
    }
}
}
