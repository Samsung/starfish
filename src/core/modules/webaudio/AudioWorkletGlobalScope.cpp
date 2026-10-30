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
#include "binding/ScriptEngineInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ErrorEvent.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePort.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/webaudio/AudioWorklet.h"
#include "core/modules/webaudio/AudioWorkletNode.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/page/WebBase.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "platform/loader/ResourceURL.h"

#include "core/serialize/Serializer.h"

#include <EscargotPublic.h>

#include <algorithm>

namespace Starfish {

using namespace Escargot;

AudioWorkletProcessorRecord::AudioWorkletProcessorRecord(
    AudioWorkletNode* node, AudioWorkletProcessorDefinition* definition,
    MessagePort* port, SerializeWithTransferResult* options)
    : definition(definition)
    , port(port)
    , options(options)
    , m_nodeLink(static_cast<void**>(GC_MALLOC_ATOMIC(sizeof(void*))))
{
    *m_nodeLink = reinterpret_cast<void*>(GC_HIDE_POINTER(node));
    GC_GENERAL_REGISTER_DISAPPEARING_LINK(m_nodeLink, node);
}

Optional<AudioWorkletNode*> AudioWorkletProcessorRecord::node() const
{
    if (!*m_nodeLink) {
        return nullptr;
    }
    return static_cast<AudioWorkletNode*>(
        GC_REVEAL_POINTER(reinterpret_cast<GC_hidden_pointer>(*m_nodeLink)));
}

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

Optional<MessagePort*> AudioWorkletGlobalScope::takePendingPort()
{
    Optional<MessagePort*> port = m_pendingPort;
    m_pendingPort = nullptr;
    return port;
}

void AudioWorkletGlobalScope::sweepRecords()
{
    std::vector<void*> released;
    {
        AudioGraphLock graphLock(m_context->graph());
        m_context->graph()->takeReleasedWorkletRecords(released);
    }
    for (void* record : released) {
        m_records.erase(
            std::remove(m_records.begin(), m_records.end(),
                        static_cast<AudioWorkletProcessorRecord*>(record)),
            m_records.end());
    }
}

AudioWorkletProcessorRecord* AudioWorkletGlobalScope::createProcessor(
    AudioWorkletNode* node, AudioWorkletProcessorDefinition* definition,
    MessagePort* port, SerializeWithTransferResult* options)
{
    sweepRecords();
    auto* record =
        new AudioWorkletProcessorRecord(node, definition, port, options);
    m_records.push_back(record);
    // The ED queues a control message for the rendering thread; with the
    // realm on the main thread a task constructs the processor even while
    // the context is not rendering, which the port tests rely on.
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* scope, void* record) {
            static_cast<AudioWorkletGlobalScope*>(scope)->constructProcessor(
                static_cast<AudioWorkletProcessorRecord*>(record));
        },
        this, record);
    return record;
}

// https://webaudio.github.io/web-audio-api/#instantiation-of-AudioWorkletProcessor
void AudioWorkletGlobalScope::constructProcessor(
    AudioWorkletProcessorRecord* record)
{
    ScriptValue processorCtor = nullptr;
    for (const auto& entry : m_processorConstructors) {
        if (entry.first->equals(record->definition->name)) {
            processorCtor = entry.second;
        }
    }
    SerializeWithTransferResult* options = record->options;
    record->options = nullptr;
    DeserializeWithTransferResult deserialized;
    try {
        // StructuredDeserialize in this realm: an object of an interface
        // not exposed here fails with DataCloneError.
        Serializer::deserializeWithTransfer(m_executionContext, *options,
                                            deserialized);
    } catch (DOMException* exception) {
        delete options;
        queueProcessorError(record, exception->message(), String::emptyString,
                            0, 0);
        return;
    }
    delete options;

    MicroTaskExecutionManager microtasks(
        m_scriptBindingInstance->engineInstance());
    m_pendingPort = record->port;
    auto result = Evaluator::execute(
        m_scriptBindingInstance->scriptContext(),
        [](ExecutionStateRef* state, ValueRef* processorCtor,
           ValueRef* options) -> ValueRef* {
            return processorCtor->asObject()->construct(state, 1, &options);
        },
        processorCtor, deserialized.m_deserialized);
    m_pendingPort = nullptr;
    if (result.error.hasValue()) {
        String* message =
            toBrowserString(m_scriptBindingInstance, result.error.value());
        STARFISH_LOG_ERROR("AudioWorkletProcessor constructor: %s",
                           message->toUTF8NonGCString().data());
        String* filename = String::emptyString;
        uint32_t lineno = 0;
        uint32_t colno = 0;
        if (result.stackTrace.size()) {
            const auto& frame = result.stackTrace[result.stackTrace.size() - 1];
            filename = toBrowserString(m_scriptBindingInstance,
                                       ValueRef::create(frame.srcName));
            lineno = frame.loc.line;
            colno = frame.loc.column;
        }
        queueProcessorError(record, message, filename, lineno, colno);
        return;
    }
    record->processor = result.result->asObject();
}

struct AudioWorkletProcessorError : public gc {
    AudioWorkletProcessorRecord* record;
    String* message;
    String* filename;
    uint32_t lineno;
    uint32_t colno;
};

// https://webaudio.github.io/web-audio-api/#dom-audioworkletnode-onprocessorerror
// A task fires processorerror at the node, if it is still alive, with the
// error's position but not the error object, which belongs to this realm.
void AudioWorkletGlobalScope::queueProcessorError(
    AudioWorkletProcessorRecord* record, String* message, String* filename,
    uint32_t lineno, uint32_t colno)
{
    auto* error = new AudioWorkletProcessorError;
    error->record = record;
    error->message = message;
    error->filename = filename;
    error->lineno = lineno;
    error->colno = colno;
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* data) {
            auto* error = static_cast<AudioWorkletProcessorError*>(data);
            Optional<AudioWorkletNode*> node = error->record->node();
            if (!node) {
                return;
            }
            ErrorEventInit init;
            init.setMessage(error->message);
            init.setFilename(error->filename);
            init.setLineno(error->lineno);
            init.setColno(error->colno);
            AudioWorkletNode* target = node.value();
            target->dispatchEventByUA(new ErrorEvent(
                target->executionContext(),
                target->staticStrings()->m_processorerror.localName(), init));
        },
        error);
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
