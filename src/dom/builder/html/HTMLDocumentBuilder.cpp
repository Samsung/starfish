/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "HTMLDocumentBuilder.h"

#include "dom/DOM.h"
#include "dom/binding/ScriptBindingInstance.h"
#include "dom/parser/HTMLParser.h"

namespace StarFish {

int strcicmp(char const *a, size_t len1, char const *b, size_t len2)
{
    char const* aEnd = a + len1;
    char const* bEnd = b + len2;
    for (; a < aEnd && b < bEnd; a++, b++) {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
    return 0;
}


const char* sstrstr(const char *haystack, size_t length, const char *needle, size_t needleLength)
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
        m_htmlSource = String::createASCIIString("<div style='text-align:center;margin-top:50px;'>Cannot open page ");
        m_htmlSource = m_htmlSource->concat(m_resource->url()->urlString());
        m_htmlSource = m_htmlSource->concat(String::createASCIIString("</div>"));
        load();
    }

    virtual void didDataReceived(const char* buffer, size_t length)
    {
        m_buffer.assign(&buffer[0], &buffer[length]);
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();

        String* m = m_resource->networkRequest()->mimeType();
        if (!m->contains("charset", false)) {
            // TODO check BOM
            size_t bufferLen = m_buffer.size();
            std::string charSetInMeta;
            for (size_t i = 0; i < bufferLen; i ++) {
                if (m_buffer[i] == '<') {
                    char tagName[12];
                    size_t tagNameLength = 0;
                    bool gotChar = false;
                    for (size_t j = i + 1; j < bufferLen; j ++) {
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
                                    for (size_t k = j + 1; k < bufferLen; k ++) {
                                        if (m_buffer[k] == '>') {
                                            i = k;
                                            closeFinded = true;
                                            break;
                                        }
                                    }

                                    std::string attr;
                                    for (size_t k = attributeStart; k < i ; k ++) {
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
                                        const char* result = sstrstr(attr.c_str(), attr.length(), "charset=", 8);
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
        }

        TextConverter* converter = new TextConverter(m, String::createASCIIString("UTF-8"), m_buffer.data(), m_buffer.size());
        m_htmlSource = converter->convert(m_buffer.data(), m_buffer.size(), true);
        m_builder.document()->setCharset(converter->encoding());
        load();
    }

    void load()
    {
        Document* document = m_builder.document();
        m_builder.m_parser = m_parser = new HTMLParser(document->window()->starFish(), document, m_htmlSource);
        m_parser->startParse();
        m_parser->parseStep();
    }

protected:
    std::vector<char, gc_allocator<char>> m_buffer;
    HTMLDocumentBuilder& m_builder;
    HTMLParser* m_parser;
    String* m_htmlSource;
};

void HTMLDocumentBuilder::build(URL* url)
{
    m_resource = m_document->resourceLoader()->fetch(url);
    m_resource->addResourceClient(new HTMLResourceClient(m_resource, *this));
#ifndef STARFISH_TIZEN_WEARABLE
    m_resource->request();
#else
    if (url->isFileURL())
        m_resource->request(true);
    else
        m_resource->request();
#endif
}

void HTMLDocumentBuilder::resume()
{
    m_parser->parseStep();
}

}
