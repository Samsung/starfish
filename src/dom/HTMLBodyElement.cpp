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

#include "StarFishConfig.h"
#include "Document.h"
#include "HTMLBodyElement.h"

namespace StarFish {

void HTMLBodyElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_onload) {
        document()->window()->setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onunload) {
        document()->window()->setAttributeEventListener(ss->m_unload, value, this);
    }
}

}
