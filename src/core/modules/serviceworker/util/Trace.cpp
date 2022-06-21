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
#if !defined(NDEBUG)

#include "core/modules/serviceworker/util/Trace.h"
#include "StarfishBase.h"

#include <iomanip> // setfill and setw

#define TYPE_LENGTH_LIMIT 5
#define TRACE_ID_LENGTH_LIMIT 9
#define CLR_RESET "\033[0m"
#define CLR_DIM "\033[0;2m"

class StarfishOutput : public Logger::Output {
public:
    void flush(std::stringstream& ss) override
    {
        // TODO: We use stdout for now since there is no macro to print a raw
        // string only. e.g) STARFISH_LOG_INFO("%s", ss.str().c_str());
        std::cout << ss.str();
    };

    static std::shared_ptr<StarfishOutput> instance()
    {
        static std::shared_ptr<StarfishOutput> output =
            std::make_shared<StarfishOutput>();
        return output;
    }
};

static std::string randomString(std::string::size_type length)
{
    static const char* letters = "0123456789";
    thread_local static std::mt19937 mt{ std::random_device{}() };
    thread_local static std::uniform_int_distribution<std::string::size_type>
        dist(0, sizeof(letters) - 1);

    std::string s;
    s.reserve(length);
    while (length--) {
        s += letters[dist(mt)];
    }
    return s;
}

static void writeThreadHeader(std::ostream& os)
{
    static thread_local std::string thisThreadId;
    if (thisThreadId.empty()) {
        thisThreadId = randomString(2);
    }
    os << "[" << thisThreadId << "] ";
}

static void writeHeader(std::ostream& ss, const std::string& tag,
                        const std::string& id)
{
    writeThreadHeader(ss);
    ss << CLR_DIM << std::left << std::setfill(' ')
       << std::setw(TYPE_LENGTH_LIMIT) << tag << std::setw(0) << " ("
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
