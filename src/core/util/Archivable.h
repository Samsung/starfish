/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(__StarfishArchivable__)
#define __StarfishArchivable__

#include "core/util/Id.h"
#include "core/util/Archiver.h"

namespace Starfish {

class Archivable : public gc {
public:
    virtual ~Archivable()
    {
    }
    virtual const char* archiveId() const = 0;
    virtual void archive(Archiver& ar) = 0;
};

// GenericArchivable

#define MAX_TYPE_NAME 10

struct TypeName {
    static const char String[];
};

template <typename T>
class GenericArchivable : public Archivable {
public:
    GenericArchivable(const char* archiveId)
    {
        STARFISH_ASSERT(archiveId != nullptr);
        strncpy(m_archiveId, archiveId, MAX_TYPE_NAME);
    }

    GenericArchivable(const char* archiveId, T value)
    {
        STARFISH_ASSERT(archiveId != nullptr);
        strncpy(m_archiveId, archiveId, MAX_TYPE_NAME);
        m_value = value;
    }

    const char* archiveId() const override
    {
        return m_archiveId;
    }

    void archive(Archiver& ar) override
    {
        ar.Member("value") & m_value;
    }

    DEFINE_GETTER_SETTER(T, value, Value);

private:
    char m_archiveId[MAX_TYPE_NAME]{};
    T m_value{};
};

using StringArchivable = GenericArchivable<String*>;

} // namespace Starfish

#endif
