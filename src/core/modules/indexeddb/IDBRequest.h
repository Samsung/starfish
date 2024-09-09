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

#ifndef __StarfishIDBRequest__
#define __StarfishIDBRequest__

#include "core/dom/EventTarget.h"

namespace Starfish {

class ExecutionContext;
class DOMException;
class IDBObjectStore;
class IDBTransaction;
class IDBTaskQueueItem;

enum class IDBRequestErrorType : uint8_t {
    None,
    Unknown,
    VersionError,
    OverWriteError,
};

enum class IDBRequestReadyState : uint8_t {
    Pending,
    Done,
};

class IDBRequest : public EventTarget {
public:
    IDBRequest(ExecutionContext* executionContext);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isIDBRequest() const;
    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    virtual bool isOpenDBRequest()
    {
        return false;
    }

    static DOMException* errorCodeToDOMException(
        ExecutionContext* executionContext, IDBRequestErrorType error);

    void executeRequest(IDBObjectStore* source,
                        std::unique_ptr<IDBTaskQueueItem> operation);

    String* readyState() const;

    void success(ScriptValue result);
    void fail(DOMException* result);

    void dispatchSuccessEvent();
    void dispatchErrorEvent();

    DEFINE_GETTER_SETTER(ScriptValue, result, Result);
    DEFINE_GETTER(DOMException*, error);
    DEFINE_GETTER(ScriptValue, source);
    DEFINE_GETTER(IDBTransaction*, transaction);
    DEFINE_GETTER_SETTER(bool, processed, Processed);
    DEFINE_GETTER_SETTER(bool, done, Done);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(success);
    DECLARE_EVENT_LISTENER(error);
#undef VIRTUAL
#undef OVERRIDE

protected:
    ExecutionContext* m_executionContext;
    ScriptValue m_result;
    DOMException* m_error;
    ScriptValue m_source;
    IDBTransaction* m_transaction;
    IDBRequestReadyState m_readyState;
    bool m_processed;
    bool m_done;
};
} // namespace Starfish

#endif
#endif
