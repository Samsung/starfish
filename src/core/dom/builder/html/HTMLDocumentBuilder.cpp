/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/page/BrowsingContext.h"
#include "core/page/History.h"
#include "core/page/Window.h"
#include "browser/history/HistoryManager.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/parser/PreloadScanner.h"
#include "core/dom/HTMLFormElement.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/extra/MimeType.h"
#include "core/dom/WebOrigin.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/csp/SecurityPolicyViolationEvent.h"
#include "platform/loader/ResourceURL.h"
#include "platform/loader/ResourceLoader.h"
#include "core/page/WebView.h"

namespace Starfish {

#if defined(STARFISH_ENABLE_NETWORK_PROFILING) || \
    defined(STARFISH_ENABLE_SCRIPT_PROFILING)
uint64_t g_profilingBaseTime;
#endif

static int strcicmp(char const* a, size_t len1, char const* b, size_t len2)
{
    char const* aEnd = a + len1;
    char const* bEnd = b + len2;
    for (; a < aEnd && b < bEnd; a++, b++) {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a) {
            return d;
        }
    }
    return 0;
}

static const char* sstrstr(const char* haystack, size_t length,
                           const char* needle, size_t needleLength)
{
    for (size_t i = 0; i < length; i++) {
        if (i + needleLength > length) {
            return NULL;
        }
        if (strcicmp(&haystack[i], length - i, needle, needleLength) == 0) {
            return &haystack[i];
        }
    }
    return NULL;
}

struct EncodingResult {
    char m_encoding[10];
    size_t m_skip;

    EncodingResult()
        : m_skip(0)
    {
        memset(m_encoding, 0, sizeof(char) * 10);
    }
};

static EncodingResult detectAndRemoveBOM(GCAtomicVector<char>& buffer)
{
    EncodingResult er;
    size_t len = buffer.size();
    uint8_t c, c2, c3, c4;

    if (len > 1) {
        c = buffer[0] & 0xff;
        c2 = buffer[1] & 0xff;
        if (c == 0xff && c2 == 0xfe) {
            strncpy(er.m_encoding, "utf-16le", 8);
            er.m_skip = 2;
            return er;
        } else if (c == 0xfe && c2 == 0xff) {
            strncpy(er.m_encoding, "utf-16be", 8);
            er.m_skip = 2;
            return er;
        }
    }
    if (len > 2) {
        c3 = buffer[2] & 0xff;
        if (c == 0xef && c2 == 0xbb && c3 == 0xbf) {
            strncpy(er.m_encoding, "utf-8", 5);
            er.m_skip = 3;
            return er;
        }
    }
    if (len > 3) {
        c4 = buffer[3] & 0xff;
        if (c == 0x00 && c2 == 0x00 && c3 == 0xfe && c4 == 0xff) {
            strncpy(er.m_encoding, "utf-32be", 8);
            er.m_skip = 4;
            return er;
        } else if (c == 0xff && c2 == 0xfe && c3 == 0x00 && c4 == 0x00) {
            strncpy(er.m_encoding, "utf-32le", 8);
            er.m_skip = 4;
            return er;
        }
    }

    return er;
}

class HTMLResourceClient : public ResourceClient {
public:
    HTMLResourceClient(Resource* res, HTMLDocumentBuilder& builder)
        : ResourceClient(res)
        , m_builder(builder)
        , m_parser(nullptr)
        , m_htmlSource(String::emptyString)
        , m_isAllowedResponse(true)
    {
    }

    virtual void didHeaderReceived(
        const std::unordered_map<std::string, std::string>& headers)
    {
        auto browsingContext =
            m_resource->loader()->document()->browsingContext();

        if (!browsingContext->window()
                 ->performance()
                 ->timing()
                 ->m_responseStart) {
            browsingContext->window()
                ->performance()
                ->timing()
                ->m_responseStart = timestamp();
        }

        if (browsingContext->isTopLevelBrowsingContext()) {
            // TODO: In 'iframe' case also, check CSP
            auto csp = headers.find("Content-Security-Policy");
            if (csp != headers.end()) {
                String* value = String::createASCIIString(csp->second.data(),
                                                          csp->second.size());
                m_resource->loader()
                    ->document()
                    ->contentSecurityPolicy()
                    ->didReceiveHeader(value,
                                       ContentSecurityPolicyHeaderType::Enforce,
                                       ContentSecurityPolicyHeaderSource::HTTP);
            }
            return;
        }
        auto origin = browsingContext->document()->webOrigin();
        auto parentOrigin =
            browsingContext->parentBrowsingContext()->document()->webOrigin();

        auto it = headers.find(HTTPHeaderMap::kXFrameOptions);
        m_isAllowedResponse = true;
        if (it != headers.end()) {
            String* value =
                String::createASCIIString(it->second.data(), it->second.size());
            if (value->equalsIgnoreCase("deny")) {
                m_isAllowedResponse = false;
            } else if (value->equalsIgnoreCase("sameorigin")) {
                if (!origin->isSameOrigin(parentOrigin)) {
                    m_isAllowedResponse = false;
                }
            } else {
                GCVector<StringView> tokens;
                StringUtils::tokenize(value, " ", 1, tokens);
                if (tokens.size() > 1) {
                    if (tokens[0].string()->equalsIgnoreCase("allow-from")) {
                        m_isAllowedResponse = false;
                        for (int i = 1; i < static_cast<int>(tokens.size());
                             ++i) {
                            WebOrigin* allowedOrigin =
                                WebOrigin::createDocumentOrigin(
                                    new ResourceURL(tokens[i].string()));
                            if (allowedOrigin->isSameOrigin(parentOrigin)) {
                                m_isAllowedResponse = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (!m_isAllowedResponse) {
            browsingContext->sourceElement()->markContentDocumentDisabled();
            STARFISH_LOG_WARN(
                "Refused to display in iframe according to X-Frame-Options");
        }
#if defined(STARFISH_WEBWORKER_NOT_HOST)
        auto request = m_resource->resourceRequest();
        if (request->isRedirected()) {
            auto csp = browsingContext->document()->contentSecurityPolicy();
            auto resourceURL = new ResourceURL(request->lastLocation().data(),
                                               request->lastLocation().size());
            auto f = [](SecurityPolicyViolationEvent* event,
                        ExecutionContext* executionContext) {
                STARFISH_ASSERT(event != nullptr);
                STARFISH_ASSERT(executionContext != nullptr);

                auto parentBrowsingContext = executionContext->document()
                                                 ->browsingContext()
                                                 ->parentBrowsingContext();
                if (parentBrowsingContext) {
                    parentBrowsingContext->document()->dispatchEventByUA(event);
                } else {
                    executionContext->document()->dispatchEventByUA(event);
                }
            };

            if (csp->allowSource(CSPDirectives::ChildSrc, resourceURL, f) ==
                false) {
                m_isAllowedResponse = false;
                browsingContext->sourceElement()->markContentDocumentDisabled();
            }
        }
#endif
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        if (m_builder.m_document->window()->webView()->showLoadFailMsg()) {
            m_htmlSource = String::createASCIIString(
                "<div style='text-align:center;margin-top:50px;'>Cannot open "
                "page ");
            m_htmlSource = m_htmlSource->concat(m_resource->url()->urlString());
            m_htmlSource =
                m_htmlSource->concat(String::createASCIIString("</div>"));
        }
#ifdef STARFISH_ENABLE_TEST
        if (getenv("REF_TEST_STATE") && atoi(getenv("REF_TEST_STATE")) > 0) {
            m_htmlSource = m_htmlSource->concat(String::createASCIIString(
                "<sfrtfailed>Reference test load fail</sfrtfailed>"));
        }
#endif
        load();
    }

    virtual void didDataReceived(const char* buffer, size_t length)
    {
        m_buffer.insert(m_buffer.end(), &buffer[0], &buffer[length]);
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();

        m_builder.m_document->window()->performance()->timing()->m_responseEnd =
            timestamp();

        if (!m_isAllowedResponse) {
            load();
            return;
        }

        auto mimetype = MimeType::parseFromString(
            m_resource->resourceRequest()->responseMimeType());

        String* m = String::emptyString;
        if (mimetype.hasParameter() &&
            mimetype.parameter()->contains("charset", false)) {
            m = mimetype.parameter();
        }

        String* r = String::emptyString;
        auto& headers = m_resource->resourceRequest()->responseHeaderMap();
        auto it = headers.find(HTTPHeaderMap::kReferrerPolicy);

        if (it != headers.end()) {
            m_builder.document()->m_referrerPolicy =
                ReferrerURL::policyFromString(
                    String::fromUTF8(it->second.data(), it->second.size()));
        }

        if (!mimetype.stringWithoutParameter()->startsWith("image/", false)) {
            EncodingResult er = detectAndRemoveBOM(m_buffer);
            if (m == String::emptyString) {
                if (er.m_skip == 0) {
                    size_t bufferLen = m_buffer.size();
                    std::string charSetInMeta;
                    for (size_t i = 0;
                         i < bufferLen && charSetInMeta.length() == 0; i++) {
                        if (m_buffer[i] == '<') {
                            char tagName[12];
                            size_t tagNameLength = 0;
                            bool gotChar = false;
                            for (size_t j = i + 1; j < bufferLen; j++) {
                                if (!gotChar) {
                                    if (!String::isSpaceOrNewline(
                                            m_buffer[j])) {
                                        if (!::Starfish::isalpha(m_buffer[j])) {
                                            break;
                                        }
                                        gotChar = true;
                                        tagName[tagNameLength++] =
                                            tolower(m_buffer[j]);
                                    }
                                } else {
                                    if (!::Starfish::isalpha(m_buffer[j])) {
                                        tagName[tagNameLength] = 0;
                                        if (memcmp("meta", tagName, 4) == 0) {
                                            i = j;
                                            bool closeFinded = false;
                                            size_t attributeStart = j + 1;
                                            for (size_t k = j + 1;
                                                 k < bufferLen; k++) {
                                                if (m_buffer[k] == '>') {
                                                    i = k;
                                                    closeFinded = true;
                                                    break;
                                                }
                                            }

                                            std::string attr;
                                            for (size_t k = attributeStart;
                                                 k < i; k++) {
                                                char c = m_buffer[k];
                                                if (String::isSpaceOrNewline(
                                                        c)) {
                                                    continue;
                                                }
                                                if (c == '\'') {
                                                    continue;
                                                }
                                                if (c == '\"') {
                                                    continue;
                                                }
                                                if (c == '/') {
                                                    continue;
                                                }
                                                attr += c;
                                            }

                                            if (closeFinded) {
                                                const char* result = sstrstr(
                                                    attr.c_str(), attr.length(),
                                                    "charset=", 8);
                                                if (result) {
                                                    charSetInMeta = result;
                                                    break;
                                                }
                                            }
                                        } else {
                                            i = j;
                                            break;
                                        }

                                    } else {
                                        if (tagNameLength >= 4) {
                                            i = j;
                                            break;
                                        }
                                        tagName[tagNameLength++] =
                                            tolower(m_buffer[j]);
                                    }
                                }
                            }
                        }
                    }

                    if (charSetInMeta.length()) {
                        m = String::fromUTF8(charSetInMeta.data(),
                                             charSetInMeta.size());
                    }
                } else {
                    m = String::fromUTF8(
                        er.m_encoding,
                        strnlen(er.m_encoding, sizeof(er.m_encoding)));
                }
            }

            if (m == String::emptyString) {
                m = m_resource->resourceRequest()->responseMimeType();
            }

            if (m_buffer.size() != 0) {
                TextConverter* converter = new TextConverter(
                    m, String::createASCIIString("UTF-8"),
                    m_buffer.data() + er.m_skip, m_buffer.size() - er.m_skip);
                m_htmlSource =
                    converter->convert(m_buffer.data() + er.m_skip,
                                       m_buffer.size() - er.m_skip, true);
                m_builder.document()->setCharacterSet(converter->encoding());
            }
        } else if (m == String::emptyString) {
            m = m_resource->resourceRequest()->responseMimeType();
        }

        String* contentLanguage =
            m_resource->resourceRequest()->contentLanguage();
        if (!contentLanguage->isEmpty()) {
            m_builder.document()->setContentLanguage(contentLanguage);
        }

        if (!(m_resource->resourceRequest()->lastLocation() == "")) {
            // Change documentURI and last history when request was redirected.
            ResourceURL* newURL = nullptr;
            auto newURLString = String::createASCIIString(
                m_resource->resourceRequest()->lastLocation().data(),
                m_resource->resourceRequest()->lastLocation().size());

            if (ResourceURL::isValidURL(newURLString) == false) {
                auto baseURLString = String::createASCIIString(
                    m_resource->resourceRequest()->lastEffectiveURL().data(),
                    m_resource->resourceRequest()->lastEffectiveURL().size());
                newURL = new ResourceURL(newURLString, baseURLString);
            } else {
                newURL = new ResourceURL(newURLString);
            }

            m_builder.document()->setDocumentURI(newURL);
            m_builder.document()->setBaseURL(newURL);
            m_builder.document()->setWebOrigin(
                WebOrigin::createDocumentOrigin(newURL));
            m_builder.document()
                ->window()
                ->history()
                ->historyManager()
                ->replace(m_builder.document(), newURL);
        }
        if (m_resource->resourceRequest()->referrer()) {
            m_builder.document()->setReferrer(
                m_resource->resourceRequest()->referrer());
        }
        if (m && !m->isEmpty() && !m->contains("charset", false)) {
            m_builder.document()->setContentType(m);
        }
        m_builder.document()->resourceLoader().updateDocumentOpenTime();
        load();
    }

    void load()
    {
        Document* document = m_builder.document();
        auto mimetype = MimeType::parseFromString(
            m_resource->resourceRequest()->responseMimeType());
        if (!m_isAllowedResponse) {
            m_htmlSource = String::emptyString;
        } else if (m_htmlSource->isEmpty()) {
            if (mimetype.stringWithoutParameter()->startsWith("image/",
                                                              false)) {
                String* urlString = resource()->url()->urlString();
                StringBuilder sb;
                sb.appendString("<html><head></head><body><img src=\"");
                sb.appendString(urlString);
                sb.appendString("\" alt=\"");
                sb.appendString(urlString);
                sb.appendString("\"></img></body></html>");
                m_htmlSource = sb.finalize();

            } else if (resource() && resource()->url() &&
                       resource()->url()->urlString() &&
                       resource()->url()->urlString()->length() > 11 &&
                       resource()->url()->urlString()->startsWith(
                           "javascript:")) {
                String* urlString = resource()->url()->urlString();
                String* script =
                    urlString->substring(11, urlString->length() - 11);
                String* ret = toBrowserString(
                    document->window()->scriptBindingInstance(),
                    evaluateString(document->window()->scriptBindingInstance(),
                                   script));

                StringBuilder sb;
                sb.appendString("<html><head></head><body><text>");
                sb.appendString(ret);
                sb.appendString("</text></body></html>");
                m_htmlSource = sb.finalize();
            } else {
                m_htmlSource = createBlankHTMLSource();
            }
        } else if (mimetype.type()->equals("text") &&
                   mimetype.subtype()->equals("plain")) {
            StringBuilder sb;
            sb.appendString(
                "<html><head></head><body><pre style=\"word-wrap: break-word; "
                "white-space: pre-wrap;\">");
            sb.appendString(m_htmlSource);
            sb.appendString("</pre></body></html>");
            m_htmlSource = sb.finalize();
        }

        document->m_preloadScanner = new PreloadScanner(document, m_htmlSource);

        m_builder.m_parser = m_parser =
            new HTMLParser(document->starfish(), document, m_htmlSource);
        m_parser->startParse();
        m_parser->parseStep();
    }

protected:
    GCAtomicVector<char> m_buffer;
    HTMLDocumentBuilder& m_builder;
    HTMLParser* m_parser;
    String* m_htmlSource;
    bool m_isAllowedResponse;

private:
    String* createBlankHTMLSource()
    {
        return String::createASCIIString(
            "<html><head></head><body></body></html>");
    }
};

void HTMLDocumentBuilder::build(ResourceURL* url, ReferrerURL* referrerURL)
{
#if defined(STARFISH_ENABLE_NETWORK_PROFILING) || \
    defined(STARFISH_ENABLE_SCRIPT_PROFILING)
    if (m_document->browsingContext()->isTopLevelBrowsingContext()) {
        g_profilingBaseTime = timestamp();
    }
#endif

    m_resource = m_document->resourceLoader().fetch(url);
    m_resource->addResourceClient(new HTMLResourceClient(m_resource, *this));
    RequestData* reqData = new RequestData();
    reqData->m_url = url;
    reqData->m_referrer = referrerURL;
    reqData->m_mode = RequestMode::Navigate;
    reqData->m_destination = RequestDestination::Document;

    if (url->urlString()->isEmpty() ||
        url->urlString()->equals("about:blank")) {
        reqData->m_syncLevel = RequestSyncLevel::AlwaysSync;
    } else {
        reqData->m_syncLevel = RequestSyncLevel::NeverSync;
    }
    m_resource->request(reqData, true);
}

void HTMLDocumentBuilder::build(String* str)
{
    m_document->resourceLoader().markDocumentOpenState();
    HTMLParser parser(starfish(), m_document, str);
    parser.startParse();
    parser.parseStep();
}

void HTMLDocumentBuilder::openFunctionExplicitCalled()
{
    m_parser = new HTMLParser(starfish(), m_document, String::emptyString);
    m_parser->startParse();
}

void HTMLDocumentBuilder::resume()
{
    m_parser->parseStep();
}
} // namespace Starfish
