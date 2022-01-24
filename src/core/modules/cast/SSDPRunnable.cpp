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

#ifdef STARFISH_ENABLE_CAST_SERVICE

#include "StarfishConfig.h"

#include <arpa/inet.h>

#include "core/modules/cast/CastConfig.h"
#include "core/modules/cast/SSDPRunnable.h"

#define MAX_BUFFER_SIZE 5000
#define RECV_SLEEP_MS 300

namespace Starfish {

SSDPRunnable::SSDPRunnable(MessageLoop *messageLoop, CastConfig *config)
    : BaseRunnable(messageLoop)
    , m_config(config)
{
    STARFISH_ASSERT(messageLoop != nullptr);
    STARFISH_ASSERT(config != nullptr);
}

bool SSDPRunnable::preRun()
{
    bool isInitialized = initSocket();
    return isInitialized;
}

bool SSDPRunnable::doRun()
{
    struct sockaddr_in srcAddr;
    socklen_t addrLen = sizeof(srcAddr);

    auto options = GlobalOptions::instance();
    std::vector<char> buffer(MAX_BUFFER_SIZE, 0x00);
    std::string receivedString;
    bool hasTargetAddr = options.has("DEBUG_CAST_TARGET_IP");
    std::string clientAddr(options.get("DEBUG_CAST_TARGET_IP"));

    char msearchResData[sizeof(CastConfig::templateMSearchResponse) +
                        MAX_BUFFER_SIZE] = {
        0,
    };

    auto localAddrString = m_config->localAddress();

    int msearchResDataLen = snprintf(
        msearchResData, sizeof(msearchResData),
        CastConfig::templateMSearchResponse,
        localAddrString->toUTF8NonGCString().c_str(), LOCATION_PORT, "");

    CAST_LOG_IF_ALLOWED(1, COLOR_YELLOW "%s", msearchResData);

    while (isStopRequested() == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(RECV_SLEEP_MS));

        int nbytes = recvfrom(m_socket, &(buffer[0]), MAX_BUFFER_SIZE - 1, 0,
                              (struct sockaddr *)&srcAddr, &addrLen);
        if (nbytes < 0) {
            continue;
        } else {
            buffer[nbytes] = '\0';
            receivedString.assign(&(buffer[0]), buffer.size());

            // check if M-SEARCH body expected is valid
            if ((receivedString.find("M-SEARCH") != 0) ||
                (receivedString.find(SSDP_ST) == std::string::npos)) {
                continue;
            }

            if ((hasTargetAddr == false) ||
                (strncmp(inet_ntoa(srcAddr.sin_addr), clientAddr.c_str(),
                         clientAddr.length()) == 0)) {
                CAST_RECV_LOG_IF_ALLOWED(3, "\n%s", receivedString.c_str());
            }

            // send a response of M-SEARCH
            if (sendto(m_socket, msearchResData, msearchResDataLen, 0,
                       (struct sockaddr *)&srcAddr, addrLen) == -1) {
                STARFISH_LOG_WARN("FAILED: Responding to %s:%d",
                                  inet_ntoa(srcAddr.sin_addr),
                                  ntohs(srcAddr.sin_port));
                return false;
            } else {
                if ((hasTargetAddr == false) ||
                    (strncmp(inet_ntoa(srcAddr.sin_addr), clientAddr.c_str(),
                             clientAddr.length()) == 0)) {
                    CAST_SEND_LOG_IF_ALLOWED(3, "HTTP/1.1 200 OK: to %s:%d",
                                             inet_ntoa(srcAddr.sin_addr),
                                             ntohs(srcAddr.sin_port));
                }
            }
        }
    }

    return false;
}

bool SSDPRunnable::initSocket()
{
    struct sockaddr_in socketAddr;
    struct ip_mreq mreq;
    int allowMultipleSocketsToUseTheSamePort = 1;
    auto localAddrString = m_config->localAddress();

    m_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

    if (m_socket < 0) {
        return false;
    }

    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&allowMultipleSocketsToUseTheSamePort,
                   sizeof(allowMultipleSocketsToUseTheSamePort)) < 0) {
        return false;
    }

    // setup socket address
    memset(&socketAddr, 0, sizeof(socketAddr));
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_addr.s_addr = inet_addr(SSDP_GROUP);
    socketAddr.sin_port = htons(SSDP_PORT);

    if (bind(m_socket, (struct sockaddr *)&socketAddr, sizeof(socketAddr)) <
        0) {
        return false;
    }

    // join the multicast group
    mreq.imr_multiaddr.s_addr = inet_addr(SSDP_GROUP);
    mreq.imr_interface.s_addr =
        inet_addr(localAddrString->toUTF8NonGCString().c_str());
    if (setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
                   sizeof(mreq)) < 0) {
        return false;
    }

    return true;
}

} // namespace Starfish

#endif
