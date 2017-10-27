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
#include "binding/ScriptBindingInstance.h"
#include "core/dom/Document.h"
#include "core/page/History.h"
#include "browser/history/HistoryManager.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/dom/HTMLFormElement.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/loader/ResourceURL.h"
#include "core/page/Window.h"

namespace StarFish {

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
    }
};

static EncodingResult detectAndRemoveBOM(GCVector<char>& buffer)
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
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_htmlSource = String::createASCIIString(
            "<div style='text-align:center;margin-top:50px;'>Cannot open "
            "page ");
        m_htmlSource = m_htmlSource->concat(m_resource->url()->urlString());
        m_htmlSource =
            m_htmlSource->concat(String::createASCIIString("</div>"));
        load();
    }

    virtual void didDataReceived(const char* buffer, size_t length)
    {
        m_buffer.insert(m_buffer.end(), &buffer[0], &buffer[length]);
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();

        String* m = m_resource->resourceRequest()->responseMimeType();
        EncodingResult er = detectAndRemoveBOM(m_buffer);

        if (!m->contains("charset", false) && er.m_skip == 0) {
            size_t bufferLen = m_buffer.size();
            std::string charSetInMeta;
            for (size_t i = 0; i < bufferLen; i++) {
                if (m_buffer[i] == '<') {
                    char tagName[12];
                    size_t tagNameLength = 0;
                    bool gotChar = false;
                    for (size_t j = i + 1; j < bufferLen; j++) {
                        if (!gotChar) {
                            if (!String::isSpaceOrNewline(m_buffer[j])) {
                                if (!std::isalpha(m_buffer[j])) {
                                    break;
                                }
                                gotChar = true;
                                tagName[tagNameLength++] = tolower(m_buffer[j]);
                            }
                        } else {
                            if (!std::isalpha(m_buffer[j])) {
                                tagName[tagNameLength] = 0;
                                if (memcmp("meta", tagName, 4) == 0) {
                                    i = j;
                                    bool closeFinded = false;
                                    size_t attributeStart = j + 1;
                                    for (size_t k = j + 1; k < bufferLen; k++) {
                                        if (m_buffer[k] == '>') {
                                            i = k;
                                            closeFinded = true;
                                            break;
                                        }
                                    }

                                    std::string attr;
                                    for (size_t k = attributeStart; k < i;
                                         k++) {
                                        char c = m_buffer[k];
                                        if (String::isSpaceOrNewline(c))
                                            continue;
                                        if (c == '\'') {
                                            continue;
                                        }
                                        if (c == '\"') {
                                            continue;
                                        }
                                        if (c == '/') {
                                            continue;
                                        }
                                        if (c == '/') {
                                            continue;
                                        }
                                        attr += c;
                                    }

                                    if (closeFinded) {
                                        const char* result =
                                            sstrstr(attr.c_str(), attr.length(),
                                                    "charset=", 8);
                                        if (result) {
                                            charSetInMeta = result;
                                        }
                                    }
                                } else {
                                    i = j;
                                }
                                if (charSetInMeta.length()) {
                                    break;
                                }
                            } else {
                                if (tagNameLength >= 4) {
                                    i = j;
                                    break;
                                }
                                tagName[tagNameLength++] = tolower(m_buffer[j]);
                            }
                        }
                    }
                }
            }

            if (charSetInMeta.length()) {
                m = String::fromUTF8(charSetInMeta.data());
            }
        } else if (!m->contains("charset", false)) {
            m = String::fromUTF8(er.m_encoding);
        }

        TextConverter* converter = new TextConverter(
            m, String::createASCIIString("UTF-8"), m_buffer.data() + er.m_skip,
            m_buffer.size() - er.m_skip);
        m_htmlSource = converter->convert(m_buffer.data() + er.m_skip,
                                          m_buffer.size() - er.m_skip, true);
        m_builder.document()->setCharacterSet(converter->encoding());

        String* contentLanguage =
            m_resource->resourceRequest()->contentLanguage();
        if (!contentLanguage->isEmpty()) {
            m_builder.document()->setContentLanguage(contentLanguage);
        }

        if (!m_resource->resourceRequest()->lastLocation()->equals(
                String::emptyString)) {
            // Change documentURI and last history when request was redirected.
            ResourceURL* newURL =
                new ResourceURL(m_resource->resourceRequest()->lastLocation());
            m_builder.document()->setDocumentURI(newURL);
            m_builder.document()
                ->window()
                ->history()
                ->historyManager()
                ->replace(m_builder.document(), newURL);
        }
        if (m_resource->resourceRequest()->referrer()) {
            m_builder.document()->m_referrer =
                m_resource->resourceRequest()->referrer();
        }
        m_builder.document()->resourceLoader().updateDocumentOpenTime();
        load();
    }

    void load()
    {
        Document* document = m_builder.document();
        m_builder.m_parser = m_parser =
            new HTMLParser(document->starFish(), document, m_htmlSource);
        m_parser->startParse();
        m_parser->parseStep();
    }

protected:
    GCVector<char> m_buffer;
    HTMLDocumentBuilder& m_builder;
    HTMLParser* m_parser;
    String* m_htmlSource;
};

void HTMLDocumentBuilder::build(ResourceURL* url, ResourceURL* referrerURL)
{
    m_resource = m_document->resourceLoader().fetch(url);
    m_resource->addResourceClient(new HTMLResourceClient(m_resource, *this));
#ifndef STARFISH_TIZEN_WEARABLE
    m_resource->request(Resource::ResourceRequestSyncLevel::NeverSync,
                        referrerURL);
#else
    m_resource->request(Resource::ResourceRequestSyncLevel::AlwaysSync,
                        referrerURL);
#endif
}

void HTMLDocumentBuilder::build(String* str)
{
    m_document->resourceLoader().markDocumentOpenState();
    HTMLParser parser(starFish(), m_document, str);
    parser.startParse();
    parser.parseStep();
}

void HTMLDocumentBuilder::openFunctionExplicitCalled()
{
    m_parser = new HTMLParser(starFish(), m_document, String::emptyString);
    m_parser->startParse();
}

void HTMLDocumentBuilder::resume()
{
    m_parser->parseStep();
}
}
