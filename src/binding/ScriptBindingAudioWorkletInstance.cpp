/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#include "StarfishConfig.h"
#include "binding/ScriptBindingAudioWorkletInstance.h"

#include "binding/ScriptWrappable.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioWorkletGlobalScope.h"

#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

// Interfaces with Exposed=* or listing AudioWorklet that Starfish implements;
// the generator puts them in the COMMON group, which also holds interfaces
// that are not exposed to worklets.
#define FOR_EACH_AUDIO_WORKLET_COMMON_INTERFACE(F) \
    F(DOMException)                                \
    F(Event)                                       \
    F(EventTarget)                                 \
    F(MessageEvent)                                \
    F(MessagePort)

ScriptBindingAudioWorkletInstance::ScriptBindingAudioWorkletInstance(
    ScriptEngineInstance* engineInstance, AudioWorkletGlobalScope* globalScope)
    : ScriptBindingInstance(engineInstance)
    , m_globalScope(globalScope)
{
}

void ScriptBindingAudioWorkletInstance::initJavaScriptBinding(
    ContextRef* context, ExecutionStateRef* state)
{
    ScriptBindingInstance::initJavaScriptBinding(context, state);

    GlobalObjectRef* globalObject = context->globalObject();
#define DECLARE_NAME_FOR_BINDING(exportName)                          \
    defineGlobalBindingNameAccessor(                                  \
        state, globalObject, StringRef::createFromASCII(#exportName), \
        std::mem_fn(&ScriptBindingInstance::value##exportName),       \
        std::mem_fn(&ScriptBindingInstance::setValue##exportName));
    FOR_EACH_AUDIO_WORKLET_COMMON_INTERFACE(DECLARE_NAME_FOR_BINDING)
    STARFISH_BINDING_GROUPS_AUDIOWORKLET(DECLARE_NAME_FOR_BINDING)
#undef DECLARE_NAME_FOR_BINDING

    m_globalScope->init(this, m_globalScope);
}

Window* ScriptBindingAudioWorkletInstance::ownerWindow()
{
    return ownerDocument()->window();
}

Document* ScriptBindingAudioWorkletInstance::ownerDocument()
{
    return m_globalScope->executionContext()->document();
}

// https://html.spec.whatwg.org/multipage/webappapis.html#report-an-exception
// A WorkletGlobalScope has no error event handler; the caller already logs
// the exception to the console.
void ScriptBindingAudioWorkletInstance::dispatchErrorEventToGlobalScope(
    ErrorEventInit&)
{
}
} // namespace Starfish

#endif
