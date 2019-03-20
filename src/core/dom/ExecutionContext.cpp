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

#include "StarfishBase.h"
#include "core/modules/profiling/Profiling.h"
#include "core/util/String.h"
#include "core/page/GlobalScope.h"
#include "core/dom/ExecutionContext.h"
#include "platform/loader/ResourceURL.h"

namespace Starfish {

ExecutionContext::ExecutionContext(GlobalScope* globalScope,
                                   ScriptBindingInstance* instance,
                                   ResourceURL* uri)
    : m_globalScope(globalScope)
    , m_scriptBindingInstance(instance)
    , m_createdTick(longTickCount())
    , m_documentURI(uri)
{
}

Document* ExecutionContext::asDocument()
{
    STARFISH_ASSERT(isDocument());
    return reinterpret_cast<Document*>(this);
}

WebBase* ExecutionContext::webBase() const
{
    return m_globalScope->webBase();
}

String* ExecutionContext::urlString()
{
    return m_documentURI->urlString();
}

#ifdef STARFISH_ENABLE_SERVICE_WORKER

ServiceWorker* ExecutionContext::activeServiceWorker() const
{
    return m_activeServiceWorker;
};

void ExecutionContext::setActiveServiceWorker(ServiceWorker* serviceWorker)
{
    m_activeServiceWorker = serviceWorker;
}

#endif /* STARFISH_ENABLE_SERVICE_WORKER */
}
