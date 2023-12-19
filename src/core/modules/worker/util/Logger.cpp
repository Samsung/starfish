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

#include "StarfishConfig.h"
#include "core/modules/worker/util/Logger.h"
#include "core/util/GlobalOptions.h"

#include <map>
#include <thread>
#include <regex>
#include <set>

// --- Formatter ---

std::string getPrettyFunctionName(const std::string fullname,
                                  std::string prefixPattern)
{
    std::stringstream ss;
    if (!prefixPattern.empty()) {
        ss << "(?:" << prefixPattern << ")|";
    }
    ss << R"((?::\()|([\w:~]+)\()";

    try {
        std::smatch match;
        const std::regex re(ss.str());

        std::stringstream result;
        std::string suffix = fullname;
        while (std::regex_search(suffix, match, re)) {
            result << match[1];
            suffix = match.suffix();
        }
        return result.str();
    } catch (std::regex_error& e) {
        return "";
    }
}

std::string createCodeLocation(const char* functionName, const char* filename,
                               const int line, std::string prefixPattern)
{
    std::ostringstream oss;
    oss << getPrettyFunctionName(functionName, prefixPattern) << " ("
        << filename << ":" << line << ")";
    return oss.str();
}

void writeThreadIdentifier(std::ostream& os)
{
    static int s_id = 0;
    static thread_local int thisThreadId = 0;

    if (thisThreadId == 0) {
        thisThreadId = ++s_id;
    }
    os << "[" << thisThreadId << "] ";
}

std::function<bool(const std::string&)> LogOption::s_externalIsEnabled =
    [](const std::string& id) -> bool {
    if (Starfish::GlobalOptions::instance().has("TRACE", id.c_str())) {
        return true;
    }
    return false;
};

void LogOption::setExternalIsEnabled(
    std::function<bool(const std::string&)> func)
{
    s_externalIsEnabled = func;
}

// --- LogOption ---

bool LogOption::isEnabled(const std::string& pattern)
{
    if (!s_externalIsEnabled) {
        return false;
    }
    return s_externalIsEnabled(pattern);
}

// --- Logger::Header ---

void Logger::Header::write(std::stringstream& stream)
{
    writeThreadIdentifier(stream);
    writeHeader(stream);
}

// --- Logger ---

Logger::Logger(const std::string& header, std::shared_ptr<Output> out)
    : m_output(out)
{
    m_stream << header;
    initialize(m_output);
}

Logger::Logger(Header&& header, std::shared_ptr<Output> out)
    : m_output(out)
{
    header.write(m_stream);
    initialize(m_output);
}

Logger::~Logger()
{
    if (m_output == nullptr) {
        return;
    }
    // stream ends with both reset-styles and endl characters.
    m_stream << "\033[0m" << std::endl;
    m_output->flush(m_stream);
}

void Logger::initialize(std::shared_ptr<Output> out)
{
    static thread_local std::shared_ptr<StdOut> s_loggerOutput;

    if (out == nullptr) {
        if (s_loggerOutput == nullptr) {
            s_loggerOutput = std::make_shared<StdOut>();
        }
        m_output = s_loggerOutput;
    } else {
        m_output = out;
    }
}

Logger& Logger::print(const char* string_without_format_specifiers)
{
    if (m_output == nullptr) {
        return *this;
    }

    while (*string_without_format_specifiers) {
        if (*string_without_format_specifiers == '%' &&
            *(++string_without_format_specifiers) != '%') {
            assert(((void)"runtime error: invalid format-string", false));
        }
        m_stream << *string_without_format_specifiers++;
    }
    return *this;
}

Logger& Logger::flush()
{
    if (m_output) {
        m_output->flush(m_stream);
    }
    m_stream.str("");
    return *this;
}
// --- Output ---

void StdOut::flush(std::stringstream& stream)
{
    std::cout << stream.str();
}

// --- Utils ---

thread_local int s_indentCount = 0;
thread_local int s_deltaCount = 0;

void IndentCounter::indent(std::string id)
{
    if (!LogOption::isEnabled(id)) {
        return;
    }
    s_deltaCount++;
}

void IndentCounter::unIndent(std::string id)
{
    if (!LogOption::isEnabled(id)) {
        return;
    }
    s_deltaCount--;
}

IndentCounter::IndentCounter(std::string id)
{
    m_id = id;

    if (!LogOption::isEnabled(id)) {
        return;
    }
    s_indentCount++;
}

IndentCounter::~IndentCounter()
{
    if (!LogOption::isEnabled(m_id)) {
        return;
    }
    s_indentCount--;
}

std::string IndentCounter::getString(std::string id)
{
    assert(s_indentCount >= 0);

    std::ostringstream oss;
    int indentCount = s_indentCount + s_deltaCount;

    if (s_deltaCount > 0) {
        oss << s_deltaCount << " ";
    }

    for (int i = 1; i < std::min(30, indentCount); ++i) {
        oss << "  ";
    }

    return oss.str();
}
