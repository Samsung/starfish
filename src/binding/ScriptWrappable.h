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

#ifndef __StarFishScriptWrappable__
#define __StarFishScriptWrappable__

#include "binding/ScriptBindingInstance.h"
#include <Escargot.h>
#include "StarFishConfig.h"

namespace StarFish {

using namespace escargot;

class Document;
class Element;
class ScriptBindingInstanceDataEscargot;

const uint32_t kEscargotObjectCheckMagic = 0x0fff;
const uint32_t kEventStringAttributeCheckMagic = 0x0ffe;

// https://heycam.github.io/webidl/#common-DOMTimeStamp
typedef unsigned long long DOMTimeStamp;

typedef ESValue ScriptValue;
typedef ESObject* ScriptObject;
typedef ESFunctionObject* ScriptFunction;

void defineNativeAccessorPropertyButNeedToGenerateJSFunction(
    ESObject* obj, ESString* propertyName, NativeFunctionType getter,
    NativeFunctionType setter, bool isEnumerable = true,
    bool isConfigurable = true);

ScriptBindingInstanceDataEscargot* fetchData(ScriptBindingInstance* instance);

Document* fetchDocument(ESVMInstance* instance);
StarFish* fetchStarFish(ESVMInstance* instance);

String* toBrowserString(const ESValue& v);
String* toBrowserString(const ESString* v);
ESValue toJSString(String* v);

ESValue defaultFunction(ESVMInstance* instance);
ESValue errorOnConstructorFunction(ESVMInstance* instance);

ScriptValue createScriptString(String* str);
ScriptValue createScriptFunction(String** argNames, size_t argc,
                                 String* functionBody, bool& error);
ScriptValue createAttributeStringEventFunction(Element* target,
                                               String* functionBody,
                                               bool& result);
ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc,
                               ScriptValue thisValue);
ScriptValue createArrayBuffer(void* bufferSrc, size_t len);
ScriptValue parseJSON(String* jsonData);
String* jsonStringify(ESValue);

bool isCallableScriptValue(ScriptValue v);

#define STARFISH_ENUM_LAZY_BINDING_NAMES_DEFAULT(F) \
    F(Attr)                                         \
    F(Blob)                                         \
    F(CDATASection)                                 \
    F(CharacterData)                                \
    F(Comment)                                      \
    F(Coordinates)                                  \
    F(CSSStyleDeclaration)                          \
    F(CSSStyleRule)                                 \
    F(Document)                                     \
    F(DocumentFragment)                             \
    F(DocumentType)                                 \
    F(DOMException)                                 \
    F(DOMTokenList)                                 \
    F(DOMPoint)                                     \
    F(DOMPointReadOnly)                             \
    F(DOMQuad)                                      \
    F(DOMRect)                                      \
    F(DOMRectList)                                  \
    F(DOMRectReadOnly)                              \
    F(DOMSettableTokenList)                         \
    F(Element)                                      \
    F(Event)                                        \
    F(EventTarget)                                  \
    F(FocusEvent)                                   \
    F(Geolocation)                                  \
    F(Geoposition)                                  \
    F(History)                                      \
    F(HTMLBodyElement)                              \
    F(HTMLBRElement)                                \
    F(HTMLCaptionElement)                           \
    F(HTMLColElement)                               \
    F(HTMLColGroupElement)                          \
    F(HTMLCollection)                               \
    F(HTMLDivElement)                               \
    F(HTMLDocument)                                 \
    F(HTMLElement)                                  \
    F(HTMLHeadElement)                              \
    F(HTMLHeadingElement)                           \
    F(HTMLHtmlElement)                              \
    F(HTMLImageElement)                             \
    F(HTMLLIElement)                                \
    F(HTMLLinkElement)                              \
    F(HTMLMetaElement)                              \
    F(HTMLObjectElement)                            \
    F(HTMLParagraphElement)                         \
    F(HTMLPreElement)                               \
    F(HTMLScriptElement)                            \
    F(HTMLStyleElement)                             \
    F(HTMLSpanElement)                              \
    F(HTMLTableElement)                             \
    F(HTMLTBodyElement)                             \
    F(HTMLTDElement)                                \
    F(HTMLTFootElement)                             \
    F(HTMLTHeadElement)                             \
    F(HTMLTHElement)                                \
    F(HTMLStrongElement)                            \
    F(HTMLUListElement)                             \
    F(HTMLUnknownElement)                           \
    F(Image)                                        \
    F(KeyboardEvent)                                \
    F(Location)                                     \
    F(MouseEvent)                                   \
    F(NamedNodeMap)                                 \
    F(Navigator)                                    \
    F(Node)                                         \
    F(NodeList)                                     \
    F(PositionError)                                \
    F(ProgressEvent)                                \
    F(PseudoElement)                                \
    F(URL)                                          \
    F(Text)                                         \
    F(TouchEvent)                                   \
    F(UIEvent)                                      \
    F(Window)                                       \
    F(XMLHttpRequest)

#ifdef STARFISH_EXP
#define STARFISH_ENUM_LAZY_BINDING_NAMES_EXP(F) F(DOMImplementation)
#else // STARFISH_EXP
#define STARFISH_ENUM_LAZY_BINDING_NAMES_EXP(F)
#endif // STARFISH_EXP

#ifdef STARFISH_ENABLE_MULTIMEDIA
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MEDIA(F) \
    F(HTMLAudioElement)                           \
    F(HTMLMediaElement)                           \
    F(HTMLSourceElement)                          \
    F(HTMLTrackElement)                           \
    F(HTMLVideoElement)                           \
    F(MediaSource)                                \
    F(SourceBuffer)                               \
    F(SourceBufferList)                           \
    F(TextTrack)                                  \
    F(TextTrackCue)                               \
    F(TextTrackCueList)                           \
    F(TextTrackList)                              \
    F(TimeRanges)                                 \
    F(VTTCue)

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
    F(Avplay)                                      \
    F(WebApis)
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

#define FOR_EACH_FORWARD_DECLARATION(exportName) class exportName;
STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_FORWARD_DECLARATION)
#undef FOR_EACH_FORWARD_DECLARATION

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

#define CHECK_TYPEOF_INNER(v, type)                                         \
    (v.isObject() && (v.asESPointer()->asESObject()->extraData() ==         \
                      kEscargotObjectCheckMagic) &&                         \
     (((ScriptWrappable*)v.asESPointer()->asESObject()->extraPointerData()) \
          ->is##type()))

#define CHECK_TYPEOF(v, type)               \
    {                                       \
        if (!CHECK_TYPEOF_INNER(v, type)) { \
            THROW_ILLEGAL_INVOCATION()      \
        }                                   \
    }

#define GENERATE_THIS_AND_CHECK_TYPE(type)                         \
    ESValue thisValue =                                            \
        instance->currentExecutionContext()->resolveThisBinding(); \
    {                                                              \
        CHECK_TYPEOF(thisValue, type);                             \
    }                                                              \
    type* originalObj =                                            \
        (type*)(thisValue.asESPointer()->asESObject()->extraPointerData());

#define GENERATE_ARG_AND_CHECK_TYPE(i, type)                               \
    ESValue arg##i = instance->currentExecutionContext()->readArgument(i); \
    {                                                                      \
        CHECK_TYPEOF(arg##i, type)                                         \
    }                                                                      \
    type* val##i =                                                         \
        (type*)(arg##i.asESPointer()->asESObject()->extraPointerData());

#define GENERATE_NULLABLE_ARG_AND_CHECK_TYPE(i, type)                      \
    ESValue arg##i = instance->currentExecutionContext()->readArgument(i); \
    type* val##i = nullptr;                                                \
    {                                                                      \
        if (!arg##i.isUndefinedOrNull()) {                                 \
            CHECK_TYPEOF(arg##i, type)                                     \
            val##i = (type*)(arg##i.asESPointer()                          \
                                 ->asESObject()                            \
                                 ->extraPointerData());                    \
        }                                                                  \
    }

class ScriptWrappable : public gc {
public:
#define FOR_EACH_REFLECT_FN(exportName) \
    virtual bool is##exportName() const \
    {                                   \
        return false;                   \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_REFLECT_FN);
#undef FOR_EACH_REFLECT_FN

#define FOR_EACH_CAST_FN(exportName)           \
    virtual exportName* as##exportName() const \
    {                                          \
        STARFISH_ASSERT(is##exportName());     \
        return (exportName*)this;              \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_CAST_FN);
#undef FOR_EACH_CAST_FN

    ScriptWrappable(void* extraPointerData);

    virtual ~ScriptWrappable()
    {
    }

    ScriptObject scriptObject()
    {
        if (UNLIKELY((size_t)m_object & (size_t)1)) {
            return scriptObjectSlowCase();
        }
        return m_object;
    }

    void giveUpScriptValue()
    {
        m_object = (ESObject*)1;
    }

    ScriptObject scriptObjectSlowCase();
    ScriptValue scriptValue()
    {
        return scriptObject();
    }

    virtual void initScriptObject(ScriptBindingInstance* instance) = 0;

    void initScriptWrappable(Window* ptr);
    void initScriptWrappable(Node* ptr);
    void initScriptWrappable(Node* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DocumentType* ptr);
    void initScriptWrappable(Element* ptr);
    void initScriptWrappable(Element* ptr, ScriptBindingInstance*);
    void initScriptWrappable(Document* ptr);
    void initScriptWrappable(DocumentFragment* ptr);
    void initScriptWrappable(HTMLDocument* ptr);
    void initScriptWrappable(CharacterData* ptr);
    void initScriptWrappable(Text* ptr);
    void initScriptWrappable(CDATASection* ptr);
    void initScriptWrappable(Comment* ptr);
#ifdef STARFISH_EXP
    void initScriptWrappable(DOMImplementation* ptr, ScriptBindingInstance*);
#endif
    void initScriptWrappable(HTMLElement* ptr);
    void initScriptWrappable(HTMLHtmlElement* ptr);
    void initScriptWrappable(HTMLHeadElement* ptr);
    void initScriptWrappable(HTMLBodyElement* ptr);
    void initScriptWrappable(HTMLStyleElement* ptr);
    void initScriptWrappable(HTMLLinkElement* ptr);
    void initScriptWrappable(HTMLScriptElement* ptr);
    void initScriptWrappable(HTMLImageElement* ptr);
    void initScriptWrappable(HTMLBRElement* ptr);
    void initScriptWrappable(HTMLObjectElement* ptr);
    void initScriptWrappable(HTMLDivElement* ptr);
    void initScriptWrappable(HTMLMetaElement* ptr);
    void initScriptWrappable(HTMLParagraphElement* ptr);
    void initScriptWrappable(HTMLPreElement* ptr);
    void initScriptWrappable(HTMLSpanElement* ptr);
    void initScriptWrappable(HTMLUnknownElement* ptr);
    void initScriptWrappable(PseudoElement* ptr);
#ifdef STARFISH_ENABLE_MULTI_PAGE
    void initScriptWrappable(HTMLAnchorElement* ptr);
#endif
#ifdef STARFISH_ENABLE_MULTIMEDIA
    void initScriptWrappable(HTMLMediaElement* ptr);
    void initScriptWrappable(HTMLVideoElement* ptr);
    void initScriptWrappable(HTMLAudioElement* ptr);
    void initScriptWrappable(HTMLTrackElement* ptr);
    void initScriptWrappable(HTMLSourceElement* ptr);
    void initScriptWrappable(TextTrack* ptr);
    void initScriptWrappable(TextTrackList* ptr);
    void initScriptWrappable(TextTrackCue* ptr);
    void initScriptWrappable(TextTrackCueList* ptr);
    void initScriptWrappable(VTTCue* ptr);
    void initScriptWrappable(TimeRanges* ptr);
    void initScriptWrappable(MediaSource* ptr);
    void initScriptWrappable(SourceBuffer* ptr);
    void initScriptWrappable(SourceBufferList* ptr);
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    void initScriptWrappable(WebApis* ptr);
    void initScriptWrappable(Avplay* ptr);
#endif
#endif
    void initScriptWrappable(Event* event);
    void initScriptWrappable(UIEvent* ptr);
    void initScriptWrappable(MouseEvent* ptr);
    void initScriptWrappable(TouchEvent* ptr);
    void initScriptWrappable(KeyboardEvent* ptr);
    void initScriptWrappable(FocusEvent* ptr);
    void initScriptWrappable(ProgressEvent* ptr);
    void initScriptWrappable(HTMLCollection* ptr, ScriptBindingInstance*);
    void initScriptWrappable(NodeList* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMTokenList* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMSettableTokenList* ptr, ScriptBindingInstance*);
    void initScriptWrappable(NamedNodeMap* ptr, ScriptBindingInstance*);
    void initScriptWrappable(Attr* ptr, ScriptBindingInstance*);
    void initScriptWrappable(CSSStyleDeclaration* ptr);
    void initScriptWrappable(CSSStyleRule* ptr);
    void initScriptWrappable(XMLHttpRequest* ptr);
    void initScriptWrappable(Blob* ptr);
    void initScriptWrappable(URL* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMException* exception,
                             ScriptBindingInstance* instance);
    void initScriptWrappable(Location* ptr);
    void initScriptWrappable(History* ptr);
    void initScriptWrappable(Navigator* ptr);
    void initScriptWrappable(Geolocation* ptr);
    void initScriptWrappable(Geoposition* ptr);
    void initScriptWrappable(Coordinates* ptr);
    void initScriptWrappable(PositionError* ptr);
    void initScriptWrappable(DOMParser* ptr);
    void initScriptWrappable(DOMRectReadOnly* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMRect* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMPointReadOnly* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMPoint* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMQuad* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMRectList* ptr, ScriptBindingInstance*);

    bool hasProperty(String* name);

private:
    ESObject* m_object;
};

#ifdef USE_ES6_FEATURE
class Promise : public gc {
public:
    Promise();
    void fulfill(ScriptValue v);
    void reject(ScriptValue v);
    ScriptValue scriptValue()
    {
        return m_scriptValue;
    }

protected:
    ScriptValue m_scriptValue;
};
#endif
}

#endif
