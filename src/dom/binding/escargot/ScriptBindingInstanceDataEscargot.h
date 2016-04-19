#ifndef __StarFishScriptBindingInstanceDataEscargot__
#define __StarFishScriptBindingInstanceDataEscargot__


namespace StarFish {

escargot::ESFunctionObject* bindingHTMLBodyElement(ScriptBindingInstance* scriptBindingInstance);
class ScriptBindingInstance;

class ScriptBindingInstanceDataEscargot : public gc {
public:
    ScriptBindingInstance* m_bindingInstance;
    escargot::ESVMInstance* m_instance;
    escargot::ESFunctionObject* m_eventTarget;
    escargot::ESFunctionObject* m_window;

#ifdef TIZEN_DEVICE_API
    escargot::ESObject* m_deviceAPIObject;
#endif

    ScriptBindingInstanceDataEscargot(ScriptBindingInstance* bindingInstance)
    {
        m_bindingInstance = bindingInstance;
        m_orgToString = nullptr;
        m_node = nullptr;
        m_element = nullptr;
        m_document = nullptr;
        m_documentType = nullptr;
        m_htmlDocument = nullptr;
        m_characterData = nullptr;
        m_text = nullptr;
        m_comment = nullptr;
#ifdef STARFISH_EXP
        m_domImplementation = nullptr;
#endif
        m_htmlElement = nullptr;
        m_htmlHtmlElement = nullptr;
        m_htmlHeadElement = nullptr;
        m_htmlScriptElement = nullptr;
        m_htmlStyleElement = nullptr;
        m_htmlLinkElement = nullptr;
        m_htmlBodyElement = nullptr;
        m_htmlDivElement = nullptr;
        m_htmlImageElement = nullptr;
        m_htmlBrElement = nullptr;
        m_htmlMetaElement = nullptr;
        m_htmlParagraphElement = nullptr;
        m_htmlSpanElement = nullptr;
#ifdef STARFISH_ENABLE_AUDIO
        m_htmlAudioElement = nullptr;
#endif
        m_htmlCollection = nullptr;
        m_htmlUnknownElement = nullptr;
        m_event = nullptr;
        m_uiEvent = nullptr;
        m_mouseEvent = nullptr;
        m_progressEvent = nullptr;
        m_nodeList = nullptr;
        m_domTokenList = nullptr;
        m_domSettableTokenList = nullptr;
        m_namedNodeMap = nullptr;
        m_attr = nullptr;
        m_cssStyleDeclaration = nullptr;
        m_cssStyleRule = nullptr;
        m_xhrElement = nullptr;
        m_blobElement = nullptr;
        m_domException = nullptr;
    }

    escargot::ESFunctionObject* htmlBodyElement()
    {
        if (UNLIKELY(m_htmlBodyElement == nullptr)) {
            m_htmlBodyElement = bindingHTMLBodyElement(m_bindingInstance);
        }
        return m_htmlBodyElement;
    }
private:
    escargot::ESFunctionObject* m_htmlBodyElement;
public:
// private:
    escargot::ESFunctionObject* m_orgToString;
    escargot::ESFunctionObject* m_node;
    escargot::ESFunctionObject* m_element;
    escargot::ESFunctionObject* m_document;
    escargot::ESFunctionObject* m_documentType;
    escargot::ESFunctionObject* m_htmlDocument;
    escargot::ESFunctionObject* m_characterData;
    escargot::ESFunctionObject* m_text;
    escargot::ESFunctionObject* m_comment;
#ifdef STARFISH_EXP
    escargot::ESFunctionObject* m_domImplementation;
#endif
    escargot::ESFunctionObject* m_htmlElement;
    escargot::ESFunctionObject* m_htmlHtmlElement;
    escargot::ESFunctionObject* m_htmlHeadElement;
    escargot::ESFunctionObject* m_htmlScriptElement;
    escargot::ESFunctionObject* m_htmlStyleElement;
    escargot::ESFunctionObject* m_htmlLinkElement;
    // escargot::ESFunctionObject* m_htmlBodyElement;
    escargot::ESFunctionObject* m_htmlDivElement;
    escargot::ESFunctionObject* m_htmlImageElement;
    escargot::ESFunctionObject* m_htmlBrElement;
    escargot::ESFunctionObject* m_htmlMetaElement;
    escargot::ESFunctionObject* m_htmlParagraphElement;
    escargot::ESFunctionObject* m_htmlSpanElement;
#ifdef STARFISH_ENABLE_AUDIO
    escargot::ESFunctionObject* m_htmlAudioElement;
#endif
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
#ifdef TIZEN_DEVICE_API
    escargot::ESObject* m_deviceAPIObject;
#endif
};

String* toBrowserString(const escargot::ESValue& v);

}

#endif
