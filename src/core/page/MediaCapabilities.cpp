/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "MediaCapabilities.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "platform/multimedia/StreamInfo.h"
#include "platform/multimedia/MediaPlayer.h"

namespace Starfish {

// VideoConfiguration implementation
VideoConfiguration::VideoConfiguration()
    : m_contentType(String::emptyString)
    , m_width(0)
    , m_height(0)
    , m_bitrate(0)
    , m_framerate(0)
    , m_hasAlphaChannel(false)
    , m_hdrMetadataType(String::emptyString)
    , m_colorGamut(String::emptyString)
    , m_transferFunction(String::emptyString)
    , m_scalabilityMode(String::emptyString)
    , m_spatialScalability(false)
{
}

void VideoConfiguration::setHdrMetadataType(String* hdrMetadataType)
{
    m_hdrMetadataType = hdrMetadataType;
}

void VideoConfiguration::setColorGamut(String* colorGamut)
{
    m_colorGamut = colorGamut;
}

void VideoConfiguration::setTransferFunction(String* transferFunction)
{
    m_transferFunction = transferFunction;
}

// AudioConfiguration implementation
AudioConfiguration::AudioConfiguration()
    : m_contentType(String::emptyString)
    , m_channels(String::emptyString)
    , m_bitrate(0)
    , m_samplerate(0)
    , m_spatialRendering(false)
{
}

// MediaConfiguration implementation
MediaConfiguration::MediaConfiguration()
    : m_video()
    , m_audio()
{
}

// MediaDecodingConfiguration implementation
MediaDecodingConfiguration::MediaDecodingConfiguration()
    : MediaConfiguration()
    , m_type(MediaDecodingType::File)
{
}

void MediaDecodingConfiguration::setType(String* type)
{
    if (type->equals("file")) {
        m_type = MediaDecodingType::File;
    } else if (type->equals("media-source")) {
        m_type = MediaDecodingType::MediaSource;
    } else if (type->equals("webrtc")) {
        m_type = MediaDecodingType::Webrtc;
    }
}

String* MediaDecodingConfiguration::type() const
{
    switch (m_type) {
    case MediaDecodingType::File:
        return String::createASCIIString("file");
    case MediaDecodingType::MediaSource:
        return String::createASCIIString("media-source");
    case MediaDecodingType::Webrtc:
        return String::createASCIIString("webrtc");
    }
    return String::emptyString;
}

// MediaEncodingConfiguration implementation
MediaEncodingConfiguration::MediaEncodingConfiguration()
    : MediaConfiguration()
    , m_type(MediaEncodingType::Record)
{
}

void MediaEncodingConfiguration::setType(String* type)
{
    if (type->equals("record")) {
        m_type = MediaEncodingType::Record;
    } else if (type->equals("webrtc")) {
        m_type = MediaEncodingType::Webrtc;
    }
}

String* MediaEncodingConfiguration::type() const
{
    switch (m_type) {
    case MediaEncodingType::Record:
        return String::createASCIIString("record");
    case MediaEncodingType::Webrtc:
        return String::createASCIIString("webrtc");
    }
    return String::emptyString;
}

// MediaCapabilitiesInfo implementation
MediaCapabilitiesInfo::MediaCapabilitiesInfo()
    : m_supported(false)
    , m_smooth(false)
    , m_powerEfficient(false)
{
}

// MediaCapabilitiesDecodingInfo implementation
MediaCapabilitiesDecodingInfo::MediaCapabilitiesDecodingInfo()
    : MediaCapabilitiesInfo()
    , m_configuration()
{
}

// MediaKeySystemMediaCapability implementation
MediaKeySystemMediaCapability::MediaKeySystemMediaCapability()
    : m_contentType(String::emptyString)
    , m_robustness(String::emptyString)
{
}

// MediaCapabilities implementation
MediaCapabilities::MediaCapabilities(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
}

ScriptBindingInstance* MediaCapabilities::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

Promise* MediaCapabilities::decodingInfo(
    MediaDecodingConfiguration configuration)
{
    Promise* promise = new Promise(scriptBindingInstance());

    // For now, we return a basic MediaCapabilitiesInfo with default values
    bool isVideoSupported = true;
    bool isAudioSupported = true;

    if (configuration.video().contentType()->length()) {
        auto type =
            suggestVideoCodecFromString(configuration.video().contentType());
        isVideoSupported = MediaPlayer::isSupport(type);
    }
    if (configuration.audio().contentType()->length()) {
        auto type =
            suggestAudioCodecFromString(configuration.audio().contentType());
        isAudioSupported = MediaPlayer::isSupport(type);
    }

    bool isSupported = isAudioSupported && isVideoSupported;

#if !defined(STARFISH_ENABLE_MSE_WEBM)
    if (configuration.video().contentType()->contains("webm") ||
        configuration.audio().contentType()->contains("webm")) {
        isSupported = false;
    }
#endif

    STARFISH_LOG_INFO(
        "MediaCapabilities::decodingInfo -> isSupported(%d<-v%d,a%d) "
        "%s %s",
        (int)isSupported, (int)isVideoSupported, (int)isAudioSupported,
        configuration.video().contentType()->toUTF8NonGCString().data(),
        configuration.audio().contentType()->toUTF8NonGCString().data());

    ScriptObject result = createEmptyScriptObject(scriptBindingInstance());
    setScriptObjectPropertyThrowsException(scriptBindingInstance(), result,
                                           createScriptASCIIString("supported"),
                                           createScriptValue(isSupported));
    setScriptObjectPropertyThrowsException(scriptBindingInstance(), result,
                                           createScriptASCIIString("smooth"),
                                           createScriptValue(isSupported));
    setScriptObjectPropertyThrowsException(
        scriptBindingInstance(), result,
        createScriptASCIIString("powerEfficient"),
        createScriptValue(isSupported));

    promise->fulfill(createScriptValue(result));

    return promise;
}

} // namespace Starfish

#endif
