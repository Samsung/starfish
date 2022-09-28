/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include <fstream>
#include <sys/stat.h>
#include "StarfishConfig.h"

#include "platform/file/PlatformDirectory.h"
#include "core/modules/serviceworker/WorkerConfig.h"
#include "core/modules/serviceworker/util/LocalStorageHelper.h"

namespace Starfish {

namespace LocalStorageHelper {

    bool File::exists(const std::string& path)
    {
        std::ifstream infile(path);
        return infile.good();
    }

    void File::mkdirIfNotExists(const std::string& path)
    {
        if (LocalStorageHelper::File::exists(path)) {
            return;
        }

        TRACE(LOCALSTORAGE, path.data());

        if (mkdir(path.data(), 0755) != 0) {
            STARFISH_LOG_ERROR("cannot mkDir: %s", path.data());
        }
    }

    Writer::Writer(std::string& path)
    {
        TRACEF(LOCALSTORAGE, path.data());
        m_fileStream.open(path.data(), std::ios::binary);
    }

    Writer::~Writer()
    {
        m_fileStream.close();
    }

    bool Writer::write(const char* buffer, const size_t size)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        TRACEF(LOCALSTORAGE, "size(%zu)", size);

        m_fileStream.write(buffer, sizeof(char) * size);
        return true;
    }

    bool Writer::writeBuffer(const char* buffer, const size_t size)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        TRACEF(LOCALSTORAGE, "size(%zu)", size);

        m_fileStream << size;
        m_fileStream.write(buffer, sizeof(char) * size);
        return true;
    }

    bool Writer::writeString(String* string)
    {
        if (!string->bufferAccessData().hasASCIIData()) {
            return false;
        }

        auto utf8String = string->toUTF8NonGCString();

        return writeBuffer(utf8String.data(), utf8String.size());
    }

    bool Writer::writeString(const std::string& string)
    {
        return writeBuffer(string.data(), string.size());
    }

    bool Writer::writeVector(const std::vector<char>& vector)
    {
        return writeBuffer(vector.data(), vector.size());
    }

    Reader::Reader(std::string& path)
    {
        TRACEF(LOCALSTORAGE, path.data());
        m_fileStream.open(path.data(), std::ios::binary);
    }

    Reader::~Reader()
    {
        m_fileStream.close();
    }

    bool Reader::readString(String*& string)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        size_t size = 0;
        m_fileStream >> size;
        if (size > 0) {
            TRACEF(LOCALSTORAGE, "size(%zu)", size);

            auto buffer = reinterpret_cast<char*>(calloc(1, size));
            m_fileStream.read(buffer, sizeof(char) * size);
            string = String::fromUTF8(buffer, size);
            free(reinterpret_cast<void*>(buffer));
        }

        return true;
    }

    bool Reader::readString(std::string& string)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        size_t size = 0;
        m_fileStream >> size;
        if (size > 0) {
            TRACEF(LOCALSTORAGE, "size(%zu)", size);

            auto buffer = reinterpret_cast<char*>(calloc(1, size));
            m_fileStream.read(buffer, sizeof(char) * size);
            string = std::string(buffer, size);
            free(reinterpret_cast<void*>(buffer));
        }

        return true;
    }

    bool Reader::readVector(std::vector<char>& vector)
    {
        if (!m_fileStream.is_open()) {
            return false;
        }

        STARFISH_ASSERT(vector.size() == 0);

        size_t size = 0;
        m_fileStream >> size;
        if (size > 0) {
            TRACEF(LOCALSTORAGE, "size(%zu)", size);

            vector.resize(size);
            m_fileStream.read(vector.data(), sizeof(char) * size);
        }

        return true;
    }

} // namespace LocalStorageHelper
} // namespace Starfish

#endif
