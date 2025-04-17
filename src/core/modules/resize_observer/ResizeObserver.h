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
#include "binding/generated/ElementOrDocumentUnion.h"
#include "ResizeObserverEntry.h"
#include "ResizeObserverOptions.h"

namespace Starfish {

class ResizeObserverCallback : public gc {
public:
    static ResizeObserverCallback* toResizeObserverCallback(ScriptValue fn);
    ScriptValue scriptValue()
    {
        return m_resizeObserverCallback;
    }

private:
    ResizeObserverCallback(ScriptValue fn);
    ScriptValue m_resizeObserverCallback;
};

class ResizeObserver : public ScriptWrappable {
public:
    ResizeObserver(ExecutionContext* executionContext,
                   ResizeObserverCallback* callBack);

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_executionContext->scriptBindingInstance();
    }

    void init(ScriptBindingInstance*, void*) override;

    bool isResizeObserver() const override;

    void observe(Element* target);
    void observe(Element* target, ResizeObserverOptions options);
    void unobserve(Element* target);
    void disconnect();

    GCVector<ResizeObserverEntry*> takeRecords();
    bool hasRecords()
    {
        return !m_queuedEntries.empty();
    }
    void notify();
    GCVector<Element*>& targets()
    {
        return m_targets;
    }
    void queueResizeObserverEntry(ResizeObserverEntry* entry);

private:
    ExecutionContext* m_executionContext;
    Optional<ElementOrDocument> m_root;
    ResizeObserverCallback* m_callback = nullptr;
    GCVector<Element*> m_targets;
    GCVector<ResizeObserverEntry*> m_queuedEntries;
};

struct ResizeObserverRegistration : public gc {
    ResizeObserver* observer = nullptr;
    LayoutSize previousSize = { LayoutUnit::min(), LayoutUnit::min() };
};

} // namespace Starfish

#endif
