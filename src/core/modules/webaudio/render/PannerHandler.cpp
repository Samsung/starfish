/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/PannerHandler.h"

#include <algorithm>
#include <cmath>

namespace Starfish {
namespace {
    constexpr double Pi = 3.14159265358979323846;

    struct Vec3 {
        double x;
        double y;
        double z;

        Vec3 operator-(const Vec3& other) const
        {
            return { x - other.x, y - other.y, z - other.z };
        }
        double dot(const Vec3& other) const
        {
            return x * other.x + y * other.y + z * other.z;
        }
        Vec3 cross(const Vec3& other) const
        {
            return { y * other.z - z * other.y, z * other.x - x * other.z,
                     x * other.y - y * other.x };
        }
        double length() const
        {
            return std::sqrt(dot(*this));
        }
        Vec3 normalized() const
        {
            const double size = length();
            return size > 0 ? Vec3{ x / size, y / size, z / size }
                            : Vec3{ 0, 0, 0 };
        }
    };

    double distanceGain(double distance, PannerHandler::DistanceModel model,
                        double reference, double maximum, double rolloff)
    {
        // https://webaudio.github.io/web-audio-api/#enumdef-distancemodeltype
        if (model == PannerHandler::DistanceModel::Linear) {
            const double lower = std::min(reference, maximum);
            const double upper = std::max(reference, maximum);
            return lower == upper
                       ? 1 - std::min(rolloff, 1.0)
                       : 1 - std::min(rolloff, 1.0) *
                                 (std::max(lower, std::min(distance, upper)) -
                                  lower) /
                                 (upper - lower);
        }
        if (reference == 0) {
            return 0;
        }
        const double clamped = std::max(distance, reference);
        if (model == PannerHandler::DistanceModel::Inverse) {
            return reference / (reference + rolloff * (clamped - reference));
        }
        return std::pow(clamped / reference, -rolloff);
    }

    double coneGain(const Vec3& source, const Vec3& listener,
                    const Vec3& orientation, double inner, double outer,
                    double outerGain)
    {
        // https://webaudio.github.io/web-audio-api/#sound-cones
        if (orientation.length() == 0 || (inner == 360 && outer == 360)) {
            return 1;
        }
        const Vec3 toListener = (listener - source).normalized();
        const double cosine = std::max(
            -1.0, std::min(1.0, toListener.dot(orientation.normalized())));
        const double angle = std::acos(cosine) * 180 / Pi;
        const double innerHalf = std::abs(inner) / 2;
        const double outerHalf = std::abs(outer) / 2;
        if (angle <= innerHalf) {
            return 1;
        }
        if (angle >= outerHalf) {
            return outerGain;
        }
        const double portion = (angle - innerHalf) / (outerHalf - innerHalf);
        return 1 - portion + outerGain * portion;
    }

    double azimuth(const Vec3& source, const Vec3& listener,
                   const Vec3& forward, const Vec3& up)
    {
        // https://webaudio.github.io/web-audio-api/#azimuth-and-elevation
        const Vec3 direction = (source - listener).normalized();
        const Vec3 right = forward.cross(up).normalized();
        if (direction.length() == 0 || right.length() == 0 ||
            forward.length() == 0) {
            return 0;
        }
        const Vec3 forwardUnit = forward.normalized();
        return std::atan2(direction.dot(right), direction.dot(forwardUnit)) *
               180 / Pi;
    }
} // namespace

PannerHandler::PannerHandler(double sampleRate,
                             const std::array<AudioParamTimeline*, 6>& source,
                             const std::array<AudioParamTimeline*, 9>& listener)
    : AudioHandler(1, 1, 2)
    , m_sampleRate(sampleRate)
{
    std::copy(source.begin(), source.end(), m_params.begin());
    std::copy(listener.begin(), listener.end(), m_params.begin() + 6);
    input(0).configure(2, AudioInputChannelMode::ClampedMax, true);
}

void PannerHandler::process(uint64_t frameStart, size_t frames)
{
    const AudioBus& sourceBus = input(0).bus();
    if (sourceBus.isSilent()) {
        return;
    }
    AudioBus& target = output(0).bus();
    std::array<const AudioBus*, 15> modulation;
    for (size_t index = 0; index < m_params.size(); index++) {
        modulation[index] = m_params[index]->modulation(frames);
    }
    const auto value = [&](size_t index, size_t frame) {
        const size_t sample = m_params[index]->isARate() ? frame : 0;
        const double time =
            static_cast<double>(frameStart + sample) / m_sampleRate;
        const float mod =
            modulation[index] ? modulation[index]->channel(0)[sample] : 0;
        return static_cast<double>(m_params[index]->computedValueAt(time, mod));
    };
    const DistanceModel model = m_distanceModel.load(std::memory_order_relaxed);
    const double reference = m_refDistance.load(std::memory_order_relaxed);
    const double maximum = m_maxDistance.load(std::memory_order_relaxed);
    const double rolloff = m_rolloffFactor.load(std::memory_order_relaxed);
    const double inner = m_coneInnerAngle.load(std::memory_order_relaxed);
    const double outer = m_coneOuterAngle.load(std::memory_order_relaxed);
    const double outerGain = m_coneOuterGain.load(std::memory_order_relaxed);
    for (size_t frame = 0; frame < frames; frame++) {
        const Vec3 source{ value(0, frame), value(1, frame), value(2, frame) };
        const Vec3 orientation{ value(3, frame), value(4, frame),
                                value(5, frame) };
        const Vec3 listener{ value(6, frame), value(7, frame),
                             value(8, frame) };
        const Vec3 forward{ value(9, frame), value(10, frame),
                            value(11, frame) };
        const Vec3 up{ value(12, frame), value(13, frame), value(14, frame) };
        double angle = std::max(
            -180.0, std::min(180.0, azimuth(source, listener, forward, up)));
        if (angle < -90) {
            angle = -180 - angle;
        } else if (angle > 90) {
            angle = 180 - angle;
        }
        const bool mono = sourceBus.channels() == 1;
        const double x = mono ? (angle + 90) / 180
                              : (angle <= 0 ? (angle + 90) / 90 : angle / 90);
        const double leftGain = std::cos(x * Pi / 2);
        const double rightGain = std::sin(x * Pi / 2);
        const float distance = static_cast<float>(distanceGain(
            (source - listener).length(), model, reference, maximum, rolloff));
        const float cone = static_cast<float>(
            coneGain(source, listener, orientation, inner, outer, outerGain));
        const float left = sourceBus.channel(0)[frame];
        double outputLeft;
        double outputRight;
        if (mono) {
            outputLeft = left * leftGain;
            outputRight = left * rightGain;
        } else {
            const float right = sourceBus.channel(1)[frame];
            if (angle <= 0) {
                outputLeft = left + right * leftGain;
                outputRight = right * rightGain;
            } else {
                outputLeft = left * leftGain;
                outputRight = right + left * rightGain;
            }
        }
        // The equal-power stage produces float PCM before distance and cone
        // gain are applied; keep that rounding order for small gains.
        target.channel(0)[frame] =
            static_cast<float>(outputLeft) * distance * cone;
        target.channel(1)[frame] =
            static_cast<float>(outputRight) * distance * cone;
    }
    target.setSilent(false);
}

} // namespace Starfish
#endif
