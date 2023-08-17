/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#pragma once

#if defined(STARFISH_ENABLE_SERVICE_WORKER)

#include "StarfishConfig.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace Starfish {

class GlobalScope;

class IdleTask : public gc {
public:
    IdleTask(GlobalScope* globalScope);

    static void queue(IdleTask* task);

    virtual void run() = 0;
    virtual void end(){};
    void start();

    GlobalScope* globalScope()
    {
        return m_globalScope;
    }

private:
    GlobalScope* m_globalScope;
};

#if defined(STARFISH_WEBWORKER_HOST)

class ParallelTask : public gc {
public:
    static void queue(ParallelTask* task);

    virtual void run() = 0;
    virtual void end(){};
    void start();
};

#endif

#endif
}
