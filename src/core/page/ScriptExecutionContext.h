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

#ifndef __StarfishScriptExecutionContext__
#define __StarfishScriptExecutionContext__

namespace Starfish {

class ServiceWorker;

class ScriptExecutionContext {
    virtual bool isDocument() const
    {
        return false;
    }
    virtual bool isWorkerGlobalScope() const
    {
        return false;
    }

#ifdef STARFISH_ENABLE_SERVICE_WORKER
public:
    ServiceWorker* activeServiceWorker() const;
    void setActiveServiceWorker(ServiceWorker* serviceWorker);

private:
    ServiceWorker* m_activeServiceWorker{ nullptr };
#endif
};

} // end of namespace Starfish

#endif
