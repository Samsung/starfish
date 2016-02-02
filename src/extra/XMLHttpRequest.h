#ifndef __StarFishXHRElement__
#define __StarFishXHRElement__

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"
#include "platform/canvas/Canvas.h"

namespace StarFish {

class XMLHttpRequestEventTarget : public EventTarget<ScriptWrappable>{

public:
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
        }else{
            return false;
        }
        return true;
    }

    escargot::ESValue getHandler(String* eventName){

        if(eventName->equals("loadstart")){
            return m_onloadstart;
        }else if(eventName->equals("progress")){
            return m_onprogress;
        }else if(eventName->equals("error")){
            return m_onerror;
        }else if(eventName->equals("abort")){
            return m_onabort;
        }else if(eventName->equals("timeout")){
            return m_ontimeout;
        }else if(eventName->equals("load")){
            return m_onload;
        }else if(eventName->equals("loadend")){
            return m_onloadend;
        }
        return escargot::ESValue::ESNull;
    }

private:
    escargot::ESValue  m_onloadstart;
    escargot::ESValue m_onprogress;
    escargot::ESValue m_onabort;
    escargot::ESValue m_onerror;
    escargot::ESValue m_onload;
    escargot::ESValue m_ontimeout;
    escargot::ESValue m_onloadend;
};


class XMLHttpRequest : public XMLHttpRequestEventTarget{
public:
    enum METHOD_TYPE{
        POST_METHOD,
        GET_METHOD,
        UNKNOWN_METHOD
    };

    struct Buffer {
        char *memory;
        size_t size;
    };

    XMLHttpRequest()
    {
        //FIXME: temp soluation
        set(escargot::ESString::create("responseText"),escargot::ESValue(escargot::ESValue::ESNull));

        //init
        initScriptWrappable(this);
        m_method = UNKNOWN_METHOD;
        m_url = nullptr;
    }

    void setOpen(const char* method,String* url)
    {
        if(method){
            std::string data = method;
            std::transform(data.begin(), data.end(), data.begin(), ::tolower);
            String* lowerName = String::fromUTF8(data.c_str());

            if(lowerName->equals("get")){
                m_method = GET_METHOD;
            }else if(lowerName->equals("post")){
                m_method = POST_METHOD;
            }
        }
        m_url = url;
    }

    void send(String* body);

    // String* getResponse()
    //     return m_response;
    // }
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


};


}

#endif
