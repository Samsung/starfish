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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishLocalStorageHelper__)
#define __StarfishLocalStorageHelper__

#include <fstream>
#include <iostream>

namespace Starfish {

class String;

namespace LocalStorageHelper {

    class File {
    public:
        static bool exists(const std::string& path);
        static void mkdirIfNotExists(const std::string& path);
        static void remove(const std::string& path);
    };

    class Writer {
    public:
        Writer(std::string& path);
        ~Writer();

        template <typename T>
        bool write(const T& value, const char* postfix = "")
        {
            if (!m_fileStream.is_open()) {
                return false;
            }

            m_fileStream << value << postfix;

            return true;
        }

        bool write(const char* buffer, const size_t size);

        // Save the buffer size together in the file.
        bool writeBuffer(const char* buffer, const size_t size);

        bool writeString(String* string);

        bool writeString(const std::string& string);

        bool writeVector(const std::vector<char>& vector);

    private:
        std::ofstream m_fileStream;
    };

    class Reader {
    public:
        Reader(std::string& path);
        ~Reader();

        template <typename T>
        bool read(T& value)
        {
            if (!m_fileStream.is_open()) {
                return false;
            }

            m_fileStream >> value;

            return true;
        }

        bool readAll(std::string& string);

        bool readString(String*& string);

        bool readString(std::string& string);

        bool readVector(std::vector<char>& vector);

    private:
        std::ifstream m_fileStream;
    };

} // namespace LocalStorageHelper
} // namespace Starfish

#endif
