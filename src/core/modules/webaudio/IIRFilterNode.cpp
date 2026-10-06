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
#include "core/modules/webaudio/IIRFilterNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <vector>

#include "EscargotPublic.h"

namespace Starfish {

IIRFilterNode::IIRFilterNode(ExecutionContext* executionContext,
                             BaseAudioContext* context,
                             IIRFilterOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-createiirfilter
    const auto& feedforward = options.feedforward();
    const auto& feedback = options.feedback();
    if (feedforward.empty() || feedforward.size() > 20 || feedback.empty() ||
        feedback.size() > 20) {
        throw new DOMException(
            executionContext, DOMException::NOT_SUPPORTED_ERR,
            "IIR coefficient arrays must contain 1 to 20 values");
    }
    if (std::any_of(feedforward.begin(), feedforward.end(),
                    [](double value) { return !std::isfinite(value); }) ||
        std::any_of(feedback.begin(), feedback.end(),
                    [](double value) { return !std::isfinite(value); })) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "IIR coefficients must be finite");
    }
    if (std::all_of(feedforward.begin(), feedforward.end(),
                    [](double value) { return value == 0; }) ||
        feedback[0] == 0) {
        throw new DOMException(executionContext,
                               DOMException::INVALID_STATE_ERR,
                               "IIR filter must have nonzero coefficients");
    }
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_handler = context->graph()->addHandler(
        std::unique_ptr<AudioHandler>(new IIRFilterHandler(
            std::vector<double>(feedforward.begin(), feedforward.end()),
            std::vector<double>(feedback.begin(), feedback.end()))));
    configureInputs();
    applyOptions(options);
}

void IIRFilterNode::getFrequencyResponse(ScriptFloat32Array frequencyHz,
                                         ScriptFloat32Array magResponse,
                                         ScriptFloat32Array phaseResponse)
{
    AudioGraphLock graphLock(context()->graph());
    const size_t length = frequencyHz->arrayLength();
    if (magResponse->arrayLength() != length ||
        phaseResponse->arrayLength() != length) {
        throw new DOMException(
            executionContext(), DOMException::INVALID_ACCESS_ERR,
            "Frequency response arrays must have equal lengths");
    }

    const float* frequency = reinterpret_cast<const float*>(
        frequencyHz->rawBuffer() + frequencyHz->byteOffset());
    float* magnitude = reinterpret_cast<float*>(magResponse->rawBuffer() +
                                                magResponse->byteOffset());
    float* phase = reinterpret_cast<float*>(phaseResponse->rawBuffer() +
                                            phaseResponse->byteOffset());
    const auto* handler = static_cast<const IIRFilterHandler*>(m_handler);
    const double nyquist = context()->sampleRate() / 2;
    constexpr double pi = 3.14159265358979323846;
    for (size_t i = 0; i < length; i++) {
        if (!std::isfinite(frequency[i]) || frequency[i] < 0 ||
            frequency[i] > nyquist) {
            magnitude[i] = std::numeric_limits<float>::quiet_NaN();
            phase[i] = std::numeric_limits<float>::quiet_NaN();
            continue;
        }
        const double angle = -pi * frequency[i] / nyquist;
        const std::complex<double> unit(std::cos(angle), std::sin(angle));
        std::complex<double> power(1, 0);
        std::complex<double> numerator(0, 0);
        std::complex<double> denominator(0, 0);
        const auto& feedforward = handler->feedforward();
        const auto& feedback = handler->feedback();
        for (size_t k = 0; k < std::max(feedforward.size(), feedback.size());
             k++) {
            if (k < feedforward.size()) {
                numerator += feedforward[k] * power;
            }
            if (k < feedback.size()) {
                denominator += feedback[k] * power;
            }
            power *= unit;
        }
        const std::complex<double> response = numerator / denominator;
        magnitude[i] = static_cast<float>(std::abs(response));
        phase[i] = static_cast<float>(std::arg(response));
    }
}

ScriptBindingInstance* IIRFilterNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
