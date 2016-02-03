#ifndef __StarFishXHRElement__
#define __StarFishXHRElement__

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"
#include "platform/canvas/Canvas.h"

namespace StarFish {

class XMLHttpRequestEventTarget : public EventTarget<ScriptWrappable>{

public:
    XMLHttpRequestEventTarget()
    : m_onloadstart(escargot::ESValue::ESNull),
      m_onprogress(escargot::ESValue::ESNull),
      m_onabort(escargot::ESValue::ESNull),
      m_onerror(escargot::ESValue::ESNull),
      m_onload(escargot::ESValue::ESNull),
      m_ontimeout(escargot::ESValue::ESNull),
      m_onloadend(escargot::ESValue::ESNull),
      m_onreadystatechange(escargot::ESValue::ESNull)
    {}
    bool addEventListener(String* eventName,escargot::ESValue handler){
        if(eventName->equals("loadstart")){
            m_onloadstart = handler;
        }else if(eventName->equals("progress")){
            m_onprogress = handler;
        }else if(eventName->equals("error")){
            m_onerror = handler;
        }else if(eventName->equals("abort")){
            m_onabort = handler;
        }else if(eventName->equals("timeout")){
            m_ontimeout = handler;
        }else if(eventName->equals("load")){
            m_onload = handler;
        }else if(eventName->equals("loadend")){
            m_onloadend = handler;
        }else if(eventName->equals("readystatechange")){
            m_onreadystatechange = handler;
        }else{
            return false;
        }
        return true;
    }

    escargot::ESValue getHandler(String* eventName){

        if(eventName->equals("loadstart")){
            if(m_onloadstart!=escargot::ESValue::ESNull)
                return m_onloadstart;
        }else if(eventName->equals("progress")){
            if(m_onprogress!=escargot::ESValue::ESNull)
                return m_onprogress;
        }else if(eventName->equals("error")){
            if(m_onerror!=escargot::ESValue::ESNull)
                return m_onerror;
        }else if(eventName->equals("abort")){
            if(m_onabort!=escargot::ESValue::ESNull)
                return m_onabort;
        }else if(eventName->equals("timeout")){
            if(m_ontimeout!=escargot::ESValue::ESNull)
                return m_ontimeout;
        }else if(eventName->equals("load")){
            if(m_onload!=escargot::ESValue::ESNull)
                return m_onload;
        }else if(eventName->equals("loadend")){
            if(m_onloadend!=escargot::ESValue::ESNull)
                return m_onloadend;
        }else if(eventName->equals("readystatechange")){
            if(m_onreadystatechange!=escargot::ESValue::ESNull)
                return m_onreadystatechange;
        }
        return escargot::ESValue::ESNull;
    }

private:
    escargot::ESValue m_onloadstart;
    escargot::ESValue m_onprogress;
    escargot::ESValue m_onabort;
    escargot::ESValue m_onerror;
    escargot::ESValue m_onload;
    escargot::ESValue m_ontimeout;
    escargot::ESValue m_onloadend;

    //FIXME: mh.byun
    escargot::ESValue m_onreadystatechange;

};


class XMLHttpRequest : public XMLHttpRequestEventTarget{
public:
    enum METHOD_TYPE{
        POST_METHOD,
        GET_METHOD,
        UNKNOWN_METHOD
    };

    enum RESPONSE_TYPE{
        TEXT_RESPONSE,
        ARRAY_BUFFER_RESPONSE,
        BLOB_RESPONSE,
        DOCUMENT_RESPONSE,
        JSON_RESPONSE,
        DEFAULT_RESPONSE
    };

    struct Buffer {
        char *memory;
        size_t size;
    };
    XMLHttpRequest();

    RESPONSE_TYPE getResponseType(){
        return m_response_type;
    }

    String* getResponseTypeStr();
    void setResponseType(const char* responseType);
    void setOpen(const char* method,String* url);
    void send(String* body);
    void callEventHandler(String* eventName,bool isMainThread);
    //void callReadystatechangeHandler(escargot::ESVMInstance* instance);

    static escargot::ESValue callJSFunction(escargot::ESVMInstance* instance, const escargot::ESValue& callee, const escargot::ESValue& receiver, escargot::ESValue arguments[], const size_t& argumentCount){
        escargot::ESValue result;
        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            result = escargot::ESFunctionObject::call(instance, callee, receiver, arguments, argumentCount, false);
            instance->unregisterTryPos(&tryPosition);
        } else {
            result = instance->getCatchedError();
            printf("Uncaught %s\n", result.toString()->utf8Data());
        }
        return result;
    }


    static void* CURL_realloc(void *ptr, size_t size)
    {
        /* There might be a realloc() out there that doesn't like reallocing
         *      NULL pointers, so we take care of it here */
        if(ptr)
            return realloc(ptr, size);
        else
            return malloc(size);
    }

    static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
    {
        size_t realsize = size * nmemb;
        struct Buffer *mem = (struct Buffer *)data;

        mem->memory = (char *)
            CURL_realloc(mem->memory, mem->size + realsize + 1);
        if (mem->memory) {
            memcpy(&(mem->memory[mem->size]), ptr, realsize);
            mem->size += realsize;
            mem->memory[mem->size] = 0;
        }
        return realsize;
    }

    virtual void paint(Canvas* canvas){}

protected:
    String* m_url;
    METHOD_TYPE m_method;
    RESPONSE_TYPE m_response_type;

};


}

#endif
