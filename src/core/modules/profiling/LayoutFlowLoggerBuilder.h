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

#ifndef __StarfishLayoutFlowLoggerBuilder__
#define __StarfishLayoutFlowLoggerBuilder__

#include "Logger.h"

namespace Starfish {

class Frame;

struct UserData {
    Frame* m_frame;
    const char* incommingMessage;
    const char* infoMessage;
    const char* outgoingMessage;
    int resolveWhat;
};

class LayoutFlowLoggerBuilder : public LoggerBuilder {
public:
    static int INDENT_COUNTER;

    LayoutFlowLoggerBuilder(UserData& UserData);
    ~LayoutFlowLoggerBuilder();

    virtual void buildUserData() override;
    virtual void buildIndentCounter() override;
    virtual void buildIncommingMessageWritter() override;
    virtual void buildOutgoingMessageWritter() override;

private:
    UserData m_userData;
};

#define INSTALL_LAYOUT_FLOW_LOGGER(ptr, incomming, info, outgoing,       \
                                   resolveWhat)                          \
    UserData userData = { ptr, incomming, info, outgoing, resolveWhat }; \
    LayoutFlowLoggerBuilder builder(userData);                           \
    LoggerDirector director;                                             \
    director.setLoggerBuilder(&builder);                                 \
    auto tracker = director.constructLogger();                           \
    tracker->installLogger();
}

#endif
