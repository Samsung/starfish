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
#if !defined(STARFISH_TIZEN)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "MediaPlayerAudioLinux.h"

#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/resource_request/ResourceRequest.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"
#include "core/modules/webaudio/MediaElementAudioSourceNode.h"
#include "core/modules/webaudio/render/AudioBus.h"
#include "core/modules/webaudio/render/AudioBufferData.h"
#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioHandlers.h"
#include "platform/loader/ResourceURL.h"
#include "platform/webaudio/AudioOutputDevice.h"

#include <atomic>
#include <limits>
#include <utility>

namespace Starfish {
class MediaAudioDecodeWork {
public:
    MediaAudioDecodeWork(std::vector<char>&& bytes, MessageLoop* messageLoop)
        : m_bytes(std::move(bytes))
        , m_messageLoop(messageLoop)
    {
    }

    ~MediaAudioDecodeWork()
    {
        if (m_pcm) {
            m_pcm->release();
        }
    }

    void cancel()
    {
        m_cancelled.store(true, std::memory_order_release);
    }

    bool cancelled() const
    {
        return m_cancelled.load(std::memory_order_acquire);
    }

    AudioBufferData* takePCM()
    {
        AudioBufferData* pcm = m_pcm;
        m_pcm = nullptr;
        return pcm;
    }

    static void* run(void* data)
    {
        auto* work = static_cast<MediaAudioDecodeWork*>(data);
        if (!work->cancelled()) {
            const auto* bytes =
                reinterpret_cast<const uint8_t*>(work->m_bytes.data());
            work->m_pcm = decodeWaveAudio(bytes, work->m_bytes.size(), 48000);
            if (!work->m_pcm && !work->cancelled()) {
                work->m_pcm =
                    decodeCompressedAudio(bytes, work->m_bytes.size(), 48000);
            }
            // The element keeps its decoded PCM for as long as it holds the
            // resource. Bound it to the compressed decoder's output limit so
            // a WAVE resource (e.g. 8 kHz upsampled to 48 kHz) cannot pin the
            // full AudioBuffer allocation limit per element.
            constexpr size_t MaxElementSamples =
                32 * 1024 * 1024 / sizeof(float);
            if (work->m_pcm &&
                work->m_pcm->frames() >
                    MaxElementSamples / work->m_pcm->channels()) {
                work->m_pcm->release();
                work->m_pcm = nullptr;
            }
        }
        std::vector<char>().swap(work->m_bytes);
        work->m_messageLoop->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                auto* work = static_cast<MediaAudioDecodeWork*>(data);
                work->m_job->complete();
            },
            work);
        return nullptr;
    }

    MediaAudioDecodeJob* m_job{ nullptr }; // Main thread only.

private:
    std::vector<char> m_bytes;
    MessageLoop* m_messageLoop;
    std::atomic<bool> m_cancelled{ false };
    AudioBufferData* m_pcm{ nullptr };
};

MediaAudioDecodeJob::MediaAudioDecodeJob(MediaPlayerAudioLinux* player,
                                         WebView* webView, Window* window,
                                         MediaAudioDecodeWork* work)
    : m_player(player)
    , m_webView(webView)
    , m_window(window)
    , m_work(work)
{
}

Window* MediaAudioDecodeJob::window() const
{
    return m_window;
}

void MediaAudioDecodeJob::cancel()
{
    m_work->cancel();
}

void MediaAudioDecodeJob::complete()
{
    std::unique_ptr<MediaAudioDecodeWork> work(m_work);
    m_work = nullptr;
    m_webView->unregisterMediaAudioDecodeJob(this);
    if (m_player->m_decodeJob == this) {
        m_player->m_decodeJob = nullptr;
    }
    if (!work->cancelled() && m_player->alive()) {
        m_player->didDecodeAudio(work->takePCM());
    }
}

void MediaAudioDecodeJob::discard()
{
    delete m_work;
    m_work = nullptr;
}

MediaPlayerAudioLinux::MediaPlayerAudioLinux(AudioNode* element)
    : MediaPlayerAudio(element)
    , m_playbackState(new MediaAudioPlaybackState)
{
}

MediaPlayerAudioLinux::MediaPlayerAudioLinux(HTMLMediaElement* element)
    : MediaPlayerAudio(element)
    , m_playbackState(new MediaAudioPlaybackState)
{
    if (element->audioSourceNode()) {
        element->audioSourceNode()->setPlaybackState(m_playbackState);
    }
}

void MediaPlayerAudioLinux::play()
{
    if (playbackState() == PLAYBACK_STATE_PLAYING ||
        std::isnan(m_playbackState->duration())) {
        return;
    }
    if (!m_playbackState->routed()) {
        std::unique_ptr<AudioOutputDevice> output =
            AudioOutputDevice::create(48000);
        m_outputDevice = output.release();
        if (m_outputDevice) {
            m_outputBus = new AudioBus(2);
        }
    }
    m_playbackState->setPlaying(true);
    m_outputPosition = m_playbackState->currentTime();
    m_nextOutputFrame = 0;
    m_outputClockStart = std::chrono::steady_clock::now();
    setPlaybackState(PLAYBACK_STATE_PLAYING);
    m_container->executionContext()->addPointerInRootSet(this);
    m_currentTimeUpdateTimer = m_container->window()->setInterval(
        [](void* data) {
            static_cast<MediaPlayerAudioLinux*>(data)->playbackTick();
        },
        10, this);
}

void MediaPlayerAudioLinux::pause()
{
    if (playbackState() != PLAYBACK_STATE_PLAYING) {
        return;
    }
    m_playbackState->setPlaying(false);
    setPlaybackState(PLAYBACK_STATE_PAUSED);
    m_container->window()->clearInterval(m_currentTimeUpdateTimer);
    m_container->executionContext()->removePointerFromRootSet(this);
    delete m_outputDevice;
    m_outputDevice = nullptr;
    delete m_outputBus;
    m_outputBus = nullptr;
}

void MediaPlayerAudioLinux::seek(double time)
{
    m_playbackState->seek(time);
    m_outputPosition = m_playbackState->currentTime();
    m_container->mediaPlayerNotifySeekedItsContainer(m_outputPosition);
}

void MediaPlayerAudioLinux::setLoop(bool loop)
{
    MediaPlayer::setLoop(loop);
    m_playbackState->setLoop(loop);
    m_outputPosition = m_playbackState->currentTime();
}

MediaAudioPlaybackState* MediaPlayerAudioLinux::audioPlaybackState()
{
    return m_playbackState;
}

double MediaPlayerAudioLinux::currentTime()
{
    return m_playbackState->currentTime();
}

void MediaPlayerAudioLinux::setVolume(double volume)
{
    m_playbackState->setVolume(volume);
}

void MediaPlayerAudioLinux::setMuted(bool muted)
{
    m_playbackState->setMuted(muted);
}

void MediaPlayerAudioLinux::setPlaybackRate(double rate)
{
    m_playbackState->setRate(rate);
    m_outputPosition = m_playbackState->currentTime();
}

void MediaPlayerAudioLinux::playbackTick()
{
    if (!alive()) {
        return;
    }
    if (m_playbackState->ended()) {
        pause();
        m_container->mediaPlayerNotifyEndedItsContainer();
        return;
    }
    const double position = m_playbackState->currentTime();
    if (++m_tickCount % 25 == 0) {
        m_container->setOfficialPlaybackPosition(position);
    }
    if (m_playbackState->routed()) {
        delete m_outputDevice;
        m_outputDevice = nullptr;
        delete m_outputBus;
        m_outputBus = nullptr;
        return;
    }
    if (!m_outputDevice) {
        return;
    }
    // The device consumes frames at a fixed rate even when the media clock
    // changes speed or wraps at a loop boundary.
    // https://html.spec.whatwg.org/#playing-the-media-resource
    const double elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                      m_outputClockStart)
            .count();
    const uint64_t currentOutputFrame = static_cast<uint64_t>(elapsed * 48000);
    if (currentOutputFrame > m_nextOutputFrame + 4800) {
        m_nextOutputFrame = currentOutputFrame;
        m_outputPosition = position;
    }
    const uint64_t target = currentOutputFrame + 480;
    const double rate = m_playbackState->playbackRate();
    for (size_t quantum = 0; quantum < 8 && m_nextOutputFrame < target;
         quantum++) {
        m_outputBus->zero();
        m_playbackState->render(*m_outputBus, AudioBus::RenderQuantumFrames,
                                48000, m_outputPosition, false);
        m_outputDevice->submit(*m_outputBus, m_nextOutputFrame);
        m_outputPosition += AudioBus::RenderQuantumFrames * rate / 48000.0;
        m_nextOutputFrame += AudioBus::RenderQuantumFrames;
    }
    if (m_outputDevice->failed()) {
        delete m_outputDevice;
        m_outputDevice = nullptr;
        delete m_outputBus;
        m_outputBus = nullptr;
    }
}

void MediaPlayerAudioLinux::destroy()
{
    STARFISH_LOG_INFO("MediaPlayerAudioLinux::%s", __func__);
    pause();
    m_alive = false;
    if (m_decodeJob) {
        m_decodeJob->cancel();
        m_decodeJob = nullptr;
    }
    if (m_playbackState) {
        m_playbackState->release();
        m_playbackState = nullptr;
    }
    MediaPlayerAudio::destroy();
}

void MediaPlayerAudioLinux::prepare(ResourceURL* url)
{
    STARFISH_LOG_INFO("MediaPlayerAudioLinux::%s", __func__);
    // https://webaudio.github.io/web-audio-api/#MediaElementAudioSourceNode-security
    // A no-CORS cross-origin media resource may play directly, but its PCM
    // must never become script-observable through the Web Audio graph.
    m_playbackState->setOriginClean(
        url->protocolKind() != ResourceURL::DATA_PROTOCOL &&
        url->origin()->equals(m_container->document()->origin()));
    MediaPlayerAudio::prepare(url);
}

void MediaPlayerAudioLinux::onAudioDownloadCompleted()
{
    // The resource loader does not expose a CORS-approved final response URL
    // to media clients. Fail closed on redirects, including a same-origin
    // initial URL that may have redirected to a cross-origin resource.
    if (m_audioResource->resourceRequest()->isRedirected()) {
        m_playbackState->setOriginClean(false);
    }
    if (m_audioData.empty() || !acceptsEncodedSize(m_audioData.size())) {
        ReadableStreamChunk().swap(m_audioData);
        notifyMediaSourceFailure();
        return;
    }
    WebView* webView = m_container->webView();
    auto* work = new MediaAudioDecodeWork(std::move(m_audioData),
                                          webView->messageLoop());
    auto* job =
        new MediaAudioDecodeJob(this, webView, m_container->window(), work);
    work->m_job = job;
    m_decodeJob = job;
    webView->registerMediaAudioDecodeJob(job);
    webView->audioDecodeThreadPool()->addWork(m_container->executionContext(),
                                              MediaAudioDecodeWork::run, work);
}

void MediaPlayerAudioLinux::didDecodeAudio(AudioBufferData* pcm)
{
    if (!pcm) {
        notifyMediaSourceFailure();
        return;
    }
    m_playbackState->setPCM(pcm);
    if (m_container->audioSourceNode()) {
        m_container->audioSourceNode()->setPlaybackState(m_playbackState);
    }
    MediaPlayerAudio::onAudioDownloadCompleted();
}

double MediaPlayerAudioLinux::duration()
{
    return m_playbackState->duration();
}

MediaPlayerAudio* MediaPlayerAudio::create(HTMLMediaElement* element)
{
    return new MediaPlayerAudioLinux(element);
}

MediaPlayerAudio* MediaPlayerAudio::create(AudioNode* element)
{
    return new MediaPlayerAudioLinux(element);
}
} // namespace Starfish

#endif
#endif
