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

    bool setHandler(QualifiedName& eventType, const ScriptValue& f)
    {
        EventListener* l = new EventListener(f, true);
        setAttributeEventListener(eventType, l);
        return false;
    }

    ScriptValue getHandler(String* keyName, StarFish* starfish)
    {
        auto eventType = QualifiedName::fromString(starfish, keyName->utf8Data());
        EventListener* l = getAttributeEventListener(eventType);
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

    RESPONSE_TYPE getResponseType()
    {
        return m_response_type;
    }

    String* getResponseTypeStr();
    int getReadyState()
    {
        return m_ready_state;
    }

    void setTimeout(uint32_t timeout)
    {
        m_timeout = timeout;
    }
    uint32_t getTimeout()
    {
        return m_timeout;
    }
    void setResponseType(const char* responseType);
    void setOpen(const char* method, String* url);
    void send(String* body);
    void callEventHandler(String* eventName, bool isMainThread, uint32_t loaded, uint32_t total);

    void setResponseHeader(String* responseHeader)
    {
        m_response_header = responseHeader;
    }

    String* getAllResponseHeadersStr()
    {
        if (m_response_header != nullptr)
            return m_response_header;
        return String::emptyString;
    }

    void abort()
    {
        m_abort_flag = true;
    }

    bool checkAbort()
    {
        if (m_abort_flag) {
            m_abort_flag = false;
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
            this_obj->callEventHandler(String::fromUTF8("progress"), false, p_data->loaded, p_data->total);

            // printf("TOTAL TIME: %f \r\n", curtime);
            // printf("UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
            //         "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
            //         "\r\n",
            //         ulnow, ultotal, dlnow, dltotal);
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

    virtual void paint(Canvas* canvas)
    {
    }

protected:
    String* m_url;
    String* m_response_header;
    METHOD_TYPE m_method;
    RESPONSE_TYPE m_response_type;
    READY_STATE m_ready_state;
    uint32_t m_timeout;
    bool m_abort_flag;
    ScriptBindingInstance* m_bindingInstance;
    StarFish* m_starfish;
};
}

#endif
