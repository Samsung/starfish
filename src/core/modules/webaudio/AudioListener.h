/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioListener__
#define __StarfishAudioListener__

#include "binding/ScriptWrappable.h"

namespace Starfish {
class AudioParam;
class BaseAudioContext;
class ExecutionContext;

class AudioListener : public ScriptWrappable {
public:
    AudioListener(ExecutionContext* executionContext,
                  BaseAudioContext* context);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioListener)

    DEFINE_GETTER(AudioParam*, positionX)
    DEFINE_GETTER(AudioParam*, positionY)
    DEFINE_GETTER(AudioParam*, positionZ)
    DEFINE_GETTER(AudioParam*, forwardX)
    DEFINE_GETTER(AudioParam*, forwardY)
    DEFINE_GETTER(AudioParam*, forwardZ)
    DEFINE_GETTER(AudioParam*, upX)
    DEFINE_GETTER(AudioParam*, upY)
    DEFINE_GETTER(AudioParam*, upZ)

    void setPosition(double x, double y, double z);
    void setOrientation(double x, double y, double z, double xUp, double yUp,
                        double zUp);

private:
    ExecutionContext* m_executionContext;
    BaseAudioContext* m_context;
    AudioParam* m_positionX;
    AudioParam* m_positionY;
    AudioParam* m_positionZ;
    AudioParam* m_forwardX;
    AudioParam* m_forwardY;
    AudioParam* m_forwardZ;
    AudioParam* m_upX;
    AudioParam* m_upY;
    AudioParam* m_upZ;
};

} // namespace Starfish
#endif
#endif
