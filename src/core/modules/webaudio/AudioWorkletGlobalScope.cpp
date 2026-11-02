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
#include "core/modules/webaudio/render/AudioNodeInput.h"
#include "core/modules/webaudio/render/AudioNodeOutput.h"
#include "core/modules/webaudio/render/AudioWorkletHandler.h"
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
    AudioWorkletGlobalScope* scope, AudioWorkletNode* node,
    AudioWorkletProcessorDefinition* definition, MessagePort* port,
    SerializeWithTransferResult* options)
    : scope(scope)
    , definition(definition)
    , port(port)
    , options(options)
    , m_nodeLink(static_cast<void**>(GC_MALLOC_ATOMIC(sizeof(void*))))
{
    *m_nodeLink = reinterpret_cast<void*>(GC_HIDE_POINTER(node));
    GC_GENERAL_REGISTER_DISAPPEARING_LINK(m_nodeLink, node);
    m_retainedNode = node;
}

void AudioWorkletProcessorRecord::retainNode(bool retain)
{
    m_retainedNode = retain ? node() : nullptr;
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
    m_disposed = true;
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
        new AudioWorkletProcessorRecord(this, node, definition, port, options);
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

// An exception from the processor's constructor or process(): logged, and
// fired as processorerror with the position of the frame that threw.
static void errorPosition(ScriptBindingInstance* instance,
                          Evaluator::EvaluatorResult& result, String*& message,
                          String*& filename, uint32_t& lineno, uint32_t& colno)
{
    message = toBrowserString(instance, result.error.value());
    STARFISH_LOG_ERROR("AudioWorkletProcessor: %s",
                       message->toUTF8NonGCString().data());
    filename = String::emptyString;
    lineno = 0;
    colno = 0;
    if (result.stackTrace.size()) {
        const auto& frame = result.stackTrace[result.stackTrace.size() - 1];
        filename = toBrowserString(instance, ValueRef::create(frame.srcName));
        lineno = frame.loc.line;
        colno = frame.loc.column;
    }
}

// https://webaudio.github.io/web-audio-api/#instantiation-of-AudioWorkletProcessor
void AudioWorkletGlobalScope::constructProcessor(
    AudioWorkletProcessorRecord* record)
{
    // https://html.spec.whatwg.org/multipage/document-lifecycle.html#destroy-a-document
    // The worklet's global scopes go with their document: a node constructed
    // later gets no processor and outputs silence.
    if (m_disposed) {
        record->constructionFailed = true;
        return;
    }
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
        record->constructionFailed = true;
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
        record->constructionFailed = true;
        String* message;
        String* filename;
        uint32_t lineno;
        uint32_t colno;
        errorPosition(m_scriptBindingInstance, result, message, filename,
                      lineno, colno);
        queueProcessorError(record, message, filename, lineno, colno);
        return;
    }
    record->processor = result.result->asObject();
}

struct AudioWorkletProcessCall {
    AudioWorkletProcessorRecord* record;
    AudioWorkletHandler* handler;
    size_t quantum;
    bool rebuild;
};

static ValueRef* createFloat32Array(ExecutionStateRef* state, size_t length)
{
    auto* buffer = ArrayBufferObjectRef::create(state);
    buffer->allocateBuffer(state, length * sizeof(float));
    auto* view = Float32ArrayObjectRef::create(state);
    view->setBuffer(buffer, 0, length * sizeof(float), length);
    return view;
}

static float* floatData(ScriptValue array)
{
    return reinterpret_cast<float*>(array->asArrayBufferView()->rawBuffer());
}

// "New arrays are also reallocated if any part of the inputs or outputs
// arrays are transferred."
static bool anyDetached(const GCVector<ScriptValue>& arrays)
{
    for (ScriptValue array : arrays) {
        if (!array->asArrayBufferView()->byteLength()) {
            return true;
        }
    }
    return false;
}

// Builds the frozen inputs/outputs arrays and parameters object of
// |call->record| for its current shape.
static void buildProcessArguments(ExecutionStateRef* state,
                                  AudioWorkletProcessCall* call)
{
    AudioWorkletProcessorRecord* record = call->record;
    AudioWorkletHandler& handler = *call->handler;
    const GCAtomicVector<uint32_t>& shape = record->shape;
    record->arrays.clear();
    size_t next = 0;
    auto channels = [&](size_t count) {
        ArrayObjectRef* list = ArrayObjectRef::create(state);
        for (size_t channel = 0; channel < count; channel++) {
            ValueRef* array = createFloat32Array(state, call->quantum);
            record->arrays.push_back(array);
            list->defineDataProperty(state, ValueRef::create(channel), array,
                                     true, true, true);
        }
        list->setIntegrityLevel(state, false);
        return list;
    };
    ArrayObjectRef* inputs = ArrayObjectRef::create(state);
    for (size_t index = 0; index < handler.inputCount(); index++) {
        inputs->defineDataProperty(state, ValueRef::create(index),
                                   channels(shape[next++]), true, true, true);
    }
    inputs->setIntegrityLevel(state, false);
    ArrayObjectRef* outputs = ArrayObjectRef::create(state);
    for (size_t index = 0; index < handler.outputCount(); index++) {
        outputs->defineDataProperty(state, ValueRef::create(index),
                                    channels(shape[next++]), true, true, true);
    }
    outputs->setIntegrityLevel(state, false);
    ObjectRef* parameters = ObjectRef::create(state);
    const auto& descriptors = record->definition->descriptors;
    for (size_t index = 0; index < descriptors.size(); index++) {
        ValueRef* values = createFloat32Array(state, shape[next++]);
        record->arrays.push_back(values);
        // CreateDataProperty: an accessor on Object.prototype must not
        // intercept the parameter (param-getter-overridden).
        parameters->defineDataProperty(state,
                                       toJSString(descriptors[index].name()),
                                       values, true, true, true);
    }
    parameters->setIntegrityLevel(state, false);
    record->inputs = inputs;
    record->outputs = outputs;
    record->parameters = parameters;
}

// https://webaudio.github.io/web-audio-api/#rendering-a-graph
// The AudioWorkletNode substeps, on the main thread inside a render quantum.
void AudioWorkletGlobalScope::process(AudioWorkletProcessorRecord* record,
                                      AudioWorkletHandler& handler)
{
    if (!record->processor) {
        if (record->constructionFailed) {
            handler.setActiveSourceFlag(false);
        }
        return;
    }
    const size_t quantum = m_context->graph()->quantumFrames();
    // The arrays' shape: channels of each input (0 without an actively
    // processing source) and output, then each parameter array's length
    // (1 when its values are constant this quantum).
    m_shape.clear();
    for (size_t index = 0; index < handler.inputCount(); index++) {
        AudioNodeInput& input = handler.input(index);
        m_shape.push_back(input.hasActiveSource() ? input.bus().channels() : 0);
    }
    for (size_t index = 0; index < handler.outputCount(); index++) {
        m_shape.push_back(handler.output(index).channels());
    }
    for (size_t index = 0; index < handler.timelineCount(); index++) {
        bool constant;
        handler.parameterValues(index, constant);
        m_shape.push_back(constant ? 1 : quantum);
    }
    AudioWorkletProcessCall call = { record, &handler, quantum, false };
    if (!record->inputs || record->shape.size() != m_shape.size() ||
        !std::equal(m_shape.begin(), m_shape.end(), record->shape.begin()) ||
        anyDetached(record->arrays)) {
        record->shape = m_shape;
        call.rebuild = true;
    }

    auto result = Evaluator::execute(
        m_scriptBindingInstance->scriptContext(),
        [](ExecutionStateRef* state,
           AudioWorkletProcessCall* call) -> ValueRef* {
            AudioWorkletProcessorRecord* record = call->record;
            AudioWorkletHandler& handler = *call->handler;
            if (call->rebuild) {
                buildProcessArguments(state, call);
            }
            // Inputs are copied in, outputs zero-filled, parameters filled.
            size_t next = 0;
            for (size_t index = 0; index < handler.inputCount(); index++) {
                const AudioBus& bus = handler.input(index).bus();
                for (size_t channel = 0; channel < record->shape[index];
                     channel++) {
                    std::copy_n(bus.channel(channel), call->quantum,
                                floatData(record->arrays[next++]));
                }
            }
            for (size_t index = 0; index < handler.outputCount(); index++) {
                const size_t channels =
                    record->shape[handler.inputCount() + index];
                for (size_t channel = 0; channel < channels; channel++) {
                    std::fill_n(floatData(record->arrays[next++]),
                                call->quantum, 0.0f);
                }
            }
            for (size_t index = 0; index < handler.timelineCount(); index++) {
                bool constant;
                const float* values = handler.parameterValues(index, constant);
                std::copy_n(values, constant ? 1 : call->quantum,
                            floatData(record->arrays[next++]));
            }
            ObjectRef* processor = record->processor.value();
            ValueRef* process =
                processor->get(state, StringRef::createFromASCII("process"));
            if (!process->isCallable()) {
                const char* message = "process is not a function";
                state->throwException(TypeErrorObjectRef::create(
                    state,
                    StringRef::createFromASCII(message, strlen(message))));
            }
            ValueRef* argv[] = { record->inputs, record->outputs,
                                 record->parameters };
            return ValueRef::create(
                process->call(state, processor, 3, argv)->toBoolean(state));
        },
        &call);

    if (result.error.hasValue()) {
        // "Set [[callable process]] to false. Set processor's active source
        // flag to false. Make a silent output buffer available for reading.
        // Queue a task ... processorerror."
        handler.clearCallableProcess();
        handler.setActiveSourceFlag(false);
        for (size_t index = 0; index < handler.outputCount(); index++) {
            handler.output(index).bus().zero();
        }
        String* message;
        String* filename;
        uint32_t lineno;
        uint32_t colno;
        errorPosition(m_scriptBindingInstance, result, message, filename,
                      lineno, colno);
        queueProcessorError(record, message, filename, lineno, colno);
        return;
    }
    const bool active = result.result->asBoolean();
    handler.setActiveSourceFlag(active);
    record->retainNode(active);
    // The outputs' copies are made available for reading; an output array
    // that process() transferred is silent.
    size_t next = 0;
    for (size_t index = 0; index < handler.inputCount(); index++) {
        next += record->shape[index];
    }
    for (size_t index = 0; index < handler.outputCount(); index++) {
        AudioBus& bus = handler.output(index).bus();
        const size_t channels = record->shape[handler.inputCount() + index];
        for (size_t channel = 0; channel < channels; channel++) {
            ScriptValue array = record->arrays[next++];
            if (array->asArrayBufferView()->byteLength()) {
                std::copy_n(floatData(array), quantum, bus.channel(channel));
            }
        }
        bus.setSilent(false);
    }
}

struct AudioWorkletProcessorError : public gc {
    Optional<AudioWorkletNode*> node;
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
    // The processor outputs silence from now on and no longer retains the
    // node; the queued event does until it is fired.
    error->node = record->node();
    record->retainNode(false);
    error->message = message;
    error->filename = filename;
    error->lineno = lineno;
    error->colno = colno;
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* data) {
            auto* error = static_cast<AudioWorkletProcessorError*>(data);
            Optional<AudioWorkletNode*> node = error->node;
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
