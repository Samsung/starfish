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

namespace Escargot {
class VMInstanceRef;
class ContextRef;
class StringRef;
class ValueRef;
class PointerValueRef;
class ObjectRef;
class GlobalObjectRef;
class FunctionObjectRef;
class ScriptRef;
class ScriptParserRef;
class ExecutionStateRef;
typedef ValueRef* (*ScriptNativeFunctionPointer)(ExecutionStateRef* state,
                                                 ValueRef* thisValue,
                                                 size_t argc, ValueRef** argv,
                                                 bool isNewExpression);
}

#include "binding/ScriptEngineInstance.h"

#define STARFISH_ENUM_LAZY_BINDING_NAMES_DEFAULT(F) \
    F(Attr)                                         \
    F(Blob)                                         \
    F(CDATASection)                                 \
    F(CharacterData)                                \
    F(Comment)                                      \
    F(Coordinates)                                  \
    F(CSSConditionRule)                             \
    F(CSSGroupingRule)                              \
    F(CSSImportRule)                                \
    F(CSSMediaRule)                                 \
    F(CSSRule)                                      \
    F(CSSStyleDeclaration)                          \
    F(CSSStyleRule)                                 \
    F(CSSStyleSheet)                                \
    F(CSSRuleList)                                  \
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
    F(HTMLTableCaptionElement)                      \
    F(HTMLTableCellElement)                         \
    F(HTMLTableColElement)                          \
    F(HTMLTableElement)                             \
    F(HTMLTableSectionElement)                      \
    F(HTMLTableRowElement)                          \
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
    F(Screen)                                       \
    F(Storage)                                      \
    F(StorageImpl)                                  \
    F(StyleSheet)                                   \
    F(URL)                                          \
    F(Text)                                         \
    F(TouchEvent)                                   \
    F(UIEvent)                                      \
    F(Window)                                       \
    F(XMLDocument)                                  \
    F(XMLHttpRequest)                               \
    F(XMLHttpRequestEventTarget)

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

namespace StarFish {

class StarFish;
class Window;
class Document;
class ScriptEngineInstance;
class ScriptBindingInstance;
class String;

#define FOR_EACH_DECLARE_FN(exportName)               \
    Escargot::FunctionObjectRef* binding##exportName( \
        ScriptBindingInstance* scriptBindingInstance);

STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_DECLARE_FN);
#undef FOR_EACH_DECLARE_FN

class ScriptBindingInstance : public gc {
public:
    ScriptBindingInstance(ScriptEngineInstance* engineInstance,
                          Window* ownerWindow);
    void initBinding(Document* ownerDocument);
    void close();

    Document* ownerDocument()
    {
        return m_ownerDocument;
    }

    Window* ownerWindow()
    {
        return m_ownerWindow;
    }

#define FOR_EACH_GETTER_FN(exportName)                                   \
    Escargot::FunctionObjectRef* fn##exportName()                        \
    {                                                                    \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                     \
            m_fn##exportName = binding##exportName(this);                \
            m_value##exportName =                                        \
                reinterpret_cast<Escargot::ValueRef*>(m_fn##exportName); \
        }                                                                \
        return m_fn##exportName;                                         \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_FN)
#undef FOR_EACH_GETTER_FN

#define FOR_EACH_GETTER_VALUE_FN(exportName)                             \
    Escargot::ValueRef* value##exportName()                              \
    {                                                                    \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                     \
            m_fn##exportName = binding##exportName(this);                \
            m_value##exportName =                                        \
                reinterpret_cast<Escargot::ValueRef*>(m_fn##exportName); \
        }                                                                \
        return m_value##exportName;                                      \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_VALUE_FN)
#undef FOR_EACH_GETTER_VALUE_FN

#define FOR_EACH_SCRIPT_FN(exportName) \
    Escargot::FunctionObjectRef* m_fn##exportName;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPT_FN)
#undef FOR_EACH_SCRIPT_FN

public:
#define FOR_EACH_SCRIPTVALUE_FN(exportName) \
    Escargot::ValueRef* m_value##exportName;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
#undef FOR_EACH_SCRIPTVALUE_FN

    Escargot::ContextRef* scriptContext()
    {
        return m_scriptContext;
    }

protected:
    Escargot::ContextRef* m_scriptContext;
    Window* m_ownerWindow;
    Document* m_ownerDocument;
};
}

#endif
