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

#ifndef __StarFishStorageNamespaceProvider__
#define __StarFishStorageNamespaceProvider__

#include "binding/StarFishHoldable.h"

namespace StarFish {

class StorageNamespace;

class StorageNamespaceProvider : public StarFishHoldable, public gc {
public:
    virtual ~StorageNamespaceProvider()
    {
    }

    virtual StorageNamespace* createLocalStorageNamespace() = 0;
    virtual StorageNamespace* createSessionStorageNamespace() = 0;

protected:
    StorageNamespaceProvider(StarFish* starfish)
        : StarFishHoldable(starfish)
    {
    }
};
}

#endif
