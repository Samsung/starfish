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

#if defined(STARFISH_WEBWORKER_HOST) && !defined(__FetchEvent__)
#define __FetchEvent__

#include "core/modules/serviceworker/ExtendableEvent.h"

namespace Starfish {

class FetchEventInit : ExtendableEventInit {
    STARFISH_MAKE_STACK_ALLOCATED();

public:
    FetchEventInit()
        : ExtendableEventInit()
    {
    }

    DEFINE_GETTER_SETTER(Request*, request, Request);
    DEFINE_GETTER_SETTER(Promise*, preloadResponse, PreloadResponse);
    DEFINE_GETTER_SETTER(String*, clientId, ClientId);
    DEFINE_GETTER_SETTER(String*, resultingClientId, ResultingClientId);
    DEFINE_GETTER_SETTER(String*, replacesClientId, ReplacesClientId);
    DEFINE_GETTER_SETTER(Promise*, handled, Handled);

private:
    Request* m_request;
    Promise* m_preloadResponse;
    String* m_clientId;
    String* m_resultingClientId;
    String* m_replacesClientId;
    Promise* m_handled;
};

class FetchEvent : public ExtendableEvent {
public:
    FetchEvent(ExecutionContext* executionContext, String* eventType)
        : ExtendableEvent(executionContext, eventType)
    {
    }

    FetchEvent(ExecutionContext* executionContext, String* eventType,
               const FetchEventInit& init)
        : ExtendableEvent(executionContext, eventType)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isFetchEvent() const override;

    void respondWith(Promise* response);

    // binding
    DEFINE_GETTER_SETTER(Request*, request, Request);
    DEFINE_GETTER_SETTER(Promise*, preloadResponse, PreloadResponse);
    DEFINE_GETTER_SETTER(String*, clientId, ClientId);
    DEFINE_GETTER_SETTER(String*, resultingClientId, ResultingClientId);
    DEFINE_GETTER_SETTER(String*, replacesClientId, ReplacesClientId);
    DEFINE_GETTER_SETTER(Promise*, handled, Handled);

    DEFINE_GETTER_SETTER(Response*, potentialResponse, PotentialResponse);
    DEFINE_GETTER_SETTER(bool, waitToRespond, WaitToRespond);
    DEFINE_GETTER_SETTER(bool, respondWithEntered, RespondWithEntered);
    DEFINE_GETTER_SETTER(bool, respondWithError, RespondWithError);

private:
    Request* m_request;
    Promise* m_preloadResponse;
    String* m_clientId;
    String* m_resultingClientId;
    String* m_replacesClientId;
    Promise* m_handled;

    Response* m_potentialResponse{ nullptr };
    bool m_waitToRespond{ false };
    bool m_respondWithEntered{ false };
    bool m_respondWithError{ false };
};

} // namespace Starfish

#endif
