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

#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <functional>

class LogOption {
public:
    static bool isEnabled(const std::string&);
    static void setExternalIsEnabled(std::function<bool(const std::string&)>);

private:
    static std::function<bool(const std::string&)> s_externalIsEnabled;
};

class Logger {
public:
    class Header {
    protected:
        virtual void writeHeader(std::stringstream&) = 0;

    private:
        void write(std::stringstream& stream);
        friend class Logger;
    };

    class Output {
    public:
        virtual void flush(std::stringstream& ss) = 0;
    };

    Logger() = default;
    Logger(const std::string& header, std::shared_ptr<Output> out = nullptr);
    Logger(Header&& header, std::shared_ptr<Output> out = nullptr);
    ~Logger();

    template <class T>
    Logger& operator<<(const T& msg)
    {
        m_stream << msg;
        return *this;
    }

    template <typename T, typename... Args>
    Logger& print(const char* format, T value, Args... args)
    {
        if (m_output == nullptr) {
            return *this;
        }

        while (*format) {
            if (*format == '%' && *(++format) != '%') {
                m_stream << value;

                // handle sub-specifiers
                if ((*format == 'z')) {
                    format++;
                } else if ((*format == 'l') || (*format == 'h')) {
                    format++;
                    if (*format == *(format + 1)) {
                        format++;
                    }
                }
                format++;

                print(format, args...);
                return *this;
            }
            m_stream << *format++;
        }
        assert(((void)"logical error: should not come here", false));
        return *this;
    };

    Logger& print(const char* string_without_format_specifiers = "");
    Logger& flush();

protected:
    std::stringstream m_stream;
    void initialize(std::shared_ptr<Output> out = nullptr);

private:
    std::shared_ptr<Output> m_output;
};

class StdOut : public Logger::Output {
public:
    void flush(std::stringstream& ss) override;
};

// --- Utils ---
#define __FILE_NAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __FUNCTION_NAME__ getPrettyFunctionName(__PRETTY_FUNCTION__)

#define __CODE_LOCATION__ \
    createCodeLocation(__PRETTY_FUNCTION__, __FILE_NAME__, __LINE__).c_str()

std::string createCodeLocation(const char* functionName, const char* filename,
                               const int line, std::string prefixPattern = "");

std::string getPrettyFunctionName(const std::string fullname,
                                  std::string prefixPattern = "");

void writeThreadIdentifier(std::ostream& ss);

class IndentCounter {
public:
    IndentCounter(std::string id);
    ~IndentCounter();
    static std::string getString(std::string id = "");
    static void indent(std::string id);
    static void unIndent(std::string id);

private:
    std::string m_id;
};
