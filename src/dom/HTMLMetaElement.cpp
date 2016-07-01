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
#include "dom/Document.h"
#include "HTMLMetaElement.h"

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
#endif

void HTMLMetaElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_name) {
#ifdef STARFISH_ENABLE_TEST
        if (value->equals("pixel-test")) {
            g_enablePixelTest = true;
            document()->setStyle(document()->styleResolver()->resolveDocumentStyle(document()));
            document()->window()->setWholeDocumentNeedsStyleRecalc();
        }
#endif
    }

}

}
