#include "StarFishConfig.h"
#include "ScriptWrappable.h"

#include "platform/window/Window.h"
#include "dom/DOM.h"

#include "vm/ESVMInstance.h"

#include "dom/binding/ScriptBindingInstance.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

static ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance)
{
    return (ScriptBindingInstanceDataEscargot*)instance->data();
}

ScriptWrappable::ScriptWrappable(void* extraPointerData)
{
    STARFISH_ASSERT(!((size_t)extraPointerData & (size_t)1));
    m_object = (escargot::ESFunctionObject*)((size_t)extraPointerData | (size_t)1);
}

ScriptObject ScriptWrappable::scriptObjectSlowCase()
{
    void* extraPointerData = (void*)((size_t)m_object - 1);
    m_object = escargot::ESObject::create(0);
    STARFISH_ASSERT(!((size_t)m_object & (size_t)1));
    m_object->setExtraPointerData(extraPointerData);

    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    initScriptObject(window->starFish()->scriptBindingInstance());

    return m_object;
}

void ScriptWrappable::initScriptWrappable(Window* window)
{
    m_object = escargot::ESVMInstance::currentInstance()->globalObject();
    auto data = fetchData(window->starFish()->scriptBindingInstance());
    scriptObject()->set__proto__(data->m_window->protoType());
    scriptObject()->setExtraData(ScriptWrappable::WindowObject);
    scriptObject()->setExtraPointerData(window);

    // [setTimeout]
    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-settimeout
    // long setTimeout(Function handler, optional long timeout, any... arguments);

    // TODO : Pass "any... arguments" if exist
    escargot::ESFunctionObject* setTimeoutFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();

        auto handler = [](Window* wnd, void* data)
        {
            escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
            std::jmp_buf tryPosition;
            if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
            } else {
                escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
            }
        };

        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                if (instance->currentExecutionContext()->readArgument(1).isUndefinedOrNull()) {
                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->setTimeout(handler, 0,
                    instance->currentExecutionContext()->readArgument(0).asESPointer()));
                } else if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->setTimeout(handler, instance->currentExecutionContext()->readArgument(1).toUint32(),
                    instance->currentExecutionContext()->readArgument(0).asESPointer()));
                }
            } else if (instance->currentExecutionContext()->readArgument(0).isESString()) {
                if (instance->currentExecutionContext()->readArgument(1).isUndefinedOrNull()) {
                    String* bodyStr = toBrowserString(instance->currentExecutionContext()->readArgument(0).asESString());
                    String* name[] = {String::emptyString};
                    bool error = false;
                    ScriptValue m_listener = createScriptFunction(name, 1, bodyStr, error);
                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->setTimeout(handler, 0,
                    m_listener.asESPointer()));
                } else if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    String* bodyStr = toBrowserString(instance->currentExecutionContext()->readArgument(0).asESString());
                    String* name[] = {String::emptyString};
                    bool error = false;
                    ScriptValue m_listener = createScriptFunction(name, 1, bodyStr, error);

                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->setTimeout(handler, instance->currentExecutionContext()->readArgument(1).toUint32(),
                    m_listener.asESPointer()));

                }
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("setTimeout"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("setTimeout"), true, true, true, setTimeoutFunction);

    // [clearTimeout]
    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-cleartimeout
    escargot::ESFunctionObject* clearTimeoutFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                wnd->clearTimeout(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("clearTimeout"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("clearTimeout"), true, true, true, clearTimeoutFunction);

    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-setinterval
    escargot::ESFunctionObject* setIntervalFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        auto handler = [](Window* wnd, void* data)
        {
            escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
            std::jmp_buf tryPosition;
            if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
            } else {
                escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
            }
        };

        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->setInterval(handler,
                    instance->currentExecutionContext()->readArgument(1).toUint32(),
                    instance->currentExecutionContext()->readArgument(0).asESPointer()));
                }
            } else if (instance->currentExecutionContext()->readArgument(0).isESString()) {
                if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    String* bodyStr = toBrowserString(instance->currentExecutionContext()->readArgument(0).asESString());
                    String* name[] = {String::emptyString};
                    bool error = false;
                    ScriptValue m_listener = createScriptFunction(name, 1, bodyStr, error);

                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->setInterval(handler, instance->currentExecutionContext()->readArgument(1).toUint32(),
                    m_listener.asESPointer()));
                }
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("setInterval"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("setInterval"), true, true, true, setIntervalFunction);

    // https://www.w3.org/TR/html5/webappapis.html#dom-windowtimers-clearinterval
    escargot::ESFunctionObject* clearIntervalFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                wnd->clearInterval(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    }, escargot::ESString::create("clearInterval"), 0, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("clearInterval"), true, true, true, clearIntervalFunction);

    // TODO : Pass "any... arguments" if exist
    // TODO : First argument can be function or script source (currently allow function only)
    escargot::ESFunctionObject* requestAnimationFrameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()
                && instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                    Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                    return escargot::ESValue(wnd->requestAnimationFrame([](Window* wnd, void* data) {
                        escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
                        std::jmp_buf tryPosition;
                        if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                            escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                            escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                        } else {
                            std::jmp_buf tryPosition;
                            escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                            if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                                STARFISH_LOG_INFO("Uncaught %s\n", err.toString()->utf8Data());
                                escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                            } else {
                                STARFISH_LOG_INFO("Uncaught Error\n");
                            }
                        }
                    }, instance->currentExecutionContext()->readArgument(0).asESPointer()));
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("requestAnimationFrame"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("requestAnimationFrame"), false, false, false, requestAnimationFrameFunction);

    // https://www.w3.org/TR/html5/webappapis.html
    escargot::ESFunctionObject* cancelAnimationFrameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
                wnd->cancelAnimationFrame(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        // FIXME what return value should return?
        return escargot::ESValue(1000);
    }, escargot::ESString::create("cancelAnimationFrame"), 1, false);
    ((escargot::ESObject*)this->m_object)->defineDataProperty(escargot::ESString::create("cancelAnimationFrame"), false, false, false, cancelAnimationFrameFunction);

    // https://www.w3.org/TR/cssom-view/#dom-window-innerwidth
    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("innerWidth"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            double innerWidth = ((Window*)originalObj->extraPointerData())->innerWidth();
            return escargot::ESValue(innerWidth);
        }
        return escargot::ESValue(0);
        },
        NULL, true, true, true);

    // https://www.w3.org/TR/cssom-view/#dom-window-innerheight
    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("innerHeight"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            double innerHeight = ((Window*)originalObj->extraPointerData())->innerHeight();
            return escargot::ESValue(innerHeight);
        }
        return escargot::ESValue(0);
        },
        NULL, true, true, true);

    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("onclick"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            return wnd->onclick();
        }
        return escargot::ESValue();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const escargot::ESValue& value)
        {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            if (value.asESPointer()->isESFunctionObject()) {
                wnd->setOnclick(value);
            } else {
                wnd->clearOnClick();
            }
        }
        },
        true, true, true);

    ((escargot::ESObject*)this->m_object)->defineAccessorProperty(escargot::ESString::create("onload"),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            return wnd->onload();
        }
        return escargot::ESValue();
        },
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const escargot::ESValue& value)
        {
        escargot::ESValue v = originalObj;
        if (v.isObject() && v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            Window* wnd = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
            if (value.asESPointer()->isESFunctionObject()) {
                wnd->setOnload(value);
            } else {
                wnd->clearOnload();
            }
        }
        },
        true, true, true);
}

void ScriptWrappable::initScriptWrappable(Node* ptr)
{
    Node* node = (Node*)this;
    initScriptWrappable(ptr, node->document()->scriptBindingInstance());
}

void ScriptWrappable::initScriptWrappable(Node* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->node()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Element* element)
{
    Node* node = (Node*)this;
    initScriptWrappable(element, node->document()->scriptBindingInstance());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(DocumentType* element)
{
    auto data = fetchData(element->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->documentType()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Element* element, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->element()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Document*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->document()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(DocumentFragment* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->documentFragment()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

#ifdef STARFISH_EXP
void ScriptWrappable::initScriptWrappable(DOMImplementation* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->m_domImplementation()->protoType());
    scriptObject()->setExtraData(DOMImplementationObject);
}
#endif

void ScriptWrappable::initScriptWrappable(HTMLDocument*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlDocument()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(CharacterData* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->characterData()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Text* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->text()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Comment* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->comment()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLHtmlElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlHtmlElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLHeadElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlHeadElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLBodyElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlBodyElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLStyleElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlStyleElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLLinkElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlLinkElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLScriptElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlScriptElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLImageElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlImageElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLDivElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlDivElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLBRElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlBrElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLMetaElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlMetaElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLParagraphElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlParagraphElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(HTMLSpanElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlSpanElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

#ifdef STARFISH_ENABLE_AUDIO
void ScriptWrappable::initScriptWrappable(HTMLAudioElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlAudioElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}
#endif

void ScriptWrappable::initScriptWrappable(HTMLUnknownElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->htmlUnknownElement()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(XMLHttpRequest* xhr)
{
    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    xhr->setScriptBindingInstance(instance);
    xhr->setStarfishInstance(window->starFish());
    scriptObject()->set__proto__(data->xhrElement()->protoType());
    scriptObject()->setExtraData(XMLHttpRequestObject);
}

void ScriptWrappable::initScriptWrappable(Blob* blob)
{
    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->blobElement()->protoType());
    scriptObject()->setExtraData(BlobObject);
}

void ScriptWrappable::initScriptWrappable(URL* url, ScriptBindingInstance* instance)
{
    // auto data = fetchData(instance);
    // scriptObject()->set__proto__(data->urlElement()->protoType());
    // scriptObject()->setExtraData(URLObject);
}

void ScriptWrappable::initScriptWrappable(DOMException* exception, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->domException()->protoType());
    scriptObject()->setExtraData(DOMExceptionObject);

    scriptObject()->defineDataProperty(escargot::ESString::create("code"), false, false, false, escargot::ESValue(exception->code()));
}

bool ScriptWrappable::hasProperty(String* name)
{
    return m_object->escargot::ESObject::hasProperty(createScriptString(name));
}

void ScriptWrappable::initScriptWrappable(Event* event)
{
    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->event()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(UIEvent* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->uiEvent()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(MouseEvent* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->mouseEvent()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(ProgressEvent* ptr)
{
    Window* window = (Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->progressEvent()->protoType());
    scriptObject()->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(HTMLCollection* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->htmlCollection()->protoType());
    scriptObject()->setExtraData(HTMLCollectionObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::HTMLCollectionObject);
        HTMLCollection* self = (HTMLCollection*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx < self->length()) {
            return true;
        }
        Element* e = self->namedItem(toBrowserString(key));
        return e;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::HTMLCollectionObject);
        HTMLCollection* self = (HTMLCollection*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, [](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::HTMLCollectionObject);
        HTMLCollection* self = (HTMLCollection*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx == escargot::ESValue::ESInvalidIndexValue) {
            Element* e = self->namedItem(toBrowserString(key));
            if (e != nullptr)
                return e->scriptValue();
        } else if (idx < self->length()) {
            return self->item(idx)->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    });
}

void ScriptWrappable::initScriptWrappable(NodeList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->nodeList()->protoType());
    scriptObject()->setExtraData(NodeListObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NodeListObject);
        NodeList* self = (NodeList*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx < self->length()) {
            return true;
        }
        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NodeListObject);
        NodeList* self = (NodeList*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, [](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NodeListObject);
        NodeList* self = (NodeList*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        ASSERT(idx < self->length());
        return self->item(idx)->scriptValue();
    });
}

void ScriptWrappable::initScriptWrappable(DOMTokenList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->domTokenList()->protoType());
    scriptObject()->setExtraData(DOMTokenListObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::DOMTokenListObject);
        DOMTokenList* self = (DOMTokenList*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx < self->length()) {
            return true;
        }
        return false;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::DOMTokenListObject);
        DOMTokenList* self = (DOMTokenList*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, [](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::DOMTokenListObject);
        DOMTokenList* self = (DOMTokenList*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        ASSERT(idx < self->length());
        return createScriptString(self->item(idx));
    });
}

void ScriptWrappable::initScriptWrappable(DOMSettableTokenList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->domSettableTokenList()->protoType());
    scriptObject()->setExtraData(DOMSettableTokenListObject);
}

void ScriptWrappable::initScriptWrappable(NamedNodeMap* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->namedNodeMap()->protoType());
    scriptObject()->setExtraData(NamedNodeMapObject);

    scriptObject()->setPropertyInterceptor([](const escargot::ESValue& key, escargot::ESObject* obj) -> bool {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NamedNodeMapObject);
        NamedNodeMap* self = (NamedNodeMap*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx < self->length()) {
            return true;
        }
        Attr* e = self->getNamedItem(QualifiedName::fromString(((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish(), key.asESString()->utf8Data()));
        return e;
    }, [](escargot::ESObject* obj) -> escargot::ESValueVector {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NamedNodeMapObject);
        NamedNodeMap* self = (NamedNodeMap*)obj->extraPointerData();
        size_t len = self->length();
        escargot::ESValueVector v(len);
        for (size_t i = 0; i < len; i ++) {
            v[i] = escargot::ESValue(i);
        }
        return v;
    }, [](const escargot::ESValue& key, escargot::ESObject* obj) -> escargot::ESValue {
        STARFISH_ASSERT(obj->extraData() == ScriptWrappable::Type::NamedNodeMapObject);
        NamedNodeMap* self = (NamedNodeMap*)obj->extraPointerData();
        uint32_t idx = key.toIndex();
        if (idx == escargot::ESValue::ESInvalidIndexValue) {
            Attr* e = self->getNamedItem(QualifiedName::fromString(((Window*)escargot::ESVMInstance::currentInstance()->globalObject()->extraPointerData())->starFish(), key.asESString()->utf8Data()));
            if (e != nullptr)
                return e->scriptValue();
        } else if (idx < self->length()) {
            return self->item(idx)->scriptValue();
        }
        return escargot::ESValue(escargot::ESValue::ESUndefined);
    });
}

void ScriptWrappable::initScriptWrappable(Attr* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    scriptObject()->set__proto__(data->attr()->protoType());
    scriptObject()->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(CSSStyleDeclaration* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->cssStyleDeclaration()->protoType());
    scriptObject()->setExtraData(CSSStyleDeclarationObject);
}

void ScriptWrappable::initScriptWrappable(CSSStyleRule* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    scriptObject()->set__proto__(data->cssStyleRule()->protoType());
    scriptObject()->setExtraData(CSSStyleRuleObject);
}

static int utf32ToUtf16(char32_t i, char16_t *u)
{
    if (i < 0xffff) {
        *u= (char16_t)(i & 0xffff);
        return 1;
    } else if (i < 0x10ffff) {
        i-= 0x10000;
        *u++= 0xd800 | (i >> 10);
        *u= 0xdc00 | (i & 0x3ff);
        return 2;
    } else {
        // produce error char
        // U+FFFD
        *u = 0xFFFD;
        return 1;
    }
}


ScriptValue createScriptString(String* str)
{
    if (str->isASCIIString()) {
        ASCIIString s(str->asASCIIString()->begin(), str->asASCIIString()->end());
        return escargot::ESString::create(std::move(s));
    } else {
        escargot::UTF16String out;
        for (size_t i = 0; i < str->length(); i++) {
            char32_t src = str->charAt(i);
            char16_t dst[2];
            int ret = utf32ToUtf16(src, dst);

            if (ret == 1) {
                out.push_back(src);
            } else if (ret == 2) {
                out.push_back(dst[0]);
                out.push_back(dst[1]);
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }

        }

        return escargot::ESString::create(std::move(out));
    }
}

ScriptValue createScriptFunction(String** argNames, size_t argc, String* functionBody, bool& error)
{
    error = false;
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();

    escargot::ESValueVector arg(0);
    for (size_t i = 0; i < argc; i++) {
        arg.push_back(createScriptString(argNames[i]));
    }

    arg.push_back(createScriptString(functionBody));

    ScriptValue result;
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        result = escargot::ESFunctionObject::call(instance, instance->globalObject()->function(), escargot::ESValue(),
            arg.data(), argc + 1, false);
        instance->unregisterTryPos(&tryPosition);
    } else {
        result = instance->getCatchedError();
        error = true;
        STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
    }
    return result;
}

ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc, ScriptValue thisValue)
{
    ScriptValue result;
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        result = escargot::ESFunctionObject::call(instance, fn, thisValue, argv, argc, false);
        instance->unregisterTryPos(&tryPosition);
    } else {
        result = instance->getCatchedError();
        STARFISH_LOG_INFO("Uncaught %s\n", result.toString()->utf8Data());
    }
    return result;
}

ScriptValue parseJSON(String* jsonData)
{
    ScriptValue ret;
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();
    ScriptValue json_arg[1] = { ScriptValue(createScriptString(jsonData)) };
    ScriptValue json_parse_fn = instance->globalObject()->json()->get(ScriptValue(createScriptString(String::fromUTF8("parse"))));
    return callScriptFunction(json_parse_fn, json_arg, 1, instance->globalObject()->json());
}
}
