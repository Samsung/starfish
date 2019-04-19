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

struct TypeName {
    static const char String[];
};

template <typename T>
class GenericArchivable : public Archivable {
public:
    GenericArchivable(const char* archiveId)
        : m_archiveId(archiveId)
    {
    }

    GenericArchivable(const char* archiveId, T value)
    {
        m_archiveId = archiveId;
        m_value = value;
    }

    const char* archiveId() const override
    {
        return m_archiveId.c_str();
    }

    void archive(Archiver& ar) override
    {
        ar.Member("value") & m_value;
    }

    DEFINE_GETTER_SETTER(T, value, Value);

private:
    T m_value{};
    std::string m_archiveId;
};

} // namespace Starfish

#endif
