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

namespace StarFish {

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
        FontFace* face = lookupCache(familyNameArray[i], size, style, weight);
        if (!face) {
            face = loadFontImpl(familyNameArray[i], size, style, weight);
            if (face) {
                m_fontCache.push_back(std::make_tuple(face, familyNameArray[i],
                                                      size, style, weight));
            }
        }

        if (face) {
            result->m_fontFaceList.push_back(face);
        }
    }

    String* familyName = String::fromUTF8("" STARFISH_DEFAULT_FONT_FAMILY);
    FontFace* face = lookupCache(familyName, size, style, weight);

    if (!face) {
        face = loadFontImpl(familyName, size, style, weight);
        if (face) {
            m_fontCache.push_back(
                std::make_tuple(face, familyName, size, style, weight));
        }
    }

    if (face) {
        result->m_fontFaceList.push_back(face);
    }

    result->m_spaceWidth = result->measureText(String::spaceString);
    return result;
}
};
