#include "StarFishConfig.h"
#include "XMLHttpRequest.h"
#include "dom/Event.h"
#include "extra/Blob.h"
#include "dom/DOMException.h"
#include <future>
#include <Elementary.h>
#include <fcntl.h>

namespace StarFish {

XMLHttpRequest::XMLHttpRequest()
{
    m_method = UNKNOWN_METHOD;
    m_response_type = DEFAULT_RESPONSE;
    m_ready_state = UNSENT;
    m_status = 0;
    m_url = nullptr;
    m_response_header = nullptr;
    m_timeout = 0;
    m_abort_flag = false;
    m_send_flag = false;
    m_bindingInstance = nullptr;

    // init
    initScriptWrappable(this);
}

void XMLHttpRequest::send(String* body)
{
    if (m_ready_state != OPENED || m_send_flag) {
        throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    }

    const char* url = m_url->utf8Data();

    // invoke loadstart Event.
    m_send_flag = true;
    callEventHandler(LOADSTART, true, 0, 0);

    GC_add_roots(this, this + sizeof(XMLHttpRequest*));
    auto task =
    [](XMLHttpRequest* xhrobj, const char* url, XMLHttpRequest::METHOD_TYPE methodType, const char* body, uint32_t timeout) -> bool {
        bool fromOnline = false;
        char tmp[4];
        for (int i = 0; i < 4; i++)
            tmp[i] = url[i];
        if (strcmp(tmp, "http") == 0)
            fromOnline = true;

        CURL* curl;
        if (fromOnline)
            curl = curl_easy_init();
        CURLcode res;

        HeaderBuffer header;
        header.memory = NULL;
        header.contentType = NULL;
        header.size = 0;

        Buffer buffer;
        buffer.memory = NULL;
        buffer.size = 0;

        ProgressData progressData;
        progressData.curl = curl;
        progressData.obj = xhrobj;
        progressData.lastruntime = 0;

        if (fromOnline && !curl) {
            GC_remove_roots(xhrobj, xhrobj + sizeof(XMLHttpRequest*));
            return false;
        }

        long res_code;
        if (fromOnline) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<unsigned long>(timeout));
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressData);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

            curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) &header);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &buffer);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);

            if (methodType == POST_METHOD) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
            } else if (methodType == GET_METHOD) {

            } else {
                printf("XMLHttpRequest UnSupportted method!!  \n");

                // invoke loadend event
                xhrobj->callEventHandler(LOADEND, false, progressData.loaded, progressData.total);
                GC_remove_roots(xhrobj, xhrobj + sizeof(XMLHttpRequest*));
                return false;
            }

            res = curl_easy_perform(curl);

            res_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        } else {
            const int maxLen = 1000;
            char path[maxLen];
            strcpy(path, xhrobj->m_starfish->currentPath()->utf8Data());
            int currentLen = xhrobj->m_starfish->currentPath()->length();
            if (path[currentLen - 1] != '/') {
                path[currentLen] = '/';
                currentLen++;
            }
            strcpy(path+currentLen, url);
            int ms = -1;
            char msValue[100];
            int index = 0;
            for (int i = 0; i < maxLen; i++) {
                if (path[i] == '?') {
                    path[i] = 0;
                    bool afterEqual = false;
                    for (int j = i+1; j < maxLen; j++) {
                        if (afterEqual) {
                            if (path[j] != 32) {
                                if (path[j] != 0) {
                                    msValue[index] = path[j];
                                } else {
                                    break;
                                }
                            }
                        } else if (path[j] == '=') {
                            afterEqual = true;
                        }
                        path[j] = 0;
                    }
                    ms = atoi(msValue);
                    break;
                }
            }

            int fd;
            if (0 < (fd = open(path, O_RDONLY))) {
                int BUFF_SIZE = 2048;
                char* buff = new char[BUFF_SIZE];
                read(fd, buff, BUFF_SIZE);
                close(fd);

                res_code = 200;
                xhrobj->setStatus(res_code);
                res = CURLE_OK;
                header.memory = buff;

                ecore_thread_main_loop_begin();
                ecore_idler_add([](void *data)->Eina_Bool {
                    XMLHttpRequest* this_obj = (XMLHttpRequest*)data;
                    this_obj->m_ready_state = HEADERS_RECEIVED;
                    this_obj->callEventHandler(NONE, true, 0, 0, this_obj->m_ready_state);
                    this_obj->m_ready_state = LOADING;
                    this_obj->callEventHandler(NONE, true, 0, 0, this_obj->m_ready_state);
                    return ECORE_CALLBACK_CANCEL;
                }, xhrobj);
                ecore_thread_main_loop_end();

                buffer.memory = buff;
                header.contentType = buff;

                int size = strlen(buff);
                if (buff[size - 1] == '\n')
                    buff[size - 1] = 0;

                buffer.size = size;
                header.contentLength = size;
                progressData.loaded = size;
                progressData.total = size;
            } else {
                res_code = 0;
                xhrobj->setStatus(res_code);
            }

        }

        if (((res_code == 200 || res_code == 201) && res == CURLE_OK )) {
            xhrobj->setResponseHeader(header.memory);

            if (fromOnline)
                curl_easy_cleanup(curl);

            if (buffer.size == 0) {
                GC_remove_roots(xhrobj, xhrobj + sizeof(XMLHttpRequest*));
                return false;
            }

            struct Pass {
                XMLHttpRequest* obj;
                char* buf;
                char* header;
                char* header_contentType;
                uint32_t loaded;
                uint32_t total;
                uint32_t contentSize;
            };

            Pass* pass = new Pass;
            pass->buf = buffer.memory;
            pass->header = header.memory;
            pass->header_contentType = header.contentType;
            pass->contentSize = header.contentLength;
            pass->obj = xhrobj;
            pass->loaded = progressData.loaded;
            pass->total = progressData.total;

            ecore_thread_main_loop_begin();
            ecore_idler_add([](void *data)->Eina_Bool {
                Pass* pass = (Pass*)data;

                XMLHttpRequest* this_obj = pass->obj;
                ScriptObject script_obj = this_obj->scriptObject();

                switch (this_obj->getResponseType()) {
                case JSON_RESPONSE:
                    {
                        ScriptValue ret = parseJSON(String::fromUTF8(pass->buf));
                        script_obj->set(createScriptString(String::fromUTF8("response")), ret);
                    }
                    break;

                case TEXT_RESPONSE:
                default:
                    script_obj->set(createScriptString(String::fromUTF8("response")), ScriptValue(createScriptString(String::fromUTF8(pass->buf))));
                    script_obj->set(createScriptString(String::fromUTF8("responseText")), ScriptValue(createScriptString(String::fromUTF8(pass->buf))));

                }
                this_obj->m_ready_state = DONE;
                this_obj->callEventHandler(NONE, true, 0, 0, this_obj->m_ready_state);
                this_obj->callEventHandler(PROGRESS, true, pass->loaded, pass->total);
                this_obj->callEventHandler(LOAD, true, pass->loaded, pass->total);
                this_obj->callEventHandler(LOADEND, true, pass->loaded, pass->total);

                delete pass;
                delete pass->buf;
                return ECORE_CALLBACK_CANCEL;
            }, pass);
            ecore_thread_main_loop_end();
            GC_remove_roots(xhrobj, xhrobj + sizeof(XMLHttpRequest*));
            return true;

        } else {
            switch (res) {
            case CURLE_OPERATION_TIMEDOUT:
                xhrobj->callEventHandler(TIMEOUT, false, 0, 0);
                break;

            case CURLE_ABORTED_BY_CALLBACK:
                xhrobj->callEventHandler(ABORT, false, 0, 0);
                break;

            default:
                break;
            }

            ecore_thread_main_loop_begin();
            ecore_idler_add([](void *data)->Eina_Bool {
                XMLHttpRequest* this_obj = (XMLHttpRequest*)data;
                this_obj->callEventHandler(ERROR, false, 0, 0);
                return ECORE_CALLBACK_CANCEL;
            }, xhrobj);
            ecore_thread_main_loop_end();
        }
        GC_remove_roots(xhrobj, xhrobj + sizeof(XMLHttpRequest*));
        return false;

    };

    std::thread(task, this, url, m_method, body->utf8Data(), m_timeout).detach();
}

void XMLHttpRequest::setResponseType(const char* responseType)
{
    if (m_ready_state == LOADING || m_ready_state == DONE)
            throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    if (responseType) {
        std::string data = responseType;
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
        String* lowerName = String::fromUTF8(data.c_str());

        if (lowerName->equals("text")) {
            m_response_type = TEXT_RESPONSE;
        } else if (lowerName->equals("arraybuffer")) {
            m_response_type = ARRAY_BUFFER_RESPONSE;
        } else if (lowerName->equals("blob")) {
            m_response_type = BLOB_RESPONSE;
        } else if (lowerName->equals("document")) {
            m_response_type = DOCUMENT_RESPONSE;
        } else if (lowerName->equals("json")) {
            m_response_type = JSON_RESPONSE;
        }
    }
}

void XMLHttpRequest::setOpen(const char* method, String* url)
{
    if (method) {
        std::string data = method;
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
        String* lowerName = String::fromUTF8(data.c_str());

        if (lowerName->equals("get")) {
            m_method = GET_METHOD;
        } else if (lowerName->equals("post")) {
            m_method = POST_METHOD;
        }
    }
    m_send_flag = false;
    m_ready_state = OPENED;
    callEventHandler(NONE, true, 0, 0, OPENED);
    m_url = url;
}

void XMLHttpRequest::abort()
{
    m_abort_flag = true;
    if (!(((m_ready_state == UNSENT || m_ready_state == OPENED) && !m_send_flag) || m_ready_state == DONE)) {
        m_ready_state = DONE;
        callEventHandler(NONE, true, 0, 0, m_ready_state);
        callEventHandler(PROGRESS, true, 0, 0);
        callEventHandler(ABORT, true, 0, 0);
        callEventHandler(LOADEND, true, 0, 0);
    }
    m_ready_state = UNSENT;
}

String* XMLHttpRequest::getResponseTypeStr()
{
    switch (m_response_type) {
    case TEXT_RESPONSE:
        return String::fromUTF8("text");
    case ARRAY_BUFFER_RESPONSE:
        return String::fromUTF8("arraybuffer");
    case BLOB_RESPONSE:
        return String::fromUTF8("blob");
    case DOCUMENT_RESPONSE:
        return String::fromUTF8("document");
    case JSON_RESPONSE:
        return String::fromUTF8("json");
    case DEFAULT_RESPONSE:
        return String::emptyString;
    }
    return String::emptyString;
}

String* XMLHttpRequest::getResponseText()
{
    if (!(m_response_type == DEFAULT_RESPONSE || m_response_type == TEXT_RESPONSE))
        throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    if (m_ready_state != LOADING && m_ready_state != DONE)
         return String::emptyString;
    return String::emptyString;
}

void XMLHttpRequest::setRequestHeader(const char* header, const char* value)
{
    if (m_ready_state != OPENED || m_send_flag) {
        throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    }
}

void XMLHttpRequest::callEventHandler(PROG_STATE progState, bool isMainThread, uint32_t loaded, uint32_t total, int readyState)
{
    String* eventName = nullptr;
    if (readyState >= 0) {
        ScriptValue rsc = getHandler(String::fromUTF8("readystatechange"), starfishInstance());
        if (rsc.isObject() && rsc.asESPointer()->isESFunctionObject()) {
            escargot::ESErrorObject* e = escargot::ESErrorObject::create();
            e->defineDataProperty(escargot::ESString::create("target"), false, false, false, scriptValue());
            ScriptValue json_arg[1] = { e };
            callScriptFunction(rsc, json_arg, 1, scriptValue());
        }
        return;
    } else if (progState == PROGRESS)
        eventName = String::fromUTF8("progress");
    else if (progState == ERROR)
        eventName = String::fromUTF8("error");
    else if (progState == ABORT)
        eventName = String::fromUTF8("abort");
    else if (progState == TIMEOUT)
        eventName = String::fromUTF8("timeout");
    else if (progState == LOAD)
        eventName = String::fromUTF8("load");
    else if (progState == LOADEND)
        eventName = String::fromUTF8("loadend");

    if (eventName) {
        ScriptValue en = getHandler(eventName, starfishInstance());
        if (en.isObject() && en.asESPointer()->isESFunctionObject()) {
            ProgressEvent* pe = new ProgressEvent(striptBindingInstance(), loaded, total);
            QualifiedName eventType = QualifiedName::fromString(starfishInstance(), eventName);
            pe->setType(eventType);
            ScriptValue json_arg[1] = { ScriptValue(pe->scriptObject()) };
            callScriptFunction(en, json_arg, 1, scriptValue());
        }
        return;
    } else {
        eventName = String::fromUTF8("loadstart");
    }

    if (isMainThread) {
        QualifiedName eventType = QualifiedName::fromString(starfishInstance(), eventName);
        auto clickListeners = getEventListeners(eventType);
        if (clickListeners) {
            for (unsigned i = 0; i < clickListeners->size(); i++) {
                STARFISH_ASSERT(clickListeners->at(i)->scriptValue() != ScriptValueNull);
                ProgressEvent* pe = new ProgressEvent(striptBindingInstance(), loaded, total);
                pe->setType(eventType);
                ScriptValue json_arg[1] = { ScriptValue(pe->scriptObject()) };
                ScriptValue fn = clickListeners->at(i)->scriptValue();
                if (!fn.isNull())
                    callScriptFunction(fn, json_arg, 1, scriptValue());
            }
        }
    } else {

        struct Pass {
            XMLHttpRequest* obj;
            String* buf;
            uint32_t loaded;
            uint32_t total;
        };

        Pass* pass = new Pass;
        pass->buf = eventName;
        pass->obj = this;
        pass->loaded = loaded;
        pass->total = total;

        ecore_thread_main_loop_begin();
        ecore_idler_add(
            [](void* data) -> Eina_Bool {
                Pass* pass = (Pass*)data;
                XMLHttpRequest* this_obj = pass->obj;

                QualifiedName eventType = QualifiedName::fromString(this_obj->starfishInstance(), pass->buf);
                auto clickListeners = this_obj->getEventListeners(eventType);
                if (clickListeners) {
                    for (unsigned i = 0; i < clickListeners->size(); i++) {
                        STARFISH_ASSERT(clickListeners->at(i)->scriptValue() != ScriptValueNull);
                        ProgressEvent* pe = new ProgressEvent(this_obj->striptBindingInstance(), pass->loaded, pass->total);
                        ScriptValue json_arg[1] = {ScriptValue(pe->scriptObject())};
                        ScriptValue fn = clickListeners->at(i)->scriptValue();
                        if (!fn.isNull())
                            callScriptFunction(fn, json_arg, 1, this_obj->scriptObject());
                    }
                }

                delete pass;
                return ECORE_CALLBACK_CANCEL;
            },
            pass);
        ecore_thread_main_loop_end();
    }
}
}
