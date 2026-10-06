/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/WaveShaperNode.h"

#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include "EscargotPublic.h"

namespace Starfish {

WaveShaperNode::WaveShaperNode(ExecutionContext* executionContext,
                               BaseAudioContext* context,
                               WaveShaperOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_shaperHandler =
        static_cast<WaveShaperHandler*>(context->graph()->addHandler(
            std::unique_ptr<AudioHandler>(new WaveShaperHandler())));
    m_handler = m_shaperHandler;
    if (options.hasCurve()) {
        if (options.curve().size() < 2) {
            throw new DOMException(
                executionContext, DOMException::INVALID_STATE_ERR,
                "WaveShaper curve must have at least two values");
        }
        ScriptArrayBuffer arrayBuffer = createScriptArrayBuffer(
            scriptBindingInstance(), options.curve().size() * sizeof(float));
        m_curve =
            Escargot::Evaluator::execute(
                scriptBindingInstance()->scriptContext(),
                [](Escargot::ExecutionStateRef* state,
                   ScriptArrayBuffer arrayBuffer,
                   const GCAtomicVector<double>* values)
                    -> Escargot::ValueRef* {
                    auto* view = Escargot::Float32ArrayObjectRef::create(state);
                    view->setBuffer(arrayBuffer, 0,
                                    values->size() * sizeof(float),
                                    values->size());
                    float* data = reinterpret_cast<float*>(view->rawBuffer());
                    for (size_t i = 0; i < values->size(); i++) {
                        data[i] = static_cast<float>((*values)[i]);
                    }
                    return view;
                },
                arrayBuffer, &options.curve())
                .result->asFloat32ArrayObject();
        m_shaperHandler->setCurve(
            std::vector<float>(options.curve().begin(), options.curve().end()));
        m_curveSet = true;
    }
    setOversample(options.oversample());
    applyOptions(options);
}

void WaveShaperNode::setCurve(ScriptFloat32Array curve)
{
    AudioGraphLock graphLock(context()->graph());
    if (!curve) {
        m_curve = nullptr;
        m_shaperHandler->setCurve({});
        return;
    }
    if (m_curveSet || curve->arrayLength() < 2) {
        throw new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "WaveShaper curve cannot be replaced or too short");
    }
    // https://webaudio.github.io/web-audio-api/#dom-waveshapernode-curve
    // The attribute returns the assigned array itself, while rendering uses
    // "an internal copy of the curve" so later writes to that array have no
    // effect. Both are therefore kept; the copy is the only native one.
    const size_t length = curve->arrayLength();
    const float* data = reinterpret_cast<const float*>(curve->rawBuffer() +
                                                       curve->byteOffset());
    m_shaperHandler->setCurve(std::vector<float>(data, data + length));
    m_curve = curve;
    m_curveSet = true;
}

void WaveShaperNode::setOversample(String* oversample)
{
    AudioGraphLock graphLock(context()->graph());
    if (!oversample->equals("none") && !oversample->equals("2x") &&
        !oversample->equals("4x")) {
        return;
    }
    m_oversample = oversample;
    m_shaperHandler->setOversample(oversample->equals("2x")   ? 2
                                   : oversample->equals("4x") ? 4
                                                              : 1);
}

ScriptBindingInstance* WaveShaperNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
