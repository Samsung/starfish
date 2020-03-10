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

#if defined(STARFISH_ENABLE_CAST_SERVICE) && !defined(__StarfishSSDPRunnable__)
#define __StarfishSSDPRunnable__

#include <netinet/in.h>
#include "core/modules/cast/BaseRunnable.h"

namespace Starfish {

class CastConfig;

class SSDPRunnable : public BaseRunnable {
public:
    SSDPRunnable(MessageLoop* messageLoop, CastConfig* config);
    bool preRun() override;
    bool doRun() override;

private:
    bool initSocket();
    int m_socket{ -1 };
    CastConfig* m_config;
};

} // namespace Starfish

#endif
