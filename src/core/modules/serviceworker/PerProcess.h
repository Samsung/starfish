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

#include <GCUtil.h>
#include <StarfishBase.h>

namespace Starfish {

class IThread;
class ThreadPool;
class IORunnable;
class MessageLoop;

class PerProcess : public gc {
public:
    PerProcess();
    ~PerProcess() = default;

    DEFINE_GETTER(IORunnable *, ioRunnable);
    DEFINE_GETTER(ThreadPool *, threadPool);
    DEFINE_GETTER(MessageLoop *, messageLoop);

    void initialize();
    void destroy();

private:
    MessageLoop *m_messageLoop{ nullptr };
    ThreadPool *m_threadPool{ nullptr };
    IThread *m_ioThread{ nullptr };
    IORunnable *m_ioRunnable{ nullptr };
};

} // namespace Starfish
