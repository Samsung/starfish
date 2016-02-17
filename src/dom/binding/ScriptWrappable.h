#ifndef __StarFishScriptWrappable__
#define __StarFishScriptWrappable__

#include <Escargot.h>

namespace StarFish {

class EventTarget;
class Window;
class Node;
class Element;
class Document;
class DocumentType;
class HTMLDocument;
class CharacterData;
class Text;
class Comment;
class HTMLElement;
class HTMLHtmlElement;
class HTMLHeadElement;
class HTMLBodyElement;
class HTMLScriptElement;
class HTMLDivElement;
class HTMLImageElement;
class HTMLBRElement;
class HTMLAudioElement;
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
        AttrObject = 1 << 9,
        CSSStyleDeclarationObject = 1 << 10,
        CSSStyleRuleObject = 1 << 11,
        XMLHttpRequestObject = 1 << 12 | EventTargetObject,
        BlobObject = 1 << 13,
        URLObject = 1 << 14,
        DOMExceptionObject = 1 << 15,
#ifdef TIZEN_DEVICE_API
        NativePluginManagerObject = 1 << 16,
#endif
    };
    ScriptWrappable(void* extraPointerData);

    ScriptValue scriptValue()
    {
        return m_object;
    }

    ScriptObject scriptObject()
    {
        return m_object;
    }

    void initScriptWrappable(Window* ptr);
    void initScriptWrappable(Node* ptr);
    void initScriptWrappable(Node* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DocumentType* ptr);
    void initScriptWrappable(Element* ptr);
    void initScriptWrappable(Element* ptr, ScriptBindingInstance*);
    void initScriptWrappable(Document* ptr);
    void initScriptWrappable(HTMLDocument* ptr);
    void initScriptWrappable(CharacterData* ptr);
    void initScriptWrappable(Text* ptr);
    void initScriptWrappable(Comment* ptr);
    void initScriptWrappable(HTMLElement* ptr);
    void initScriptWrappable(HTMLHtmlElement* ptr);
    void initScriptWrappable(HTMLHeadElement* ptr);
    void initScriptWrappable(HTMLBodyElement* ptr);
    void initScriptWrappable(HTMLScriptElement* ptr);
    void initScriptWrappable(HTMLImageElement* ptr);
    void initScriptWrappable(HTMLBRElement* ptr);
    void initScriptWrappable(HTMLDivElement* ptr);
    void initScriptWrappable(HTMLUnknownElement* ptr);
    void initScriptWrappable(HTMLAudioElement* ptr);

    void initScriptWrappable(Event* ptr, ScriptBindingInstance*);
    void initScriptWrappable(UIEvent* ptr, ScriptBindingInstance*);
    void initScriptWrappable(MouseEvent* ptr, ScriptBindingInstance*);
    void initScriptWrappable(ProgressEvent* ptr, ScriptBindingInstance* instance,uint32_t loaded,uint32_t total);
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
    void initScriptWrappable(URL* ptr,ScriptBindingInstance*);
    void initScriptWrappable(DOMException* exception, ScriptBindingInstance* instance);

    bool hasProperty(String* name);

protected:
    escargot::ESObject* m_object;
};

ScriptValue createScriptString(String* str);
ScriptValue createScriptFunction(String** argNames, size_t argc, String* functionBody);
ScriptValue callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc, ScriptValue thisValue);
ScriptValue parseJSON(String* jsonData);

}

#endif
