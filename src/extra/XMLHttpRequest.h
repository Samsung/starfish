#ifndef __StarFishXHRElement__
#define __StarFishXHRElement__

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"
#include "platform/canvas/Canvas.h"
#include <curl/curl.h>

#define MINIMAL_PROGRESS_INTERVAL 0.1

namespace StarFish {

class XMLHttpRequestEventTarget : public EventTarget {

public:
    XMLHttpRequestEventTarget()
        : EventTarget()
    {
    }

    bool setHandler(String* eventType, const ScriptValue& f)
    {
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
        return false;
    }

    ScriptValue getHandler(String* keyName, StarFish* starfish)
    {
        EventListener* l = getAttributeEventListener(keyName);
        if (!l)
            return ScriptValue::ESNull;
        return l->scriptValue();
    }
};

class XMLHttpRequest : public XMLHttpRequestEventTarget {
public:
    enum METHOD_TYPE {
        POST_METHOD,
        GET_METHOD,
        UNKNOWN_METHOD
    };

    enum RESPONSE_TYPE {
        TEXT_RESPONSE,
        ARRAY_BUFFER_RESPONSE,
        BLOB_RESPONSE,
        DOCUMENT_RESPONSE,
        JSON_RESPONSE,
        DEFAULT_RESPONSE
    };

    enum READY_STATE {
        UNSENT,
        OPENED,
        HEADERS_RECEIVED,
        LOADING,
        DONE
    };

    enum PROG_STATE {
        LOADSTART,
        PROGRESS,
        LOAD,
        LOADEND,
        ERROR,
        ABORT,
        TIMEOUT,
        NONE
    };

    struct Buffer {
        char* memory;
        size_t size;
    };

    struct HeaderBuffer {
        char* memory;
        char* contentType;
        size_t contentLength;
        size_t size;
    };

    struct ProgressData {
        double lastruntime;
        CURL* curl;
        XMLHttpRequest* obj;
        int32_t loaded;
        int32_t total;
    };

    XMLHttpRequest();

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    RESPONSE_TYPE getResponseType()
    {
        return m_responseType;
    }

    String* getResponseTypeStr();
    int getReadyState()
    {
        return m_readyState;
    }

    void setStatus(int status)
    {
        m_status = status;
    }

    int getStatus()
    {
        return m_status;
    }

    String* getStatusText()
    {
        if (m_readyState == UNSENT || m_readyState == OPENED || m_status == 0)
            return String::emptyString;
        return String::emptyString;
    }

    String* getResponseText();

    String* getResponseXML()
    {
        if (m_readyState != DONE)
            return String::emptyString;
        return String::emptyString;
    }

    void setTimeout(uint32_t timeout);

    uint32_t getTimeout()
    {
        return m_timeout;
    }
    void setResponseType(const char* responseType);
    void setOpen(const char* method, String* url, bool async);
    void send(String* body);
    void callEventHandler(PROG_STATE progState, bool isMainThread, uint32_t loaded, uint32_t total, int readyState = -1);

    void setRequestHeader(const char* header, const char* value);
    void setResponseHeader(char* responseHeader)
    {
        m_responseHeader = responseHeader;
    }

    char* getResponseHeader(const char* header)
    {
        if (m_responseHeader != nullptr)
            return m_responseHeader;
        return nullptr;
    }

    char* getAllResponseHeadersStr()
    {
        if (m_readyState == UNSENT || m_readyState == OPENED)
            return nullptr;
        if (m_responseHeader != nullptr)
            return m_responseHeader;
        return nullptr;
    }

    void abort();

    bool checkAbort()
    {
        if (m_abortFlag) {
            m_abortFlag = false;
            return true;
        }
        return false;
    }

    void setStarfishInstance(StarFish* instance)
    {
        m_starfish = instance;
    }

    StarFish* starfishInstance()
    {
        return m_starfish;
    }

    void setScriptBindingInstance(ScriptBindingInstance* instance)
    {
        m_bindingInstance = instance;
    }

    ScriptBindingInstance* striptBindingInstance()
    {
        return m_bindingInstance;
    }

    // FIXME:mh.byun (FIX!!! memory leak)
    static void* CURL_realloc(void* ptr, size_t size)
    {
        if (ptr)
            return realloc(ptr, size);
        else
            return malloc(size);
    }

    static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data)
    {
        size_t realsize = size * nmemb;
        struct Buffer* mem = (struct Buffer*)data;

        mem->memory = (char*)
            CURL_realloc(mem->memory, mem->size + realsize + 1);
        if (mem->memory) {
            memcpy(&(mem->memory[mem->size]), ptr, realsize);
            mem->size += realsize;
            mem->memory[mem->size] = 0;
        }
        return realsize;
    }

    static size_t WriteHeaderCallback(void* ptr, size_t size, size_t nmemb, void* data)
    {
        size_t realsize = size * nmemb;
        struct HeaderBuffer* mem = (struct HeaderBuffer*)data;

        mem->memory = (char*)
            CURL_realloc(mem->memory, mem->size + realsize + 1);
        if (mem->memory) {
            memcpy(&(mem->memory[mem->size]), ptr, realsize);
            mem->size += realsize;
            mem->memory[mem->size] = 0;
        }

        if (strncmp((char*)(ptr), "Content-Type:", 13) == 0) {
            mem->contentType = (char*)CURL_realloc(mem->contentType, realsize + 1 - 13);
            sscanf((char*)(ptr), "Content-Type: %s", mem->contentType);
        } else if (strncmp((char*)(ptr), "Content-Length:", 15) == 0) {
            sscanf((char*)(ptr), "Content-Length: %zu", &(mem->contentLength));
        }
        return realsize;
    }
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
    {
        ProgressData* p_data = (ProgressData*)clientp;
        CURL* curl = p_data->curl;
        XMLHttpRequest* this_obj = p_data->obj;
        double curtime = 0;

        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

        if ((curtime - p_data->lastruntime) >= MINIMAL_PROGRESS_INTERVAL) {
            p_data->lastruntime = curtime;

            p_data->loaded = static_cast<uint32_t>(dlnow);
            p_data->total = static_cast<uint32_t>(dltotal);
            this_obj->m_readyState = LOADING;
        }

        // check abort
        if (this_obj->checkAbort())
            return 1;

        return 0;
    }

    // static int progress_callback_old(void *p,
    //                           double dltotal, double dlnow,
    //                           double ultotal, double ulnow)
    // {
    //     printf("test??\n");
    //   return progress_callback(p,
    //                   (curl_off_t)dltotal,
    //                   (curl_off_t)dlnow,
    //                   (curl_off_t)ultotal,
    //                   (curl_off_t)ulnow);
    // }

protected:
    String* m_url;
    char* m_responseHeader;
    METHOD_TYPE m_method;
    RESPONSE_TYPE m_responseType;
    READY_STATE m_readyState;
    int m_status;
    uint32_t m_timeout;
    bool m_abortFlag;
    bool m_sendFlag;
    ScriptBindingInstance* m_bindingInstance;
    StarFish* m_starfish;
    bool m_sync;
};
}

#endif
