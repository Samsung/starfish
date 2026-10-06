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

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/webaudio/AudioBuffer.h"

#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/render/AudioBufferData.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "EscargotPublic.h"

namespace Starfish {
AudioBuffer::AudioBuffer(ExecutionContext* executionContext,
                         AudioBufferOptions options)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    if (!options.hasLength() || !options.hasSampleRate()) {
        throw new DOMException(
            executionContext, DOMException::SCRIPT_TYPE_ERR,
            "AudioBuffer length and sampleRate are required");
    }
    if (options.m_numberOfChannels < 1 || options.m_numberOfChannels > 32 ||
        options.m_length < 1 || !std::isfinite(options.m_sampleRate) ||
        options.m_sampleRate < 3000 || options.m_sampleRate > 768000) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unsupported AudioBuffer configuration");
    }

    m_numberOfChannels = options.m_numberOfChannels;
    m_length = options.m_length;
    m_sampleRate = options.m_sampleRate;
    m_duration = static_cast<double>(m_length) / m_sampleRate;
    m_data = AudioBufferData::create(m_numberOfChannels, m_length);
    if (!m_data) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unable to allocate AudioBuffer");
    }
    m_channelViews.resize(m_numberOfChannels);
    for (uint32_t i = 0; i < m_numberOfChannels; i++) {
        m_channelViews[i] = nullptr;
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((AudioBuffer*)obj)->~AudioBuffer(); },
        NULL, NULL, NULL);
}

AudioBuffer::AudioBuffer(ExecutionContext* executionContext,
                         AudioBufferData* data, double sampleRate)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_data(data)
{
    m_numberOfChannels = static_cast<uint32_t>(data->channels());
    m_length = static_cast<uint32_t>(data->frames());
    m_sampleRate = sampleRate;
    m_duration = static_cast<double>(m_length) / sampleRate;
    m_channelViews.resize(m_numberOfChannels);
    for (uint32_t i = 0; i < m_numberOfChannels; i++) {
        m_channelViews[i] = nullptr;
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((AudioBuffer*)obj)->~AudioBuffer(); },
        NULL, NULL, NULL);
}

AudioBuffer::~AudioBuffer()
{
    if (m_data) {
        m_data->release();
    }
}

ScriptBindingInstance* AudioBuffer::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

void AudioBuffer::acquireContents()
{
    // https://webaudio.github.io/web-audio-api/#acquire-the-content
    // The node keeps the old data; defer the spec's copied channel arrays
    // until script next asks to mutate them.
    for (auto& view : m_channelViews) {
        if (view) {
            view->buffer()->asArrayBufferObject()->detachArrayBuffer();
            view = nullptr;
        }
    }
    m_dataAcquired = true;
}

void AudioBuffer::ensureMutableData()
{
    if (!m_dataAcquired) {
        return;
    }
    // https://webaudio.github.io/web-audio-api/#acquire-the-content
    // Acquisition detached every channel view, so once each node that
    // acquired the contents has dropped them, nothing can observe a write in
    // place and the copy can be skipped.
    if (m_data->isOnlyReferencedByOwnerAndViews()) {
        m_dataAcquired = false;
        return;
    }
    AudioBufferData* copy =
        AudioBufferData::create(m_numberOfChannels, m_length);
    if (!copy) {
        throw new DOMException(m_executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unable to allocate AudioBuffer");
    }
    for (uint32_t channel = 0; channel < m_numberOfChannels; channel++) {
        memcpy(copy->channel(channel), m_data->channel(channel),
               m_length * sizeof(float));
    }
    m_data->release();
    m_data = copy;
    m_dataAcquired = false;
}

ScriptFloat32Array AudioBuffer::getChannelData(uint32_t channel)
{
    if (channel >= m_numberOfChannels) {
        throw new DOMException(m_executionContext, DOMException::INDEX_SIZE_ERR,
                               "Channel index exceeds AudioBuffer channels");
    }

    ScriptFloat32Array& cached = m_channelViews[channel];
    if (cached &&
        !cached->buffer()->asArrayBufferObject()->isDetachedBuffer()) {
        return cached;
    }

    ensureMutableData();

    m_data->retainForView();
    AudioBufferData* data = m_data;
    cached =
        Escargot::Evaluator::execute(
            scriptBindingInstance()->scriptContext(),
            [](Escargot::ExecutionStateRef* state, AudioBufferData* data,
               uint32_t channel, uint32_t length) -> Escargot::ValueRef* {
                auto* arrayBuffer =
                    Escargot::ArrayBufferObjectRef::create(state);
                auto* backingStore =
                    Escargot::BackingStoreRef::createNonSharedBackingStore(
                        data->channel(channel), length * sizeof(float),
                        [](void*, size_t, void* userData) {
                            static_cast<AudioBufferData*>(userData)
                                ->releaseForView();
                        },
                        data);
                arrayBuffer->attachBuffer(backingStore);
                auto* view = Escargot::Float32ArrayObjectRef::create(state);
                view->setBuffer(arrayBuffer, 0, length * sizeof(float), length);
                return view;
            },
            data, channel, m_length)
            .result->asFloat32ArrayObject();
    return cached;
}

void AudioBuffer::copyFromChannel(ScriptFloat32Array destination,
                                  uint32_t channelNumber, uint32_t bufferOffset)
{
    if (channelNumber >= m_numberOfChannels) {
        throw new DOMException(m_executionContext, DOMException::INDEX_SIZE_ERR,
                               "Channel index exceeds AudioBuffer channels");
    }
    if (bufferOffset >= m_length) {
        return;
    }

    size_t count =
        std::min<size_t>(destination->arrayLength(), m_length - bufferOffset);
    if (!count) {
        return;
    }
    float* target = reinterpret_cast<float*>(destination->rawBuffer() +
                                             destination->byteOffset());
    memmove(target, m_data->channel(channelNumber) + bufferOffset,
            count * sizeof(float));
}

void AudioBuffer::copyToChannel(ScriptFloat32Array source,
                                uint32_t channelNumber, uint32_t bufferOffset)
{
    if (channelNumber >= m_numberOfChannels) {
        throw new DOMException(m_executionContext, DOMException::INDEX_SIZE_ERR,
                               "Channel index exceeds AudioBuffer channels");
    }
    if (bufferOffset >= m_length) {
        return;
    }

    size_t count =
        std::min<size_t>(source->arrayLength(), m_length - bufferOffset);
    if (!count) {
        return;
    }
    ensureMutableData();
    const float* samples = reinterpret_cast<const float*>(source->rawBuffer() +
                                                          source->byteOffset());
    memmove(m_data->channel(channelNumber) + bufferOffset, samples,
            count * sizeof(float));
}
} // namespace Starfish

#endif
