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

#include "core/dom/HTMLMetaElement.h"

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
extern bool g_enablePixelTest;
#endif

String* HTMLMetaElement::localName()
{
    return starFish()->staticStrings()->m_metaTagName.localName();
}

QualifiedName HTMLMetaElement::name()
{
    return starFish()->staticStrings()->m_metaTagName;
}

void HTMLMetaElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_name) {
        m_name = value;
        checkPlatformFlags();
    } else if (name == starFish()->staticStrings()->m_content) {
        m_content = value;
        checkPlatformFlags();
    }
}

void HTMLMetaElement::didNodeInsertedToDocumenTree()
{
    checkPlatformFlags();
}

void HTMLMetaElement::didNodeRemovedFromDocumenTree()
{
    checkPlatformFlags();
}

void HTMLMetaElement::checkPlatformFlags()
{
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
#ifdef STARFISH_ENABLE_TEST
        if (m_name->equalsWithoutCase("pixel-test")) {
            g_enablePixelTest = true;
            document()->setStyle(
                document()->styleResolver().resolveDocumentStyle(document()));
            window()->browsingContext()->setWholeDocumentNeedsStyleRecalc();
        }
#endif
#ifdef STARFISH_TIZEN
        if (m_name->equalsWithoutCase("tizen-widget-transparent-background") &&
            m_content->equalsWithoutCase("yes")) {
            if (!m_tizenWidgetTransparentBackground) {
                document()->m_tizenWidgetTransparentBackground++;
                setNeedsPainting();
            }
            m_tizenWidgetTransparentBackground = true;
        } else {
            if (m_tizenWidgetTransparentBackground) {
                document()->m_tizenWidgetTransparentBackground--;
                setNeedsPainting();
            }
            m_tizenWidgetTransparentBackground = false;
        }
#endif

    } else {
    }
}
}
