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

static UTF8StringDataNonGCStd mergeStyleWeightWithString(
    const UTF8StringDataNonGCStd& name, char style, char weight)
{
    UTF8StringDataNonGCStd result = name;
    result += " s:" + (style + 'a');
    result += " w:" + (weight + 'a');
    return result;
}

static UTF8StringDataNonGCStd mergeFamilyNames(String* familyNameArray[],
                                               size_t len, float size,
                                               char style, char weight)
{
    UTF8StringDataNonGCStd result;
    result.reserve(128);

    for (size_t i = 0; i < len; i++) {
        auto bad = familyNameArray[i]->bufferAccessData();
        for (size_t j = 0; j < bad.length; j++) {
            char32_t c = bad.charAt(j);
            if (c >= 'A' && c <= 'Z') {
                c -= ('A' - 'a');
            }

            if (LIKELY(c < 128)) {
                result += (char)c;
            } else {
                char buf[16];
                size_t l = utf32ToUtf8(c, buf);
                result.append(buf, l);
            }
        }
    }

    result += " s:" + std::to_string(int(size + 0.5f));
    result += " s:" + (style + 'a');
    result += " w:" + (weight + 'a');
    return result;
}

static UTF8StringDataNonGCStd mergeFamilyNames(String* familyNameArray[],
                                               size_t len, char style,
                                               char weight)
{
    UTF8StringDataNonGCStd result;
    result.reserve(128);

    for (size_t i = 0; i < len; i++) {
        auto bad = familyNameArray[i]->bufferAccessData();
        for (size_t j = 0; j < bad.length; j++) {
            char32_t c = bad.charAt(j);
            if (c >= 'A' && c <= 'Z') {
                c -= ('A' - 'a');
            }

            if (LIKELY(c < 128)) {
                result += (char)c;
            } else {
                char buf[16];
                size_t l = utf32ToUtf8(c, buf);
                result.append(buf, l);
            }
        }
    }

    result += " s:" + (style + 'a');
    result += " w:" + (weight + 'a');
    return result;
}

FontFace* FontSelector::lookupFaceCache(
    const UTF8StringDataNonGCStd& familyName, char style, char weight,
    bool& exist)
{
    auto iter = m_fontFaceCache.find(familyName);
    if (iter == m_fontFaceCache.end()) {
        exist = false;
        return nullptr;
    } else {
        exist = true;
        return iter->second;
    }
}

void FontSelector::insertFaceCache(const UTF8StringDataNonGCStd& familyName,
                                   char style, char weight, FontFace* face)
{
    STARFISH_ASSERT(m_fontFaceCache.find(familyName) == m_fontFaceCache.end());
    m_fontFaceCache.insert(std::make_pair(familyName, face));
}

static bool isGenericFontName(const UTF8StringDataNonGCStd& familyName)
{
    bool isGenericName = false;
    if (familyName == "sans") {
        isGenericName = true;
    } else if (familyName == "sans-serif") {
        isGenericName = true;
    } else if (familyName == "serif") {
        isGenericName = true;
    } else if (familyName == "monospace") {
        isGenericName = true;
    } else if (familyName == "fantasy") {
        isGenericName = true;
    } else if (familyName == "cursive") {
        isGenericName = true;
    }

    return isGenericName;
}

Font* FontSelector::loadFont(String* familyNameArray[],
                             size_t familyNameArraySize, float size, char style,
                             char weight)
{
    auto cacheFontName = mergeFamilyNames(familyNameArray, familyNameArraySize,
                                          size, style, weight);
    auto iter = m_fontCache.find(cacheFontName);
    if (iter != m_fontCache.end()) {
        return iter->second;
    }

    Font* result = Font::createEmptyFont(this);

    auto cacheFontListName =
        mergeFamilyNames(familyNameArray, familyNameArraySize, style, weight);
    auto iter2 = m_fontFaceListCache.find(cacheFontListName);
    if (iter2 != m_fontFaceListCache.end()) {
        result->m_fontFaceList = iter2->second;
        result->m_size = size;
        result->m_spaceWidth = result->measureText(String::spaceString);
        return result;
    }

    result->m_fontFaceList = new FontFaceList;

    for (size_t i = 0; i < familyNameArraySize; i++) {
        auto fm = familyNameArray[i]->toUTF8NonGCString();
        std::transform(fm.begin(), fm.end(), fm.begin(), ::tolower);
        UTF8StringDataNonGCStd cacheStr =
            mergeStyleWeightWithString(fm, style, weight);

        bool exist;
        FontFace* face = lookupFaceCache(cacheStr, style, weight, exist);
        if (!exist) {
            bool g = isGenericFontName(fm);
            face = loadFontFaceImpl(fm, g, style, weight);
            insertFaceCache(cacheStr, style, weight, face);
        }

        if (face) {
            result->m_fontFaceList->push_back(face);
        }
    }

    UTF8StringDataNonGCStd familyName =
        g_initialFontFamilyDatas[1].m_familyName->toUTF8NonGCString();
    UTF8StringDataNonGCStd cacheStr =
        mergeStyleWeightWithString(familyName, style, weight);
    bool exist;
    FontFace* face = lookupFaceCache(cacheStr, style, weight, exist);

    if (!exist) {
        bool g = isGenericFontName(familyName);
        face = loadFontFaceImpl(familyName, g, style, weight);
        insertFaceCache(cacheStr, style, weight, face);
    }

    STARFISH_RELEASE_ASSERT(face);

    result->m_fontFaceList->push_back(face);
    result->m_size = size;
    result->m_spaceWidth = result->measureText(String::spaceString);

    m_fontFaceListCache.insert(
        std::make_pair(cacheFontListName, result->m_fontFaceList));
    m_fontCache.insert(std::make_pair(cacheFontName, result));

    return result;
}
};
