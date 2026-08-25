/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */
#ifndef __LWEDelegateContract__
#define __LWEDelegateContract__

#include "LWEDelegateConfig.h"

#include <cstdint>

namespace LWEDelegate {

// Increment this ABI epoch only when an intentional contract change prevents
// an older API .so from safely using a newer impl .so. Append-only changes
// remain compatible and keep the current epoch.
constexpr uint32_t kDelegateAbiEpoch = 1;

} // namespace LWEDelegate

extern "C" {
uint32_t EXPORT_UNMANAGED_API LWEDelegate_GetAbiEpoch();

typedef struct {
    uint32_t (*GetAbiEpoch)();
} DelegateContractProcTable;
}

#endif
