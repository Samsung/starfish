#ifndef __StarFishScriptBindingInstanceDataEscargot__
#define __StarFishScriptBindingInstanceDataEscargot__


namespace StarFish {

#define STARFISH_ENUM_LAZY_BINDING_NAMES(F) \
    F(node, Node) \
    F(element, Element) \
    F(document, Document) \
    F(documentType, DocumentType) \
    F(documentFragment, DocumentFragment) \
    F(htmlDocument, HTMLDocument) \
    F(characterData, CharacterData) \
    F(text, Text) \
    F(comment, Comment) \
    F(htmlElement, HTMLElement) \
    F(htmlHtmlElement, HTMLHtmlElement) \
    F(htmlHeadElement, HTMLHeadElement) \
    F(htmlScriptElement, HTMLScriptElement) \
    F(htmlStyleElement, HTMLStyleElement) \
    F(htmlLinkElement, HTMLLinkElement) \
    F(htmlBodyElement, HTMLBodyElement) \
    F(htmlDivElement, HTMLDivElement) \
    F(htmlImageElement, HTMLImageElement) \
    F(htmlBrElement, HTMLBRElement) \
    F(htmlMetaElement, HTMLMetaElement) \
    F(htmlParagraphElement, HTMLParagraphElement) \
    F(htmlSpanElement, HTMLSpanElement) \
    F(htmlUnknownElement, HTMLUnknownElement) \
    F(htmlCollection, HTMLCollection) \
    F(event, Event) \
    F(uiEvent, UIEvent) \
    F(mouseEvent, MouseEvent) \
    F(progressEvent, ProgressEvent) \
    F(nodeList, NodeList) \
    F(domTokenList, DOMTokenList) \
    F(domSettableTokenList, DOMSettableTokenList) \
    F(namedNodeMap, NamedNodeMap) \
    F(attr, Attr) \
    F(cssStyleDeclaration, CSSStyleDeclaration) \
    F(cssStyleRule, CSSStyleRule) \
    F(xhrElement, XMLHttpRequest) \
    F(blobElement, Blob) \
    F(domException, DOMException)



#define FOR_EACH_DECLARE_FN(codeName, exportName) \
    escargot::ESFunctionObject* binding##exportName(ScriptBindingInstance* scriptBindingInstance);

STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_DECLARE_FN);

class ScriptBindingInstance;

class ScriptBindingInstanceDataEscargot : public gc {
    friend void ScriptBindingInstance::initBinding(StarFish* sf);
public:
    ScriptBindingInstance* m_bindingInstance;
    escargot::ESVMInstance* m_instance;
    escargot::ESFunctionObject* m_orgToString;
    escargot::ESFunctionObject* m_eventTarget;
    escargot::ESFunctionObject* m_window;
#ifdef STARFISH_EXP
    escargot::ESFunctionObject* m_domImplementation;
#endif
#ifdef STARFISH_ENABLE_AUDIO
    escargot::ESFunctionObject* m_htmlAudioElement;
#endif

    ScriptBindingInstanceDataEscargot(ScriptBindingInstance* bindingInstance)
    {
        memset(this, 0, sizeof(ScriptBindingInstanceDataEscargot));
        m_bindingInstance = bindingInstance;
    }

#define FOR_EACH_GETTER_FN(codeName, exportName) \
    escargot::ESFunctionObject* codeName() \
    { \
        if (UNLIKELY(m_##codeName == nullptr)) { \
            m_##codeName = binding##exportName(m_bindingInstance); \
            m_value##codeName = m_##codeName; \
        } \
        return m_##codeName; \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_FN)

#define FOR_EACH_GETTER_VALUE_FN(codeName, exportName) \
    escargot::ESValue codeName##Value() \
    { \
        if (UNLIKELY(m_##codeName == nullptr)) { \
            m_##codeName = binding##exportName(m_bindingInstance); \
            m_value##codeName = m_##codeName; \
        } \
        return m_value##codeName; \
    }

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_GETTER_VALUE_FN)
private:
    escargot::ESFunctionObject* m_node;
    escargot::ESFunctionObject* m_element;
    escargot::ESFunctionObject* m_document;
    escargot::ESFunctionObject* m_documentType;
    escargot::ESFunctionObject* m_documentFragment;
    escargot::ESFunctionObject* m_htmlDocument;
    escargot::ESFunctionObject* m_characterData;
    escargot::ESFunctionObject* m_text;
    escargot::ESFunctionObject* m_comment;
    escargot::ESFunctionObject* m_htmlElement;
    escargot::ESFunctionObject* m_htmlHtmlElement;
    escargot::ESFunctionObject* m_htmlHeadElement;
    escargot::ESFunctionObject* m_htmlScriptElement;
    escargot::ESFunctionObject* m_htmlStyleElement;
    escargot::ESFunctionObject* m_htmlLinkElement;
    escargot::ESFunctionObject* m_htmlBodyElement;
    escargot::ESFunctionObject* m_htmlDivElement;
    escargot::ESFunctionObject* m_htmlImageElement;
    escargot::ESFunctionObject* m_htmlBrElement;
    escargot::ESFunctionObject* m_htmlMetaElement;
    escargot::ESFunctionObject* m_htmlParagraphElement;
    escargot::ESFunctionObject* m_htmlSpanElement;
    escargot::ESFunctionObject* m_htmlCollection;
    escargot::ESFunctionObject* m_htmlUnknownElement;
    escargot::ESFunctionObject* m_event;
    escargot::ESFunctionObject* m_uiEvent;
    escargot::ESFunctionObject* m_mouseEvent;
    escargot::ESFunctionObject* m_progressEvent;
    escargot::ESFunctionObject* m_nodeList;
    escargot::ESFunctionObject* m_domTokenList;
    escargot::ESFunctionObject* m_domSettableTokenList;
    escargot::ESFunctionObject* m_namedNodeMap;
    escargot::ESFunctionObject* m_attr;
    escargot::ESFunctionObject* m_cssStyleDeclaration;
    escargot::ESFunctionObject* m_cssStyleRule;
    escargot::ESFunctionObject* m_xhrElement;
    escargot::ESFunctionObject* m_blobElement;
    escargot::ESFunctionObject* m_domException;

public:
#define FOR_EACH_SCRIPTVALUE_FN(codeName, exportName) \
    escargot::ESValue m_value##codeName;

    STARFISH_ENUM_LAZY_BINDING_NAMES(FOR_EACH_SCRIPTVALUE_FN)
};

String* toBrowserString(const escargot::ESValue& v);

}

#endif
