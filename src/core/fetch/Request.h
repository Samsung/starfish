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

#ifndef __StarfishRequest__
#define __StarfishRequest__

#include "binding/ScriptWrappable.h"
#include "core/fetch/RequestInit.h"
#include "core/fetch/Body.h"
#include "core/fetch/RequestData.h"

namespace Starfish {

class RequestOrUSVString;
typedef RequestOrUSVString RequestInfo;

class Request : public ScriptWrappable, public Body {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;

    virtual bool isRequest() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return WindowHoldable::scriptBindingInstance();
    }

    Request(Window* window, RequestInfo& input);
    Request(Window* window, RequestInfo& input, RequestInit& init);

    String* method();
    String* url();
    String* destination();
    String* referrer();
    String* referrerPolicy();

    String* mode();
    String* credentials();
    String* cache();
    String* redirect();
    String* integrity();

    Headers* headers();

    bool keepalive();
    bool isReloadNavigation();
    bool isHistoryNavigation();

    Request* clone();

private:
    Request(ScriptBindingInstance* instance, RequestData* data);
    void initialize(RequestInfo* input, RequestInit* init = nullptr);

protected:
    RequestData m_data;
    Headers m_headers;
};
}

#endif
