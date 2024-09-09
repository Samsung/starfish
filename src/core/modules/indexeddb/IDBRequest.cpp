/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_IDB)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/Event.h"
#include "core/dom/DOMException.h"
#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/IDBObjectStore.h"
#include "core/modules/indexeddb/IDBTaskQueue.h"
#include "core/modules/indexeddb/IDBTransaction.h"
#include "core/modules/indexeddb/IDBRequest.h"

namespace Starfish {

IDBRequest::IDBRequest(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_result(scriptUndefined())
    , m_error(nullptr)
    , m_source(scriptNull())
    , m_transaction(nullptr)
    , m_readyState(IDBRequestReadyState::Pending)
    , m_processed(false)
    , m_done(false)
{
}

DOMException* IDBRequest::errorCodeToDOMException(
    ExecutionContext* executionContext, IDBRequestErrorType error)
{
    if (error == IDBRequestErrorType::Unknown) {
        return new DOMException(executionContext,
                                DOMException::Code::DOM_EXCEPTION,
                                "Unknown Error");
    } else if (error == IDBRequestErrorType::VersionError) {
        return new DOMException(
            executionContext,
            String::createASCIIString(
                "The requested version is less than the existing version."),
            String::createASCIIString("VersionError"));
    } else if (error == IDBRequestErrorType::OverWriteError) {
        return new DOMException(
            executionContext,
            String::createASCIIString("The data cannot be overwritten."),
            String::createASCIIString("ConstraintError"));
    }

    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

String* IDBRequest::readyState() const
{
    if (m_readyState == IDBRequestReadyState::Pending) {
        return String::createASCIIString("pending");
    } else if (m_readyState == IDBRequestReadyState::Done) {
        return String::createASCIIString("done");
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}

void IDBRequest::executeRequest(IDBObjectStore* source,
                                std::unique_ptr<IDBTaskQueueItem> operation)
{
    // https://w3c.github.io/IndexedDB/#asynchronously-execute-a-request

    m_transaction = source->transaction();

    STARFISH_ASSERT(m_transaction->state() == IDBTransaction::State::Active);

    m_transaction->addRequest(this);

    IDBStorageManager::instance().taskQueue()->addTask(std::move(operation));
}

void IDBRequest::success(ScriptValue result)
{
    // https://w3c.github.io/IndexedDB/#fire-a-success-event

    STARFISH_ASSERT(m_executionContext->isContextThread());

    m_result = result;
    // TODO: Set request’s error to undefined.

    STARFISH_ASSERT(m_transaction);
    if (m_transaction->state() == IDBTransaction::State::Inactive) {
        m_transaction->setState(IDBTransaction::State::Active);
    }

    dispatchSuccessEvent();

    m_transaction->setState(IDBTransaction::State::Inactive);

    // TODO: 8-3. If transaction’s request list is empty, then run commit a
    // transaction with transaction.
}

void IDBRequest::fail(DOMException* result)
{
    // https://w3c.github.io/IndexedDB/#fire-an-error-event

    STARFISH_ASSERT(m_executionContext->isContextThread());
    m_result = scriptUndefined();
    m_error = result;

    STARFISH_ASSERT(m_transaction);
    if (m_transaction->state() == IDBTransaction::State::Inactive) {
        m_transaction->setState(IDBTransaction::State::Active);
    }

    dispatchErrorEvent();

    m_transaction->setState(IDBTransaction::State::Inactive);

    // TODO: 8-3. If event’s canceled flag is false, then run abort a
    // transaction using transaction and request's error, and terminate these
    // steps.

    // TODO: 8-4. If transaction’s request list is empty, then run commit a
    // transaction with transaction.
}

void IDBRequest::dispatchSuccessEvent()
{
    Event* event =
        new Event(m_executionContext, staticStrings()->m_success.localName());

    EventTarget::dispatchEventIdleTimeByUA(event);
}

void IDBRequest::dispatchErrorEvent()
{
    Event* event =
        new Event(m_executionContext, staticStrings()->m_error.localName(),
                  EventInit(true, true));

    EventTarget::dispatchEventIdleTimeByUA(event);
}

DEFINE_EVENT_LISTENER(IDBRequest, success);
DEFINE_EVENT_LISTENER(IDBRequest, error);

} // namespace Starfish

#endif
