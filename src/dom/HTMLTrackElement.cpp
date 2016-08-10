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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLTrackElement.h"
#include "TextTrack.h"

#include "loader/ElementResourceClient.h"
// #include "platform/message_loop/MessageLoop.h"
// #include "style/CSSParser.h"
// #include "platform/file_io/FileIO.h"
#include "webvttparser.h"

namespace StarFish {

class VttResourceReader : public libwebvtt::Reader {
public:
    VttResourceReader()
        : m_resource(nullptr)
        , m_pointer(-1)
    {
    }

    virtual ~VttResourceReader() { }

    void setResource(Resource* resource)
    {
        m_resource = resource;
    }

    void resetPointer()
    {
        m_pointer = -1;
    }

    unsigned int nextPointer()
    {
        return ++m_pointer;
    }

    // ERROR:negative / SUCCESS:0 / EOS:positive
    virtual int GetChar(char* c)
    {
        if (c == NULL || m_resource == nullptr || m_resource->networkRequest() == nullptr) {
            return -1;
        }

        unsigned int size = m_resource->networkRequest()->responseData().size();
        unsigned int pointer = nextPointer();
        if (pointer == size) {
            resetPointer();
            return 1;
        }

        *c = m_resource->networkRequest()->responseData().at(pointer);
        return 0;

        // const int result = fgetc(file_);VttResourceReader
        // if (result != EOF) {
        // *c = static_cast<char>(result);
        // return 0;  // success
        // }

        // if (ferror(file_))
        // return -1;  // error

        // if (feof(file_))
        // return 1;  // EOF

        // return -1;  // weird
    }

private:
    Resource* m_resource;
    unsigned int m_pointer;

    VttResourceReader(const VttResourceReader&);
    VttResourceReader& operator=(const VttResourceReader&);
};

class VTTFileDownloadClient : public ResourceClient {
public:
    VTTFileDownloadClient(HTMLTrackElement* element, Resource* res)
        : ResourceClient(res)
        , m_element(element)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        // String* text = m_resource->asTextResource()->text();
        m_element->generateCues();
    }

protected:
    HTMLTrackElement* m_element;
};

HTMLTrackElement::HTMLTrackElement(Document* document)
    : HTMLElement(document)
    , m_track(new TextTrack())
    , m_VTTFileResource(nullptr)
    , m_hasPendingRequest(false)
    , m_live(false)
    , m_default(true)
{

}

void HTMLTrackElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        m_track->cues()->clearAll();
        if (!value->equals(String::emptyString)) {
            loadSrc(value); 
        }
    }
}

void HTMLTrackElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    m_live = true;
    if (m_hasPendingRequest) {
        loadSrc();
    }
}

void HTMLTrackElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    m_live = false;
    if (m_VTTFileResource) {
        m_VTTFileResource->cancel();
        m_VTTFileResource = nullptr;
    }
}

void HTMLTrackElement::loadSrc()
{
    loadSrc(src());
}

void HTMLTrackElement::loadSrc(String* srcURL)
{
    if (!m_live || !m_default) {
        m_hasPendingRequest = true;
        return;
    }
    if (srcURL->equals(String::emptyString)) {
        STARFISH_ASSERT(m_track->cues()->length() == 0);
        return;
    }

    URL* url = URL::createURL(document()->documentURI()->baseURI(), srcURL);

    if (m_VTTFileResource) {
        m_VTTFileResource->cancel();
    }
    m_VTTFileResource = document()->resourceLoader()->fetch(url);
    m_VTTFileResource->addResourceClient(new VTTFileDownloadClient(this, m_VTTFileResource));
    m_VTTFileResource->addResourceClient(new ElementResourceClient(this, m_VTTFileResource));
    m_VTTFileResource->request();

    m_hasPendingRequest = false;
}

void HTMLTrackElement::generateCues()
{
    if (!m_VTTFileResource) {
        return;
    }

    VttResourceReader reader;
    reader.setResource(m_VTTFileResource);
    libwebvtt::Parser parser(&reader);

    if (parser.Init()) {
        return;
    }
    STARFISH_ASSERT(m_track);

    // Note : third_party/webm/dumpvtt.cc
    for (libwebvtt::Cue cue;;) {
        const int e = parser.Parse(&cue);

        if (e < 0 || e > 0) // error or EOF
            return;

        String* id = String::fromUTF8(cue.identifier.c_str());
        double startTime = (cue.start_time.presentation()) / 1000.f;
        double endTime = (cue.stop_time.presentation()) / 1000.f;
        String* newline = String::fromUTF8("\n");
        String* payload = String::emptyString;
        // TODO : settings

        typedef libwebvtt::Cue::payload_t::const_iterator iter_t;
        iter_t i = cue.payload.begin();
        const iter_t j = cue.payload.end();
        while (i != j) {
            payload = payload->concat(String::fromUTF8((*i++).c_str()))->concat(newline);
        }

        TextTrackCue* newcue = new TextTrackCue(startTime, endTime, payload);
        newcue->setId(id);
        newcue->setTrack(m_track);

        m_track->addCue(newcue);
    }
}

}

#endif
