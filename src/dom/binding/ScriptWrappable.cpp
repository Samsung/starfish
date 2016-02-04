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
    return (ScriptBindingInstanceDataEscargot *)instance->data();
}

ScriptWrappable::ScriptWrappable()
    : escargot::ESObject(escargot::ESPointer::Type::ESObject, escargot::ESValue())
{

}

ScriptWrappableGlobalObject::ScriptWrappableGlobalObject()
{
}

ScriptWrappableGlobalObject* ScriptWrappableGlobalObject::fetch()
{
    auto a = escargot::ESVMInstance::currentInstance()->globalObject()->get(escargot::ESString::create("__windowData"));
    return (ScriptWrappableGlobalObject*)a.asESPointer();
}



void ScriptWrappableGlobalObject::initScriptWrappableWindow(Window* window)
{
    m_object = escargot::ESVMInstance::currentInstance()->globalObject();
    escargot::ESVMInstance::currentInstance()->globalObject()->defineDataProperty(escargot::ESString::create("__windowData"), false, false, false, escargot::ESValue((escargot::ESPointer*)this));
    auto data = fetchData(window->starFish()->scriptBindingInstance());
    ((escargot::ESObject *)this->m_object)->set__proto__(data->m_window->protoType());
    ((escargot::ESObject *)this->m_object)->setExtraData(ScriptWrappable::WindowObject);

    // [setTimeout]
    // https://www.w3.org/TR/html5/webappapis.html
    // long setTimeout(Function handler, optional long timeout, any... arguments);

    // TODO : Pass "any... arguments" if exist
    // TODO : First argument can be function or script source (currently allow function only)
    escargot::ESFunctionObject* setTimeoutFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer() &&
                    instance->currentExecutionContext()->readArgument(0).asESPointer() &&
                    instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                if (instance->currentExecutionContext()->readArgument(1).isNumber()) {
                    Window* wnd = (Window*)ScriptWrappableGlobalObject::fetch();
                    return  escargot::ESValue(wnd->setTimeout([](Window* wnd, void* data) {
                                escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
                                std::jmp_buf tryPosition;
                                if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                                    escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                                    escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                                } else {
                                    escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                                    printf("Uncaught %s\n", err.toString()->utf8Data());
                                }
                            }, instance->currentExecutionContext()->readArgument(1).toUint32(),
                            instance->currentExecutionContext()->readArgument(0).asESPointer()));
                }
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("setTimeout"), 2, false);
    ((escargot::ESObject *)this->m_object)->defineDataProperty(escargot::ESString::create("setTimeout"), false, false, false, setTimeoutFunction);

    // [clearTimeout]
    // https://www.w3.org/TR/html5/webappapis.html
    escargot::ESFunctionObject* clearTimeoutFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)ScriptWrappableGlobalObject::fetch();
                wnd->clearTimeout(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        // FIXME what return value should return?
        return escargot::ESValue(1000);
    }, escargot::ESString::create("clearTimeout"), 1, false);
    ((escargot::ESObject *)this->m_object)->defineDataProperty(escargot::ESString::create("clearTimeout"), false, false, false, clearTimeoutFunction);


    // TODO : Pass "any... arguments" if exist
    // TODO : First argument can be function or script source (currently allow function only)
    escargot::ESFunctionObject* requestAnimationFrameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isESPointer() &&
                    instance->currentExecutionContext()->readArgument(0).asESPointer() &&
                    instance->currentExecutionContext()->readArgument(0).asESPointer()->isESFunctionObject()) {
                    Window* wnd = (Window*)ScriptWrappableGlobalObject::fetch();
                    return  escargot::ESValue(wnd->requestAnimationFrame([](Window* wnd, void* data) {
                                escargot::ESFunctionObject* fn = (escargot::ESFunctionObject*)data;
                                std::jmp_buf tryPosition;
                                if (setjmp(escargot::ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                                    escargot::ESFunctionObject::call(escargot::ESVMInstance::currentInstance(), fn, escargot::ESValue(), NULL, 0, false);
                                    escargot::ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                                } else {
                                    escargot::ESValue err = escargot::ESVMInstance::currentInstance()->getCatchedError();
                                    printf("Uncaught %s\n", err.toString()->utf8Data());
                                }

                            }, instance->currentExecutionContext()->readArgument(0).asESPointer()));
            }
        }
        return escargot::ESValue();
    }, escargot::ESString::create("requestAnimationFrame"), 2, false);
    ((escargot::ESObject *)this->m_object)->defineDataProperty(escargot::ESString::create("requestAnimationFrame"), false, false, false, requestAnimationFrameFunction);

    // https://www.w3.org/TR/html5/webappapis.html
    escargot::ESFunctionObject* cancelAnimationFrameFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
        escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (v.isUndefinedOrNull() || v.asESPointer()->asESObject()->extraData() == ScriptWrappable::WindowObject) {
            if (instance->currentExecutionContext()->readArgument(0).isNumber()) {
                Window* wnd = (Window*)ScriptWrappableGlobalObject::fetch();
                wnd->cancelAnimationFrame(instance->currentExecutionContext()->readArgument(0).toUint32());
            }
        }
        // FIXME what return value should return?
        return escargot::ESValue(1000);
    }, escargot::ESString::create("cancelAnimationFrame"), 1, false);
    ((escargot::ESObject *)this->m_object)->defineDataProperty(escargot::ESString::create("cancelAnimationFrame"), false, false, false, cancelAnimationFrameFunction);
}

void ScriptWrappableGlobalObject::callFunction(String* name)
{
    escargot::ESObject* obj = ((escargot::ESObject *)this->m_object);
    escargot::ESValue fn = obj->get(escargot::ESString::create(name->utf8Data()));
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();

    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        escargot::ESFunctionObject::call(instance, fn, obj, NULL, 0, false);
        instance->unregisterTryPos(&tryPosition);
    } else {
        escargot::ESValue err = instance->getCatchedError();
        printf("Uncaught %s\n", err.toString()->utf8Data());
    }
}

void ScriptWrappable::initScriptWrappable(Node* ptr)
{
    Node* node = (Node*)this;
    initScriptWrappable(ptr, node->document()->scriptBindingInstance());
}

void ScriptWrappable::initScriptWrappable(Node* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_node->protoType());
    ((escargot::ESObject *)this)->setExtraData(NodeObject);
}

void ScriptWrappable::initScriptWrappable(Element* element)
{
    Node* node = (Node*)this;
    initScriptWrappable(element, node->document()->scriptBindingInstance());
}

void ScriptWrappable::initScriptWrappable(DocumentType* element)
{
    auto data = fetchData(element->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_documentType->protoType());
}

void ScriptWrappable::initScriptWrappable(Element* element, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_element->protoType());

}

void ScriptWrappable::initScriptWrappable(Document*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_document->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLDocument*)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlDocument->protoType());
}

void ScriptWrappable::initScriptWrappable(CharacterData* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_characterData->protoType());
}

void ScriptWrappable::initScriptWrappable(Text* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_text->protoType());
}

void ScriptWrappable::initScriptWrappable(Comment* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_comment->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLHtmlElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlHtmlElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLHeadElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlHeadElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLBodyElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlBodyElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLScriptElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlScriptElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLImageElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlImageElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLDivElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlDivElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLBRElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlBrElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLAudioElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlAudioElement->protoType());
}

void ScriptWrappable::initScriptWrappable(HTMLUnknownElement* ptr)
{
    Node* node = (Node*)this;
    auto data = fetchData(node->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlUnknownElement->protoType());
}

void ScriptWrappable::initScriptWrappable(XMLHttpRequest* xhr)
{
    Window* window = (Window*)ScriptWrappableGlobalObject::fetch();
    ScriptBindingInstance* instance = window->starFish()->scriptBindingInstance();
    auto data = fetchData(instance);
    xhr->setScriptBindingInstance(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_xhrElement->protoType());
    ((escargot::ESObject *)this)->setExtraData(XMLHttpRequestObject);
}

bool ScriptWrappable::hasProperty(String* name)
{
    return escargot::ESObject::hasProperty(escargot::ESString::create(name->utf8Data()));
}

void ScriptWrappable::initScriptWrappable(Event* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_event->protoType());
    ((escargot::ESObject *)this)->setExtraData(EventObject);
}

void ScriptWrappable::initScriptWrappable(UIEvent* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_uiEvent->protoType());
}

void ScriptWrappable::initScriptWrappable(MouseEvent* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_mouseEvent->protoType());
}

void ScriptWrappable::initScriptWrappable(ProgressEvent* ptr, ScriptBindingInstance* instance,uint32_t loaded,uint32_t total)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_progressEvent->protoType());

    bool lengthComputable=false;
    if(total>0)
        lengthComputable=true;

    ((escargot::ESObject *)this)->defineDataProperty(escargot::ESString::create("lengthComputable"), false, false, false, escargot::ESValue(lengthComputable));
    ((escargot::ESObject *)this)->defineDataProperty(escargot::ESString::create("loaded"), false, false, false, escargot::ESValue(loaded));
    ((escargot::ESObject *)this)->defineDataProperty(escargot::ESString::create("total"), false, false, false, escargot::ESValue(total));
}

void ScriptWrappable::initScriptWrappable(HTMLCollection* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_htmlCollection->protoType());
    ((escargot::ESObject *)this)->setExtraData(HTMLCollectionObject);
}

void ScriptWrappable::initScriptWrappable(NodeList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_nodeList->protoType());
    ((escargot::ESObject *)this)->setExtraData(NodeListObject);
}

void ScriptWrappable::initScriptWrappable(DOMTokenList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_domTokenList->protoType());
    ((escargot::ESObject *)this)->setExtraData(DOMTokenListObject);
}

void ScriptWrappable::initScriptWrappable(DOMSettableTokenList* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_domSettableTokenList->protoType());
    ((escargot::ESObject *)this)->setExtraData(DOMSettableTokenListObject);
}

void ScriptWrappable::initScriptWrappable(NamedNodeMap* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_namedNodeMap->protoType());
    ((escargot::ESObject *)this)->setExtraData(NamedNodeMapObject);
}

void ScriptWrappable::initScriptWrappable(Attr* ptr, ScriptBindingInstance* instance)
{
    auto data = fetchData(instance);
    ((escargot::ESObject *)this)->set__proto__(data->m_attr->protoType());
    ((escargot::ESObject *)this)->setExtraData(AttrObject);
}

void ScriptWrappable::initScriptWrappable(CSSStyleDeclaration* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_cssStyleDeclaration->protoType());
    ((escargot::ESObject *)this)->setExtraData(CSSStyleDeclarationObject);
}

void ScriptWrappable::initScriptWrappable(CSSStyleRule* ptr)
{
    auto data = fetchData(ptr->document()->scriptBindingInstance());
    ((escargot::ESObject *)this)->set__proto__(data->m_cssStyleRule->protoType());
    ((escargot::ESObject *)this)->setExtraData(CSSStyleRuleObject);
}

ScriptValue createScriptString(String* str)
{
    return escargot::ESString::create(str->utf8Data());
}

ScriptValue createScriptFunction(String** argNames, size_t argc, String* functionBody)
{
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();

    escargot::ESValueVector arg(0);
    for (size_t i = 0; i < argc; i ++) {
        arg.push_back(createScriptString(argNames[i]));
    }

    arg.push_back(createScriptString(functionBody));

    ScriptValue value = escargot::ESFunctionObject::call(instance, instance->globalObject()->function(), escargot::ESValue(),
            arg.data(), argc + 1, false);

    return value;
}

void callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc, ScriptValue thisValue)
{
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        escargot::ESFunctionObject::call(instance, fn, thisValue, argv, argc, false);
        instance->unregisterTryPos(&tryPosition);
    } else {
        escargot::ESValue err = instance->getCatchedError();
        printf("Uncaught %s\n", err.toString()->utf8Data());
    }
}

}
