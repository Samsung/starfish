/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "core/dom/AbortSignal.h"
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include <EscargotPublic.h>

namespace Starfish {

AbortSignal::AbortSignal(ExecutionContext* executionContext)
    : EventTargetWithExecutionContext(executionContext)
    , m_reason(scriptUndefined())
{
}

bool AbortSignal::aborted() const
{
    return !m_reason->isUndefined();
}

void AbortSignal::setAbortReason(ScriptValue reason)
{
    // https://dom.spec.whatwg.org/#abortsignal-abort-reason
    // Only undefined selects the default exception; null is a valid reason.
    m_reason =
        reason->isUndefined()
            ? (new DOMException(executionContext(), DOMException::ABORT_ERR))
                  ->scriptValue()
            : reason;
}

AbortSignal* AbortSignal::abort(ExecutionContext* executionContext,
                                ScriptValue reason)
{
    auto signal = new AbortSignal(executionContext);
    // A signal created already aborted must not dispatch an abort event.
    signal->setAbortReason(reason);
    return signal;
}

void AbortSignal::signalAbort(ScriptValue reason)
{
    if (aborted()) {
        return;
    }
    setAbortReason(reason);
    // Set the reason before synchronous dispatch, including reentrant abort().
    // https://dom.spec.whatwg.org/#run-the-abort-steps
    dispatchEventByUA(
        new Event(executionContext(), staticStrings()->m_abort.localName()));
}

void AbortSignal::throwIfAborted()
{
    if (aborted()) {
        throwScriptException(scriptBindingInstance(), m_reason);
    }
}

EventListener* AbortSignal::onabort()
{
    return attributeEventListener(staticStrings()->m_abort);
}

void AbortSignal::setOnabort(EventListener* listener)
{
    if (listener) {
        setAttributeEventListener(staticStrings()->m_abort, listener);
    } else {
        clearAttributeEventListener(staticStrings()->m_abort);
    }
}

} // namespace Starfish
