/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLFormElement.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/resource_request/ResourceRequestJob.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/util/URL.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace StarFish {

class ActiveResourceRequestTracker : public ResourceRequestClient {
public:
    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction) override
    {
        if (request->progressState() == ResourceRequest::LOADSTART) {
            request->document()->m_activeResourceRequests.push_back(request);
        } else if (request->progressState() == ResourceRequest::LOADEND) {
            auto& v = request->document()->m_activeResourceRequests;
            auto iter = std::find(v.begin(), v.end(), request);
            if (iter != v.end()) {
                v.erase(iter);
            }
        }
    }
};

ResourceRequest::ResourceRequest(Document* document)
    : DocumentHoldable(document)
    , m_url(nullptr)
    , m_referrer(nullptr)
    , m_readyState(UNSENT)
    , m_progressState(NONE)
    , m_method(UNKNOWN_METHOD)
    , m_responseType(DEFAULT_RESPONSE)
    , m_status(0)
    , m_timeout(0)
    , m_activeNetworkURLWorkerData(nullptr)
    , m_mutex(new Mutex())
    , m_lastLocation(String::emptyString)
    , m_networkRequestJobDelegate(nullptr)
    , m_pendingOnHeaderReceivedEventIdlerHandle(SIZE_MAX)
    , m_pendingOnProgressEventIdlerHandle(SIZE_MAX)
    , m_loaded(0)
    , m_total(0)
    , m_pendingNetworkWorkerEndIdlerHandle(SIZE_MAX)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            // STARFISH_LOG_INFO("ResourceRequest::~ResourceRequest %p\n", obj);
            ResourceRequest* nr = (ResourceRequest*)obj;
            NetworkRequestResponse().swap(nr->m_response);
            ResponseHeaderMap().swap(nr->m_responseHeaderMap);
        },
        NULL, NULL, NULL);

    initVariables();
    addResourceRequestClient(new ActiveResourceRequestTracker());
}

void ResourceRequest::initVariables()
{
    m_responseMimeType = String::emptyString;
    m_contentLanguage = String::emptyString;
    NetworkRequestResponse().swap(m_response);
    ResponseHeaderMap().swap(m_responseHeaderMap);
    m_isSync = false;
    m_gotError = false;
    m_containsBase64Content = false;
    m_didSend = false;
    m_total = 0;
    m_loaded = 0;
    m_status = 0;
}

void ResourceRequest::clearIdlers()
{
    auto iter2 = m_requstedIdlers.begin();
    while (iter2 != m_requstedIdlers.end()) {
        starFish()->messageLoop()->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();

    if (m_pendingOnHeaderReceivedEventIdlerHandle != SIZE_MAX) {
        starFish()->messageLoop()->removeIdlerWithNoGCRooting(
            m_pendingOnHeaderReceivedEventIdlerHandle);
        m_pendingOnHeaderReceivedEventIdlerHandle = SIZE_MAX;
    }

    if (m_pendingOnProgressEventIdlerHandle != SIZE_MAX) {
        starFish()->messageLoop()->removeIdlerWithNoGCRooting(
            m_pendingOnProgressEventIdlerHandle);
        m_pendingOnProgressEventIdlerHandle = SIZE_MAX;
    }

    if (m_pendingNetworkWorkerEndIdlerHandle != SIZE_MAX) {
        starFish()->messageLoop()->removeIdlerWithNoGCRooting(
            m_pendingNetworkWorkerEndIdlerHandle);
        m_pendingNetworkWorkerEndIdlerHandle = SIZE_MAX;
    }

    if (m_activeNetworkURLWorkerData) {
        m_activeNetworkURLWorkerData->isAborted = true;
        m_activeNetworkURLWorkerData = nullptr;
    }
}

void ResourceRequest::handleResponseEOF()
{
    changeProgress(PROGRESS, true);
    changeReadyState(DONE, true);
    changeProgress(LOAD, true);
    changeProgress(LOADEND, true);
}

void ResourceRequest::handleError(ProgressState error)
{
    m_gotError = true;
    changeReadyState(DONE, true);
    changeProgress(PROGRESS, true);
    changeProgress(error, true);
    changeProgress(LOADEND, true);
}

void ResourceRequest::changeReadyState(ReadyState readyState,
                                       bool isExplicitAction)
{
    STARFISH_ASSERT(isMainThread());
    if (!m_gotError && readyState == LOADING && m_readyState == OPENED) {
        changeReadyState(HEADERS_RECEIVED, true);
    }

    if (!m_gotError && readyState == DONE && m_readyState == HEADERS_RECEIVED) {
        changeReadyState(LOADING, true);
    } else if (!m_gotError && readyState == DONE && m_readyState == OPENED) {
        changeReadyState(HEADERS_RECEIVED, true);
        changeReadyState(LOADING, true);
    }
    if (readyState == HEADERS_RECEIVED) {
        {
            auto it = m_responseHeaderMap.find("Content-Type");
            if (it != m_responseHeaderMap.end()) {
                size_t pos = it->second.find(";");
                if (pos != std::string::npos) {
                    m_responseMimeType = String::fromUTF8(it->second.data());
                } else {
                    m_responseMimeType =
                        String::fromUTF8(it->second.substr(0, pos).data());
                }
            }

            it = m_responseHeaderMap.find("Content-Language");
            if (it != m_responseHeaderMap.end()) {
                size_t pos = it->second.find(";");
                if (pos != std::string::npos) {
                    m_contentLanguage = String::fromUTF8(it->second.data());
                } else {
                    m_contentLanguage =
                        String::fromUTF8(it->second.substr(0, pos).data());
                }
            }
        }
        {
            auto it = m_responseHeaderMap.find("Content-Transfer-Encoding");
            if (it != m_responseHeaderMap.end()) {
                std::string part = it->second;
                std::transform(part.begin(), part.end(), part.begin(),
                               ::tolower);
                if (part.compare("base64") == 0) {
                    m_containsBase64Content = true;
                }
            }
        }

    } else if (readyState == DONE) {
        if (m_containsBase64Content) {
            m_response = parseBase64String(m_response, 0, m_response.size());
        }
    }

    if (readyState != m_readyState) {
        m_readyState = readyState;
        for (size_t i = 0; i < m_clients.size(); i++) {
            m_clients[i]->onReadyStateChange(this, isExplicitAction);
        }
    }

    if (m_readyState == ReadyState::DONE) {
        starFish()->messageLoop()->addIdler(
            document()->browsingContext(),
            [](size_t, void* data, void* data2) {
                ResourceRequest* self = (ResourceRequest*)data2;
                ((BrowsingContext*)data)->removePointerFromRootSet(data2);
            },
            document()->browsingContext(), this);
        if (m_networkRequestJobDelegate) {
            m_networkRequestJobDelegate = nullptr;
        }
    }
}

void ResourceRequest::changeProgress(ProgressState progress,
                                     bool isExplicitAction)
{
    STARFISH_ASSERT(isMainThread());
    if (m_progressState != progress || (progress == ProgressState::PROGRESS)) {
        m_progressState = progress;
        for (size_t i = 0; i < m_clients.size(); i++) {
            m_clients[i]->onProgressEvent(this, isExplicitAction);
        }
    }

    if (m_progressState == ProgressState::LOADEND) {
        NetworkRequestResponse().swap(m_response);
        ResponseHeaderMap().swap(m_responseHeaderMap);
    }
}

void ResourceRequest::open(MethodType method, String* url, bool async,
                           ResourceURL* referrer, String* userName,
                           String* password)
{
    bool shouldAbort = false;
    m_referrer = referrer;

    {
        STARFISH_ASSERT(!(!async && m_timeout != 0));
        shouldAbort = m_progressState >= LOADSTART;
    }
    if (shouldAbort) {
        abort(true);
    }
    {
        initVariables();
        m_method = method;
        m_url = new ResourceURL(url, document()->documentURI()->baseURI());
        if (userName->length()) {
            m_url->setUsername(userName);
        }
        if (password->length()) {
            m_url->setPassword(password);
        }
        m_isSync = !async;
    }

    STARFISH_ASSERT(!m_networkRequestJobDelegate);

    m_networkRequestJobDelegate =
        ResourceRequestJobDelegateFactory::createJob(this);
    changeReadyState(OPENED, true);
}

void ResourceRequest::abort(bool isExplicitAction)
{
    clearIdlers();

    if (m_readyState >= UNSENT) {
        m_gotError = true;
        auto theStatusWas = m_progressState;
        if (m_readyState == OPENED && m_didSend) {
            changeProgress(ABORT, false);
        } else {
            changeProgress(ABORT, isExplicitAction);
        }

        if (theStatusWas == LOADEND && isExplicitAction) {
            changeReadyState(DONE, false);
        } else {
            changeReadyState(DONE, m_didSend);
        }
        changeProgress(LOADEND, true);
        changeReadyState(UNSENT, false);
    }
}

void ResourceRequest::send(String* body)
{
    document()->browsingContext()->addPointerInRootSet(this);
    m_didSend = true;

    STARFISH_ASSERT(m_networkRequestJobDelegate);
    m_networkRequestJobDelegate->send(body);

    changeProgress(LOADSTART, true);
}

void ResourceRequest::setRequestHeader(String* h, String* c)
{
    m_requestHeaders.push_back(std::make_pair(h, c));
}

ResourceRequest::MethodType ResourceRequest::toMethodType(String* input)
{
    String* lowerMethod = input->toASCIILower();
    if (lowerMethod->equals("post")) {
        return POST_METHOD;
    } else if (lowerMethod->equals("get")) {
        return GET_METHOD;
    }
    return UNKNOWN_METHOD;
}

String* ResourceRequest::methodType(ResourceRequest::MethodType method)
{
    switch (method) {
    case POST_METHOD:
        return String::createASCIIString("post");
    case GET_METHOD:
        return String::createASCIIString("get");
    default:
        return String::emptyString;
    }
}

ResourceRequest::EncodeType ResourceRequest::toEncodeType(String* input)
{
    String* lowerMethod = input->toASCIILower();
    if (lowerMethod->equals("application/x-www-form-urlencoded")) {
        return APPLICATION_X_WWW_FORM_URLENCODED;
    } else if (lowerMethod->equals("multipart/form-data")) {
        return MULTIPART_FORM_DATA;
    } else if (lowerMethod->equals("text/plain")) {
        return TEXT_PLAIN;
    }
    return MISSING_OR_INVALID_ENCODETYPE;
}

String* ResourceRequest::encodeType(ResourceRequest::EncodeType input)
{
    switch (input) {
    case APPLICATION_X_WWW_FORM_URLENCODED:
        return String::createASCIIString("application/x-www-form-urlencoded");
    case MULTIPART_FORM_DATA:
        return String::createASCIIString("multipart/form-data");
    case TEXT_PLAIN:
        return String::createASCIIString("text/plain");
    default:
        return String::emptyString;
    }
}

// https://www.w3.org/TR/html5/forms.html#application/
// x-www-form-urlencoded-encoding-algorithm
String* ResourceRequest::encodeFormDataSet(
    GCVector<FormDataSetItem*>* formDataSet,
    ResourceRequest::EncodeType formEnctype)
{
    String* space = String::spaceString;
    String* plus = String::createASCIIString("+");

    String* result = String::createASCIIString("");
    if (formEnctype == APPLICATION_X_WWW_FORM_URLENCODED) {
        for (size_t i = 0; i < formDataSet->size(); i++) {
            FormDataSetItem* item = (*formDataSet)[i];
            String* name =
                ResourceURL::createPercentEncodingString(item->m_name)
                    ->replaceAll(space, plus);
            String* value =
                ResourceURL::createPercentEncodingString(item->m_value)
                    ->replaceAll(space, plus);
            String* type = item->m_type->replaceAll(space, plus);

            if (i == 0 && name->equalsIgnoreCase("isindex") &&
                type->equalsIgnoreCase("text")) {
                result = result->concat(value);
                continue;
            }

            if (name->equalsIgnoreCase("_charset_") &&
                type->equalsIgnoreCase("hidden")) {
                value = String::createASCIIString("UTF-8");
            }

            if (i > 0) {
                result = result->concat(String::createASCIIString("&"));
            }
            result = result->concat(name);
            result = result->concat(String::createASCIIString("="));
            result = result->concat(value);
        }
    } else if (formEnctype == MULTIPART_FORM_DATA) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    } else if (formEnctype == TEXT_PLAIN) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    return result;
}

String* ResourceRequest::mutateActionURL(DocumentURL* url,
                                         FormSubmitData* formSubmitData)
{
    String* encodedFormData = encodeFormDataSet(formSubmitData->m_formDataSet,
                                                formSubmitData->m_enctype);
    String* actionURL = url->urlString()->concat("?");
    actionURL = actionURL->concat(encodedFormData);
    return actionURL;
}

static size_t base64Table[128] = {
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos, // 0~9
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos, // 10~19
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos, // 20~29
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos, // 30~39
    std::string::npos,
    std::string::npos,
    std::string::npos,
    62,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    63,
    52,
    53, // 40~49
    54,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    std::string::npos,
    std::string::npos, // 50~59
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    0,
    1,
    2,
    3,
    4, // 60~69
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14, // 70~79
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24, // 80~89
    25,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    26,
    27,
    28, // 90~99
    29,
    30,
    31,
    32,
    33,
    34,
    35,
    36,
    37,
    38, // 100~109
    39,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    48, // 110~119
    49,
    50,
    51,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
    std::string::npos,
};

#ifndef NDEBUG
static const std::string base64CharsDebug =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
#endif

static inline bool isBase64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

template <typename StrType>
NetworkRequestResponse ResourceRequest::parseBase64String(const StrType& str,
                                                          size_t startAt,
                                                          size_t endAt)
{
    size_t inLen = endAt - startAt;
    size_t i = 0;
    size_t j = 0;
    size_t in_ = startAt;
    unsigned char charArray4[4] = {}, charArray3[3] = {};
    NetworkRequestResponse result;

    while (inLen--) {
        if (((unsigned char)str[in_] != '=') &&
            isBase64((unsigned char)str[in_])) {
            charArray4[i++] = str[in_];
            in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++) {
#ifndef NDEBUG
                    STARFISH_ASSERT(
                        (char)base64CharsDebug.find(charArray4[i]) ==
                        (char)base64Table[charArray4[i]]);
#endif
                    charArray4[i] = base64Table[charArray4[i]];
                }

                charArray3[0] =
                    (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
                charArray3[1] = ((charArray4[1] & 0xf) << 4) +
                                ((charArray4[2] & 0x3c) >> 2);
                charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

                for (i = 0; (i < 3); i++) {
                    result.push_back((char)charArray3[i]);
                }
                i = 0;
            }
        }
    }

    if (i) {
        for (j = i; j < 4; j++) {
            charArray4[j] = 0;
        }
        for (j = 0; j < 4; j++) {
#ifndef NDEBUG
            auto ret = base64CharsDebug.find(charArray4[j]);
            STARFISH_ASSERT((char)base64CharsDebug.find(charArray4[j]) ==
                            (char)base64Table[charArray4[j]]);
            ret = !ret;
#endif
            charArray4[j] = base64Table[charArray4[j]];
        }

        charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] =
            ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
        charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

        for (j = 0; (j < i - 1); j++) {
            result.push_back((char)charArray3[j]);
        }
    }

    return result;
}
}
