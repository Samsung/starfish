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
#include "core/modules/webaudio/PeriodicWave.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/render/PeriodicWaveData.h"

#include <cmath>
#include <vector>

namespace Starfish {

PeriodicWave::PeriodicWave(ExecutionContext* executionContext,
                           BaseAudioContext* context,
                           PeriodicWaveOptions options)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_context(context)
{
    // https://webaudio.github.io/web-audio-api/#dom-periodicwave-periodicwave
    size_t length = options.hasReal()   ? options.real().size()
                    : options.hasImag() ? options.imag().size()
                                        : 2;
    if ((options.hasReal() && options.hasImag() &&
         options.real().size() != options.imag().size()) ||
        length < 2) {
        throw new DOMException(executionContext, DOMException::INDEX_SIZE_ERR,
                               "Invalid PeriodicWave coefficient lengths");
    }
    if (length > 8192) {
        throw new DOMException(
            executionContext, DOMException::NOT_SUPPORTED_ERR,
            "PeriodicWave supports at most 8192 coefficients");
    }
    if ((options.hasReal() && !std::isfinite(options.real()[0])) ||
        (options.hasImag() && !std::isfinite(options.imag()[0]))) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "PeriodicWave coefficients must be finite");
    }
    std::vector<PeriodicWaveData::Partial> partials;
    for (size_t i = 1; i < length; i++) {
        const double real = options.hasReal() ? options.real()[i] : 0;
        const double imag = options.hasImag()
                                ? options.imag()[i]
                                : (!options.hasReal() && i == 1 ? 1 : 0);
        if (!std::isfinite(real) || !std::isfinite(imag)) {
            throw new DOMException(executionContext,
                                   DOMException::SCRIPT_TYPE_ERR,
                                   "PeriodicWave coefficients must be finite");
        }
        if (real != 0 || imag != 0) {
            partials.push_back({ i, real, imag });
        }
    }
    m_data = new PeriodicWaveData(partials, options.disableNormalization());
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void*) { ((PeriodicWave*)obj)->~PeriodicWave(); },
        NULL, NULL, NULL);
}

PeriodicWave::~PeriodicWave()
{
    if (m_data) {
        m_data->release();
    }
}

ScriptBindingInstance* PeriodicWave::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

} // namespace Starfish
#endif
