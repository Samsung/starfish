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

#ifndef __StarFishDirectory__
#define __StarFishDirectory__

namespace StarFish {

class Directory : public gc_cleanup {
public:
    static Directory* create();
    static Directory* createInNonGCArea();
    Directory()
        : m_path(String::emptyString)
        , m_isOpen(false)
    {
    }
    virtual ~Directory()
    {
    }

    virtual bool open(String* path) = 0;
    virtual bool mkDir() = 0;
    virtual bool close() = 0;
    virtual void removeDir() = 0;
    virtual void clearDir() = 0;
    virtual bool isOpen() = 0;
    virtual size_t fileCount() = 0;

protected:
    String* m_path;
    bool m_isOpen;
};
} // namespace StarFish
#endif
