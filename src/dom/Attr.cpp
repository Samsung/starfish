/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
#include "dom/Attr.h"
#include "dom/Document.h"
#include "dom/Element.h"

namespace StarFish {

String* Attr::value() const
{
    if (m_element) {
        return m_element->getAttribute(m_qname);
    }
    return m_standAloneValue;
}

void Attr::setValue(String* value)
{
    if (m_element) {
        m_element->setAttribute(m_qname, value);
    } else {
        m_standAloneValue = value;
    }
}

Node* Attr::clone()
{
    return (Node*)(new Attr(document(), m_qname, value()));
}
}
