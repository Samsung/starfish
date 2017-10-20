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
#include "Font.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

static String* removeQuoteFromName(String* name)
{
    auto bad = name->bufferAccessData();
    char32_t c = bad.charAt(0);
    if (c == '"') {
        if (bad.charAt(bad.length - 1) != '"') {
            return name;
        }
        if (bad.length > 2) {
            return name->substring(0, bad.length - 2);
        }
        return name;
    } else if (c == '\'') {
        if (bad.charAt(bad.length - 1) != '\'') {
            return name;
        }
        if (bad.length > 2) {
            return name->substring(0, bad.length - 2);
        }
        return name;
    }
    return name;
}

Font* FontSelector::loadFont(String* familyNameArray[],
                             size_t familyNameArraySize, float size, char style,
                             char weight)
{
#if defined(PORT_CANVAS_BACKEND_EFL)
    Font* result;
    if (isGenericFontSelector()) {
        result = Font::createGenericEmptyFont(this);
    } else {
        result = Font::createEmptyFont(this);
    }
#else
    Font* result = Font::createEmptyFont(this);
#endif

    for (size_t i = 0; i < familyNameArraySize; i++) {
        String* fm = removeQuoteFromName(familyNameArray[i]);
        bool exist;
        FontFace* face = lookupCache(fm, size, style, weight, exist);
        if (!exist) {
            face = loadFontImpl(fm, size, style, weight);
            m_fontCache.push_back(
                std::make_tuple(face, fm, size, style, weight));
        }

        if (face) {
            result->m_fontFaceList.push_back(face);
        }
    }

    String* familyName = g_initialFontFamilyDatas[1].m_familyName;
    bool exist;
    FontFace* face = lookupCache(familyName, size, style, weight, exist);

    if (!exist) {
        face = loadFontImpl(familyName, size, style, weight);
        m_fontCache.push_back(
            std::make_tuple(face, familyName, size, style, weight));
    }

    if (face) {
        result->m_fontFaceList.push_back(face);
    }

    STARFISH_RELEASE_ASSERT(result->m_fontFaceList.size() >= 1);

    result->m_spaceWidth = result->measureText(String::spaceString);
    return result;
}
};
