/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPTargetDomain__)
#define __StarfishCDPTargetDomain__

#include <string>
#include <vector>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;
class CDPSession;
class TargetContext;
class Document;

// Engine lifecycle bridge; ignores detached and non-top-level documents.
void notifyTargetInfoChanged(Document* document);

class TargetDomain {
public:
    TargetDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);
    void notifyTargetInfoChanged(TargetContext* ctx);
    void reset();

private:
    struct Subscription {
        std::string sessionId;
        bool discover = false;
        bool autoAttach = false;
        bool waitForDebugger = false;
        bool allowPage = true;
        bool allowTab = false;
    };
    Subscription& subscription(const std::string& sessionId);
    bool canAutoAttach(const Subscription& subscriber,
                       CDPSession* session) const;
    void rememberTargetInfo(TargetContext* ctx);
    std::vector<Subscription> m_subscriptions;
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
