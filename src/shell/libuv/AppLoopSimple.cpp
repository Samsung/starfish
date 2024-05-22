/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "ShellConfig.h"

#if defined(STARFISH_SHELL_GLFW) || defined(STARFISH_SHELL_X11)
#include "AppLoop.h"

#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <memory.h>
#include <stdio.h>

namespace {

volatile sig_atomic_t doneFlag = 0;

void updateDoneFlagFromENV()
{
    if (getenv("SHELL_DONE_FLAG") && (atoi(getenv("SHELL_DONE_FLAG")) == 1)) {
        doneFlag = 1;
    }
}

void setDoneFlag(int sig, siginfo_t* siginfo, void* context)
{
    doneFlag = 1;
}

} // namespace

namespace StarfishShell {

AppLoop::AppLoop()
{
}

AppLoop::~AppLoop()
{
}

void AppLoop::init()
{
    // Do nothing.
}

int AppLoop::start()
{
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = setDoneFlag;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    while (!doneFlag) {
        usleep(100);
        updateDoneFlagFromENV();
    }

    return 0;
}

void AppLoop::stop()
{
    doneFlag = 1;
}

void AppLoop::deinit()
{
    // Do nothing.
}

} // namespace StarfishShell

#endif
