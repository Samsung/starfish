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
#include "Starfish.h"

#include "core/modules/webaudio/AudioWorkletGlobalScope.h"

#include "binding/ScriptBindingAudioWorkletInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePort.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/webaudio/AudioWorklet.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/page/WebBase.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/loader/ResourceURL.h"

#include <EscargotPublic.h>

namespace Starfish {

using namespace Escargot;

AudioWorkletGlobalScope::AudioWorkletGlobalScope(BaseAudioContext* context,
                                                 AudioWorklet* worklet)
    : WorkletGlobalScope()
    , m_context(context)
    , m_worklet(worklet)
    , m_scriptBindingInstance(nullptr)
    , m_executionContext(nullptr)
    , m_port(nullptr)
{
    // https://html.spec.whatwg.org/multipage/worklets.html#set-up-a-worklet-environment-settings-object
    // The owner document is the context's document; the realm shares its
    // Window's script engine and task queues, so unloading the document
    // drops the worklet's tasks too.
    ExecutionContext* owner = context->executionContext();
    Document* document = owner->document();
    Window* window = document->window();
    m_scriptBindingInstance = new ScriptBindingAudioWorkletInstance(
        window->webView()->scriptEngineInstance(), this);
    m_executionContext = new ExecutionContext(
        window, m_scriptBindingInstance, owner->documentURI(),
        owner->characterSet(), document, true);
    m_executionContext->setIsWorklet();
    m_executionContext->setBaseURL(owner->baseURL());
    m_port = new MessagePort(m_executionContext);
    m_scriptBindingInstance->initBinding(false);

    window->registerDisposer(this, [](void* holder) {
        static_cast<AudioWorkletGlobalScope*>(holder)->dispose();
    });
}

ScriptBindingInstance* AudioWorkletGlobalScope::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

// Unloading the owner document discards this realm's queued jobs, as it does
// for the Window's own realm.
void AudioWorkletGlobalScope::dispose()
{
    m_port->close();
    m_scriptBindingInstance->destroy();
}

bool AudioWorkletGlobalScope::hasProcessor(String* name) const
{
    for (const auto& entry : m_processorConstructors) {
        if (entry.first->equals(name)) {
            return true;
        }
    }
    return false;
}

// https://webaudio.github.io/web-audio-api/#dom-audioworkletglobalscope-registerprocessor
void AudioWorkletGlobalScope::addProcessor(
    AudioWorkletProcessorDefinition* definition, ScriptValue processorCtor)
{
    m_processorConstructors.push_back(
        std::make_pair(definition->name, processorCtor));
    // "queue a media element task to append the key-value pair name →
    // parameterDescriptorSequence to the node name to parameter descriptor
    // map": queued before addModule() settles, so the map is complete when
    // its promise resolves.
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* worklet, void* definition) {
            static_cast<AudioWorklet*>(worklet)->addDefinition(
                static_cast<AudioWorkletProcessorDefinition*>(definition));
        },
        m_worklet, definition);
}

uint64_t AudioWorkletGlobalScope::currentFrame() const
{
    return m_context->graph()->renderedFrames();
}

double AudioWorkletGlobalScope::currentTime() const
{
    return m_context->currentTime();
}

float AudioWorkletGlobalScope::sampleRate() const
{
    return m_context->sampleRate();
}

uint32_t AudioWorkletGlobalScope::renderQuantumSize() const
{
    return m_context->renderQuantumSize();
}

static ScriptValue createSyntaxError(ScriptBindingInstance* instance)
{
    return Evaluator::execute(instance->scriptContext(),
                              [](ExecutionStateRef* state) -> ValueRef* {
                                  return ErrorObjectRef::create(
                                      state, ErrorObjectRef::SyntaxError,
                                      StringRef::createFromASCII(
                                          "Invalid worklet module script"));
                              })
        .result;
}

// https://html.spec.whatwg.org/multipage/webappapis.html#run-a-module-script
bool AudioWorkletGlobalScope::runModuleScript(String* source, ResourceURL* url,
                                              ScriptValue& error)
{
    Optional<ScriptModule> module =
        initModule(m_scriptBindingInstance, source, url->urlString());
    if (!module) {
        // In the realm of the addModule() promise, where WPT (worklets/)
        // expects the parse error's SyntaxError.
        error = createSyntaxError(m_context->scriptBindingInstance());
        return false;
    }
    // shortcut: worklet modules with import declarations are rejected
    // (AbortError); upgrade: fetch the module graph like
    // WorkerScriptController::fetchModuleTree.
    if (!moduleRequests(module.value()).empty()) {
        error = (new DOMException(m_context->executionContext(),
                                  DOMException::ABORT_ERR,
                                  "Worklet module imports are not supported"))
                    ->scriptValue();
        return false;
    }
    executeModule(m_scriptBindingInstance, module.value());
    return true;
}
} // namespace Starfish

#endif
