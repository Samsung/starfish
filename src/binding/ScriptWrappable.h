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

#include <Escargot.h>
#include "binding/ScriptBindingInstance.h"

namespace StarFish {

using namespace escargot;

#define FOR_EACH_FORWARD_DECLARATION(exportName) class exportName;
STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_FORWARD_DECLARATION)
#undef FOR_EACH_FORWARD_DECLARATION

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
}

#endif
