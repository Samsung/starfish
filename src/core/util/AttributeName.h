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

#ifndef __StarFishAttributeName__
#define __StarFishAttributeName__

namespace StarFish {

class AttributeName {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    enum MatchType {
        MatchName, /* Match name in the HTML namespace and its node document is
                      an HTML document */
        MatchNS,   /* Match namespace and localname */
        MatchAll   /* Match all (internal use) */
    };

    explicit AttributeName(const QualifiedName& name,
                           MatchType type = MatchAll);

    bool isNamespaceAware() const
    {
        return m_type == MatchNS || m_type == MatchAll;
    }

    bool isMatch(const QualifiedName& qname) const;

    QualifiedName qname() const
    {
        return m_qname;
    }

private:
    MatchType m_type;
    QualifiedName m_qname;
};
}
#endif
