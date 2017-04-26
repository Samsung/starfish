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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFish.h"
#include "dom/Document.h"
#include "dom/HTMLTrackElement.h"
#include "dom/TextTrack.h"
#include "dom/VTTCue.h"
#include "loader/ElementResourceClient.h"
#include "platform/window/Window.h"
#include "webvttparser.h"

namespace StarFish {

class VttResourceReader : public libwebvtt::Reader {
public:
    VttResourceReader()
        : m_resource(nullptr)
        , m_pointer(-1)
    {
    }

    virtual ~VttResourceReader()
    {
    }

    void setResource(Resource* resource)
    {
        m_resource = resource;
    }

    unsigned int nextPointer()
    {
        return ++m_pointer;
    }

    // ERROR:negative / SUCCESS:0 / EOS:positive
    virtual int GetChar(char* c)
    {
        if (c == NULL || m_resource == nullptr ||
            m_resource->networkRequest() == nullptr) {
            return -1;
        }

        unsigned int size = m_resource->networkRequest()->response().size();
        unsigned int pointer = nextPointer();
        if (pointer >= size) {
            return 1;
        }

        *c = m_resource->networkRequest()->response()[pointer];
        return 0;
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
        STARFISH_ASSERT(m_element);
        m_element->setReadyState(HTMLTrackElement::LOADING);
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_element->setReadyState(HTMLTrackElement::ERROR);
        m_element->clearResource();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        m_element->setReadyState(HTMLTrackElement::NONE);
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        m_element->setReadyState(HTMLTrackElement::LOADED);
        m_element->generateCues();
        m_element->m_VTTFileResource = nullptr;
    }

protected:
    HTMLTrackElement* m_element;
};

HTMLTrackElement::HTMLTrackElement(Document* document)
    : HTMLElement(document)
    , m_track(new TextTrack(document))
    , m_VTTFileResource(nullptr)
    , m_hasPendingRequest(false)
    , m_live(false)
    , m_readyState(HTMLTrackElement::NONE)
{
    m_track->setTrackElement(this);
}

String* HTMLTrackElement::localName()
{
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_trackTagName.localName();
}

QualifiedName HTMLTrackElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_trackTagName;
}

void HTMLTrackElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_kind) {
        m_track->setKind(value);
    } else if (name ==
               document()->window()->starFish()->staticStrings()->m_src) {
        // TODO : FIX HERE
        m_track->clear();
        if (!value->equals(String::emptyString)) {
            load(value);
        }
    } else if (name ==
               document()->window()->starFish()->staticStrings()->m_srclang) {
        m_track->setLanguage(value);
    } else if (name ==
               document()->window()->starFish()->staticStrings()->m_label) {
        m_track->setLabel(value);
    }
}

void HTMLTrackElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    m_live = true;
    if (m_hasPendingRequest) {
        load();
    }
    // Set Track Mode
    // FIXME
    STARFISH_ASSERT(m_track);
    if (defaultAttr()) {
        m_track->setMode(TextTrack::Mode::Showing);
    }
}

void HTMLTrackElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    m_live = false;
    clearResource();
}

void HTMLTrackElement::clearResource()
{
    if (isReadyState(HTMLTrackElement::LOADING)) {
        STARFISH_ASSERT(m_VTTFileResource);
        m_VTTFileResource->cancel();
    }
    m_VTTFileResource = nullptr;
}

void HTMLTrackElement::load()
{
    load(src());
}

void HTMLTrackElement::load(String* srcURL)
{
    if (!m_live) {
        m_hasPendingRequest = true;
        return;
    }
    if (srcURL->equals(String::emptyString)) {
        return;
    }

    URL* url = URL::createURL(document()->documentURI()->baseURI(), srcURL);
    clearResource();

    m_VTTFileResource = document()->resourceLoader().fetch(url);
    m_VTTFileResource->addResourceClient(
        new VTTFileDownloadClient(this, m_VTTFileResource));
    m_VTTFileResource->addResourceClient(
        new ElementResourceClient(this, m_VTTFileResource));
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

        // error or EOF
        if (e < 0 || e > 0) {
            return;
        }

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
            payload = payload->concat(String::fromUTF8((*i++).c_str()))
                          ->concat(newline);
        }

        VTTCue* newcue = new VTTCue(m_document, startTime, endTime, payload);
        newcue->setId(id);
        newcue->setTrack(m_track);

        m_track->addCue(newcue);
    }
}

String* HTMLTrackElement::kind()
{
    return TextTrack::kindToString(m_track->kindValue());
}

String* HTMLTrackElement::src()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_src);
}

String* HTMLTrackElement::srclang()
{
    return m_track->language();
}

String* HTMLTrackElement::label()
{
    return m_track->label();
}

bool HTMLTrackElement::defaultAttr()
{
    size_t siz = hasAttribute(
        document()->window()->starFish()->staticStrings()->m_default);
    if (siz == SIZE_MAX) {
        return false;
    }
    return true;
}

void HTMLTrackElement::setKind(String* kind)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_kind,
                 kind);
}

void HTMLTrackElement::setSrc(String* src)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
}

void HTMLTrackElement::setSrclang(String* srclang)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_srclang,
                 srclang);
}

void HTMLTrackElement::setLabel(String* label)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_label,
                 label);
}

void HTMLTrackElement::setDefaultAttr(bool value)
{
    QualifiedName name =
        document()->window()->starFish()->staticStrings()->m_default;
    if (value) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::fromUTF8(""));
        }
    } else {
        removeAttribute(name);
    }
}
}

#endif
