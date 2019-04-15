/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishGlobalScope__
#define __StarfishGlobalScope__

#include "core/util/Id.h"

namespace Starfish {

class WebBase;
class ScriptBindingInstance;
class ExecutionContext;

// TODO: Move WindowOrWorkerGlobalScope feature included in Window class
class GlobalScope {
public:
    virtual ~GlobalScope()
    {
    }

    virtual ExecutionContext* executionContext() const = 0;

    DEFINE_GETTER(WebBase*, webBase);
    DEFINE_GETTER(Id<GlobalScope>, uid);

protected:
    GlobalScope(WebBase* webBase)
        : m_webBase(webBase)
    {
        m_uid = Id<GlobalScope>::generate();
    }

private:
    WebBase* m_webBase;
    Id<GlobalScope> m_uid;
};
}

#endif
