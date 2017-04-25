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

#ifndef __StarFishScriptBindingInstanceDataEscargot__
#define __StarFishScriptBindingInstanceDataEscargot__

#include "StarFishConfig.h"
#include "binding/ScriptBindingInstance.h"

#include <Escargot.h>

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
    F(HTMLStrongElement)                            \
    F(HTMLStyleElement)                             \
    F(HTMLSpanElement)                              \
    F(HTMLTableElement)                             \
    F(HTMLTBodyElement)                             \
    F(HTMLTDElement)                                \
    F(HTMLTFootElement)                             \
    F(HTMLTHeadElement)                             \
    F(HTMLTHElement)                                \
    F(HTMLTRElement)                                \
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

using namespace escargot;

#define FOR_EACH_DECLARE_FN(exportName)    \
    ESFunctionObject* binding##exportName( \
        ScriptBindingInstance* scriptBindingInstance);

STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_DECLARE_FN);
#undef FOR_EACH_DECLARE_FN

class ScriptBindingInstanceDataEscargot : public gc {
    friend void ScriptBindingInstance::initBinding(StarFish* sf);

public:
    ScriptBindingInstance* m_bindingInstance;
    ESVMInstance* m_instance;
    ESFunctionObject* m_orgToString;

    ScriptBindingInstanceDataEscargot(ScriptBindingInstance* bindingInstance)
    {
        memset(this, 0, sizeof(ScriptBindingInstanceDataEscargot));
        m_bindingInstance = bindingInstance;
    }

#define FOR_EACH_GETTER_FN(exportName)                                 \
    ESFunctionObject* fn##exportName()                                 \
    {                                                                  \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                   \
            m_fn##exportName = binding##exportName(m_bindingInstance); \
            m_value##exportName = m_fn##exportName;                    \
        }                                                              \
        return m_fn##exportName;                                       \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_FN)
#undef FOR_EACH_GETTER_FN

#define FOR_EACH_GETTER_VALUE_FN(exportName)                           \
    ESValue value##exportName()                                        \
    {                                                                  \
        if (UNLIKELY(m_fn##exportName == nullptr)) {                   \
            m_fn##exportName = binding##exportName(m_bindingInstance); \
            m_value##exportName = m_fn##exportName;                    \
        }                                                              \
        return m_value##exportName;                                    \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_VALUE_FN)
#undef FOR_EACH_GETTER_VALUE_FN

#define FOR_EACH_SCRIPT_FN(exportName) ESFunctionObject* m_fn##exportName;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPT_FN)
#undef FOR_EACH_SCRIPT_FN

public:
#define FOR_EACH_SCRIPTVALUE_FN(exportName) ESValue m_value##exportName;
    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
#undef FOR_EACH_SCRIPTVALUE_FN
};
}

#endif
