/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishPannerHandler__
#define __StarfishPannerHandler__

#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"

#include <array>
#include <atomic>

namespace Starfish {

class PannerHandler final : public AudioHandler {
public:
    enum class DistanceModel { Linear, Inverse, Exponential };

    PannerHandler(double sampleRate,
                  const std::array<AudioParamTimeline*, 6>& source,
                  const std::array<AudioParamTimeline*, 9>& listener);

    void setDistanceModel(DistanceModel model)
    {
        m_distanceModel.store(model, std::memory_order_relaxed);
    }
    void setRefDistance(double value)
    {
        m_refDistance.store(value, std::memory_order_relaxed);
    }
    void setMaxDistance(double value)
    {
        m_maxDistance.store(value, std::memory_order_relaxed);
    }
    void setRolloffFactor(double value)
    {
        m_rolloffFactor.store(value, std::memory_order_relaxed);
    }
    void setConeInnerAngle(double value)
    {
        m_coneInnerAngle.store(value, std::memory_order_relaxed);
    }
    void setConeOuterAngle(double value)
    {
        m_coneOuterAngle.store(value, std::memory_order_relaxed);
    }
    void setConeOuterGain(double value)
    {
        m_coneOuterGain.store(value, std::memory_order_relaxed);
    }
    // Its own position/orientation timelines and the shared listener ones.
    size_t timelineCount() const override
    {
        return m_params.size();
    }
    AudioParamTimeline* timeline(size_t index) override
    {
        return m_params[index];
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    double m_sampleRate;
    std::array<AudioParamTimeline*, 15> m_params;
    std::atomic<DistanceModel> m_distanceModel{ DistanceModel::Inverse };
    std::atomic<double> m_refDistance{ 1 };
    std::atomic<double> m_maxDistance{ 10000 };
    std::atomic<double> m_rolloffFactor{ 1 };
    std::atomic<double> m_coneInnerAngle{ 360 };
    std::atomic<double> m_coneOuterAngle{ 360 };
    std::atomic<double> m_coneOuterGain{ 0 };
};

} // namespace Starfish
#endif
#endif
