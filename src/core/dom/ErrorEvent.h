/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishErrorEvent__
#define __StarFishErrorEvent__

#include "binding/ScriptWrappable.h"
#include "core/dom/Event.h"

namespace StarFish {

class ErrorData {
    friend ErrorEvent;
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    ErrorData()
        : m_message(String::emptyString)
        , m_filename(String::emptyString)
        , m_lineno(0)
        , m_colno(0)
        , m_error(scriptUndefined())
    {
    }
    String* message()
    {
        return m_message;
    }
    void setMessage(String* message)
    {
        m_message = message;
    }
    String* filename() const
    {
        return m_filename;
    }
    void setFilename(String* filename)
    {
        m_filename = filename;
    }
    uint32_t lineno() const
    {
        return m_lineno;
    }
    void setLineno(uint32_t lineno)
    {
        m_lineno = lineno;
    }
    uint32_t colno() const
    {
        return m_colno;
    }
    void setColno(uint32_t colno)
    {
        m_colno = colno;
    }
    ScriptValue error() const
    {
        return m_error;
    }
    void setError(ScriptValue error)
    {
        m_error = error;
    }

private:
    String* m_message;
    String* m_filename;
    uint32_t m_lineno;
    uint32_t m_colno;
    ScriptValue m_error;
};

class ErrorEventInit : public EventInit, public ErrorData {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    ErrorEventInit()
        : EventInit()
        , ErrorData()
    {
    }
};

class ErrorEvent : public Event {
protected:
public:
    ErrorEvent(Document* document)
        : Event(document)
        , m_errorData()
    {
    }

    ErrorEvent(Document* document, String* eventType)
        : Event(document, eventType)
        , m_errorData()
    {
    }

    ErrorEvent(Document* document, String* eventType, ErrorEventInit& init)
        : Event(document, eventType, init)
        , m_errorData(init)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isErrorEvent() const override;

    // Read-only interface
    String* message()
    {
        return m_errorData.m_message;
    }
    String* filename() const
    {
        return m_errorData.m_filename;
    }
    uint32_t lineno() const
    {
        return m_errorData.m_lineno;
    }
    uint32_t colno() const
    {
        return m_errorData.m_colno;
    }
    ScriptValue error() const
    {
        return m_errorData.m_error;
    }

private:
    ErrorData m_errorData;
};
}

#endif
