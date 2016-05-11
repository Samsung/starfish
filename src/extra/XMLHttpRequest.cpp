#include "StarFishConfig.h"
#include "XMLHttpRequest.h"
#include "dom/Event.h"
#include "extra/Blob.h"
#include "dom/DOMException.h"
#include <future>
#include <Elementary.h>
#include <fcntl.h>
#include "platform/file_io/FileIO.h"
#include "platform/message_loop/MessageLoop.h"

namespace StarFish {

XMLHttpRequest::XMLHttpRequest()
{
    m_method = UNKNOWN_METHOD;
    m_responseType = DEFAULT_RESPONSE;
    m_readyState = UNSENT;
    m_status = 0;
    m_url = nullptr;
    m_responseText = nullptr;
    m_response = ScriptValueNull;
    m_responseHeader = nullptr;
    m_timeout = 0;
    m_abortFlag = false;
    m_sendFlag = false;
    m_starfish = nullptr;
    m_bindingInstance = nullptr;
    m_sync = false;
}

void XMLHttpRequest::send(String* body)
{
    if (m_readyState != OPENED || m_sendFlag) {
        throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    }

    std::string url = m_url->utf8Data();
    std::string bodyString = body->utf8Data();

    // invoke loadstart Event.
    m_sendFlag = true;
    callEventHandler(LOADSTART, true, 0, 0);

    m_starfish->addPointerInRootSet(this);
    auto task =
    [](XMLHttpRequest* xhrobj, std::string url, XMLHttpRequest::METHOD_TYPE methodType, std::string body, uint32_t timeout) -> bool {
        bool fromOnline = false;
        if (strncmp(url.data(), "http", 4) == 0)
            fromOnline = true;

        CURL* curl = nullptr;
        if (fromOnline)
            curl = curl_easy_init();
        CURLcode res;

        HeaderBuffer header;
        header.memory = NULL;
        header.contentType = NULL;
        header.contentLength = 0;
        header.size = 0;

        Buffer buffer;
        buffer.memory = NULL;
        buffer.size = 0;

        ProgressData progressData;
        progressData.curl = curl;
        progressData.obj = xhrobj;
        progressData.lastruntime = 0;

        if (fromOnline && !curl) {
            // invoke loadend event
            // FIXME here is another thread
            // we should move this code area into main thread
            xhrobj->m_starfish->removePointerFromRootSet(xhrobj);
            // ------------
            return false;
        }

        long res_code;
        if (fromOnline) {
            curl_easy_setopt(curl, CURLOPT_URL, url.data());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<unsigned long>(timeout));
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressData);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

            curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) &header);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &buffer);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);

            if (methodType == POST_METHOD) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
            } else if (methodType == GET_METHOD) {

            } else {
                STARFISH_LOG_ERROR("XMLHttpRequest UnSupportted method!!  \n");

                // invoke loadend event
                // FIXME here is another thread
                // we should move this code area into main thread
                xhrobj->callEventHandler(LOADEND, false, progressData.loaded, progressData.total);
                xhrobj->m_starfish->addPointerInRootSet(xhrobj);
                // ----------------------
                return false;
            }

            res = curl_easy_perform(curl);

            res_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
            ecore_thread_main_loop_begin();
            xhrobj->setStatus(res_code);
        } else {
            ecore_thread_main_loop_begin();
            STARFISH_ASSERT(!curl);
            std::string path = xhrobj->m_starfish->currentPath()->utf8Data();
            if ((path.length() != 0)&&(path.back() != '/')) {
                path += '/';
            }
            path += url;
            int ms = -1;
            std::string msValue;
            for (size_t i = 0; i < path.length(); i++) {
                if (path[i] == '?') {
                    path[i] = 0;
                    bool afterEqual = false;
                    for (size_t j = i + 1; j < path.length(); j++) {
                        if (afterEqual) {
                            if (path[j] != 32) {
                                if (path[j] != 0) {
                                    msValue.push_back(path[j]);
                                } else {
                                    break;
                                }
                            }
                        } else if (path[j] == '=') {
                            afterEqual = true;
                        }
                        path[j] = 0;
                    }
                    ms = atoi(msValue.data());
                    break;
                }
            }

            FileIO* fio = FileIO::createInNonGCArea();
            if (fio->open(path.data())) {
                long int BUFF_SIZE = fio->length();
                char* buff = new char[BUFF_SIZE + 1];
                fio->read(buff, sizeof(char), BUFF_SIZE);
                buff[BUFF_SIZE] = 0;
                fio->close();

                res_code = 200;
                res = CURLE_OK;
                header.memory = buff; // FIXME
                header.contentType = buff; // FIXME

                buffer.memory = buff;
                buffer.size = BUFF_SIZE;
                header.contentLength = BUFF_SIZE;
                progressData.loaded = BUFF_SIZE;
                progressData.total = BUFF_SIZE;

                xhrobj->setStatus(res_code);
                xhrobj->m_starfish->addPointerInRootSet(xhrobj);
                xhrobj->m_starfish->messageLoop()->addIdler([](void *data) {
                    XMLHttpRequest* this_obj = (XMLHttpRequest*)data;
                    this_obj->m_readyState = HEADERS_RECEIVED;
                    this_obj->callEventHandler(NONE, true, 0, 0, this_obj->m_readyState);
                    this_obj->m_readyState = LOADING;
                    this_obj->callEventHandler(NONE, true, 0, 0, this_obj->m_readyState);
                    this_obj->m_starfish->removePointerFromRootSet(this_obj);
                }, xhrobj);
            } else {
                res_code = 0;
                xhrobj->setStatus(res_code);
            }

            fio->close();
            free(fio);
            // FIXME
            // delete fio;
        }

        if (((res_code == 200 || res_code == 201) && res == CURLE_OK )) {
            xhrobj->setResponseHeader(header.memory);

            if (fromOnline)
                curl_easy_cleanup(curl);

            if (buffer.size == 0) {
                xhrobj->m_starfish->removePointerFromRootSet(xhrobj);
                ecore_thread_main_loop_end();
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

            xhrobj->m_starfish->addPointerInRootSet(xhrobj);
            xhrobj->m_starfish->messageLoop()->addIdler([](void *data) {
                Pass* pass = (Pass*)data;
                XMLHttpRequest* this_obj = pass->obj;

                switch (this_obj->getResponseType()) {
                case JSON_RESPONSE:
                    {
                        this_obj->m_response = parseJSON(String::fromUTF8(pass->buf, pass->total));
                    }
                    break;

                case TEXT_RESPONSE:
                default:
                    this_obj->m_response = ScriptValue(createScriptString(String::fromUTF8(pass->buf)));
                    this_obj->m_responseText = String::fromUTF8(pass->buf, pass->total);
                }
                if (pass->contentSize == 0)
                    pass->total = 0;

                this_obj->m_readyState = DONE;
                this_obj->callEventHandler(NONE, true, 0, 0, this_obj->m_readyState);
                this_obj->callEventHandler(PROGRESS, true, pass->loaded, pass->total);
                this_obj->callEventHandler(LOAD, true, pass->loaded, pass->total);
                this_obj->callEventHandler(LOADEND, true, pass->loaded, pass->total);

                delete []pass->buf;
                delete pass;
                this_obj->m_starfish->removePointerFromRootSet(this_obj);
            }, pass);
            xhrobj->m_starfish->removePointerFromRootSet(xhrobj);
            ecore_thread_main_loop_end();
            return true;

        } else {
            // FIXME (SHOULD check this part with jaeman.park - mh.byun)
            /*
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
            */

            xhrobj->m_starfish->addPointerInRootSet(xhrobj);
            xhrobj->m_starfish->messageLoop()->addIdler([](void *data) {
                XMLHttpRequest* this_obj = (XMLHttpRequest*)data;
                this_obj->callEventHandler(ERROR, true, 0, 0);
                this_obj->m_starfish->removePointerFromRootSet(this_obj);
            }, xhrobj);
            ecore_thread_main_loop_end();
            return false;
        }
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    };

    std::thread(task, this, std::move(url), m_method, std::move(bodyString), m_timeout).detach();
}

void XMLHttpRequest::setResponseType(const char* responseType)
{
    if (m_readyState == LOADING || m_readyState == DONE)
        throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    if (responseType) {
        std::string data = responseType;
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
        String* lowerName = String::fromUTF8(data.c_str());

        if (lowerName->equals("text")) {
            m_responseType = TEXT_RESPONSE;
        } else if (lowerName->equals("arraybuffer")) {
            m_responseType = ARRAY_BUFFER_RESPONSE;
        } else if (lowerName->equals("blob")) {
            m_responseType = BLOB_RESPONSE;
        } else if (lowerName->equals("document")) {
            m_responseType = DOCUMENT_RESPONSE;
        } else if (lowerName->equals("json")) {
            m_responseType = JSON_RESPONSE;
        }
    }
}

void XMLHttpRequest::setOpen(const char* method, String* url, bool async)
{
    if (!async && m_timeout != 0)
        throw new DOMException(m_bindingInstance, DOMException::INVALID_ACCESS_ERR, "InvalidAccessError");
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
    m_sync = !async;
    m_sendFlag = false;
    m_readyState = OPENED;
    callEventHandler(NONE, true, 0, 0, OPENED);
    m_url = url;
}

void XMLHttpRequest::abort()
{
    m_abortFlag = true;
    if (!(((m_readyState == UNSENT || m_readyState == OPENED) && !m_sendFlag) || m_readyState == DONE)) {
        m_readyState = DONE;
        callEventHandler(NONE, true, 0, 0, m_readyState);
        callEventHandler(PROGRESS, true, 0, 0);
        callEventHandler(ABORT, true, 0, 0);
        callEventHandler(LOADEND, true, 0, 0);
    }
    m_readyState = UNSENT;
}

String* XMLHttpRequest::getResponseTypeStr()
{
    switch (m_responseType) {
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
    if (!(m_responseType == DEFAULT_RESPONSE || m_responseType == TEXT_RESPONSE))
        throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    if (m_readyState != LOADING && m_readyState != DONE)
        return String::emptyString;

    if (m_responseText != nullptr)
        return m_responseText;
    return String::emptyString;
}

ScriptValue XMLHttpRequest::getResponse()
{
    if (m_readyState != LOADING && m_readyState != DONE)
        return ScriptValueNull;

    return m_response;
}

void XMLHttpRequest::setRequestHeader(const char* header, const char* value)
{
    if (m_readyState != OPENED || m_sendFlag) {
        throw new DOMException(m_bindingInstance, DOMException::INVALID_STATE_ERR, "InvalidStateError");
    }
}

void XMLHttpRequest::setTimeout(uint32_t timeout)
{
    if (m_sync == true)
        throw new DOMException(m_bindingInstance, DOMException::INVALID_ACCESS_ERR, "InvalidAccessError");
    m_timeout = timeout;
}

void XMLHttpRequest::callEventHandler(PROG_STATE progState, bool isMainThread, uint32_t loaded, uint32_t total, int readyState)
{
    String* eventName = nullptr;
    bool lengthComputable = false;
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
    else if (progState == LOADSTART)
        eventName = String::fromUTF8("loadstart");

    if (total>0)
        lengthComputable = true;

    if (eventName&&isMainThread) {
        /*
        {
            ScriptValue en = getHandler(eventName, starfishInstance());
            if (en.isObject() && en.asESPointer()->isESFunctionObject()) {
                ProgressEvent* pe = new ProgressEvent(eventName, ProgressEventInit(false, false, lengthComputable, loaded, total));
                ScriptValue json_arg[1] = { ScriptValue(pe->scriptObject()) };
                callScriptFunction(en, json_arg, 1, scriptValue());
            }
        }
        */
        auto clickListeners = getEventListeners(eventName);
        if (clickListeners) {
            for (unsigned i = 0; i < clickListeners->size(); i++) {
                STARFISH_ASSERT(clickListeners->at(i)->scriptValue() != ScriptValueNull);
                ProgressEvent* pe = new ProgressEvent(eventName, ProgressEventInit(false, false, lengthComputable, loaded, total));
                ScriptValue json_arg[1] = { ScriptValue(pe->scriptObject()) };
                ScriptValue fn = clickListeners->at(i)->scriptValue();
                if (!fn.isNull())
                    callScriptFunction(fn, json_arg, 1, scriptValue());
            }
        }
    }
}
}
