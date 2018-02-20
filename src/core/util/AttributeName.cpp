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
