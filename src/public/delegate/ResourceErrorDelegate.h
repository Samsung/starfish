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
#ifndef __ResourceErrorDelegate__
#define __ResourceErrorDelegate__

#include "LWEDelegateConfig.h"

#include <string>

namespace LWEDelegate {

class EXPORT_UNMANAGED_API ResourceError {
public:
    static ResourceError* Create(int code, const std::string& description,
                                 const std::string& url);

    ResourceError() = default;
    virtual ~ResourceError() = default;

    virtual int GetErrorCode() = 0;
    virtual std::string GetDescription() = 0;
    virtual std::string GetUrl() = 0;
};

} // namespace LWEDelegate

// C wrappers used for dlopen/dlsym.
extern "C" {
uintptr_t EXPORT_UNMANAGED_API LWEDelegate_ResourceError_Create(
    int code, const char* description, const char* url);

typedef struct {
    uintptr_t (*Create)(int, const char*, const char*);
} ResourceErrorProcTable;
}
#endif
