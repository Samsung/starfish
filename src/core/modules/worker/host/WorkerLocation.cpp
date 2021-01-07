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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "core/modules/worker/host/WorkerLocation.h"
#include "platform/loader/ResourceURL.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

WorkerLocation::WorkerLocation(ExecutionContext* executionContext,
                               ResourceURL* url)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_url(url)
{
    STARFISH_ASSERT(executionContext != nullptr && url != nullptr);
}

ScriptBindingInstance* WorkerLocation::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

String* WorkerLocation::href()
{
    return m_url->href();
}

String* WorkerLocation::origin()
{
    return m_url->origin();
}

String* WorkerLocation::protocol()
{
    return m_url->protocol();
}

String* WorkerLocation::host()
{
    return m_url->host();
}

String* WorkerLocation::hostname()
{
    return m_url->hostname();
}

String* WorkerLocation::port()
{
    return m_url->port();
}

String* WorkerLocation::pathname()
{
    return m_url->pathname();
}

String* WorkerLocation::search()
{
    return m_url->search();
}

String* WorkerLocation::hash()
{
    return m_url->hash();
}
} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
