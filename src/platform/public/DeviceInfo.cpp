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

#include "StarfishConfig.h"

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "platform/public/DeviceInfo.h"

namespace Starfish {

bool DeviceInfo::getLocalIPAddress(std::string queriedInfName,
                                   std::string& ipAddressQueried)
{
    int sock;
    char buffer[5000];
    struct ifconf ifconf;
    const char* ifname = queriedInfName.c_str();
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in* s_in;
    int ifs, i;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        goto error;
    }

    ifconf.ifc_buf = buffer;
    ifconf.ifc_len = sizeof(buffer);

    // get a list of interface
    if (ioctl(sock, SIOCGIFCONF, &ifconf) == -1) {
        goto error;
    }

    ifs = ifconf.ifc_len / sizeof(ifconf.ifc_req[0]);

    for (i = 0; i < ifs; i++) {
        // skip local loopback address
        if (ioctl(sock, SIOCGIFFLAGS, &ifconf.ifc_req[i]) == -1) {
            goto error;
        }

        if ((ifconf.ifc_req[i].ifr_flags & IFF_LOOPBACK) != 0) {
            continue;
        }

        s_in = (struct sockaddr_in*)(&ifconf.ifc_req[i].ifr_addr);
        if (inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip)) == nullptr) {
            goto error;
        }

        STARFISH_LOG_INFO("%s - inet addr:%s\n", ifconf.ifc_req[i].ifr_name,
                          ip);

        // NOTE: if the given inteface name is empty, we currently return the
        // first matched interface's IP.
        if ((queriedInfName.empty() == true) ||
            (strncmp(ifconf.ifc_req[i].ifr_name, ifname,
                     queriedInfName.length()) == 0)) {
            ipAddressQueried = ip;
            close(sock);
            return true;
        }
    }

error:
    close(sock);

    return false;
}

} // namespace Starfish
