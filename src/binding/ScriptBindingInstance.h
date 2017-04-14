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

#ifndef __StarFishScriptBindingInstance__
#define __StarFishScriptBindingInstance__

#include <Escargot.h>

namespace StarFish {

using namespace escargot;

class ScriptBindingInstanceDataEscargot;
class String;

const uint32_t kEscargotObjectCheckMagic = 0x0fff;
const uint32_t kEventStringAttributeCheckMagic = 0x0ffe;

class ScriptBindingInstance : public gc {
public:
    ScriptBindingInstance();
    ~ScriptBindingInstance();
    void enter();
    void exit();
    void close();
    void initBinding(StarFish* window);
    void* data()
    {
        return m_data;
    }
    String* evaluate(String* str);

protected:
    void* m_data;
#ifdef USE_ES6_FEATURE
    void* m_promiseJobQueue;
#endif // USE_ES6_FEATURE
    size_t m_enterCount;
};

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    ESObject* obj, ESString* propertyName, NativeFunctionType getter,
    NativeFunctionType setter, bool isEnumerable = true,
    bool isConfigurable = true);

ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance);

String* toBrowserString(const ESValue& v);
ESValue toJSString(String* v);

ESValue defaultFunction(ESVMInstance* instance);
ESValue errorOnConstructorFunction(ESVMInstance* instance);

typedef ESValue ScriptValue;
typedef ESObject* ScriptObject;
typedef ESFunctionObject* ScriptFunction;

#define STARFISH_ENUM_LAZY_BINDING_NAMES_DEFAULT(F) \
    F(Node)                                         \
    F(EventTarget)                                  \
    F(Window)                                       \
    F(Element)                                      \
    F(Document)                                     \
    F(DocumentType)                                 \
    F(DocumentFragment)                             \
    F(HTMLDocument)                                 \
    F(CharacterData)                                \
    F(Text)                                         \
    F(CDataSection)                                 \
    F(Comment)                                      \
    F(HTMLElement)                                  \
    F(HTMLHtmlElement)                              \
    F(HTMLHeadElement)                              \
    F(HTMLScriptElement)                            \
    F(HTMLStyleElement)                             \
    F(HTMLLinkElement)                              \
    F(HTMLBodyElement)                              \
    F(HTMLDivElement)                               \
    F(HTMLImageElement)                             \
    F(Image)                                        \
    F(HTMLBRElement)                                \
    F(HTMLObjectElement)                            \
    F(HTMLMetaElement)                              \
    F(HTMLParagraphElement)                         \
    F(HTMLPreElement)                               \
    F(HTMLSpanElement)                              \
    F(HTMLUnknownElement)                           \
    F(PseudoElement)                                \
    F(HTMLCollection)                               \
    F(Event)                                        \
    F(UIEvent)                                      \
    F(MouseEvent)                                   \
    F(TouchEvent)                                   \
    F(KeyboardEvent)                                \
    F(FocusEvent)                                   \
    F(ProgressEvent)                                \
    F(NodeList)                                     \
    F(DOMTokenList)                                 \
    F(DOMSettableTokenList)                         \
    F(NamedNodeMap)                                 \
    F(Attr)                                         \
    F(CSSStyleDeclaration)                          \
    F(CSSStyleRule)                                 \
    F(XMLHttpRequest)                               \
    F(Blob)                                         \
    F(URL)                                          \
    F(DOMRectReadOnly)                              \
    F(DOMRect)                                      \
    F(DOMPointReadOnly)                             \
    F(DOMPoint)                                     \
    F(DOMQuad)                                      \
    F(DOMRectList)                                  \
    F(Location)                                     \
    F(DOMException)                                 \
    F(History)                                      \
    F(Navigator)                                    \
    F(Geolocation)                                  \
    F(Geoposition)                                  \
    F(Coordinates)                                  \
    F(PositionError)

#ifdef STARFISH_EXP
#define STARFISH_ENUM_LAZY_BINDING_NAMES_EXP(F) F(DOMImplementation)
#else // STARFISH_EXP
#define STARFISH_ENUM_LAZY_BINDING_NAMES_EXP(F)
#endif // STARFISH_EXP

#ifdef STARFISH_ENABLE_MULTIMEDIA
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MEDIA(F) \
    F(HTMLMediaElement)                           \
    F(HTMLVideoElement)                           \
    F(HTMLAudioElement)                           \
    F(Audio)                                      \
    F(HTMLTrackElement)                           \
    F(HTMLSourceElement)                          \
    F(TextTrack)                                  \
    F(TextTrackList)                              \
    F(TextTrackCue)                               \
    F(TextTrackCueList)                           \
    F(VTTCue)                                     \
    F(TimeRanges)                                 \
    F(MediaSource)                                \
    F(SourceBuffer)                               \
    F(SourceBufferList)
#else // STARFISH_ENABLE_MULTIMEDIA
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MEDIA(F)
#endif // STARFISH_ENABLE_MULTIMEDIA

#ifdef STARFISH_ENABLE_MULTI_PAGE
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MULTI_PAGE(F) F(HTMLAnchorElement)
#else // STARFISH_ENABLE_MULTI_PAGE
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MULTI_PAGE(F)
#endif // STARFISH_ENABLE_MULTI_PAGE

#ifdef STARFISH_ENABLE_DOMPARSER
#define STARFISH_ENUM_LAZY_BINDING_NAMES_DOMPARSER(F) F(DOMParser)
#else // STARFISH_ENABLE_DOMPARSER
#define STARFISH_ENUM_LAZY_BINDING_NAMES_DOMPARSER(F)
#endif // STARFISH_ENABLE_DOMPARSER

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#define STARFISH_ENUM_LAZY_BINDING_NAMES_AVPLAY(F) \
    F(WebApis)                                     \
    F(Avplay)
#else // STARFISH_TIZEN_TV && STARFISH_ENABLE_AVPLAY
#define STARFISH_ENUM_LAZY_BINDING_NAMES_AVPLAY(F)
#endif // STARFISH_TIZEN_TV && STARFISH_ENABLE_AVPLAY

#define STARFISH_ENUM_LAZY_BINDING_NAMES(F)        \
    STARFISH_ENUM_LAZY_BINDING_NAMES_DEFAULT(F)    \
    STARFISH_ENUM_LAZY_BINDING_NAMES_EXP(F)        \
    STARFISH_ENUM_LAZY_BINDING_NAMES_MEDIA(F)      \
    STARFISH_ENUM_LAZY_BINDING_NAMES_MULTI_PAGE(F) \
    STARFISH_ENUM_LAZY_BINDING_NAMES_DOMPARSER(F)  \
    STARFISH_ENUM_LAZY_BINDING_NAMES_AVPLAY(F)

#define DEFINE_FUNCTION(functionName, parentName)                         \
    ESString* functionName##String = ESString::create(#functionName);     \
    ESFunctionObject* functionName##Function = ESFunctionObject::create(  \
        NULL, defaultFunction, functionName##String, 0, true, true);      \
    functionName##Function->defineAccessorProperty(                       \
        ESVMInstance::currentInstance()->strings().prototype.string(),    \
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), \
        false, false, false);                                             \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->forceNonVectorHiddenClass(false);                               \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->set__proto__(parentName);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentName)         \
    ESString* functionName##String = ESString::create(#functionName);     \
    ESFunctionObject* functionName##Function =                            \
        ESFunctionObject::create(NULL, errorOnConstructorFunction,        \
                                 functionName##String, 0, true, true);    \
    functionName##Function->defineAccessorProperty(                       \
        ESVMInstance::currentInstance()->strings().prototype.string(),    \
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), \
        false, false, false);                                             \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->forceNonVectorHiddenClass(false);                               \
    functionName##Function->protoType()                                   \
        .asESPointer()                                                    \
        ->asESObject()                                                    \
        ->set__proto__(parentName);

#define DEFINE_FUNCTION_WITH_PARENTFUNC(functionName, parentFunction) \
    DEFINE_FUNCTION(functionName, parentFunction->protoType())        \
    functionName##Function->set__proto__(parentFunction);

#define DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(functionName,          \
                                                        parentFunction)        \
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(functionName, parentFunction->protoType()) \
    functionName##Function->set__proto__(parentFunction);

#define INVALID_INDEX (ESValue::ESInvalidIndexValue)
#define TO_INDEX_UINT32(argValue, idx)                                        \
    uint32_t idx;                                                             \
    idx = argValue.toIndex();                                                 \
    if (idx == INVALID_INDEX) {                                               \
        double __number = argValue.toNumber();                                \
        idx = __number < 0 ? INVALID_INDEX                                    \
                           : (std::isnan(__number) ? 0 : (uint32_t)__number); \
    }

// TypeError: Illegal invocation
#define THROW_ILLEGAL_INVOCATION()                                       \
    {                                                                    \
        ESVMInstance::currentInstance()->throwError(ESValue(             \
            TypeError::create(ESString::create("Illegal invocation")))); \
        STARFISH_RELEASE_ASSERT_NOT_REACHED();                           \
    }

#define THROW_DOM_EXCEPTION(instance, errcode)                                 \
    {                                                                          \
        auto __sf = ((Window*)instance->globalObject()->extraPointerData());   \
        auto __err =                                                           \
            new DOMException(__sf->scriptBindingInstance(), errcode, nullptr); \
        ESVMInstance::currentInstance()->throwError(__err->scriptValue());     \
    }

#define CHECK_TYPEOF(v, type)                                                \
    {                                                                        \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() == \
                               kEscargotObjectCheckMagic) &&                 \
              (((ScriptWrappable*)v.asESPointer()                            \
                    ->asESObject()                                           \
                    ->extraPointerData())                                    \
                   ->is##type()))) {                                         \
            THROW_ILLEGAL_INVOCATION()                                       \
        }                                                                    \
    }

#define CHECK_TYPEOF_WITH_ERRCODE(v, type, instance, errcode)                \
    {                                                                        \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() == \
                               kEscargotObjectCheckMagic) &&                 \
              (((ScriptWrappable*)v.asESPointer()                            \
                    ->asESObject()                                           \
                    ->extraPointerData())                                    \
                   ->is##type()))) {                                         \
            THROW_DOM_EXCEPTION(instance, errcode);                          \
        }                                                                    \
    }

#define GENERATE_THIS_AND_CHECK_TYPE(type)                                   \
    ESValue thisValue =                                                      \
        instance->currentExecutionContext()->resolveThisBinding();           \
    {                                                                        \
        ESValue v = thisValue;                                               \
        if (!(v.isObject() && (v.asESPointer()->asESObject()->extraData() == \
                               kEscargotObjectCheckMagic) &&                 \
              (((ScriptWrappable*)v.asESPointer()                            \
                    ->asESObject()                                           \
                    ->extraPointerData())                                    \
                   ->is##type()))) {                                         \
            THROW_ILLEGAL_INVOCATION()                                       \
        }                                                                    \
    }                                                                        \
    type* originalObj =                                                      \
        (type*)(thisValue.asESPointer()->asESObject()->extraPointerData());

#ifdef STARFISH_ENABLE_MULTIMEDIA

#define DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F) \
    [](ESVMInstance* instance) -> ESValue {                             \
        GENERATE_THIS_AND_CHECK_TYPE(Node);                             \
        Node* nd = originalObj;                                         \
        if (nd->isElement() && nd->asElement()->isHTMLElement() &&      \
            nd->asElement()                                             \
                ->asHTMLElement()                                       \
                ->isHTML##ElementName##Element()) {                     \
            HTML##ElementName##Element* __element =                     \
                nd->asElement()                                         \
                    ->asHTMLElement()                                   \
                    ->asHTML##ElementName##Element();                   \
            RETURN_##TYPE_F(getter)                                     \
        }                                                               \
        return ESValue();                                               \
    }

#define DEFINE_HTMLELEMENT_PROPERTY_SETTER(ElementName, setter, TYPE_F)        \
    [](ESVMInstance* instance) -> ESValue {                                    \
        GENERATE_THIS_AND_CHECK_TYPE(Node);                                    \
        Node* nd = originalObj;                                                \
        if (nd->isElement() && nd->asElement()->isHTMLElement() &&             \
            nd->asElement()                                                    \
                ->asHTMLElement()                                              \
                ->isHTML##ElementName##Element()) {                            \
            ESValue v = instance->currentExecutionContext()->readArgument(0);  \
            HTML##ElementName##Element* __element =                            \
                nd->asElement()                                                \
                    ->asHTMLElement()                                          \
                    ->asHTML##ElementName##Element();                          \
            try {                                                              \
                ARG_##TYPE_F(setter) return v;                                 \
            } catch (DOMException * e) {                                       \
                ESVMInstance::currentInstance()->throwError(e->scriptValue()); \
                STARFISH_RELEASE_ASSERT_NOT_REACHED();                         \
            }                                                                  \
        }                                                                      \
        return ESValue();                                                      \
    }

#define DEFINE_HTMLELEMENT_READ_WRITE_PROPERTY(ElementName, getter, setter, \
                                               TYPE_F)                      \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(                \
        HTML##ElementName##ElementFunction->protoType()                     \
            .asESPointer()                                                  \
            ->asESObject(),                                                 \
        ESString::create(#getter),                                          \
        DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F),    \
        DEFINE_HTMLELEMENT_PROPERTY_SETTER(ElementName, setter, TYPE_F));

#define DEFINE_HTMLELEMENT_READ_ONLY_PROPERTY(ElementName, getter, TYPE_F) \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(               \
        HTML##ElementName##ElementFunction->protoType()                    \
            .asESPointer()                                                 \
            ->asESObject(),                                                \
        ESString::create(#getter),                                         \
        DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F),   \
        nullptr);

#define DEFINE_HTMLELEMENT_EVENT_PROPERTY(ElementName, getter, setter, TYPE_F) \
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(                   \
        HTML##ElementName##ElementFunction->protoType()                        \
            .asESPointer()                                                     \
            ->asESObject(),                                                    \
        ESString::create("on" #getter),                                        \
        DEFINE_HTMLELEMENT_PROPERTY_GETTER(ElementName, getter, TYPE_F),       \
        DEFINE_HTMLELEMENT_PROPERTY_SETTER(ElementName, getter, TYPE_F));

#define RETURN_TYPE_STRING(getter) return toJSString(__element->getter());
#define RETURN_TYPE_PRIMITIVE(getter) return ESValue(__element->getter());
#define RETURN_TYPE_NUMBER(getter) RETURN_TYPE_PRIMITIVE(getter)
#define RETURN_TYPE_BOOLEAN(getter) RETURN_TYPE_PRIMITIVE(getter)
#define RETURN_TYPE_SCRIPTVALUE(getter)  \
    auto __result = __element->getter(); \
    if (__result) {                      \
        return __result->scriptValue();  \
    }
#define RETURN_TYPE_EVENT(getter)                                  \
    return __element->attributeEventListener(__element->document() \
                                                 ->window()        \
                                                 ->starFish()      \
                                                 ->staticStrings() \
                                                 ->m_##getter);

#define ARG_TYPE_STRING(setter) \
    __element->setter(toBrowserString(v.toString()));
#define ARG_TYPE_NUMBER(setter)          \
    double __doubleValue = v.toNumber(); \
    if (std::isnan(__doubleValue)) {     \
        THROW_ILLEGAL_INVOCATION();      \
    }                                    \
    __element->setter(__doubleValue);
#define ARG_TYPE_BOOLEAN(setter)          \
    if (v.isBoolean()) {                  \
        __element->setter(v.asBoolean()); \
    }
#define ARG_TYPE_EVENT(setter)                                        \
    auto eventType = __element->document()                            \
                         ->window()                                   \
                         ->starFish()                                 \
                         ->staticStrings()                            \
                         ->m_##setter;                                \
    if (v.isObject() ||                                               \
        (v.isESPointer() && v.asESPointer()->isESFunctionObject())) { \
        __element->setAttributeEventListener(eventType, v);           \
    } else {                                                          \
        __element->clearAttributeEventListener(eventType);            \
    }
#endif // STARFISH_ENABLE_MULTIMEDIA
}

#endif
