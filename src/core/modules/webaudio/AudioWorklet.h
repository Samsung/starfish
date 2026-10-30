/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishAudioWorklet__
#define __StarfishAudioWorklet__

#include "core/modules/worklet/Worklet.h"

namespace Starfish {
class AudioWorkletGlobalScope;
struct AudioWorkletProcessorDefinition;
class BaseAudioContext;
class MessagePort;
class ResourceURL;

// https://webaudio.github.io/web-audio-api/#AudioWorklet
// Created on the first audioWorklet access; its global scope (a realm) only
// on the first addModule() or port access.
class AudioWorklet : public Worklet {
public:
    explicit AudioWorklet(BaseAudioContext* context);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioWorklet)

    Promise* addModule(String* moduleURL, WorkletOptions options) override;
    MessagePort* port();

    BaseAudioContext* context() const
    {
        return m_context;
    }
    AudioWorkletGlobalScope* globalScope();
    // https://webaudio.github.io/web-audio-api/#dom-audioworklet-node-name-to-parameter-descriptor-map
    Optional<AudioWorkletProcessorDefinition*> definition(String* name) const;
    void addDefinition(AudioWorkletProcessorDefinition* definition);

private:
    void runModule(ResourceURL* url, Promise* promise, Optional<String*> text);
    void queueSettle(Promise* promise, Optional<ScriptValue> error);

    BaseAudioContext* m_context;
    Optional<AudioWorkletGlobalScope*> m_globalScope;
    Optional<MessagePort*> m_port;
    GCVector<AudioWorkletProcessorDefinition*> m_definitions;
    // URLs whose module script ran in the global scope: adding one again
    // resolves without running it twice, as the module map does.
    GCVector<ResourceURL*> m_moduleURLs;

    friend class AudioWorkletModuleFetchClient;
};
} // namespace Starfish

#endif
#endif
