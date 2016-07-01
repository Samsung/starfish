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
#include "Text.h"
#include "HTMLElement.h"


namespace StarFish {

void HTMLElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    Element::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_onclick) {
        setAttributeEventListener(ss->m_click, value, this);
    } else if (name == ss->m_onload) {
        setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onunload) {
        setAttributeEventListener(ss->m_unload, value, this);
    } else if (name == ss->m_dir) {
        if (attributeCreated)
            m_hasDirAttribute = true;
        if (attributeRemoved)
            m_hasDirAttribute = false;
        String* orgValue = value;
        value = value->toLower();
        if (value->equals("")) {
            return;
        } else if (value->equals("ltr") || value->equals("rtl")) {
            if (!orgValue->equals(value))
                setAttribute(ss->m_dir, value);
        } else {
            setAttribute(ss->m_dir, String::emptyString);
        }
    }
}

}
