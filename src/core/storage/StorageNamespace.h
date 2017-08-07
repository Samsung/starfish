/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishWebStorageNamespace__
#define __StarFishWebStorageNamespace__

#include "binding/WindowHoldable.h"

namespace StarFish {

class SecurityOriginData;
class Storage;
class Window;

class StorageNamespace : public gc {
public:
    virtual Storage* storage(Window* window, SecurityOriginData* origin) = 0;
    virtual void clearWindow(Window* window) = 0;
    virtual ~StorageNamespace()
    {
    }

protected:
    StorageNamespace()
    {
    }
};
}

#endif
