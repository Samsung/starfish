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

#include <algorithm>
#include <cstdlib>
#include <limits>

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
        STARFISH_LOG_ERROR("AudioDownloadClient::%s: %d", __func__, (int)error);
        if (m_element->alive()) {
            m_element->onAudioDownloadFailed();
        }
    }

    virtual void didLoadFinished()
    {
        STARFISH_LOG_INFO("AudioDownloadClient::%s", __func__);

        ResourceClient::didLoadFinished();

        if (!m_element->alive()) {
            return;
        }

        if (m_rejected) {
            return;
        }
        ResponseBody& response =
            m_element->m_audioResource->resourceRequest()->response();
        if (!m_element->acceptsEncodedSize(response.size())) {
            ResponseBody().swap(response);
            m_element->onAudioDownloadFailed();
            return;
        }
        // The request is released after this callback; take its body rather
        // than holding a second copy of the encoded resource.
        m_element->m_audioData = std::move(response);

        m_element->onAudioDownloadCompleted();
    }

    virtual void didHeaderReceived(
        const std::unordered_map<std::string, std::string>& headers)
    {
        if (!m_element->alive() || m_rejected) {
            return;
        }
        for (const auto& header : headers) {
            if (!StringUtils::equalsIgnoreCase(header.first,
                                               "content-length")) {
                continue;
            }
            const char* value = header.second.c_str();
            char* end = nullptr;
            unsigned long long length = strtoull(value, &end, 10);
            if (end != value &&
                !m_element->acceptsEncodedSize(
                    static_cast<size_t>(std::min<unsigned long long>(
                        length, std::numeric_limits<size_t>::max())))) {
                reject();
            }
            return;
        }
    }

protected:
    // Fail an oversized resource as soon as its declared length is known
    // instead of buffering it to completion.
    // https://html.spec.whatwg.org/#media-data-processing-steps-list
    void reject()
    {
        m_rejected = true;
        m_element->onAudioDownloadFailed();
        // Canceling inside this callback would mutate the client list that
        // Resource is iterating, so it is deferred.
        size_t handle =
            m_element->container()->webView()->messageLoop()->addIdler(
                m_element->container()->window(),
                [](size_t handle, void* data) {
                    auto* self = static_cast<AudioDownloadClient*>(data);
                    Resource* resource = self->m_resource;
                    resource->removeIdlerHandle(handle);
                    if (resource->state() == Resource::BeforeSend ||
                        resource->state() == Resource::Receiving) {
                        resource->cancel();
                    }
                },
                this);
        m_resource->pushIdlerHandle(handle);
    }

    MediaPlayerAudio* m_element{ nullptr };
    bool m_rejected{ false };
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
    STARFISH_LOG_INFO("MediaPlayerAudio::%s", __func__);
}

void MediaPlayerAudio::destroy()
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s", __func__);
    m_alive = false;
    ReadableStreamChunk().swap(m_audioData);
}

void MediaPlayerAudio::onAudioDownloadFailed()
{
    notifyMediaSourceFailure();
}

void MediaPlayerAudio::setBuffer(uint8_t* buffer, uint32_t length)
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s", __func__);

    m_audioData.resize(length);
    memcpy(m_audioData.data(), buffer, length);
}

void MediaPlayerAudio::prepare(ResourceURL* url)
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s", __func__);

    if (m_container == nullptr) {
        STARFISH_LOG_WARN("MediaPlayerAudio::%s: container is null", __func__);
        return;
    }

    downloadAudioData(url);
}

void MediaPlayerAudio::downloadAudioData(ResourceURL* url)
{
    STARFISH_LOG_INFO("MediaPlayerAudio::%s", __func__);

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
    STARFISH_LOG_INFO("MediaPlayerAudio::%s", __func__);

    MessageLoop* msgLoop = m_container->webView()->messageLoop();
    msgLoop->addIdler(
        m_container->window(),
        [](size_t, void* data) {
            MediaPlayerAudio* self = (MediaPlayerAudio*)data;
            if (!self->alive()) {
                return;
            }
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
