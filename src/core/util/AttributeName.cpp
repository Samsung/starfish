/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/util/AttributeName.h"

namespace StarFish {

AttributeName::AttributeName(const QualifiedName& name, MatchType type)
    : m_type(type)
    , m_qname(name)
{
}

bool AttributeName::isMatch(const QualifiedName& qname) const
{
    switch (m_type) {
    case MatchType::MatchName:
        if (UNLIKELY(qname.hasPrefix())) {
            return m_qname.toString()->equals(qname.toString());
        }
        return m_qname.equalsLocalName(qname);
    case MatchType::MatchNS:
        return m_qname.equalsNamespace(qname) && m_qname.equalsLocalName(qname);
    case MatchType::MatchAll:
        return m_qname == qname;
    }
    return false;
}
}
