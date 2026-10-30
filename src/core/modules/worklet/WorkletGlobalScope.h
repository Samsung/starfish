/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishWorkletGlobalScope__
#define __StarfishWorkletGlobalScope__

#include "core/dom/EventTarget.h"

namespace Starfish {

// https://html.spec.whatwg.org/multipage/worklets.html#workletglobalscope
// WorkletGlobalScope is not an EventTarget in IDL. The C++ class derives from
// EventTarget only because a realm's global object must carry one as its
// extra data (fetchExecutionContext()).
class WorkletGlobalScope : public EventTarget {
public:
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWorkletGlobalScope() const override;
};
} // namespace Starfish

#endif
#endif
