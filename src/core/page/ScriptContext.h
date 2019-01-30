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

#ifndef __StarfishScriptContext__
#define __StarfishScriptContext__

namespace Starfish {

class ServiceWorker;
class ScriptBindingInstance;
class MessageLoop;

class ScriptContext {
public:
    ScriptContext(MessageLoop* messageLoop)
        : m_messageLoop(messageLoop)
        , m_scriptBindingInstance(nullptr)
    {
    }

    virtual ~ScriptContext()
    {
    }

    virtual bool isDocument() const
    {
        return false;
    }
    virtual bool isWorkerGlobalScope() const
    {
        return false;
    }

    void setScriptBindingInstance(ScriptBindingInstance* scriptBindingInstance);
    ScriptBindingInstance* scriptBindingInstance() const;
    MessageLoop* messageLoop() const;

protected:
    MessageLoop* m_messageLoop;
    ScriptBindingInstance* m_scriptBindingInstance;

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
