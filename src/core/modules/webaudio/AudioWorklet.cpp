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

#include "core/modules/webaudio/AudioWorklet.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePort.h"
#include "core/fetch/HeadersData.h"
#include "core/fetch/RequestData.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/modules/webaudio/AudioWorkletGlobalScope.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/page/WebBase.h"
#include "platform/loader/ResourceURL.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

namespace Starfish {

// https://mimesniff.spec.whatwg.org/#javascript-mime-type
static bool isJavaScriptMIMEType(String* type)
{
    if (!type) {
        return false;
    }
    std::string essence = type->toUTF8NonGCString().data();
    essence = essence.substr(0, essence.find(';'));
    essence.erase(essence.find_last_not_of(" \t") + 1);
    essence.erase(0, essence.find_first_not_of(" \t"));
    std::transform(essence.begin(), essence.end(), essence.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    static const char* const types[] = { "application/ecmascript",
                                         "application/javascript",
                                         "application/x-ecmascript",
                                         "application/x-javascript",
                                         "text/ecmascript",
                                         "text/javascript",
                                         "text/javascript1.0",
                                         "text/javascript1.1",
                                         "text/javascript1.2",
                                         "text/javascript1.3",
                                         "text/javascript1.4",
                                         "text/javascript1.5",
                                         "text/jscript",
                                         "text/livescript",
                                         "text/x-ecmascript",
                                         "text/x-javascript" };
    for (const char* candidate : types) {
        if (essence == candidate) {
            return true;
        }
    }
    return false;
}

// https://html.spec.whatwg.org/multipage/webappapis.html#fetch-a-single-module-script
// A network error, a non-ok status or a non-JavaScript MIME type gives a
// null script.
class AudioWorkletModuleFetchClient : public ResourceRequestClient {
public:
    AudioWorkletModuleFetchClient(AudioWorklet* worklet, ResourceURL* url,
                                  Promise* promise)
        : m_worklet(worklet)
        , m_url(url)
        , m_promise(promise)
    {
    }

    void onReadyStateChange(ResourceRequest* request,
                            bool isExplicitAction) override
    {
        if (!isExplicitAction || request->readyState() != ReadyState::Done) {
            return;
        }
        Optional<String*> text;
        if (!request->isError() && request->status() >= 200 &&
            request->status() <= 299 &&
            isJavaScriptMIMEType(request->responseMimeType())) {
            auto& response = request->response();
            text = String::fromUTF8(response.data(), response.size());
        }
        m_worklet->runModule(m_url, m_promise, text);
    }

private:
    AudioWorklet* m_worklet;
    ResourceURL* m_url;
    Promise* m_promise;
};

AudioWorklet::AudioWorklet(BaseAudioContext* context)
    : Worklet()
    , m_context(context)
{
}

ScriptBindingInstance* AudioWorklet::scriptBindingInstance()
{
    return m_context->scriptBindingInstance();
}

AudioWorkletGlobalScope* AudioWorklet::globalScope()
{
    if (!m_globalScope) {
        // https://html.spec.whatwg.org/multipage/worklets.html#create-a-worklet-global-scope
        AudioWorkletGlobalScope* scope =
            new AudioWorkletGlobalScope(m_context, this);
        // The AudioWorklet's port and the global scope's port are the two
        // ends of one channel.
        MessagePort* port = new MessagePort(m_context->executionContext());
        MessagePort::entangle(port, scope->port());
        m_globalScope = scope;
        m_port = port;
    }
    return m_globalScope.value();
}

MessagePort* AudioWorklet::port()
{
    globalScope();
    return m_port.value();
}

Optional<AudioWorkletProcessorDefinition*> AudioWorklet::definition(
    String* name) const
{
    for (AudioWorkletProcessorDefinition* definition : m_definitions) {
        if (definition->name->equals(name)) {
            return definition;
        }
    }
    return nullptr;
}

void AudioWorklet::addDefinition(AudioWorkletProcessorDefinition* definition)
{
    m_definitions.push_back(definition);
}

// https://html.spec.whatwg.org/multipage/worklets.html#dom-worklet-addmodule
Promise* AudioWorklet::addModule(String* moduleURL, WorkletOptions)
{
    ExecutionContext* outside = m_context->executionContext();
    Promise* promise = new Promise(scriptBindingInstance());
    ResourceURL* url =
        new ResourceURL(moduleURL, outside->baseURL()->baseURI());
    if (!url->isValid()) {
        promise->reject((new DOMException(outside, DOMException::SYNTAX_ERR,
                                          "Invalid worklet module URL"))
                            ->scriptValue());
        return promise;
    }
    globalScope();
    for (ResourceURL* added : m_moduleURLs) {
        if (*added == *url) {
            queueSettle(promise, nullptr);
            return promise;
        }
    }

    // https://html.spec.whatwg.org/multipage/worklets.html#fetch-a-worklet-script-graph
    RequestData* requestData = new RequestData();
    requestData->m_url = url;
    requestData->m_destination = RequestDestination::AudioWorkLet;
    requestData->m_mode = RequestMode::CORS;
    requestData->m_syncLevel = RequestSyncLevel::NeverSync;
    ResourceRequest* request = new ResourceRequest(outside);
    request->addResourceRequestClient(
        new AudioWorkletModuleFetchClient(this, url, promise));
    request->open(requestData, new HeadersData());
    request->send();
    return promise;
}

void AudioWorklet::runModule(ResourceURL* url, Promise* promise,
                             Optional<String*> text)
{
    if (!text) {
        queueSettle(promise,
                    (new DOMException(
                         m_context->executionContext(), DOMException::ABORT_ERR,
                         "Failed to fetch the worklet module script"))
                        ->scriptValue());
        return;
    }
    // A concurrent addModule() of the same URL ran it already.
    for (ResourceURL* added : m_moduleURLs) {
        if (*added == *url) {
            queueSettle(promise, nullptr);
            return;
        }
    }
    ScriptValue error = nullptr;
    if (!globalScope()->runModuleScript(text.value(), url, error)) {
        queueSettle(promise, error);
        return;
    }
    m_moduleURLs.push_back(url);
    queueSettle(promise, nullptr);
}

// The promise settles in a queued task, after the tasks registerProcessor()
// queued while the module ran.
void AudioWorklet::queueSettle(Promise* promise, Optional<ScriptValue> error)
{
    ExecutionContext* executionContext = m_context->executionContext();
    executionContext->webBase()->messageLoop()->addIdler(
        executionContext->globalScope(),
        [](size_t, void* promise, void* error) {
            if (error) {
                static_cast<Promise*>(promise)->reject(
                    static_cast<ScriptValue>(error));
            } else {
                static_cast<Promise*>(promise)->fulfill(scriptUndefined());
            }
        },
        promise, error ? error.value() : nullptr);
}
} // namespace Starfish

#endif
