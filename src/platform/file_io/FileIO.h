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

#ifndef __FileIO__
#define __FileIO__

#include "StarFishConfig.h"

namespace StarFish {

class Window;
class FileIO : public gc_cleanup {
protected:
    FileIO() { }
public:
    static FileIO* create();
    static FileIO* createInNonGCArea();
    virtual ~FileIO()
    {

    }

    bool open(String* filePath)
    {
        return open(filePath->utf8Data());
    }

    virtual bool open(const char* filePath) = 0;
    virtual long int length() = 0;
    virtual size_t read(void* buf, size_t size, size_t count) = 0;
    virtual int close() = 0;
};

class PathResolver {
public:
    PathResolver() = delete;
    static String* matchLocation(String* filePath);
};

}

#endif
