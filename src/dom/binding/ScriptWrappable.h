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
#ifdef STARFISH_ENABLE_AUDIO
class HTMLAudioElement;
#endif
class HTMLUnknownElement;
class Event;
class UIEvent;
class MouseEvent;
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
class ScriptBindingInstance;

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
#ifdef TIZEN_DEVICE_API
        NativePluginManagerObject = 1 << 16,
#endif
#ifdef STARFISH_EXP
        DOMImplementationObject = 1 << 17,
#endif
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
#ifdef STARFISH_ENABLE_AUDIO
    void initScriptWrappable(HTMLAudioElement* ptr);
#endif

    void initScriptWrappable(Event* event);
    void initScriptWrappable(UIEvent* ptr, ScriptBindingInstance*);
    void initScriptWrappable(MouseEvent* ptr, ScriptBindingInstance*);
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

    bool hasProperty(String* name);

private:
    escargot::ESObject* m_object;
};

ScriptValue createScriptString(String* str);
ScriptValue createScriptFunction(String** argNames, size_t argc, String* functionBody, bool& error);
ScriptValue createAttributeStringEventFunction(Element* target, String* functionBody, bool& result);
ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc, ScriptValue thisValue);
ScriptValue parseJSON(String* jsonData);
bool isCallableScriptValue(ScriptValue v);

}

#endif
