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

#ifndef __StarfishResizeObserver__
#define __StarfishResizeObserver__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

class ResizeObserverCallback {
public:
    static ResizeObserverCallback* toResizeObserverCallback(ScriptValue fn)
    {
        if (!isCallableScriptValue(fn)) {
            return nullptr;
        }
        return new ResizeObserverCallback(fn);
    }

private:
    ResizeObserverCallback(ScriptValue fn)
        : m_callback(fn)
    {
    }

    ScriptValue m_callback;
};

class ResizeObserver : public ScriptWrappable {
public:
    ResizeObserver(ExecutionContext* executionContext,
                   ResizeObserverCallback* callBack)
        : ScriptWrappable(this)
        , m_scriptBindingInstance(executionContext->scriptBindingInstance())
    {
    }

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    void init(ScriptBindingInstance*, void*) override;

    bool isResizeObserver() const override;

    void observe(Element* target)
    {
        STARFISH_UNIMPLEMENTED();
    }

    void observe(Element* target, ResizeObserverOptions options)
    {
        STARFISH_UNIMPLEMENTED();
    }

    void unobserve(Element* target)
    {
        STARFISH_UNIMPLEMENTED();
    }

    void disconnect()
    {
        STARFISH_UNIMPLEMENTED();
    }

private:
    ScriptBindingInstance* m_scriptBindingInstance;
};
} // namespace Starfish

#endif
