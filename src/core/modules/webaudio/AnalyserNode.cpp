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
#include "core/modules/webaudio/AnalyserNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <algorithm>
#include <cmath>

#include "EscargotPublic.h"

namespace Starfish {

AnalyserNode::AnalyserNode(ExecutionContext* executionContext,
                           BaseAudioContext* context, AnalyserOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    if (options.fftSize() < 32 || options.fftSize() > 32768 ||
        (options.fftSize() & (options.fftSize() - 1)) ||
        !(options.minDecibels() < options.maxDecibels()) ||
        !(options.smoothingTimeConstant() >= 0 &&
          options.smoothingTimeConstant() <= 1)) {
        throw new DOMException(executionContext, DOMException::INDEX_SIZE_ERR,
                               "Invalid analyser options");
    }
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_analyserHandler =
        static_cast<AnalyserHandler*>(context->graph()->addHandler(
            std::unique_ptr<AudioHandler>(new AnalyserHandler())));
    m_handler = m_analyserHandler;
    m_fftSize = options.fftSize();
    m_minDecibels = options.minDecibels();
    m_maxDecibels = options.maxDecibels();
    m_smoothingTimeConstant = options.smoothingTimeConstant();
    m_analyserHandler->setFftSize(m_fftSize);
    m_analyserHandler->setSmoothing(m_smoothingTimeConstant);
    context->graph()->registerAnalysisHandler(m_analyserHandler);
    applyOptions(options);
}

void AnalyserNode::setFftSize(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value < 32 || value > 32768 || (value & (value - 1))) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "FFT size must be a power of two in 32..32768");
    }
    m_fftSize = value;
    m_analyserHandler->setFftSize(value);
}

void AnalyserNode::setMinDecibels(double value)
{
    AudioGraphLock graphLock(context()->graph());
    if (!(value < m_maxDecibels)) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "Minimum dB must be less than maximum dB");
    }
    m_minDecibels = value;
}

void AnalyserNode::setMaxDecibels(double value)
{
    AudioGraphLock graphLock(context()->graph());
    if (!(value > m_minDecibels)) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "Maximum dB must exceed minimum dB");
    }
    m_maxDecibels = value;
}

void AnalyserNode::setSmoothingTimeConstant(double value)
{
    AudioGraphLock graphLock(context()->graph());
    if (!(value >= 0 && value <= 1)) {
        throw new DOMException(executionContext(), DOMException::INDEX_SIZE_ERR,
                               "Smoothing constant must be in 0..1");
    }
    m_smoothingTimeConstant = value;
    m_analyserHandler->setSmoothing(value);
}

void AnalyserNode::getFloatTimeDomainData(ScriptFloat32Array array)
{
    AudioGraphLock graphLock(context()->graph());
    float* data =
        reinterpret_cast<float*>(array->rawBuffer() + array->byteOffset());
    m_analyserHandler->copyTimeDomain(data, array->arrayLength());
}

void AnalyserNode::getByteTimeDomainData(ScriptUint8Array array)
{
    AudioGraphLock graphLock(context()->graph());
    uint8_t* data = array->rawBuffer() + array->byteOffset();
    const size_t count = std::min<size_t>(array->arrayLength(), m_fftSize);
    for (size_t i = 0; i < count; i++) {
        const double scaled =
            std::floor(128 * (1 + m_analyserHandler->timeSample(i)));
        data[i] = static_cast<uint8_t>(std::max(0.0, std::min(255.0, scaled)));
    }
}

void AnalyserNode::getFloatFrequencyData(ScriptFloat32Array array)
{
    AudioGraphLock graphLock(context()->graph());
    float* data =
        reinterpret_cast<float*>(array->rawBuffer() + array->byteOffset());
    const std::vector<float>& frequencies = m_analyserHandler->frequencyData();
    std::copy_n(frequencies.begin(),
                std::min<size_t>(array->arrayLength(), frequencies.size()),
                data);
}

void AnalyserNode::getByteFrequencyData(ScriptUint8Array array)
{
    AudioGraphLock graphLock(context()->graph());
    uint8_t* data = array->rawBuffer() + array->byteOffset();
    const std::vector<float>& frequencies = m_analyserHandler->frequencyData();
    const size_t count =
        std::min<size_t>(array->arrayLength(), frequencies.size());
    for (size_t i = 0; i < count; i++) {
        const double scaled =
            std::floor(255 * (frequencies[i] - m_minDecibels) /
                       (m_maxDecibels - m_minDecibels));
        data[i] = static_cast<uint8_t>(std::max(0.0, std::min(255.0, scaled)));
    }
}

ScriptBindingInstance* AnalyserNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
