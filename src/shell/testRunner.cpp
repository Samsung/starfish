/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "shell/testRunner.h"

#ifdef STARFISH_ENABLE_TEST
namespace StarFish {

void testRunner::dumpAsText()
{
    g_enableDumpAsText = true;
}

void testRunner::waitUntilDone()
{
    g_DumpAsText_Async = true;
}

void testRunner::notifyDone()
{
    g_DumpAsText_Async = false;
}
}
#endif
