/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "MediaPlayerAudio.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLAudioElement.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"

#include "core/fetch/RequestData.h"
#include "platform/loader/ResourceLoader.h"

namespace Starfish {

class AudioDownloadClient : public ResourceClient {
public:
    AudioDownloadClient(MediaPlayerAudio* element, Resource* res)
        : ResourceClient(res)
        , m_element(element)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        RequestErrorType error = m_resource->requestErrorType();
        STARFISH_LOG_ERROR("AudioDownloadClient::%s: %d\n", __func__,
                           (int)error);
    }

    virtual void didLoadFinished()
    {
        STARFISH_LOG_INFO("AudioDownloadClient::%s\n", __func__);

        ResourceClient::didLoadFinished();

        m_element->m_audioData =
            m_element->m_audioResource->resourceRequest()->response();

        m_element->onAudioDownloadCompleted();
    }

protected:
    MediaPlayerAudio* m_element{ nullptr };
};

MediaPlayerAudio::MediaPlayerAudio(AudioNode* element)
    : MediaPlayer(nullptr)
{
}

MediaPlayerAudio::MediaPlayerAudio(HTMLMediaElement* element)
    : MediaPlayer(element)
{
}

void MediaPlayerAudio::play()
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s\n", __func__);
}

void MediaPlayerAudio::destroy()
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s\n", __func__);
}

void MediaPlayerAudio::setBuffer(uint8_t* buffer, uint32_t length)
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s\n", __func__);

    m_audioData.reserve(length);
    memcpy(m_audioData.data(), buffer, length);
}

void MediaPlayerAudio::prepare(ResourceURL* url)
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s\n", __func__);

    if (m_container == nullptr) {
        STARFISH_LOG_WARN("MediaPlayerAudio::%s: container is null\n",
                          __func__);
        return;
    }

    downloadAudioData(url);
}

void MediaPlayerAudio::downloadAudioData(ResourceURL* url)
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s: %s\n", __func__,
                      url->urlString()->toUTF8NonGCString().data());

    m_audioResource = m_container->document()->resourceLoader().fetch(url);
    m_audioResource->addResourceClient(
        new AudioDownloadClient(this, m_audioResource));

    RequestData* data = new RequestData();
    data->m_url = m_audioResource->url();
    data->m_referrer = new ReferrerURL(m_container->document()->documentURI());
    data->m_syncLevel = RequestSyncLevel::SyncIfAlreadyLoaded;

    m_audioResource->request(data, true);
}

void MediaPlayerAudio::onAudioDownloadCompleted()
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s\n", __func__);

    MessageLoop* msgLoop = m_container->webView()->messageLoop();
    msgLoop->addIdler(
        m_container->window(),
        [](size_t, void* data) {
            MediaPlayerAudio* self = (MediaPlayerAudio*)data;
            self->processNextOperationQueueInContainer();
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_METADATA);
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_ENOUGH_DATA);
        },
        this);
}
} // namespace Starfish

#endif
