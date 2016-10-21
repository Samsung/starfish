/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include <Escargot.h>

namespace StarFish {

class EventTarget;
class Window;
class Node;
class Element;
class Document;
class DocumentFragment;
class DocumentType;
class HTMLDocument;
class CharacterData;
class Text;
class Comment;
#ifdef STARFISH_ENABLE_MULTIMEDIA
class TimeRanges;
class TextTrack;
class TextTrackList;
class TextTrackCue;
class TextTrackCueList;
class VTTCue;
class MediaSource;
class SourceBuffer;
class SourceBufferList;
#endif
class HTMLElement;
class HTMLHtmlElement;
class HTMLHeadElement;
class HTMLScriptElement;
class HTMLStyleElement;
class HTMLLinkElement;
class HTMLBodyElement;
class HTMLDivElement;
class HTMLImageElement;
class HTMLBRElement;
class HTMLMetaElement;
class HTMLParagraphElement;
class HTMLSpanElement;
#ifdef STARFISH_ENABLE_MULTI_PAGE
class HTMLAnchorElement;
#endif
#ifdef STARFISH_ENABLE_MULTIMEDIA
class HTMLMediaElement;
class HTMLVideoElement;
class HTMLAudioElement;
class HTMLTrackElement;
#endif
class HTMLUnknownElement;
class Event;
class UIEvent;
class MouseEvent;
class KeyboardEvent;
class ProgressEvent;
class HTMLCollection;
class NodeList;
class DOMTokenList;
class DOMSettableTokenList;
class NamedNodeMap;
class Attr;
class CSSStyleDeclaration;
class CSSStyleRule;
class XMLHttpRequest;
class Blob;
class URL;
class DOMException;
#ifdef STARFISH_EXP
class DOMImplementation;
#endif
class Navigator;
class ScriptBindingInstance;
class Geolocation;
class Geoposition;
class PositionError;
class Coordinates;
class DOMParser;

typedef escargot::ESValue ScriptValue;
typedef escargot::ESObject* ScriptObject;
typedef escargot::ESFunctionObject* ScriptFunction;
#define ScriptValueUndefined escargot::ESValue()
#define ScriptValueNull escargot::ESValue(escargot::ESValue::ESNull)

class ScriptWrappable : public gc {
public:
    enum Type {
        None = 0,
        EventTargetObject = 1 << 0,
        WindowObject = 1 << 1 | EventTargetObject,
        NodeObject = 1 << 2 | EventTargetObject,
        EventObject = 1 << 3,
        HTMLCollectionObject = 1 << 4,
        NodeListObject = 1 << 5,
        DOMTokenListObject = 1 << 6,
        DOMSettableTokenListObject = 1 << 7,
        NamedNodeMapObject = 1 << 8,
        CSSStyleDeclarationObject = 1 << 9,
        CSSStyleRuleObject = 1 << 10,
        XMLHttpRequestObject = 1 << 11 | EventTargetObject,
        BlobObject = 1 << 12,
        URLObject = 1 << 13,
        DOMExceptionObject = 1 << 14,
        AttributeStringEventFunctionObject = 1 << 15,
#ifdef STARFISH_EXP
        DOMImplementationObject = 1 << 16,
#endif
#ifdef STARFISH_ENABLE_MULTIMEDIA
        TextTrackObject = 1 << 17 | EventTargetObject,
        TextTrackListObject = 1 << 18 | EventTargetObject,
        TextTrackCueObject = 1 << 19 | EventTargetObject,
        TextTrackCueListObject = 1 << 20,
        TimeRangesObject = 1 << 21,
        MediaSourceObject = 1 << 22 | EventTargetObject,
        SourceBufferObject = 1 << 23 | EventTargetObject,
        SourceBufferListObject = 1 << 24 | EventTargetObject,
#endif
        NavigatorObject = 1 << 25,
        GeolocationObject = 1 << 26,
        GeopositionObject = 1 << 27,
        PositionErrorObject = 1 << 28,
        CoordinatesObject = 1 << 29,
        DOMParserObject = 1 << 30,
    };
    ScriptWrappable(void* extraPointerData);

    ScriptObject scriptObject()
    {
        if (UNLIKELY((size_t)m_object & (size_t)1)) {
            return scriptObjectSlowCase();
        }
        return m_object;
    }

    ScriptObject scriptObjectSlowCase();
    ScriptValue scriptValue()
    {
        return scriptObject();
    }


    virtual void initScriptObject(ScriptBindingInstance* instance) = 0;
    virtual Type type() = 0;

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
    void initScriptWrappable(HTMLDivElement* ptr);
    void initScriptWrappable(HTMLMetaElement* ptr);
    void initScriptWrappable(HTMLParagraphElement* ptr);
    void initScriptWrappable(HTMLSpanElement* ptr);
    void initScriptWrappable(HTMLUnknownElement* ptr);
#ifdef STARFISH_ENABLE_MULTI_PAGE
    void initScriptWrappable(HTMLAnchorElement* ptr);
#endif
#ifdef STARFISH_ENABLE_MULTIMEDIA
    void initScriptWrappable(HTMLMediaElement* ptr);
    void initScriptWrappable(HTMLVideoElement* ptr);
    void initScriptWrappable(HTMLAudioElement* ptr);
    void initScriptWrappable(HTMLTrackElement* ptr);
    void initScriptWrappable(TextTrack* ptr);
    void initScriptWrappable(TextTrackList* ptr);
    void initScriptWrappable(TextTrackCue* ptr);
    void initScriptWrappable(TextTrackCueList* ptr);
    void initScriptWrappable(VTTCue* ptr);
    void initScriptWrappable(TimeRanges* ptr);
    void initScriptWrappable(MediaSource* ptr);
    void initScriptWrappable(SourceBuffer* ptr);
    void initScriptWrappable(SourceBufferList* ptr);
#endif
    void initScriptWrappable(Event* event);
    void initScriptWrappable(UIEvent* ptr);
    void initScriptWrappable(MouseEvent* ptr);
    void initScriptWrappable(KeyboardEvent* ptr);
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
    void initScriptWrappable(DOMException* exception, ScriptBindingInstance* instance);
    void initScriptWrappable(Navigator* ptr);
    void initScriptWrappable(Geolocation* ptr);
    void initScriptWrappable(Geoposition* ptr);
    void initScriptWrappable(Coordinates* ptr);
    void initScriptWrappable(PositionError* ptr);
    void initScriptWrappable(DOMParser* ptr);

    bool hasProperty(String* name);

private:
    escargot::ESObject* m_object;
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

ScriptValue createScriptString(String* str);
ScriptValue createScriptFunction(String** argNames, size_t argc, String* functionBody, bool& error);
ScriptValue createAttributeStringEventFunction(Element* target, String* functionBody, bool& result);
ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc, ScriptValue thisValue);
ScriptValue createArrayBuffer(void* bufferSrc, size_t len);
ScriptValue parseJSON(String* jsonData);

bool isCallableScriptValue(ScriptValue v);

}

#endif
