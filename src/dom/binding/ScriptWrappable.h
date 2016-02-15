#ifndef __StarFishScriptWrappable__
#define __StarFishScriptWrappable__

#include <Escargot.h>

namespace StarFish {

template <typename T>
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
typedef escargot::ESFunctionObject* ScriptFunction;
#define ScriptValueUndefined escargot::ESValue()
#define ScriptValueNull escargot::ESValue(escargot::ESValue::ESNull)

class ScriptWrappable : public escargot::ESObject {
public:
    enum Type {
        None,
        WindowObject,
        NodeObject,
        EventObject,
        ElementObject,
        HTMLCollectionObject,
        NodeListObject,
        DOMTokenListObject,
        DOMSettableTokenListObject,
        NamedNodeMapObject,
        AttrObject,
        CSSStyleDeclarationObject,
        CSSStyleRuleObject,
        XMLHttpRequestObject,
        BlobObject,
        URLObject,
        DOMExceptionObject,
#ifdef TIZEN_DEVICE_API
        NativePluginManagerObject,
#endif
    };
    ScriptWrappable();

    inline void* operator new(size_t size)
    {
        return GC_MALLOC(size);
    }

    inline void operator delete(void* obj)
    {
        GC_free(obj);
    }

    ScriptValue scriptValue()
    {
        return this;
    }

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
};

class ScriptWrappableGlobalObject {
public:
    ScriptWrappableGlobalObject();
    void initScriptWrappableWindow(Window* window);
    static ScriptWrappableGlobalObject* fetch();

    // deprecated
    void callFunction(String* name);
protected:
    void* m_object;
};

ScriptValue createScriptString(String* str);
ScriptValue createScriptFunction(String** argNames, size_t argc, String* functionBody);
void callScriptFunction(ScriptValue fn, ScriptValue* argv, size_t argc, ScriptValue thisValue);

}

#endif
