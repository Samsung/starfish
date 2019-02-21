/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarfishLogger__
#define __StarfishLogger__

namespace Starfish {

typedef void (*MessageWritter)(void* data);

class Logger {
public:
    Logger()
        : m_isInstalled(false)
        , m_data(nullptr)
        , m_indentCounter(nullptr)
        , m_indent(0)
    {
    }

    ~Logger()
    {
        if (m_isInstalled) {
            writeIndent();
            m_outgoingMessageWritter(m_data);
            (*m_indentCounter)--;
        }
        m_data = nullptr;
        m_incommingMessageWritter = nullptr;
        m_outgoingMessageWritter = nullptr;
    }

    void installLogger()
    {
        m_isInstalled = true;
        m_indent = (*m_indentCounter)++;
        writeIndent();
        m_incommingMessageWritter(m_data);
    }

    void setUserData(void* data)
    {
        m_data = data;
    }

    void setIndentCounter(int* indentCounter)
    {
        m_indentCounter = indentCounter;
    }
    void setIncommingMessageWritter(MessageWritter writter)
    {
        m_incommingMessageWritter = writter;
    }

    void setOutgoingMessageWritter(MessageWritter writter)
    {
        m_outgoingMessageWritter = writter;
    }

private:
    void writeIndent()
    {
        for (int i = 0; i < m_indent; ++i) {
            printf("  ");
        }
    }

    bool m_isInstalled;
    void* m_data;
    int* m_indentCounter;
    int m_indent;
    MessageWritter m_incommingMessageWritter;
    MessageWritter m_outgoingMessageWritter;
};

class LoggerBuilder {
public:
    LoggerBuilder()
    {
    }

    virtual ~LoggerBuilder()
    {
    }

    std::unique_ptr<Logger> logger()
    {
        return std::move(m_logger);
    }

    void createNewLogger()
    {
        m_logger.reset(new Logger());
    }

    virtual void buildUserData() = 0;
    virtual void buildIndentCounter() = 0;
    virtual void buildIncommingMessageWritter() = 0;
    virtual void buildOutgoingMessageWritter() = 0;

protected:
    std::unique_ptr<Logger> m_logger;
};

class LoggerDirector {
public:
    LoggerDirector()
        : m_loggerBuilder(nullptr)
    {
    }

    ~LoggerDirector()
    {
    }

    void setLoggerBuilder(LoggerBuilder* builder)
    {
        m_loggerBuilder = builder;
    }

    std::unique_ptr<Logger> constructLogger()
    {
        m_loggerBuilder->createNewLogger();
        m_loggerBuilder->buildUserData();
        m_loggerBuilder->buildIndentCounter();
        m_loggerBuilder->buildIncommingMessageWritter();
        m_loggerBuilder->buildOutgoingMessageWritter();
        return m_loggerBuilder->logger();
    }

private:
    LoggerBuilder* m_loggerBuilder;
};
}
#endif
