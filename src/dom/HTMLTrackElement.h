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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    !defined(__StarFishHTMLTrackElement__)
#define __StarFishHTMLTrackElement__

#include "dom/HTMLElement.h"
#include "loader/Resource.h"

namespace StarFish {

class TextTrack;

class HTMLTrackElement : public HTMLElement {
    friend class VTTFileDownloadClient;
    friend class VttResourceReader;

public:
    enum ReadyState { NONE, LOADING, LOADED, ERROR };

    HTMLTrackElement(Document* document);

    virtual void init(ScriptBindingInstance* instance) override
    {
        scriptObject()->set__proto__(
            fetchData(instance)->fnHTMLTrackElement()->protoType());
    }

    virtual String* localName();
    virtual QualifiedName name();

    virtual bool isHTMLTrackElement() const override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
    virtual void didNodeInsertedToDocumenTree();
    virtual void didNodeRemovedFromDocumenTree();

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
