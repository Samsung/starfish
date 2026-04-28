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
#ifndef __StarfishMediaCapabilities__
#define __StarfishMediaCapabilities__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class ExecutionContext;

// Enum types
enum class HdrMetadataType : uint8_t {
    SmpteSt2086,
    SmpteSt2094_10,
    SmpteSt2094_40
};

enum class ColorGamut : uint8_t { Srgb, P3, Rec2020 };

enum class TransferFunction : uint8_t { Srgb, Pq, Hlg };

enum class MediaDecodingType : uint8_t { File, MediaSource, Webrtc };

enum class MediaEncodingType : uint8_t { Record, Webrtc };

enum class MediaKeysRequirement : uint8_t { Required, Optional, NotAllowed };

// Dictionary structures
struct VideoConfiguration {
public:
    VideoConfiguration();

    void setContentType(String* contentType)
    {
        m_contentType = contentType;
    }
    String* contentType() const
    {
        return m_contentType;
    }

    void setWidth(uint32_t width)
    {
        m_width = width;
    }
    uint32_t width() const
    {
        return m_width;
    }

    void setHeight(uint32_t height)
    {
        m_height = height;
    }
    uint32_t height() const
    {
        return m_height;
    }

    void setBitrate(uint64_t bitrate)
    {
        m_bitrate = bitrate;
    }
    uint64_t bitrate() const
    {
        return m_bitrate;
    }

    void setFramerate(double framerate)
    {
        m_framerate = framerate;
    }
    double framerate() const
    {
        return m_framerate;
    }

    void setHasAlphaChannel(bool hasAlphaChannel)
    {
        m_hasAlphaChannel = hasAlphaChannel;
    }
    bool hasAlphaChannel() const
    {
        return m_hasAlphaChannel;
    }

    void setHdrMetadataType(String* hdrMetadataType);
    String* hdrMetadataType() const
    {
        return m_hdrMetadataType;
    }

    void setColorGamut(String* colorGamut);
    String* colorGamut() const
    {
        return m_colorGamut;
    }

    void setTransferFunction(String* transferFunction);
    String* transferFunction() const
    {
        return m_transferFunction;
    }

    void setScalabilityMode(String* scalabilityMode)
    {
        m_scalabilityMode = scalabilityMode;
    }
    String* scalabilityMode() const
    {
        return m_scalabilityMode;
    }

    void setSpatialScalability(bool spatialScalability)
    {
        m_spatialScalability = spatialScalability;
    }
    bool spatialScalability() const
    {
        return m_spatialScalability;
    }

private:
    String* m_contentType;
    uint32_t m_width;
    uint32_t m_height;
    uint64_t m_bitrate;
    double m_framerate;
    bool m_hasAlphaChannel;
    String* m_hdrMetadataType;
    String* m_colorGamut;
    String* m_transferFunction;
    String* m_scalabilityMode;
    bool m_spatialScalability;
};

struct AudioConfiguration {
public:
    AudioConfiguration();

    void setContentType(String* contentType)
    {
        m_contentType = contentType;
    }
    String* contentType() const
    {
        return m_contentType;
    }

    void setChannels(String* channels)
    {
        m_channels = channels;
    }
    String* channels() const
    {
        return m_channels;
    }

    void setBitrate(uint64_t bitrate)
    {
        m_bitrate = bitrate;
    }
    uint64_t bitrate() const
    {
        return m_bitrate;
    }

    void setSamplerate(uint32_t samplerate)
    {
        m_samplerate = samplerate;
    }
    uint32_t samplerate() const
    {
        return m_samplerate;
    }

    void setSpatialRendering(bool spatialRendering)
    {
        m_spatialRendering = spatialRendering;
    }
    bool spatialRendering() const
    {
        return m_spatialRendering;
    }

private:
    String* m_contentType;
    String* m_channels;
    uint64_t m_bitrate;
    uint32_t m_samplerate;
    bool m_spatialRendering;
};

struct MediaConfiguration {
public:
    MediaConfiguration();

    void setVideo(const VideoConfiguration& video)
    {
        m_video = video;
    }
    VideoConfiguration video() const
    {
        return m_video;
    }

    void setAudio(const AudioConfiguration& audio)
    {
        m_audio = audio;
    }
    AudioConfiguration audio() const
    {
        return m_audio;
    }

private:
    VideoConfiguration m_video;
    AudioConfiguration m_audio;
};

struct MediaDecodingConfiguration : public MediaConfiguration {
public:
    MediaDecodingConfiguration();

    void setType(String* type);
    String* type() const;

private:
    MediaDecodingType m_type;
};

struct MediaEncodingConfiguration : public MediaConfiguration {
public:
    MediaEncodingConfiguration();

    void setType(String* type);
    String* type() const;

private:
    MediaEncodingType m_type;
};

struct MediaCapabilitiesInfo {
public:
    MediaCapabilitiesInfo();

    void setSupported(bool supported)
    {
        m_supported = supported;
    }
    bool supported() const
    {
        return m_supported;
    }

    void setSmooth(bool smooth)
    {
        m_smooth = smooth;
    }
    bool smooth() const
    {
        return m_smooth;
    }

    void setPowerEfficient(bool powerEfficient)
    {
        m_powerEfficient = powerEfficient;
    }
    bool powerEfficient() const
    {
        return m_powerEfficient;
    }

private:
    bool m_supported;
    bool m_smooth;
    bool m_powerEfficient;
};

struct MediaCapabilitiesDecodingInfo : public MediaCapabilitiesInfo {
public:
    MediaCapabilitiesDecodingInfo();

    void setConfiguration(const MediaDecodingConfiguration& configuration)
    {
        m_configuration = configuration;
    }
    MediaDecodingConfiguration configuration() const
    {
        return m_configuration;
    }

private:
    MediaDecodingConfiguration m_configuration;
};

// EME (Encrypted Media Extensions) related structures
struct MediaKeySystemMediaCapability {
public:
    MediaKeySystemMediaCapability();

    void setContentType(String* contentType)
    {
        m_contentType = contentType;
    }
    String* contentType() const
    {
        return m_contentType;
    }

    void setEncryptionScheme(Optional<String*> encryptionScheme)
    {
        m_encryptionScheme = encryptionScheme;
    }
    Optional<String*> encryptionScheme() const
    {
        return m_encryptionScheme;
    }

    void setRobustness(String* robustness)
    {
        m_robustness = robustness;
    }
    String* robustness() const
    {
        return m_robustness;
    }

private:
    String* m_contentType;
    Optional<String*> m_encryptionScheme;
    String* m_robustness;
};

class MediaCapabilities : public ScriptWrappable {
public:
    MediaCapabilities(ExecutionContext* executionContext);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMediaCapabilities() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    Promise* decodingInfo(MediaDecodingConfiguration configuration);

private:
    ExecutionContext* m_executionContext;
};

} // namespace Starfish

#endif
#endif
