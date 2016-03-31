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
    fetchData(this)->m_instance->globalObject()->defineDataProperty(functionName##String, false, false, false, functionName##Function);                                                                                    \
    functionName##Function->protoType().asESPointer()->asESObject()->set__proto__(parentName);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentName)                                                                                                                                                                          \
    escargot::ESString* functionName##String = escargot::ESString::create(#functionName);                                                                                                                                  \
    escargot::ESFunctionObject* functionName##Function = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance*) -> escargot::ESValue      \
        {      \
            escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Illegal constructor")))); \
            STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
            if (strcmp(#functionName, "Document") == 0) { \
                escargot::ESVMInstance::currentInstance()->throwError(escargot::ESValue(escargot::TypeError::create(escargot::ESString::create("Illegal constructor")))); \
                STARFISH_RELEASE_ASSERT_NOT_REACHED(); \
            } \
            return escargot::ESValue();           \
        }, functionName##String, 0, true, false); \
    functionName##Function->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);                                                                                                                     \
    fetchData(this)->m_instance->globalObject()->defineDataProperty(functionName##String, false, false, false, functionName##Function);                                                                                    \
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

String* toBrowserString(const escargot::ESValue& v)
{
    escargot::NullableUTF8String s = v.toString()->toNullableUTF8String();
    String* newStr = String::fromUTF8(s.m_buffer, s.m_bufferSize);
    // NOTE: input string contains whitecharacters as is, i.e., "\n" is stored as '\','n'
    // The right way is, input string should already have '\n', and white spaces should be removed from here.
    // For time being, we simply remove "\n" and other whitespaces strings.
    newStr = newStr->replaceAll(String::fromUTF8("\n"), String::spaceString);
    newStr = newStr->replaceAll(String::fromUTF8("\t"), String::spaceString);
    newStr = newStr->replaceAll(String::fromUTF8("\f"), String::spaceString);
    newStr = newStr->replaceAll(String::fromUTF8("\r"), String::spaceString);
    return newStr;
}

escargot::ESValue toJSString(String* v)
{
    return createScriptString(v);
}

void ScriptBindingInstance::initBinding(StarFish* sf)
{

    escargot::ESValue v;

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
        if (firstArg.isESString() && secondArg.asESPointer() && secondArg.asESPointer()->isESFunctionObject()) {
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
        if (firstArg.isESString() && secondArg.asESPointer() && secondArg.asESPointer()->isESFunctionObject()) {
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

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("nodeType"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        unsigned short nodeType = ((Node *)originalObj->extraPointerData())->nodeType();
        return escargot::ESValue(nodeType);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("nodeName"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        String* nodeName = ((Node *)originalObj->extraPointerData())->nodeName();
        return toJSString(nodeName);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("baseURI"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        String* uri = ((Node *)originalObj->extraPointerData())->baseURI();
        return toJSString(uri);
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ownerDocument"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Document* doc = ((Node *)originalObj->extraPointerData())->ownerDocument();
        if (doc == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return doc->scriptValue();
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("parentNode"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* p = ((Node *)originalObj->extraPointerData())->parentNode();
        if (p == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return p->scriptValue();
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("parentElement"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Element* p = ((Node *)originalObj->extraPointerData())->parentElement();
        if (p == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return p->scriptValue();
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("childNodes"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        NodeList* list = ((Node *)originalObj->extraPointerData())->childNodes();
        STARFISH_ASSERT(list);
        return list->scriptValue();
        },
        NULL, false, false, false);

    escargot::ESString* nextSiblingString = escargot::ESString::create("nextSibling");
    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(nextSiblingString,
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->nextSibling();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
        },
        NULL, false, false, false);

    escargot::ESString* prevSiblingString = escargot::ESString::create("previousSibling");
    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(prevSiblingString,
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->previousSibling();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("firstChild"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->firstChild();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
        },
        NULL, false, false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("lastChild"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->lastChild();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
        },
        NULL, false, false, false);

    auto nodeValueGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        String* s = ((Node *)originalObj->extraPointerData())->nodeValue();
        if (s == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return toJSString(s);
    };
    auto nodeValueSetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* node = (Node*)originalObj->extraPointerData();
        if (v.isESString()) {
            node->setNodeValue(toBrowserString(v));
        } else if (v.isNull()) {
            node->setNodeValue(nullptr);
        }
    };
    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("nodeValue"), nodeValueGetter, nodeValueSetter, false, false, false);

    auto textContentGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        String* s = ((Node *)originalObj->extraPointerData())->textContent();
        if (s == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        return toJSString(s);
    };
    auto textContentSetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        ((Node *)originalObj->extraPointerData())->setTextContent(toBrowserString(v));
    };
    NodeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("textContent"), textContentGetter, textContentSetter, false, false, false);

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
            CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
            Node* node = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
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
                CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), ScriptWrappable::Type::NodeObject);
                CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(1), ScriptWrappable::Type::NodeObject);
                Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
                Node* node = (Node*)instance->currentExecutionContext()->readArgument(0).asESPointer()->asESObject()->extraPointerData();
                Node* child = (Node*)instance->currentExecutionContext()->readArgument(1).asESPointer()->asESObject()->extraPointerData();
                Node* n = obj->insertBefore(node, child);
                return n->scriptValue();
            } catch(DOMException* e) {
                escargot::ESVMInstance::currentInstance()->throwError(e->scriptValue());
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }, escargot::ESString::create("insertBefore"), 2, false));

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Element, NodeFunction->protoType());
    fetchData(this)->m_element = ElementFunction;

    /* 4.8 Interface Element */

    auto firstElementChildGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->firstElementChild();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    };

    auto lastElementChildGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->lastElementChild();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    };

    auto nextElementChildGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->nextElementSibling();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    };

    auto previousElementChildGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData())->previousElementSibling();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESNull);
        return nd->scriptValue();
    };

    auto childElementCountGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESValue(((Node *)originalObj->extraPointerData())->childElementCount());
    };

    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("firstElementChild"),
        firstElementChildGetter, NULL, false, false, false);
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("lastElementChild"),
        lastElementChildGetter, NULL, false, false, false);
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("nextElementSibling"),
        nextElementChildGetter, NULL, false, false, false);
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("previousElementSibling"),
        previousElementChildGetter, NULL, false, false, false);
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("childElementCount"),
        childElementCountGetter, NULL, false, false, false);

    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("localName"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData());
        if (nd->isElement()) {
            return toJSString(nd->asElement()->localName());
        } else {
            THROW_ILLEGAL_INVOCATION();
        }
        },
        NULL, true, true, true);

    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("tagName"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)originalObj->extraPointerData());
            if (nd->isElement()) {
                // FIXME(JMP): We have to fix this to follow DOM spec after implementing Namespace
                return toJSString(nd->asElement()->localName()->toUpper());
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        NULL, true, true, true);

    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("id"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)originalObj->extraPointerData());
            if (nd->isElement()) {
                return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_id));
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)originalObj->extraPointerData());
            if (nd->isElement()) {
                nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_id, toBrowserString(v));
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        true, true, true);

    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("className"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)originalObj->extraPointerData());
            if (nd->isElement()) {
                return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_class));
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)originalObj->extraPointerData());
            if (nd->isElement()) {
                nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_class, toBrowserString(v));
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        true, true, true);

    auto childrenGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        HTMLCollection* nd = ((Node *)originalObj->extraPointerData())->children();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        return nd->scriptValue();
    };
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("children"), childrenGetter, NULL, false, false, false);

    auto classListGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        DOMTokenList* nd = ((Node *)originalObj->extraPointerData())->classList();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        return nd->scriptValue();
    };
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("classList"), classListGetter, NULL, false, false, false);

    auto attributesGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        NamedNodeMap* nd = ((Node *)originalObj->extraPointerData())->attributes();
        if (nd == nullptr)
            return escargot::ESValue(escargot::ESValue::ESUndefined);
        return nd->scriptValue();
    };
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("attributes"), attributesGetter, NULL, false, false, false);

    escargot::ESFunctionObject* removeFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
        Node* obj = (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        obj->remove();
        return escargot::ESValue(escargot::ESValue::ESNull);
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

    auto styleGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        if (!((Node*)originalObj->extraPointerData())->isElement()) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        }
        CSSStyleDeclaration* s = ((Element *)originalObj->extraPointerData())->inlineStyle();
        if (s == nullptr) {
            return escargot::ESValue(escargot::ESValue::ESNull);
        } else {
            return s->scriptValue();
        }
    };
    ElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("style"), styleGetter, NULL, false, false, false);

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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DocumentType, NodeFunction->protoType());
    fetchData(this)->m_documentType = DocumentTypeFunction;

    DocumentTypeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, removeFunction);

    /* 4.7 Interface DocumentType */

    DocumentTypeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("name"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        String* s = ((Node *)originalObj->extraPointerData())->nodeName();
        return toJSString(s);
        },
        NULL, false, false, false);

    DocumentTypeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("publicId"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData());
        if (!nd->isDocumentType()) {
            THROW_ILLEGAL_INVOCATION();
        }
        String* s = nd->asDocumentType()->publicId();
        return toJSString(s);
        },
        NULL, false, false, false);

    DocumentTypeFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("systemId"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData());
        if (!nd->isDocumentType()) {
            THROW_ILLEGAL_INVOCATION();
        }
        String* s = nd->asDocumentType()->publicId();
        return toJSString(s);
        },
        NULL, false, false, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Document, NodeFunction->protoType());
    fetchData(this)->m_document = DocumentFunction;

    /* 4.5 Interface Document */

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("head"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isDocument()) {
                Document* document = nd->asDocument();
                HTMLHeadElement* head = document->headElement();
                if (head) {
                    return head->scriptValue();
                }
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, NULL, false, false, false);

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("body"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
        if (nd->isDocument()) {
            Document* document = nd->asDocument();
            HTMLBodyElement* body = document->bodyElement();
            if (body) {
                return body->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, NULL, false, false, false);


    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("documentElement"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = (Node *)originalObj->extraPointerData();
        if (nd->isDocument()) {
            Document* document = nd->asDocument();
            Element* docElem = document->documentElement();
            if (docElem) {
                return docElem->scriptValue();
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, NULL, false, false, false);

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

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("characterSet"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        return escargot::ESString::create("NOT supported yet!");
        },
        NULL, false, false, false);

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("charset"),
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

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("firstElementChild"),
        firstElementChildGetter, NULL, false, false, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("lastElementChild"),
        lastElementChildGetter, NULL, false, false, false);
    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("childElementCount"),
        firstElementChildGetter, NULL, false, false, false);

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

    auto doctypeGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData());
        if (nd->isDocument()) {
            Node* docTypeNode = nd->firstChild();
            while (docTypeNode) {
                if (docTypeNode->isDocumentType()) {
                    return docTypeNode->scriptValue();
                } else {
                    docTypeNode = docTypeNode->nextSibling();
                }
            }
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    };
    auto doctypeSetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::DOMSettableTokenListObject);
        // Spec says do nothing
    };
    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("doctype"), doctypeGetter, doctypeSetter, false, true, true);

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
                for (int i = 0; array->length(); i++) {
                    escargot::ESValue val = array->get(i);
                    if (val.isESString()) {
                        listSoFar = listSoFar->concat(toBrowserString(val.asESString()));
                        listSoFar = listSoFar->concat(String::spaceString);
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

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("children"), childrenGetter, NULL, false, false, false);

    /* Page Visibility */
    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("hidden"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        bool hidden = ((Document*) originalObj->extraPointerData())->hidden();
        return escargot::ESValue(hidden);
        },
        NULL, false, false, false);

    DocumentFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("visibilityState"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        String* visibilityState = ((Document*) originalObj->extraPointerData())->visibilityState();
        return toJSString(visibilityState);
        },
        NULL, false, false, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLDocument, DocumentFunction->protoType());
    fetchData(this)->m_htmlDocument = HTMLDocumentFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CharacterData, NodeFunction->protoType());
    fetchData(this)->m_characterData = CharacterDataFunction;

    /* 4.9 Interface CharacterData */

    CharacterDataFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("data"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
        if (nd->isCharacterData()) {
            return toJSString(nd->asCharacterData()->data());
        }
        THROW_ILLEGAL_INVOCATION();
        RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
        if (nd->isCharacterData()) {
            nd->asCharacterData()->setData(toBrowserString(value));
            return;
        }
        THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    CharacterDataFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("length"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
        if (nd->isCharacterData()) {
            return escargot::ESValue(nd->asCharacterData()->length());
        }
        THROW_ILLEGAL_INVOCATION();
        RELEASE_ASSERT_NOT_REACHED();
        }
        , NULL, true, true, false);

    CharacterDataFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("nextElementSibling"),
        nextElementChildGetter, NULL, false, false, false);
    CharacterDataFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("previousElementSibling"),
        previousElementChildGetter, NULL, false, false, false);

    CharacterDataFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, removeFunction);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Text, CharacterDataFunction->protoType());
    fetchData(this)->m_text = TextFunction;

    /* 4.10 Interface Text */

    TextFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("wholeText"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)originalObj->extraPointerData());
        if (!nd->isText()) {
            THROW_ILLEGAL_INVOCATION();
        }
        String* text = (nd->asText())->wholeText();
        return toJSString(text);
        },
        NULL, false, false, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Comment, CharacterDataFunction->protoType());
    fetchData(this)->m_comment = CommentFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLElement, ElementFunction->protoType());
    fetchData(this)->m_htmlElement = HTMLElementFunction;

    HTMLElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onclick"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = (Node *)originalObj->extraPointerData();
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                return element->onclick();
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = (Node *)originalObj->extraPointerData();
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
        },
        true, true, true);

    HTMLElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onload"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = (Node *)originalObj->extraPointerData();
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                return element->onload();
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = (Node *)originalObj->extraPointerData();
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
        },
        true, true, true);

    HTMLElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onunload"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = (Node *)originalObj->extraPointerData();
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                return element->onunload();
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = (Node *)originalObj->extraPointerData();
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
        },
        true, true, true);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLHtmlElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlHtmlElement = HTMLHtmlElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLHeadElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlHeadElement = HTMLHeadElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLScriptElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlScriptElement = HTMLScriptElementFunction;

    HTMLScriptElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("src"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_src));
            }
            THROW_ILLEGAL_INVOCATION();
            RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_src, toBrowserString(value.toString()));
                return;
            }
            THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLStyleElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlStyleElement = HTMLStyleElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLLinkElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlLinkElement = HTMLLinkElementFunction;

    HTMLLinkElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("href"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_href));
            }
            THROW_ILLEGAL_INVOCATION();
            RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_href, toBrowserString(value.toString()));
                return;
            }
            THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    HTMLLinkElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("rel"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_rel));
            }
            THROW_ILLEGAL_INVOCATION();
            RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_rel, toBrowserString(value.toString()));
                return;
            }
            THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    HTMLLinkElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("type"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                return toJSString(nd->asElement()->getAttribute(nd->document()->window()->starFish()->staticStrings()->m_type));
            }
            THROW_ILLEGAL_INVOCATION();
            RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
            Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
            if (nd->isElement() && nd->asElement()->isHTMLElement() && nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                nd->asElement()->setAttribute(nd->document()->window()->starFish()->staticStrings()->m_type, toBrowserString(value.toString()));
                return;
            }
            THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLBodyElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlBodyElement = HTMLBodyElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLDivElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlDivElement = HTMLDivElementFunction;

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLImageElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlImageElement = HTMLImageElementFunction;

    escargot::ESString* srcString = escargot::ESString::create("src");
    HTMLImageElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(srcString,
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    return toJSString(nd->asElement()->asHTMLElement()->asHTMLImageElement()->src());
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
        RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLImageElement()->setSrc(toBrowserString(value));
                    return;
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    HTMLImageElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("width"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
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
        RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));

        if (!(value.isESString() && value.asESString()->hasOnlyDigit())) {
            return;
        }

        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLImageElement()->setWidth(value.toInt32());
                    return;
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    HTMLImageElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("height"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));
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
        RELEASE_ASSERT_NOT_REACHED();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ::escargot::ESValue& value)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeObject);
        Node* nd = ((Node *)((Node *)originalObj->extraPointerData()));

        if (!(value.isESString() && value.asESString()->hasOnlyDigit())) {
            return;
        }

        if (nd->isElement()) {
            if (nd->asElement()->isHTMLElement()) {
                if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                    nd->asElement()->asHTMLElement()->asHTMLImageElement()->setHeight(value.toInt32());
                    return;
                }
            }
        }
        THROW_ILLEGAL_INVOCATION();
        },
        true, true, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(HTMLBRElement, HTMLElementFunction->protoType());
    fetchData(this)->m_htmlBrElement = HTMLBRElementFunction;

#ifdef STARFISH_ENABLE_AUDIO
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

    HTMLCollectionFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("length"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::HTMLCollectionObject);

            uint32_t len = ((HTMLCollection *)originalObj->extraPointerData())->length();
            return escargot::ESValue(len);
        },
        NULL, false, false, false);

    escargot::ESFunctionObject* itemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
    {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::HTMLCollectionObject);

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isUInt32()) {
            Element* elem = ((HTMLCollection*) thisValue.asESPointer()->asESObject()->extraPointerData())->item(argValue.asUInt32());
            if (elem != nullptr)
                return elem->scriptValue();
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("item"), 1, false);
    HTMLCollectionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false, itemFunction);

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
    HTMLCollectionFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("namedItem"), false, false, false, namedItemFunction);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NodeList, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_nodeList = NodeListFunction;

    NodeListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue
        {
            escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeListObject);

            escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
            if (argValue.isUInt32()) {
                NodeList* nodeList = (NodeList*)thisValue.asESPointer()->asESObject()->extraPointerData();
                Node* nd = nodeList->item(argValue.asUInt32());
                if (nd != nullptr) {
                    return nd->scriptValue();
                }
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, escargot::ESString::create("item"), 1, false)
    );

    NodeListFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("length"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
            CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NodeListObject);
            NodeList* nodeList = (NodeList*)originalObj->extraPointerData();
            return escargot::ESValue(nodeList->length());
        }, NULL, false, false, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMTokenList, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_domTokenList = DOMTokenListFunction;

    /* 7.1 Interface DOMTokenList */

    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("length"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::DOMTokenListObject);

        uint32_t len = ((DOMTokenList *)originalObj->extraPointerData())->length();
        return escargot::ESValue(len);
        },
        NULL, false, false, false);

    escargot::ESFunctionObject* domTokenListItemFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);

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
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("item"), false, false, false, domTokenListItemFunction);

    escargot::ESFunctionObject* domTokenListContainsFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);

        escargot::ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            bool res = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->contains(toBrowserString(argValue.asESString()));
            return escargot::ESValue(res);
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
    }, escargot::ESString::create("contains"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("contains"), false, false, false, domTokenListContainsFunction);

    escargot::ESFunctionObject* domTokenListAddFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);

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
            ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->add(&tokens);
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("add"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("add"), false, false, false, domTokenListAddFunction);

    escargot::ESFunctionObject* domTokenListRemoveFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);

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
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("remove"), false, false, false, domTokenListRemoveFunction);

    escargot::ESFunctionObject* domTokenListToggleFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);

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
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("toggle"), false, false, false, domTokenListToggleFunction);

    escargot::ESFunctionObject* domTokenListToStringFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, ScriptWrappable::Type::DOMTokenListObject);
        uint32_t len = ((DOMTokenList *) thisValue.asESPointer()->asESObject()->extraPointerData())->length();

        String* str = String::emptyString;
        for (uint32_t i = 0; i < len; i++) {
            String* elem = ((DOMTokenList*) thisValue.asESPointer()->asESObject()->extraPointerData())->item(i);
            if (elem != nullptr) {
                if (str == String::emptyString)
                    str = str->concat(elem);
                else
                    str = str->concat(String::spaceString)->concat(elem);
            }
        }
        return toJSString(str);
    }, escargot::ESString::create("toString"), 1, false);
    DOMTokenListFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("toString"), true, false, true, domTokenListToStringFunction);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMSettableTokenList, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_domSettableTokenList = DOMSettableTokenListFunction;

    /* 7.2 Interface DOMSettableTokenList */

    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("length"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::DOMSettableTokenListObject);

        uint32_t len = ((DOMTokenList *)originalObj->extraPointerData())->length();
        return escargot::ESValue(len);
        },
        NULL, false, false, false);

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

    auto DOMSettableTokenListValueGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::DOMSettableTokenListObject);

        String* value = ((DOMSettableTokenList*) originalObj->extraPointerData())->value();
        if (value != nullptr)
            return toJSString(value);
        return escargot::ESValue(escargot::ESValue::ESNull);
    };

    auto DOMSettableTokenListValueSetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::DOMSettableTokenListObject);

        ((DOMSettableTokenList*) originalObj->extraPointerData())->setValue(toBrowserString(v));
    };

    DOMSettableTokenListFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("value"), DOMSettableTokenListValueGetter, DOMSettableTokenListValueSetter, false, false, false);

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(NamedNodeMap, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_namedNodeMap = NamedNodeMapFunction;

    /* 4.8.1 Interface NamedNodeMap */

    NamedNodeMapFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("length"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::NamedNodeMapObject);

        uint32_t len = ((NamedNodeMap *)originalObj->extraPointerData())->length();
        return escargot::ESValue(len);
        },
        NULL, false, false, false);

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

    auto attrNameValueGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj,
        escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::AttrObject);

        String* n = ((Attr*) originalObj->extraPointerData())->name();
        if (n != nullptr)
            return toJSString(n);
        return escargot::ESValue(escargot::ESValue::ESNull);
    };

    auto attrValueGetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::AttrObject);

        String* value = ((Attr*) originalObj->extraPointerData())->value();
        if (value == nullptr)
            value = String::emptyString;
        return toJSString(value);
    };

    auto attrValueSetter = [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
    {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::AttrObject);

        ((Attr*) originalObj->extraPointerData())->setValue(toBrowserString(v));
        // FIXME(JMP): Actually this function have to return old Attr's value but we have to modify 'typedef void (*ESNativeSetter)(...)' in escargot/src/runtime/ESValue.h
        // Because this need to many changes, we do the modification latter
    };

    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("name"), attrNameValueGetter, NULL, false, false, false);
    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("nodeName"), attrNameValueGetter, NULL, false, false, false);
    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("value"), attrValueGetter, attrValueSetter, false, false, false);
    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("nodeValue"), attrValueGetter, attrValueSetter, false, false, false);
    AttrFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("textContent"), attrValueGetter, attrValueSetter, false, false, false);

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

        if (argCount == 1 && firstArg.isESString()) {
            auto event = new Event(QualifiedName::fromString(sf, firstArg.asESString()->utf8Data()));
            return event->scriptValue();
        } else if (argCount == 2 && firstArg.isESString() && secondArg.isObject()) {
            auto bubbles = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("bubbles"));
            auto cancelable = secondArg.asESPointer()->asESObject()->get(escargot::ESString::create("cancelable"));
            auto canBubbles = bubbles.isBoolean() ? bubbles.asBoolean() : false;
            auto canCancelable = cancelable.isBoolean() ? cancelable.asBoolean() : false;
            auto event = new Event(QualifiedName::fromString(sf, firstArg.asESString()->utf8Data()), EventInit(canBubbles, canCancelable));
            return event->scriptValue();
        } else {
            // FIXME: TypeError
            return escargot::ESValue();
        }
    }, escargot::ESString::create("Event"), 1, true, false);
    eventFunction->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(false);
    eventFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_instance->globalObject()->defineDataProperty(escargot::ESString::create("Event"), false, false, false, eventFunction);
    fetchData(this)->m_event = eventFunction;

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("type"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        String* type = ((Event*) originalObj->extraPointerData())->type().string();
        return toJSString(type);
        },
        NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("target"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        EventTarget* target = ((Event*) originalObj->extraPointerData())->target();
        if (target && (target->isNode() || target->isWindow())) {
            return target->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("currentTarget"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        EventTarget* currentTarget = ((Event*) originalObj->extraPointerData())->currentTarget();
        if (currentTarget && (currentTarget->isNode() || currentTarget->isWindow())) {
            return currentTarget->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        NULL, false, false, false);

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

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("eventPhase"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        unsigned short eventPhase = ((Event*) originalObj->extraPointerData())->eventPhase();
        return escargot::ESValue(eventPhase);
        },
        NULL, false, false, false);

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

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("bubbles"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        bool bubbles = ((Event*) originalObj->extraPointerData())->bubbles();
        return escargot::ESValue(bubbles);
        },
        NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("cancelable"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        bool cancelable = ((Event*) originalObj->extraPointerData())->cancelable();
        return escargot::ESValue(cancelable);
        },
        NULL, false, false, false);

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

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("defaultPrevented"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        bool defaultPrevented = ((Event*) originalObj->extraPointerData())->defaultPrevented();
        return escargot::ESValue(defaultPrevented);
        },
        NULL, false, false, false);

    eventFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("timeStamp"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::EventObject);
        DOMTimeStamp timeStamp = ((Event*) originalObj->extraPointerData())->timeStamp();
        return escargot::ESValue(timeStamp);
        },
        NULL, false, false, false);

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

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleDeclaration, CSSStyleDeclarationFunction->protoType());
    fetchData(this)->m_cssStyleDeclaration = CSSStyleDeclarationFunction;

#define DEFINE_ACCESSOR_PROPERTY(name, nameLower, lowerCaseName)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
    CSSStyleDeclarationFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create(#nameLower),                                                                                                                                                                                                                                                                                                                                                                                                            \
        [](::escargot::ESObject * obj, ::escargot::ESObject * originalObj, escargot::ESString * name) -> escargot::ESValue { \
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::CSSStyleDeclarationObject); \
    \
        String* c = ((CSSStyleDeclaration*) originalObj->extraPointerData())->name(); \
        STARFISH_ASSERT(c); \
        return toJSString(c);                                                                                                                                                                        \
        },                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
        [](::escargot::ESObject * obj, ::escargot::ESObject * originalObj, escargot::ESString * name, const escargot::ESValue& v) { \
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::CSSStyleDeclarationObject); \
        if (v.isESString()) { \
            ((CSSStyleDeclaration*) originalObj->extraPointerData())->set##name(v.asESString()->utf8Data()); \
        } else if (v.isNumber()) { \
            ((CSSStyleDeclaration*) originalObj->extraPointerData())->set##name(std::to_string(v.toNumber()).c_str()); \
        } \
        },                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
        false, false, false);

    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(DEFINE_ACCESSOR_PROPERTY)

    DEFINE_FUNCTION_NOT_CONSTRUCTOR(CSSStyleRule, CSSStyleRuleFunction->protoType());
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

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("responseType"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        String* c = ((XMLHttpRequest*) originalObj->extraPointerData())->getResponseTypeStr();
        if (c != nullptr)
            return toJSString(c);
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isESString()) {
            ((XMLHttpRequest*)originalObj->extraPointerData())->setResponseType(v.asESString()->utf8Data());
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onloadstart"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("loadstart"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "loadstart");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onprogress"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("progress"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "progress");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onabort"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("abort"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "abort");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onerror"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("error"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "error");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onload"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("load"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "load");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("ontimeout"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("timeout"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "timeout");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onloadend"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("loadend"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "loadend");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("onreadystatechange"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);

        escargot::ESValue c = ((XMLHttpRequest*) originalObj->extraPointerData())->getHandler(String::fromUTF8("readystatechange"), ((XMLHttpRequest*)originalObj->extraPointerData())->starfishInstance());
        if (c.isObject())
            return c;
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isObject()) {
            auto sf = ((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish();
            auto eventTypeName = QualifiedName::fromString(sf, "readystatechange");
            ((XMLHttpRequest*)originalObj->extraPointerData())->setHandler(eventTypeName, v);
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("timeout"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        uint32_t c = ((XMLHttpRequest*) originalObj->extraPointerData())->getTimeout();
            return escargot::ESValue(c);
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& v)
        {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        if (v.isNumber()) {
            ((XMLHttpRequest*)originalObj->extraPointerData())->setTimeout(v.toInt32());
        }
        },
        false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("readyState"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        int c = ((XMLHttpRequest*) originalObj->extraPointerData())->getReadyState();
            return escargot::ESValue(c);
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        NULL, false, false, false);

    xhrElementFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("status"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        CHECK_TYPEOF(originalObj, ScriptWrappable::Type::XMLHttpRequestObject);
        int c = ((XMLHttpRequest*) originalObj->extraPointerData())->getStatus();
            return escargot::ESValue(c);
        return escargot::ESValue(escargot::ESValue::ESNull);
        },
        NULL, false, false, false);

    escargot::ESFunctionObject* xhrGetAllResponseHeadersFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isObject()) {
            if (v.asESPointer()->asESObject()->extraData() == ScriptWrappable::XMLHttpRequestObject) {
                XMLHttpRequest* xhr = (XMLHttpRequest*)v.asESPointer()->asESObject()->extraPointerData();
                return toJSString(xhr->getAllResponseHeadersStr());
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
        return escargot::ESValue(escargot::ESValue::ESNull);
    }, escargot::ESString::create("setRequestHeader"), 1, false);
    xhrElementFunction->protoType().asESPointer()->asESObject()->defineDataProperty(escargot::ESString::create("setRequestHeader"), false, false, false, xhrSetRequestHeaderFunction);

    /* Blob */
    DEFINE_FUNCTION(Blob, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_blobElement = BlobFunction;

    /* DOM Exception */
    DEFINE_FUNCTION(DOMException, fetchData(this)->m_instance->globalObject()->objectPrototype());
    fetchData(this)->m_domException = DOMExceptionFunction;
    DOMExceptionFunction->protoType().asESPointer()->asESObject()->set__proto__(fetchData(this)->m_instance->globalObject()->errorPrototype());

    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(fetchData(this)->m_instance->strings().name,
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        if (originalObj->extraData() == ScriptWrappable::Type::DOMExceptionObject) {
            DOMException* exception = (DOMException*)originalObj->extraPointerData();
            return escargot::ESString::create(exception->name());
        } else {
            return escargot::ESValue();
        }
        },
        NULL, false, false, false);

    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(fetchData(this)->m_instance->strings().message,
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        if (originalObj->extraData() == ScriptWrappable::Type::DOMExceptionObject) {
            DOMException* exception = (DOMException*)originalObj->extraPointerData();
            return toJSString(exception->message());
        } else {
            return escargot::ESValue();
        }
        },
        NULL, false, false, false);

    DOMExceptionFunction->protoType().asESPointer()->asESObject()->defineAccessorProperty(escargot::ESString::create("code"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        if (originalObj->extraData() == ScriptWrappable::Type::DOMExceptionObject) {
            DOMException* exception = (DOMException*)originalObj->extraPointerData();
            return escargot::ESValue(exception->code());
        } else {
            return escargot::ESValue();
        }
        },
        NULL, false, false, false);

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
