/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFetch__
#define __StarfishFetch__

#include "binding/generated/RequestOrUSVStringUnion.h"

namespace Starfish {

extern RequestOrUSVString toRequestOrUSVStringFromValueRef(
    ExecutionStateRef* state, ValueRef* from);

class Request;

class Fetch {
    Fetch(ExecutionContext* executionContext, Request* request,
          Promise* promise);

public:
    static Promise* fetch(ExecutionContext* executionContext,
                          RequestInfo& input);
    static Promise* fetch(ExecutionContext* executionContext,
                          RequestInfo& input, RequestInit& init);

    void start();
    void success(ResourceRequest* request);
    void fail();
    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

private:
    ExecutionContext* m_executionContext;
    Request* m_request;
    Response* m_response;
    ResourceRequest* m_resourceRequest;
    Promise* m_promise;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(Fetch, m_executionContext));
        GC_set_bit(desc, GC_WORD_OFFSET(Fetch, m_request));
        GC_set_bit(desc, GC_WORD_OFFSET(Fetch, m_response));
        GC_set_bit(desc, GC_WORD_OFFSET(Fetch, m_resourceRequest));
        GC_set_bit(desc, GC_WORD_OFFSET(Fetch, m_promise));
    }
};
} // namespace Starfish

#endif
