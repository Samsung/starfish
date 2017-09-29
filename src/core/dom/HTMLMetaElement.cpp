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

void* HTMLMetaElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLMetaElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLMetaElement, m_name));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLMetaElement, m_content));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLMetaElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
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
    } else if (name == starFish()->staticStrings()->m_httpEquiv) {
        m_httpEquiv = value;
    }

    if (!m_httpEquiv->isEmpty() &&
        m_httpEquiv->equalsIgnoreCase("content-language") &&
        !m_content->isEmpty()) {
        document()->setContentLanguage(m_content);
    }
}

void HTMLMetaElement::didNodeInsertedToDocumentTree()
{
    checkPlatformFlags();
}

void HTMLMetaElement::didNodeRemovedFromDocumentTree()
{
    checkPlatformFlags();
}

void HTMLMetaElement::checkPlatformFlags()
{
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
#ifdef STARFISH_ENABLE_TEST
        if (m_name->equalsIgnoreCase("pixel-test")) {
            g_enablePixelTest = true;
            document()->setStyle(
                document()->styleResolver().resolveDocumentStyle(document()));
            window()->browsingContext()->setWholeDocumentNeedsStyleRecalc();
        }
#endif
#ifdef STARFISH_TIZEN
        if ((m_name->equalsIgnoreCase("tizen-transparent-background") ||
             m_name->equalsIgnoreCase("tizen-widget-transparent-background")) &&
            m_content->equalsIgnoreCase("yes")) {
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
