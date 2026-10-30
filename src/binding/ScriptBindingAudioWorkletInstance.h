/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishScriptBindingAudioWorkletInstance__
#define __StarfishScriptBindingAudioWorkletInstance__

#include "binding/ScriptBindingInstance.h"

namespace Starfish {
class AudioWorkletGlobalScope;

// The realm of an AudioWorkletGlobalScope: a separate Escargot context on
// the main thread, sharing the Window's VM (like an iframe's realm).
class ScriptBindingAudioWorkletInstance final : public ScriptBindingInstance {
public:
    ScriptBindingAudioWorkletInstance(ScriptEngineInstance* engineInstance,
                                      AudioWorkletGlobalScope* globalScope);

    Window* ownerWindow() override;
    Document* ownerDocument() override;
    bool isScriptingEnabled() override
    {
        return true;
    }
    void dispatchErrorEventToGlobalScope(ErrorEventInit& errorInfo) override;

private:
    void initJavaScriptBinding(Escargot::ContextRef* context,
                               Escargot::ExecutionStateRef* state) override;

    AudioWorkletGlobalScope* m_globalScope;
};
} // namespace Starfish

#endif
#endif
