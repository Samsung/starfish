/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)
#ifndef __StarfishConvolverNode__
#define __StarfishConvolverNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioBuffer;
class ConvolverHandler;

struct ConvolverOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(Optional<AudioBuffer*>, buffer, Buffer)
    DEFINE_GETTER_SETTER(bool, disableNormalization, DisableNormalization)

    Optional<AudioBuffer*> m_buffer;
    bool m_disableNormalization{ false };
};

class ConvolverNode final : public AudioNode {
public:
    ConvolverNode(ExecutionContext* executionContext, BaseAudioContext* context,
                  ConvolverOptions options = ConvolverOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ConvolverNode)
    DEFINE_GETTER(Optional<AudioBuffer*>, buffer)
    void setBuffer(Optional<AudioBuffer*> buffer);
    DEFINE_GETTER_SETTER(bool, normalize, Normalize)
    void setChannelCount(uint32_t value) override;
    void setChannelCountModeStr(String* mode) override;
    void setChannelInterpretationStr(String* interpretation) override;

private:
    Optional<AudioBuffer*> m_buffer;
    bool m_normalize{ true };
    ConvolverHandler* m_convolverHandler{ nullptr };
};
} // namespace Starfish
#endif
#endif
