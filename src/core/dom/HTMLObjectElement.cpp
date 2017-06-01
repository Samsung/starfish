/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/HTMLObjectElement.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

String* HTMLObjectElement::localName()
{
    return starFish()->staticStrings()->m_objectTagName.localName();
}

QualifiedName HTMLObjectElement::name()
{
    return starFish()->staticStrings()->m_objectTagName;
}

void HTMLObjectElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_type) {
        if (m_content) {
            m_content->unload();
        }
        m_content = new MockHTMLObjectElementContent(this);
        m_content->load();
        setNeedsFrameTreeBuild();
    }
}

void MockHTMLObjectElementContent::drawContent(Canvas* canvas,
                                               const LayoutRect& contentRect,
                                               const LayoutRect& absContentRect)
{
    canvas->punchHole(Unit::Rect(contentRect.x(), contentRect.y(),
                                 contentRect.width(), contentRect.height()));
}
}
