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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    !defined(__StarFishHTMLTrackElement__)
#define __StarFishHTMLTrackElement__

#include "core/dom/HTMLElement.h"
#include "platform/loader/Resource.h"

namespace StarFish {

class TextTrack;

class HTMLTrackElement : public HTMLElement {
    friend class VTTFileDownloadClient;
    friend class VttResourceReader;

public:
    enum ReadyState { NONE, LOADING, LOADED, ERROR };

    HTMLTrackElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTrackElement() const override;

    virtual QualifiedName name();

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
    virtual void didNodeInsertedToDocumentTree();
    virtual void didNodeRemovedFromDocumentTree();

    void load();
    void load(String* srcURL);

    /* Interface */
    // https://html.spec.whatwg.org/multipage/embedded-content.html#the-track-element
    String* kind();
    String* src();
    String* srclang();
    String* label();
    bool defaultAttr();
    ReadyState readyState()
    {
        return m_readyState;
    }
    TextTrack* track()
    {
        return m_track;
    }

    void setKind(String* kind);
    void setSrc(String* src);
    void setSrclang(String* srclang);
    void setLabel(String* label); // not public
    void setDefaultAttr(bool value);
    bool isReadyState(ReadyState state)
    {
        return m_readyState == state;
    } // not public
    void setReadyState(ReadyState readyState)
    {
        m_readyState = readyState;
    } // not public

protected:
    void generateCues();
    void clearResource();

protected:
    TextTrack* m_track;
    Resource* m_VTTFileResource;

    bool m_hasPendingRequest;
    bool m_live;
    ReadyState m_readyState;
};
}

#endif
