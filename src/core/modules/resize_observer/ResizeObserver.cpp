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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptBindingInstance.h"
#include "ResizeObserver.h"
#include "ResizeObserverOptions.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/page/Window.h"
#include "EscargotPublic.h"

namespace Starfish {

ResizeObserverCallback* ResizeObserverCallback::toResizeObserverCallback(
    ScriptValue fn)
{
    if (!isCallableScriptValue(fn)) {
        return nullptr;
    }
    return new ResizeObserverCallback(fn);
}

ResizeObserverCallback::ResizeObserverCallback(ScriptValue fn)
    : m_resizeObserverCallback(fn)
{
}

ResizeObserver::ResizeObserver(ExecutionContext* executionContext,
                               ResizeObserverCallback* callBack)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_callback(callBack)
{
}

void ResizeObserver::observe(Element* target)
{
    observe(target, ResizeObserverOptions());
}

void ResizeObserver::observe(Element* target, ResizeObserverOptions options)
{
    auto iter = std::find(m_targets.begin(), m_targets.end(), target);
    if (iter != m_targets.end()) {
        return;
    }

    target->markIsRegisteredToObserverBefore();

    ResizeObserverRegistration* resizeObserverRegistration =
        new ResizeObserverRegistration();

    resizeObserverRegistration->observer = this;
    target->appendResizeObserverRegistration(resizeObserverRegistration);
    m_targets.emplace_back(target);

    Document* document = target->document();
    if (!document->hasResizeObserver(this)) {
        document->addResizeObserver(this);
    }

    // trigger rendering
    document->window()->requestAnimationFrame([](void*) {}, nullptr);
}

void ResizeObserver::unobserve(Element* target)
{
    if (m_targets.size()) {
        target->removeResizeObserverRegistration(this);
        m_targets.erase(std::remove_if(m_targets.begin(), m_targets.end(),
                                       [target](const Element* item) {
                                           return item == target;
                                       }),
                        m_targets.end());
    }

    Document* document = target->document();
    if (!m_targets.size() && document->hasResizeObserver(this)) {
        document->removeResizeObserver(this);
    }
}

void ResizeObserver::disconnect()
{
    GCVector<Element*> targets = m_targets;
    for (auto* target : targets) {
        unobserve(target);
    }
    m_queuedEntries.clear();
}

GCVector<ResizeObserverEntry*> ResizeObserver::takeRecords()
{
    GCVector<ResizeObserverEntry*> records;
    records = m_queuedEntries;
    m_queuedEntries.clear();
    return records;
}

void ResizeObserver::notify()
{
    ScriptValue callback = m_callback->scriptValue();
    if (isCallableScriptValue(callback) && m_queuedEntries.size()) {
        ScriptValue* argv = nullptr;
        size_t argc = 0;
        const auto& result = Escargot::Evaluator::execute(
            scriptBindingInstance()->scriptContext(),
            [](Escargot::ExecutionStateRef* state,
               ResizeObserver* self) -> Escargot::ValueRef* {
                Escargot::ArrayObjectRef* arrayObj =
                    Escargot::ArrayObjectRef::create(state);
                for (size_t i = 0; i < self->m_queuedEntries.size(); i++) {
                    Escargot::ValueRef* item =
                        self->m_queuedEntries[i]->scriptValue();
                    arrayObj->set(state, Escargot::ValueRef::create(i), item);
                }
                return arrayObj;
            },
            this);
        argc = 1;
        argv = ALLOCA(sizeof(ScriptValue) * argc, ScriptValue);
        argv[0] = result.result;
        m_queuedEntries.clear();
        callScriptFunction(scriptBindingInstance(), callback, argv, 1,
                           scriptValue());
    }
}

void ResizeObserver::queueResizeObserverEntry(ResizeObserverEntry* entry)
{
    m_queuedEntries.emplace_back(entry);
}

} // namespace Starfish
