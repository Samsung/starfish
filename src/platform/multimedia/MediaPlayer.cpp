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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/dom/WebOrigin.h"
#include "core/fetch/RequestData.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/loader/HeaderResource.h"
#include "platform/loader/ResourceClient.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/multimedia/MediaPlayer.h"
#include "core/modules/renderer/Renderer.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"

namespace Starfish {

MediaPlayer::MediaPlayer(HTMLMediaElement* element)
    : m_alive(true)
    , m_foundError(false)
    , m_isLooping(false)
    , m_hasVideo(false)
    , m_seekState(SEEKSTATE_NO_SEEK)
    , m_playbackState(PLAYBACK_STATE_NONE)
    , m_container(element)
    , m_activeMediaSource(nullptr)
    , m_videoWidth(STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS)
    , m_videoHeight(STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS)
    , m_currentTimeUpdateTimer(TimerInvalidID)
    , m_playerStateMutex(new Mutex())
{
    if (m_container) {
        m_canvasSurface =
            CanvasSurface::create(m_container->webView()->renderer(), 1, 1);
    }
}

void MediaPlayer::processNextOperationQueueInContainer()
{
    if (m_container) {
        m_container->processNextOperationQueue();
    }
}

void MediaPlayer::notifyMediaSourceFailure()
{
    if (!alive()) {
        return;
    }
    m_foundError = true;
    m_container->dedicatedMediaSourceFailure();
}

void MediaPlayer::appendToOperationQueueInContainer(
    MediaOperationQueueData* data)
{
    if (m_container) {
        m_container->appendToOperationQueue(data);
    }
}

Window* MediaPlayer::window()
{
    return m_container->window();
}

CanvasSurface* MediaPlayer::contentSurface()
{
    return m_canvasSurface;
}

SourceBuffer* MediaPlayer::activeSourceBuffer(StreamType type)
{
    if (m_activeMediaSource) {
        if (type == StreamTypeAudio) {
            return m_activeMediaSource->activeAudioSourceBuffer();
        } else {
            return m_activeMediaSource->activeVideoSourceBuffer();
        }
    }
    return nullptr;
}

uint64_t MediaPlayer::activeStreamIndex(StreamType type)
{
    STARFISH_ASSERT(m_activeMediaSource);
    if (type == StreamTypeAudio) {
        return m_activeMediaSource->activeAudioStreamIndex();
    }
    STARFISH_ASSERT(type == StreamTypeVideo);
    return m_activeMediaSource->activeVideoStreamIndex();
}

bool MediaPlayer::isMSE()
{
    return m_activeMediaSource;
}

MediaPlayer::PlaybackState MediaPlayer::playbackState()
{
    Locker<Mutex> locker(*m_playerStateMutex);
    return m_playbackState;
}

void MediaPlayer::setPlaybackState(MediaPlayer::PlaybackState state)
{
    Locker<Mutex> locker(*m_playerStateMutex);
    m_playbackState = state;
}

// A playlist (HLS, DASH) fetches its segments through requests of its
// own, so where the playlist came from says nothing about where the media
// bytes come from; the FFmpeg player refuses to vouch for those formats for
// the same reason. Judged from the response's MIME type and, since servers
// often label playlists as plain text, the URL's extension.
static bool isPlaylistResponse(Resource* resource, ResourceURL* url)
{
    String* mimeType = resource->responseMimeType()->toLower();
    static const char* const playlistTypes[] = {
        "application/vnd.apple.mpegurl",
        "application/x-mpegurl",
        "audio/mpegurl",
        "audio/x-mpegurl",
        "application/dash+xml",
    };
    for (const char* type : playlistTypes) {
        if (mimeType->startsWith(type, strlen(type))) {
            return true;
        }
    }
    String* path = url->pathname();
    return path->endsWith(".m3u8", false) || path->endsWith(".mpd", false);
}

class ResponseURLProbeClient : public ResourceClient {
public:
    ResponseURLProbeClient(Resource* resource, MediaPlayer* player,
                           ResourceURL* url)
        : ResourceClient(resource)
        , m_player(player)
        , m_url(url)
    {
    }

    void didLoadFinished() override
    {
        ResourceRequest* request = m_resource->resourceRequest();
        finish(!request->isRedirected() && request->status() / 100 == 2 &&
               !isPlaylistResponse(m_resource, m_url));
    }

    void didLoadFailed() override
    {
        finish(false);
    }

    // The player disowns the probe before canceling it from its teardown;
    // a probe still owned was canceled by the loader (the document going
    // away), which the player treats like a failed one.
    void didLoadCanceled() override
    {
        Optional<Resource*> probe = m_player->m_responseURLProbe;
        if (!probe || probe.value() != m_resource) {
            return;
        }
        finish(false);
    }

private:
    void finish(bool vouched)
    {
        m_player->m_responseURLProbe = nullptr;
        if (!m_player->alive()) {
            return;
        }
        if (vouched) {
            m_player->vouchResponseURL(m_url);
        }
        m_player->didProbeResponseURL(m_url);
    }

    MediaPlayer* m_player;
    ResourceURL* m_url;
};

void MediaPlayer::probeResponseURL(ResourceURL* url)
{
    STARFISH_ASSERT(!m_responseURLProbe);
    revokeResponseURL();
    Document* document = m_container->document();
    HeaderResource* resource = document->resourceLoader().fetchHeader(url);
    resource->markThisResourceIsDoesNotAffectWindowOnLoad();
    resource->addResourceClient(
        new ResponseURLProbeClient(resource, this, url));
    m_responseURLProbe = resource;

    RequestData* request = new RequestData();
    request->m_url = url;
    request->m_referrer = new ReferrerURL(document->documentURI());
    request->m_method = String::createASCIIString("HEAD");
    request->m_mode = RequestMode::NoCORS;
    request->m_credentials = RequestCredentials::Omit;
    request->m_syncLevel = RequestSyncLevel::NeverSync;
    resource->request(request, false);
}

void MediaPlayer::cancelResponseURLProbe()
{
    if (!m_responseURLProbe) {
        return;
    }
    Resource* probe = m_responseURLProbe.value();
    m_responseURLProbe = nullptr;
    if (probe->state() <= Resource::Receiving) {
        probe->cancel();
    }
}

Optional<ResourceURL*> MediaPlayer::responseURL()
{
    if (!m_responseURLVouched.load()) {
        return nullptr;
    }
    return m_vouchedResponseURL;
}

void MediaPlayer::vouchResponseURL(ResourceURL* url)
{
    m_vouchedResponseURL = url;
    m_responseURLVouched = true;
}

void MediaPlayer::revokeResponseURL()
{
    m_responseURLVouched = false;
}

bool MediaPlayer::responseIsSameOrigin(WebOrigin* origin)
{
    Optional<ResourceURL*> response = responseURL();
    if (!response) {
        return false;
    }
    if (response.value() != m_judgedResponseURL) {
        WebOrigin* responseOrigin =
            WebOrigin::createDocumentOrigin(response.value());
        // An opaque origin is same-origin only with itself, which a
        // response's never is.
        m_judgedResponseURL = response.value();
        m_judgedSameOrigin =
            !responseOrigin->isOpaque() && origin->isSameOrigin(responseOrigin);
    }
    return m_judgedSameOrigin;
}

} // namespace Starfish
#endif /* STARFISH_ENABLE_MULTIMEDIA */
