/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishMimeType__
#define __StarFishMimeType__

namespace StarFish {

class String;

class MimeType {
public:
    MimeType();

    static MimeType parseFromString(String* str);

    bool isValid();
    bool hasParameter();

    String* type()
    {
        return m_type;
    }
    String* subtype()
    {
        return m_subtype;
    }
    String* parameter()
    {
        return m_parameter;
    }
    void setType(String* type)
    {
        m_type = type;
    }
    void setSubtype(String* subtype)
    {
        m_subtype = subtype;
    }
    void setParameter(String* param)
    {
        m_parameter = param;
    }

    String* string();
    void clear();

protected:
    String* m_type;
    String* m_subtype;
    // TODO : store parameter as dictionary
    String* m_parameter;
};
}

#endif
