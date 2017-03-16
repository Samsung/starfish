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

#include "dom/binding/ScriptBindingInstance.h"

#include <Escargot.h>

namespace StarFish {

using namespace escargot;

#define STARFISH_ENUM_LAZY_BINDING_NAMES_DEFAULT(F) \
    F(node, Node)                                   \
    F(element, Element)                             \
    F(document, Document)                           \
    F(documentType, DocumentType)                   \
    F(documentFragment, DocumentFragment)           \
    F(htmlDocument, HTMLDocument)                   \
    F(characterData, CharacterData)                 \
    F(text, Text)                                   \
    F(cDataSection, CDataSection)                   \
    F(comment, Comment)                             \
    F(htmlElement, HTMLElement)                     \
    F(htmlHtmlElement, HTMLHtmlElement)             \
    F(htmlHeadElement, HTMLHeadElement)             \
    F(htmlScriptElement, HTMLScriptElement)         \
    F(htmlStyleElement, HTMLStyleElement)           \
    F(htmlLinkElement, HTMLLinkElement)             \
    F(htmlBodyElement, HTMLBodyElement)             \
    F(htmlDivElement, HTMLDivElement)               \
    F(htmlImageElement, HTMLImageElement)           \
    F(htmlBrElement, HTMLBRElement)                 \
    F(htmlObjectElement, HTMLObjectElement)         \
    F(htmlMetaElement, HTMLMetaElement)             \
    F(htmlParagraphElement, HTMLParagraphElement)   \
    F(htmlPreElement, HTMLPreElement)               \
    F(htmlSpanElement, HTMLSpanElement)             \
    F(htmlUnknownElement, HTMLUnknownElement)       \
    F(pseudoElement, PseudoElement)                 \
    F(htmlCollection, HTMLCollection)               \
    F(event, Event)                                 \
    F(uiEvent, UIEvent)                             \
    F(mouseEvent, MouseEvent)                       \
    F(touchEvent, TouchEvent)                       \
    F(keyboardEvent, KeyboardEvent)                 \
    F(focusEvent, FocusEvent)                       \
    F(progressEvent, ProgressEvent)                 \
    F(nodeList, NodeList)                           \
    F(domTokenList, DOMTokenList)                   \
    F(domSettableTokenList, DOMSettableTokenList)   \
    F(namedNodeMap, NamedNodeMap)                   \
    F(attr, Attr)                                   \
    F(cssStyleDeclaration, CSSStyleDeclaration)     \
    F(cssStyleRule, CSSStyleRule)                   \
    F(xhrElement, XMLHttpRequest)                   \
    F(blobElement, Blob)                            \
    F(url, URL)                                     \
    F(domRectReadOnly, DOMRectReadOnly)             \
    F(domRect, DOMRect)                             \
    F(domPointReadOnly, DOMPointReadOnly)           \
    F(domPoint, DOMPoint)                           \
    F(domQuad, DOMQuad)                             \
    F(domRectList, DOMRectList)                     \
    F(location, Location)                           \
    F(domException, DOMException)                   \
    F(history, History)                             \
    F(navigator, Navigator)                         \
    F(geolocation, Geolocation)                     \
    F(geoposition, Geoposition)                     \
    F(coordinates, Coordinates)                     \
    F(positionError, PositionError)

#ifdef STARFISH_ENABLE_MULTIMEDIA
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MEDIA(F) \
    F(htmlMediaElement, HTMLMediaElement)         \
    F(htmlVideoElement, HTMLVideoElement)         \
    F(htmlAudioElement, HTMLAudioElement)         \
    F(htmlTrackElement, HTMLTrackElement)         \
    F(htmlSourceElement, HTMLSourceElement)       \
    F(textTrack, TextTrack)                       \
    F(textTrackList, TextTrackList)               \
    F(textTrackCue, TextTrackCue)                 \
    F(textTrackCueList, TextTrackCueList)         \
    F(VTTCue, VTTCue)                             \
    F(timeRanges, TimeRanges)                     \
    F(mediaSource, MediaSource)                   \
    F(sourceBuffer, SourceBuffer)                 \
    F(sourceBufferList, SourceBufferList)
#else
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MEDIA(F)
#endif

#ifdef STARFISH_ENABLE_MULTI_PAGE
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MULTI_PAGE(F) \
    F(htmlAnchorElement, HTMLAnchorElement)
#else
#define STARFISH_ENUM_LAZY_BINDING_NAMES_MULTI_PAGE(F)
#endif

#ifdef STARFISH_ENABLE_DOMPARSER
#define STARFISH_ENUM_LAZY_BINDING_NAMES_DOMPARSER(F) F(domParser, DOMParser)
#else
#define STARFISH_ENUM_LAZY_BINDING_NAMES_DOMPARSER(F)
#endif

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#define STARFISH_ENUM_LAZY_BINDING_NAMES_AVPLAY(F) \
    F(webApis, webapis)                            \
    F(avPlay, avplay)
#else
#define STARFISH_ENUM_LAZY_BINDING_NAMES_AVPLAY(F)
#endif

#define STARFISH_ENUM_LAZY_BINDING_NAMES(F)        \
    STARFISH_ENUM_LAZY_BINDING_NAMES_DEFAULT(F)    \
    STARFISH_ENUM_LAZY_BINDING_NAMES_MEDIA(F)      \
    STARFISH_ENUM_LAZY_BINDING_NAMES_MULTI_PAGE(F) \
    STARFISH_ENUM_LAZY_BINDING_NAMES_DOMPARSER(F)  \
    STARFISH_ENUM_LAZY_BINDING_NAMES_AVPLAY(F)

#define FOR_EACH_DECLARE_FN(codeName, exportName) \
    ESFunctionObject* binding##exportName(        \
        ScriptBindingInstance* scriptBindingInstance);

STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_DECLARE_FN);

class ScriptBindingInstanceDataEscargot : public gc {
    friend void ScriptBindingInstance::initBinding(StarFish* sf);

public:
    ScriptBindingInstance* m_bindingInstance;
    ESVMInstance* m_instance;
    ESFunctionObject* m_orgToString;
    ESFunctionObject* m_eventTarget;
    ESFunctionObject* m_window;
#ifdef STARFISH_EXP
    ESFunctionObject* m_domImplementation;
#endif

    ScriptBindingInstanceDataEscargot(ScriptBindingInstance* bindingInstance)
    {
        memset(this, 0, sizeof(ScriptBindingInstanceDataEscargot));
        m_bindingInstance = bindingInstance;
    }

#define FOR_EACH_GETTER_FN(codeName, exportName)                   \
    ESFunctionObject* codeName()                                   \
    {                                                              \
        if (UNLIKELY(m_##codeName == nullptr)) {                   \
            m_##codeName = binding##exportName(m_bindingInstance); \
            m_value##codeName = m_##codeName;                      \
        }                                                          \
        return m_##codeName;                                       \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_FN)

#define FOR_EACH_GETTER_VALUE_FN(codeName, exportName)             \
    ESValue codeName##Value()                                      \
    {                                                              \
        if (UNLIKELY(m_##codeName == nullptr)) {                   \
            m_##codeName = binding##exportName(m_bindingInstance); \
            m_value##codeName = m_##codeName;                      \
        }                                                          \
        return m_value##codeName;                                  \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_VALUE_FN)

private:
    ESFunctionObject* m_node;
    ESFunctionObject* m_element;
    ESFunctionObject* m_document;
    ESFunctionObject* m_documentType;
    ESFunctionObject* m_documentFragment;
    ESFunctionObject* m_htmlDocument;
    ESFunctionObject* m_characterData;
    ESFunctionObject* m_text;
    ESFunctionObject* m_cDataSection;
    ESFunctionObject* m_comment;
#ifdef STARFISH_ENABLE_MULTIMEDIA
    ESFunctionObject* m_textTrack;
    ESFunctionObject* m_textTrackList;
    ESFunctionObject* m_textTrackCue;
    ESFunctionObject* m_textTrackCueList;
    ESFunctionObject* m_VTTCue;
    ESFunctionObject* m_timeRanges;
    ESFunctionObject* m_htmlMediaElement;
    ESFunctionObject* m_htmlVideoElement;
    ESFunctionObject* m_htmlAudioElement;
    ESFunctionObject* m_htmlTrackElement;
    ESFunctionObject* m_htmlSourceElement;
    ESFunctionObject* m_mediaSource;
    ESFunctionObject* m_sourceBuffer;
    ESFunctionObject* m_sourceBufferList;
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
    ESFunctionObject* m_webApis;
    ESFunctionObject* m_avPlay;
#endif
#endif
#ifdef STARFISH_ENABLE_MULTI_PAGE
    ESFunctionObject* m_htmlAnchorElement;
#endif
    ESFunctionObject* m_htmlElement;
    ESFunctionObject* m_htmlHtmlElement;
    ESFunctionObject* m_htmlHeadElement;
    ESFunctionObject* m_htmlScriptElement;
    ESFunctionObject* m_htmlStyleElement;
    ESFunctionObject* m_htmlLinkElement;
    ESFunctionObject* m_htmlBodyElement;
    ESFunctionObject* m_htmlDivElement;
    ESFunctionObject* m_htmlImageElement;
    ESFunctionObject* m_htmlBrElement;
    ESFunctionObject* m_htmlObjectElement;
    ESFunctionObject* m_htmlMetaElement;
    ESFunctionObject* m_htmlParagraphElement;
    ESFunctionObject* m_htmlPreElement;
    ESFunctionObject* m_htmlSpanElement;
    ESFunctionObject* m_htmlCollection;
    ESFunctionObject* m_htmlUnknownElement;
    ESFunctionObject* m_pseudoElement;
    ESFunctionObject* m_event;
    ESFunctionObject* m_uiEvent;
    ESFunctionObject* m_mouseEvent;
    ESFunctionObject* m_touchEvent;
    ESFunctionObject* m_keyboardEvent;
    ESFunctionObject* m_focusEvent;
    ESFunctionObject* m_progressEvent;
    ESFunctionObject* m_nodeList;
    ESFunctionObject* m_domTokenList;
    ESFunctionObject* m_domSettableTokenList;
    ESFunctionObject* m_namedNodeMap;
    ESFunctionObject* m_attr;
    ESFunctionObject* m_cssStyleDeclaration;
    ESFunctionObject* m_cssStyleRule;
    ESFunctionObject* m_xhrElement;
    ESFunctionObject* m_blobElement;
    ESFunctionObject* m_url;
    ESFunctionObject* m_location;
    ESFunctionObject* m_domException;
    ESFunctionObject* m_history;
    ESFunctionObject* m_navigator;
    ESFunctionObject* m_geolocation;
    ESFunctionObject* m_coordinates;
    ESFunctionObject* m_geoposition;
    ESFunctionObject* m_positionError;
    ESFunctionObject* m_domParser;
    ESFunctionObject* m_domRectReadOnly;
    ESFunctionObject* m_domRect;
    ESFunctionObject* m_domPointReadOnly;
    ESFunctionObject* m_domPoint;
    ESFunctionObject* m_domQuad;
    ESFunctionObject* m_domRectList;

public:
#define FOR_EACH_SCRIPTVALUE_FN(codeName, exportName) ESValue m_value##codeName;

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
};
}

#endif
