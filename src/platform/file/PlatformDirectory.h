/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPaltformDirectory__
#define __StarfishPaltformDirectory__

#include <GCUtil.h>

namespace Starfish {
class String;

class PlatformDirectory : public gc_cleanup {
public:
    static PlatformDirectory* create();
    static PlatformDirectory* createInNonGCArea();
    PlatformDirectory()
        : m_path(String::emptyString)
        , m_isOpen(false)
    {
    }
    virtual ~PlatformDirectory()
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
} // namespace Starfish
#endif
