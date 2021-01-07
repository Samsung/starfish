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

#if defined(STARFISH_WEBWORKER_HOST) && !defined(__StarfishWorkerLocation__)
#define __StarfishWorkerLocation__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class ResourceURL;

class WorkerLocation : public ScriptWrappable {
public:
    WorkerLocation(ExecutionContext* executionContext, ResourceURL* url);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(WorkerLocation)

    String* href();
    String* origin();
    String* protocol();
    String* host();
    String* hostname();
    String* port();
    String* pathname();
    String* search();
    String* hash();

private:
    ExecutionContext* m_executionContext;
    ResourceURL* m_url;
};
} // namespace Starfish

#endif
