/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#if defined(STARFISH_ENABLE_INSPECTOR) && !defined (__StarFishInspector__)
#define __StarFishInspector__

#include <zmq.hpp>
#include "platform/threading/Thread.h"

namespace StarFish {

class Inspector : public gc {
public:
    Inspector(StarFish* starFish, uint32_t portNumber);
    ~Inspector();

    void sendInfoMessage(String* m);
    void sendErrorMessage(String* m);
    void sendWarnMessage(String* m);
protected:
    StarFish* m_starFish;
    zmq::context_t m_zmqContext;
    zmq::socket_t m_zmqSocket;
    Thread* m_ioThread;
};

}

#endif
