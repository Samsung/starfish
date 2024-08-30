/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_API_ENABLE_LOADER

#include "LWELoaderUtils.h"

#include <dlfcn.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace {

int compareVersions(const std::string& version1, const std::string& version2)
{
    std::istringstream iss1(version1);
    std::istringstream iss2(version2);

    int major1, minor1, patch1;
    int major2, minor2, patch2;
    char dot;

    iss1 >> major1 >> dot >> minor1 >> dot >> patch1;
    iss2 >> major2 >> dot >> minor2 >> dot >> patch2;

    if (major1 != major2) {
        return major1 - major2;
    }
    if (minor1 != minor2) {
        return minor1 - minor2;
    }
    return patch1 - patch2;
}

std::string readVersion(const std::string& path)
{
    const std::string versionFileName = "VERSION";
    std::ifstream versionFile(path + versionFileName);

    std::string versionString = "0.0.0";
    if (!versionFile.is_open()) {
        return versionString;
    }

    getline(versionFile, versionString);
    return versionString;
}

} // namespace

namespace LWE {

bool LWELoaderUtils::openLWELibrary(void*& handle,
                                    const std::string& targetName,
                                    bool preferUpdatedVersion)
{
    if (preferUpdatedVersion) {
        std::string defaultVersion = readVersion(STARFISH_API_DEFAULT_PATH);
        std::string uweVersion = readVersion(STARFISH_API_UWE_MOUNT_PATH);
        std::cout << "default version: " << defaultVersion << std::endl;
        std::cout << "uwe version: " << uweVersion << std::endl;

        if (compareVersions(uweVersion, defaultVersion) > 0) {
            std::cout << "Try to load updated LWE..." << std::endl;
            handle = dlopen((STARFISH_API_UWE_MOUNT_PATH + targetName).c_str(),
                            RTLD_LAZY);
            if (!handle) {
                std::cerr << "Failed to load updated LWE: " << dlerror()
                          << std::endl;
            }
        }
    }

    if (!handle) {
        // Try to open default version.
        std::cout << "Try to load default LWE..." << std::endl;
        handle = dlopen(targetName.c_str(), RTLD_LAZY);
    }

    if (!handle) {
        std::cerr << "Failed to load default LWE: " << dlerror() << std::endl;
        return false;
    }

    return true;
}

} // namespace LWE

#endif
