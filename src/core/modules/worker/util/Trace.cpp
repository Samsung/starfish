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

#include "core/modules/worker/util/Trace.h"

#if defined(ENABLE_TRACE)

#include "StarfishBase.h"
#include <unistd.h> // getpid()
#include <iomanip>  // setfill and setw

#define TYPE_LENGTH_LIMIT 5
#define TRACE_ID_LENGTH_LIMIT 10
#define CLR_RESET "\033[0m"
#define CLR_DIM "\033[0;2m"

// #define CLR_DIM "\033[0;2m"
#define CLR_RED "\033[0;31m"
#define CLR_GREEN "\033[0;32m"
#define CLR_GREY "\033[0;37m"
#define CLR_BLACK "\033[0;30m"
#define CLR_YELLOW "\033[0;33m"
#define CLR_BLUE "\033[0;34m"
#define CLR_MAGENTA "\033[0;35m"
#define CLR_CYAN "\033[0;36m"
#define CLR_DARKGREY "\033[01;30m"
#define CLR_BRED "\033[01;31m"
#define CLR_BYELLOW "\033[01;33m"
#define CLR_BBLUE "\033[01;34m"
#define CLR_BMAGENTA "\033[01;35m"
#define CLR_BCYAN "\033[01;36m"
#define CLR_BGREEN "\033[01;32m"
#define CLR_WHITE "\033[01;37m"
#define CLR_REDBG "\033[0;41m"

class StarfishOutput : public Logger::Output {
public:
    void flush(std::stringstream& ss) override
    {
        // NOTE: We use stdout for now since there is no macro to print
        // a raw string only. e.g) STARFISH_LOG_INFO("%s", "blahblah");
        std::cout << ss.str();
    };

    static std::shared_ptr<StarfishOutput> instance()
    {
        static std::shared_ptr<StarfishOutput> output =
            std::make_shared<StarfishOutput>();
        return output;
    }
};

static std::random_device s_seed;
static std::mt19937 s_generator{ s_seed() };

static int randomNumber(int start, int end)
{
    std::uniform_int_distribution<std::string::size_type> range(start, end);
    return range(s_generator);
}

static std::string randomString(std::string::size_type length)
{
    static const char letters[] = "2345678";
    std::uniform_int_distribution<std::string::size_type> range(
        0, strnlen(letters, 20) - 1);
    std::string s;
    s.reserve(length);
    while (length--) {
        auto i = range(s_generator);
        s += letters[i];
    }
    return s;
}

static std::string randomColorCode()
{
    static const char* code[] = {
        CLR_RED,      CLR_GREEN, CLR_GREY,   CLR_YELLOW,  CLR_BLUE,
        CLR_MAGENTA,  CLR_CYAN,  CLR_BRED,   CLR_BYELLOW, CLR_BBLUE,
        CLR_BMAGENTA, CLR_BCYAN, CLR_BGREEN, CLR_WHITE,
    };
    int max = sizeof(code) / sizeof(code[0]) - 1;
    int index = randomNumber(0, max);
    return code[index];
}

static void writeProcessHeader(std::ostream& os, std::string resetCode)
{
    static auto processId = getpid();
    static std::string processIdColorCode = randomColorCode();
    static int thread_count = 0;

    static thread_local std::string thisThreadId;
    if (thisThreadId.empty()) {
        thisThreadId = ('a' + thread_count);
        thread_count = ++thread_count < 26 ? thread_count : 0;
    }

    os << "[" << processIdColorCode << processId << resetCode;
    os << "|" << thisThreadId << "] ";
}

static std::ostream& writeTimestamp(std::ostream& os)
{
    const auto now = std::chrono::system_clock::now();
    const auto s = std::chrono::duration_cast<std::chrono::seconds>(
                       now.time_since_epoch()) %
                   std::chrono::minutes(1);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) %
                    std::chrono::seconds(1);
    os << std::right << std::setfill('0') << std::setw(2) << s.count() << "."
       << std::setw(3) << ms.count() << " " << std::setw(0)
       << std::setfill(' ');

    return os;
}

static void writeTag(std::ostream& os, const std::string& tag)
{
    os << std::setw(TYPE_LENGTH_LIMIT) << tag << std::setw(0) << " ";
}

static void writeHeader(std::ostream& ss, const std::string& tag,
                        const std::string& id)
{
    ss << CLR_DIM;
    writeProcessHeader(ss, CLR_DIM);
    writeTimestamp(ss);
    ss << std::left << std::setfill(' ') << "("
       << std::setw(TRACE_ID_LENGTH_LIMIT)
       << std::string(id).substr(0, TRACE_ID_LENGTH_LIMIT) << ") ";
}

static const char* kNamespacePattern = "Starfish::";

Trace::Trace(std::string id, const char* functionName, const char* filename,
             const int line)
{
    if (!LogOption::isEnabled(id)) {
        return;
    }

    writeHeader(m_stream, "TRACE", id);
    m_stream << IndentCounter::getString(id)
             << createCodeLocation(functionName, filename, line,
                                   kNamespacePattern)
             << " " << CLR_RESET;
    initialize(StarfishOutput::instance());
}

Trace::Trace(std::string id)
{
    if (!LogOption::isEnabled(id)) {
        return;
    }

    writeHeader(m_stream, "INFO", id);
    initialize(StarfishOutput::instance());
}

#endif
