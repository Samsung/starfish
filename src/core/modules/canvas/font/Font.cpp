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
#include "core/style/WebFont.h"
#include "core/dom/Document.h"

namespace StarFish {

static UTF8StringDataNonGCStd mergeStyleWeightWithString(
    const UTF8StringDataNonGCStd& name, char style, char weight)
{
    UTF8StringDataNonGCStd result = name;
    result += "@s:";
    result += (style + 'a');
    result += "@w:";
    result += (weight + 'a');
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

    result += "@s:" + std::to_string(int(size + 0.5f));
    result += "@s:";
    result += (style + 'a');
    result += "@w:";
    result += (weight + 'a');
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

    result += "@s:";
    result += (style + 'a');
    result += "@w:";
    result += (weight + 'a');
    return result;
}

FontFace* PlatformFontCache::lookupFaceCache(
    const UTF8StringDataNonGCStd& mergredFamilyName, bool& exist)
{
    auto iter = m_loadedPlatformFonts.find(mergredFamilyName);
    if (iter == m_loadedPlatformFonts.end()) {
        exist = false;
        return nullptr;
    } else {
        exist = true;
        return iter->second;
    }
}

void PlatformFontCache::insertFaceCache(
    const UTF8StringDataNonGCStd& mergredFamilyName, FontFace* face)
{
    STARFISH_ASSERT(m_loadedPlatformFonts.find(mergredFamilyName) ==
                    m_loadedPlatformFonts.end());
    m_loadedPlatformFonts.insert(std::make_pair(mergredFamilyName, face));
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

FontFace* FontSelector::loadFromPlatform(const UTF8StringDataNonGCStd& fm,
                                         bool isGenericName, char style,
                                         char weight)
{
    if (m_platformFontCache->m_absencePlatformFontNames.find(fm) !=
        m_platformFontCache->m_absencePlatformFontNames.end()) {
        // early give up
        return nullptr;
    } else {
        auto fontPath =
            m_platformFontSelector->findFont(fm, isGenericName, style, weight);
        if (fontPath.length() == 0) {
            m_platformFontCache->m_absencePlatformFontNames.insert(fm);
            return nullptr;
        } else {
            return m_platformFontSelector->loadFontFace(fontPath);
        }
    }
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
    result->m_style = style;
    result->m_weight = weight;
    result->m_size = size;

    auto cacheFontListName =
        mergeFamilyNames(familyNameArray, familyNameArraySize, style, weight);
    auto iter2 = m_fontFaceListCache.find(cacheFontListName);
    if (iter2 != m_fontFaceListCache.end()) {
        result->m_fontFaceList = iter2->second;
        result->m_spaceWidth = result->measureText(String::spaceString);
        return result;
    }

    result->m_fontFaceList = new FontFaceList;

    for (size_t i = 0; i < familyNameArraySize; i++) {
        auto fm = familyNameArray[i]->toUTF8NonGCString();
        std::transform(fm.begin(), fm.end(), fm.begin(), ::tolower);
        UTF8StringDataNonGCStd cacheStr =
            mergeStyleWeightWithString(fm, style, weight);

        bool existInPlatformLayerCache = false;
        bool isGenericName = isGenericFontName(fm);

        FontFace* face = m_platformFontCache->lookupFaceCache(
            cacheStr, existInPlatformLayerCache);

        if (existInPlatformLayerCache) {
            if (face) {
                result->m_fontFaceList->push_back(face);
                continue;
            }
        } else {
            face = loadFromPlatform(fm, isGenericName, style, weight);
            if (face) {
                m_platformFontCache->insertFaceCache(cacheStr, face);
                result->m_fontFaceList->push_back(face);
                continue;
            } else {
                m_platformFontCache->insertFaceCache(cacheStr, nullptr);
            }
        }

        // search webfont Path
        int fitScore = 0;
        WebFont* selectedWebFont = nullptr;

        for (size_t k = 0; k < document()->m_webFontList.size(); k++) {
            auto& webFont = document()->m_webFontList[k];
            if (webFont.familyName()->equalsIgnoreCase(familyNameArray[i])) {
                int currentScore = 1;

                if (webFont.isFontStyleSpecified() &&
                    (char)webFont.fontStyleValue() == style) {
                    currentScore++;
                }

                if (webFont.isFontWeightSpecified() &&
                    webFont.fontWeightValue() == weight) {
                    currentScore++;
                }

                if (currentScore > fitScore) {
                    selectedWebFont = &webFont;
                    fitScore = currentScore;
                }
            }
        }
        if (selectedWebFont) {
            if (!selectedWebFont->fromLocal()) {
                // download from web
                if (selectedWebFont->fontResource()->fontFace()) {
                    // fontface loaded!
                    face = selectedWebFont->fontResource()->fontFace();
                    result->m_fontFaceList->push_back(face);
                } else if (!selectedWebFont->fontResource()
                                ->isFaildToFetchResource()) {
                    if (result->m_fontFaceList->m_seenUnresolvedWebFontIndex ==
                        SIZE_MAX) {
                        result->m_fontFaceList->m_seenUnresolvedWebFontIndex =
                            result->m_fontFaceList->size();
                    }
                }
            } else {
                // local font
                auto fm = selectedWebFont->localFontName()->toUTF8NonGCString();
                std::transform(fm.begin(), fm.end(), fm.begin(), ::tolower);

                auto iter = m_webFontLocalSrcCache.find(cacheStr);
                if (iter == m_webFontLocalSrcCache.end()) {
                    face = loadFromPlatform(fm, isGenericName, style, weight);
                    m_webFontLocalSrcCache.insert(
                        std::make_pair(cacheStr, face));
                } else {
                    face = iter->second;
                }

                if (face) {
                    result->m_fontFaceList->push_back(face);
                }
            }
        }
    }

    UTF8StringDataNonGCStd familyName =
        g_initialFontFamilyDatas[1].m_familyName->toUTF8NonGCString();
    UTF8StringDataNonGCStd cacheStr =
        mergeStyleWeightWithString(familyName, style, weight);

    bool existInPlatformLayerCache = false;
    FontFace* face = m_platformFontCache->lookupFaceCache(
        cacheStr, existInPlatformLayerCache);

    if (!existInPlatformLayerCache) {
        bool g = isGenericFontName(familyName);
        auto fontPath =
            m_platformFontSelector->findFont(familyName, g, style, weight);
        STARFISH_RELEASE_ASSERT(fontPath.length());
        face = m_platformFontSelector->loadFontFace(fontPath);
        m_platformFontCache->insertFaceCache(cacheStr, face);
    }

    STARFISH_RELEASE_ASSERT(face != nullptr);

    result->m_fontFaceList->push_back(face);
    result->m_spaceWidth = result->measureText(String::spaceString);

    m_fontFaceListCache.insert(
        std::make_pair(cacheFontListName, result->m_fontFaceList));
    m_fontCache.insert(std::make_pair(cacheFontName, result));

    return result;
}

template <typename T>
void cacheDeleter(T& cache, const UTF8StringDataNonGCStd& str)
{
    auto iter = cache.begin();
    while (iter != cache.end()) {
        if (iter->first.find(str) != std::string::npos) {
            iter = cache.erase(iter);
        } else {
            iter++;
        }
    }
}

void FontSelector::clearCache(String* relatedFamilyName)
{
    auto fm = relatedFamilyName->toUTF8NonGCString();
    std::transform(fm.begin(), fm.end(), fm.begin(), ::tolower);
    cacheDeleter(m_fontFaceListCache, fm);
    cacheDeleter(m_fontCache, fm);
    cacheDeleter(m_webFontLocalSrcCache, fm);
}
};
