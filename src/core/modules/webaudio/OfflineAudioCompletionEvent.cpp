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
#include "core/modules/webaudio/OfflineAudioCompletionEvent.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {
ScriptBindingInstance* OfflineAudioCompletionEvent::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}
} // namespace Starfish

#endif
