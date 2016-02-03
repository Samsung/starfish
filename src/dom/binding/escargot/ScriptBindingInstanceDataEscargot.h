#ifndef __StarFishScriptBindingInstanceDataEscargot__
#define __StarFishScriptBindingInstanceDataEscargot__


namespace StarFish {

class ScriptBindingInstanceDataEscargot : public gc {
public:
    escargot::ESVMInstance* m_instance;
    escargot::ESFunctionObject* m_window;
    escargot::ESFunctionObject* m_node;
    escargot::ESFunctionObject* m_element;
    escargot::ESFunctionObject* m_document;
    escargot::ESFunctionObject* m_documentType;
    escargot::ESFunctionObject* m_htmlDocument;
    escargot::ESFunctionObject* m_characterData;
    escargot::ESFunctionObject* m_text;
    escargot::ESFunctionObject* m_comment;
    escargot::ESFunctionObject* m_htmlElement;
    escargot::ESFunctionObject* m_htmlHtmlElement;
    escargot::ESFunctionObject* m_htmlHeadElement;
    escargot::ESFunctionObject* m_htmlScriptElement;
    escargot::ESFunctionObject* m_htmlStyleElement;
    escargot::ESFunctionObject* m_htmlBodyElement;
    escargot::ESFunctionObject* m_htmlDivElement;
    escargot::ESFunctionObject* m_htmlImageElement;
    escargot::ESFunctionObject* m_htmlBrElement;
    escargot::ESFunctionObject* m_htmlAudioElement;
    escargot::ESFunctionObject* m_htmlCollection;
    escargot::ESFunctionObject* m_htmlUnknownElement;
    escargot::ESFunctionObject* m_event;
    escargot::ESFunctionObject* m_uiEvent;
    escargot::ESFunctionObject* m_mouseEvent;
    escargot::ESFunctionObject* m_nodeList;
    escargot::ESFunctionObject* m_domTokenList;
    escargot::ESFunctionObject* m_domSettableTokenList;
    escargot::ESFunctionObject* m_namedNodeMap;
    escargot::ESFunctionObject* m_attr;
    escargot::ESFunctionObject* m_cssStyleDeclaration;
    escargot::ESFunctionObject* m_cssStyleRule;
    escargot::ESFunctionObject* m_xhrElement;
};

}

#endif
