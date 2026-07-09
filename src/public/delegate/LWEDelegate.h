/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
#ifndef __LWEDelegate__
#define __LWEDelegate__

#include "LWEDelegateConfig.h"

namespace LWEDelegate {
class EXPORT_UNMANAGED_API LWE {
public:
    static void Initialize(const char* storageDirectoryPath,
                           bool preferMainThread);

    static bool IsInitialized();

    static void Finalize();

    static unsigned char GetGCFrequency();

    static void SetGCFrequency(unsigned char freq);

    static void GetVersion(int* major, int* minor, int* patch);

    static bool IsUsingSeparateThread();
};

} // namespace LWEDelegate

// C wrappers used for dlopen/dlsym.
extern "C" {
void EXPORT_UNMANAGED_API LWEDelegate_LWE_Initialize(
    const char* storageDirectoryPath, bool useMainThread);

bool EXPORT_UNMANAGED_API LWEDelegate_LWE_IsInitialized();

void EXPORT_UNMANAGED_API LWEDelegate_LWE_Finalize();

unsigned char EXPORT_UNMANAGED_API LWEDelegate_LWE_GetGCFrequency();

void EXPORT_UNMANAGED_API LWEDelegate_LWE_SetGCFrequency(unsigned char freq);

void EXPORT_UNMANAGED_API LWEDelegate_LWE_GetVersion(int* major, int* minor,
                                                     int* patch);

bool EXPORT_UNMANAGED_API LWEDelegate_LWE_IsUsingSeparateThread();

typedef struct {
    void (*Initialize)(const char*, bool);
    bool (*IsInitialized)();
    void (*Finalize)();
    unsigned char (*GetGCFrequency)();
    void (*SetGCFrequency)(unsigned char);
    void (*GetVersion)(int*, int*, int*);
    bool (*IsUsingSeparateThread)();
} LWEProcTable;
}
#endif
