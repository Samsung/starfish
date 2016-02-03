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
class HTMLCollection;
class NodeList;
class DOMTokenList;
class DOMSettableTokenList;
class NamedNodeMap;
class Attr;
class CSSStyleDeclaration;
class CSSStyleRule;
class XMLHttpRequest;

class ScriptBindingInstance;

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
    void initScriptWrappable(HTMLCollection* ptr, ScriptBindingInstance*);
    void initScriptWrappable(NodeList* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMTokenList* ptr, ScriptBindingInstance*);
    void initScriptWrappable(DOMSettableTokenList* ptr, ScriptBindingInstance*);
    void initScriptWrappable(NamedNodeMap* ptr, ScriptBindingInstance*);
    void initScriptWrappable(Attr* ptr, ScriptBindingInstance*);
    void initScriptWrappable(CSSStyleDeclaration* ptr);
    void initScriptWrappable(CSSStyleRule* ptr);
    void initScriptWrappable(XMLHttpRequest* ptr);

    bool hasProperty(String* name);

    void callFunction(String* name);
protected:
};

class ScriptWrappableGlobalObject {
public:
    ScriptWrappableGlobalObject();
    void initScriptWrappableWindow(Window* window);
    static ScriptWrappableGlobalObject* fetch();

    void callFunction(String* name);
protected:
    void* m_object;
};

}

#endif
