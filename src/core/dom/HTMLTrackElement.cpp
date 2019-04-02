/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/TextTrack.h"
#include "core/dom/VTTCue.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "platform/loader/ElementResourceClient.h"
#include "platform/loader/ResourceLoader.h"
#include "webvttparser.h"

namespace Starfish {

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
            m_resource->resourceRequest() == nullptr) {
            return -1;
        }

        unsigned int size = m_resource->resourceRequest()->response().size();
        unsigned int pointer = nextPointer();
        if (pointer >= size) {
            return 1;
        }

        *c = m_resource->resourceRequest()->response()[pointer];
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
        m_element->setReadyState(HTMLTrackElement::IN_ERROR);
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

HTMLTrackElement::HTMLTrackElement(Document* document,
                                   const QualifiedName& qname)
    : HTMLElement(document, qname)
    , m_track(new TextTrack(document))
    , m_VTTFileResource(nullptr)
    , m_hasPendingRequest(false)
    , m_live(false)
    , m_readyState(HTMLTrackElement::NONE)
{
    m_track->setTrackElement(this);
}

void* HTMLTrackElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLTrackElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLTrackElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLTrackElement, m_track));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLTrackElement, m_VTTFileResource));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLTrackElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLTrackElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();
    if (name == ss->m_kind) {
        m_track->setKind(value);
    } else if (name == ss->m_src) {
        // TODO : FIX HERE
        m_track->clear();
        if (!value->equals(String::emptyString)) {
            load(value);
        }
    } else if (name == ss->m_srclang) {
        m_track->setLanguage(value);
    } else if (name == ss->m_label) {
        m_track->setLabel(value);
    } else if (name == ss->m_onloadeddata) {
        setAttributeEventListener(ss->m_loadeddata, value, this);
    }
}

void HTMLTrackElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
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

void HTMLTrackElement::didNodeRemovedFromDocumentTree()
{
    HTMLElement::didNodeRemovedFromDocumentTree();
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

    ResourceURL* url =
        new ResourceURL(srcURL, document()->documentURI()->baseURI());
    if (!document()->contentSecurityPolicy()->allowSource(
            CSPDirectives::MediaSrc, url)) {
        String* eventName =
            document()->starfish()->staticStrings()->m_error.localName();
        Event* event =
            new Event(executionContext(), eventName, EventInit(false, false));
        dispatchEventIdleTimeByUA(event);
        return;
    }
    clearResource();

    m_VTTFileResource = document()->resourceLoader().fetch(url);
    m_VTTFileResource->addResourceClient(
        new VTTFileDownloadClient(this, m_VTTFileResource));
    m_VTTFileResource->addResourceClient(
        new ElementResourceClient(this, m_VTTFileResource));

    RequestData* reqData = new RequestData();
    reqData->m_url = url;
    reqData->m_referrer = new ReferrerURL(document()->documentURI());
    reqData->m_destination = RequestDestination::Track;
    reqData->m_syncLevel = RequestSyncLevel::NeverSync;

    m_VTTFileResource->request(reqData, true);
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
    return getAttributeOrEmpty(starfish()->staticStrings()->m_src);
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
    size_t siz = hasAttribute(starfish()->staticStrings()->m_default);
    if (siz == SIZE_MAX) {
        return false;
    }
    return true;
}

void HTMLTrackElement::setKind(String* kind)
{
    setAttribute(starfish()->staticStrings()->m_kind, kind);
}

void HTMLTrackElement::setSrc(String* src)
{
    setAttribute(starfish()->staticStrings()->m_src, src);
}

void HTMLTrackElement::setSrclang(String* srclang)
{
    setAttribute(starfish()->staticStrings()->m_srclang, srclang);
}

void HTMLTrackElement::setLabel(String* label)
{
    setAttribute(starfish()->staticStrings()->m_label, label);
}

void HTMLTrackElement::setDefaultAttr(bool value)
{
    QualifiedName name = starfish()->staticStrings()->m_default;
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
