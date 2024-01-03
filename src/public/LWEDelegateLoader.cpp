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

#include "LWEDelegateLoader.h"

namespace LWE {

LWEDelegateLoader* LWEDelegateLoader::getInstance()
{
    static LWEDelegateLoader* instance = nullptr;
    if (!instance) {
        instance = new LWEDelegateLoader();
    }
    return instance;
}

bool LWEDelegateLoader::load(std::string path)
{
    m_handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!m_handle) {
        std::cerr << "Failed to open library: " << dlerror() << std::endl;
        return false;
    }

    return true;
}

void LWEDelegateLoader::unload()
{
    if (m_handle) {
        dlclose(m_handle);
        m_handle = nullptr;
    }
}

} // namespace LWE
