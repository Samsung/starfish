/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishPannerNode__
#define __StarfishPannerNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioParam;
class PannerHandler;

struct PannerOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(String*, panningModel, PanningModel)
    DEFINE_GETTER_SETTER(String*, distanceModel, DistanceModel)
    DEFINE_GETTER_SETTER(float, positionX, PositionX)
    DEFINE_GETTER_SETTER(float, positionY, PositionY)
    DEFINE_GETTER_SETTER(float, positionZ, PositionZ)
    DEFINE_GETTER_SETTER(float, orientationX, OrientationX)
    DEFINE_GETTER_SETTER(float, orientationY, OrientationY)
    DEFINE_GETTER_SETTER(float, orientationZ, OrientationZ)
    DEFINE_GETTER_SETTER(double, refDistance, RefDistance)
    DEFINE_GETTER_SETTER(double, maxDistance, MaxDistance)
    DEFINE_GETTER_SETTER(double, rolloffFactor, RolloffFactor)
    DEFINE_GETTER_SETTER(double, coneInnerAngle, ConeInnerAngle)
    DEFINE_GETTER_SETTER(double, coneOuterAngle, ConeOuterAngle)
    DEFINE_GETTER_SETTER(double, coneOuterGain, ConeOuterGain)

    String* m_panningModel{ String::createASCIIString("equalpower") };
    String* m_distanceModel{ String::createASCIIString("inverse") };
    float m_positionX{ 0 };
    float m_positionY{ 0 };
    float m_positionZ{ 0 };
    float m_orientationX{ 1 };
    float m_orientationY{ 0 };
    float m_orientationZ{ 0 };
    double m_refDistance{ 1 };
    double m_maxDistance{ 10000 };
    double m_rolloffFactor{ 1 };
    double m_coneInnerAngle{ 360 };
    double m_coneOuterAngle{ 360 };
    double m_coneOuterGain{ 0 };
};

class PannerNode : public AudioNode {
public:
    PannerNode(ExecutionContext* executionContext, BaseAudioContext* context,
               PannerOptions options = PannerOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(PannerNode)
    DEFINE_GETTER(String*, panningModel)
    void setPanningModel(String* model);
    DEFINE_GETTER(String*, distanceModel)
    void setDistanceModel(String* model);
    DEFINE_GETTER(AudioParam*, positionX)
    DEFINE_GETTER(AudioParam*, positionY)
    DEFINE_GETTER(AudioParam*, positionZ)
    DEFINE_GETTER(AudioParam*, orientationX)
    DEFINE_GETTER(AudioParam*, orientationY)
    DEFINE_GETTER(AudioParam*, orientationZ)
    DEFINE_GETTER(double, refDistance)
    void setRefDistance(double value);
    DEFINE_GETTER(double, maxDistance)
    void setMaxDistance(double value);
    DEFINE_GETTER(double, rolloffFactor)
    void setRolloffFactor(double value);
    DEFINE_GETTER(double, coneInnerAngle)
    void setConeInnerAngle(double value);
    DEFINE_GETTER(double, coneOuterAngle)
    void setConeOuterAngle(double value);
    DEFINE_GETTER(double, coneOuterGain)
    void setConeOuterGain(double value);
    void setPosition(double x, double y, double z);
    void setOrientation(double x, double y, double z);
    void setChannelCount(uint32_t value) override;
    void setChannelCountModeStr(String* mode) override;

private:
    PannerHandler* m_pannerHandler{ nullptr };
    String* m_panningModel{ nullptr };
    String* m_distanceModel{ nullptr };
    AudioParam* m_positionX{ nullptr };
    AudioParam* m_positionY{ nullptr };
    AudioParam* m_positionZ{ nullptr };
    AudioParam* m_orientationX{ nullptr };
    AudioParam* m_orientationY{ nullptr };
    AudioParam* m_orientationZ{ nullptr };
    double m_refDistance{ 1 };
    double m_maxDistance{ 10000 };
    double m_rolloffFactor{ 1 };
    double m_coneInnerAngle{ 360 };
    double m_coneOuterAngle{ 360 };
    double m_coneOuterGain{ 0 };
};

} // namespace Starfish
#endif
#endif
