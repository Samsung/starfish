/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "Font.h"
#include "Starfish.h"
#include "core/style/ComputedStyle.h"
#include "core/style/WebFont.h"
#include "core/style/FontFaceSet.h"
#include "core/dom/Document.h"
#include "core/page/WebView.h"
#include "platform/loader/ResourceLoader.h"

#include <algorithm>

namespace Starfish {

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

template <typename T>
std::string to_string(T value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}

static UTF8StringDataNonGCStd mergeFamilyNames(String* familyNameArray[],
                                               size_t len, float size,
                                               char style, char weight,
                                               float letterSpacing,
                                               FontKerningValue kerningValue)
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

    result += "@s:" + ::Starfish::to_string(int(size + 0.5f));
    result += "@ls:" + ::Starfish::to_string(letterSpacing);
    result += "@s:";
    result += (style + 'a');
    result += "@w:";
    result += (weight + 'a');
    result += "@k:";
    result += (kerningValue + 'a');
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

FontFace* FontSelector::loadLocalWebFont(String* localFontName, char style,
                                         char weight)
{
    auto localFm = localFontName->toUTF8NonGCString();
    std::transform(localFm.begin(), localFm.end(), localFm.begin(), tolower);
    UTF8StringDataNonGCStd localCacheStr =
        mergeStyleWeightWithString(localFm, style, weight);

    auto iter = m_webFontLocalSrcCache.find(localCacheStr);
    if (iter != m_webFontLocalSrcCache.end()) {
        return iter->second;
    }
    FontFace* face =
        loadFromPlatform(localFm, isGenericFontName(localFm), style, weight);
    m_webFontLocalSrcCache.insert(std::make_pair(localCacheStr, face));
    return face;
}

bool FontFaceList::mayCover(size_t index, char32_t ch) const
{
    if (!m_unicodeRanges || index >= m_unicodeRanges->size()) {
        return true;
    }
    UnicodeRangeList* ranges = (*m_unicodeRanges)[index];
    return !ranges || ranges->contains(ch);
}

void FontFaceList::pushRangedFace(FontFace* face, UnicodeRangeList* ranges)
{
    insertRangedFace(size(), face, ranges);
}

void FontFaceList::insertRangedFace(size_t position, FontFace* face,
                                    UnicodeRangeList* ranges)
{
    if (!m_unicodeRanges) {
        m_unicodeRanges = new GCVector<UnicodeRangeList*>();
    }
    while (m_unicodeRanges->size() < size()) {
        m_unicodeRanges->push_back(nullptr);
    }
    insert(begin() + position, face);
    m_unicodeRanges->insert(m_unicodeRanges->begin() + position, ranges);
    if (m_pendingRangedFonts) {
        for (auto& pending : *m_pendingRangedFonts) {
            if (pending.m_position >= position) {
                pending.m_position++;
            }
        }
    }
}

void FontFaceList::addPendingRangedFont(UnicodeRangeList* ranges,
                                        FontResource* resource)
{
    if (!m_pendingRangedFonts) {
        m_pendingRangedFonts = new GCVector<PendingRangedFont>();
    }
    m_pendingRangedFonts->push_back({ ranges, resource, size() });
}

void FontFaceList::rebuildPendingCoverage()
{
    if (!m_pendingCoverage) {
        m_pendingCoverage = new GCAtomicVector<std::pair<char32_t, char32_t>>();
    }
    m_pendingCoverage->clear();
    for (const auto& pending : *m_pendingRangedFonts) {
        for (const auto& range : *pending.m_ranges) {
            m_pendingCoverage->push_back(range);
        }
    }
    std::sort(m_pendingCoverage->begin(), m_pendingCoverage->end());
    size_t merged = 0;
    for (size_t i = 0; i < m_pendingCoverage->size(); i++) {
        auto range = (*m_pendingCoverage)[i];
        if (merged &&
            range.first <= (*m_pendingCoverage)[merged - 1].second + 1) {
            auto& last = (*m_pendingCoverage)[merged - 1];
            last.second = std::max(last.second, range.second);
        } else {
            (*m_pendingCoverage)[merged++] = range;
        }
    }
    m_pendingCoverage->resize(merged);
}

bool FontFaceList::pendingRangedFontsCover(char32_t ch) const
{
    if (!hasPendingRangedFonts()) {
        return false;
    }
    auto iter =
        std::upper_bound(m_pendingCoverage->begin(), m_pendingCoverage->end(),
                         std::make_pair(ch, (char32_t)0x10FFFF));
    return iter != m_pendingCoverage->begin() && ch <= (iter - 1)->second;
}

static int webFontMatchScore(WebFont& webFont, char style, char weight)
{
    int score = 1;
    if (webFont.isFontStyleSpecified() &&
        (char)webFont.fontStyleValue() == style) {
        score++;
    }
    if (webFont.isFontWeightSpecified() &&
        webFont.fontWeightValue() == weight) {
        score++;
    }
    return score;
}

static void requestWebFont(Document* document, FontResource* resource)
{
    if (resource->isRequested()) {
        return;
    }
    RequestData* reqData = new RequestData();
    reqData->m_url = resource->url();
    reqData->m_referrer = new ReferrerURL(document->documentURI());
    reqData->m_destination = RequestDestination::Font;
    reqData->m_syncLevel = RequestSyncLevel::SyncIfAlreadyLoaded;

    resource->request(reqData, true);
    if (auto fonts = document->fontsIfCreated()) {
        fonts.value()->didChangeFontLoadingState();
    }
}

void FontSelector::loadPendingRangedFonts(FontFaceList* list, char32_t ch)
{
    if (!list->pendingRangedFontsCover(ch)) {
        return;
    }
    auto& pending = *list->m_pendingRangedFonts;
    for (size_t i = 0; i < pending.size();) {
        if (!pending[i].m_ranges->contains(ch)) {
            i++;
            continue;
        }
        FontFaceList::PendingRangedFont entry = pending[i];
        pending.erase(pending.begin() + i);
        // Once the download finishes the font lists are rebuilt; until then
        // the character falls back like any uncovered one.
        requestWebFont(document(), entry.m_resource);
        if (FontFace* face = entry.m_resource->fontFace()) {
            list->insertRangedFace(entry.m_position, face, entry.m_ranges);
        }
    }
    list->rebuildPendingCoverage();
}

Font* FontSelector::loadFont(String* familyNameArray[],
                             size_t familyNameArraySize, float size, char style,
                             char weight, float letterSpacing,
                             FontKerningValue fontKerning)
{
    auto cacheFontName =
        mergeFamilyNames(familyNameArray, familyNameArraySize, size, style,
                         weight, letterSpacing, fontKerning);
    auto iter = m_fontCache.find(cacheFontName);
    if (iter != m_fontCache.end()) {
        return iter->second;
    }

    Font* result = Font::createEmptyFont(this);
    result->m_style = style;
    result->m_weight = weight;
    result->m_size = size;
    result->m_letterSpacing = letterSpacing;

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
        // search webfont Path first per CSS Fonts font matching algorithm
        int fitScore = 0;
        WebFont* selectedWebFont = nullptr;

        for (size_t k = 0; k < document()->m_webFontList.size(); k++) {
            auto& webFont = document()->m_webFontList[k];
            if (webFont.familyName()->equalsIgnoreCase(familyNameArray[i])) {
                int currentScore = webFontMatchScore(webFont, style, weight);
                if (currentScore > fitScore) {
                    selectedWebFont = &webFont;
                    fitScore = currentScore;
                }
            }
        }

        bool hasRangedFace = false;
        for (size_t k = 0;
             selectedWebFont && k < document()->m_webFontList.size(); k++) {
            auto& webFont = document()->m_webFontList[k];
            hasRangedFace |=
                webFont.unicodeRanges() &&
                webFont.familyName()->equalsIgnoreCase(familyNameArray[i]) &&
                webFontMatchScore(webFont, style, weight) == fitScore;
        }

        if (hasRangedFace) {
            // CSS Fonts 4: the best matching faces of a family form a
            // composite font, tried in reverse declaration order; a face with
            // a unicode-range only supplies the code points in that range.
            bool usable = false;
            for (size_t k = document()->m_webFontList.size(); k-- > 0;) {
                auto& webFont = document()->m_webFontList[k];
                if (!webFont.familyName()->equalsIgnoreCase(
                        familyNameArray[i]) ||
                    webFontMatchScore(webFont, style, weight) != fitScore) {
                    continue;
                }
                UnicodeRangeList* ranges = webFont.unicodeRanges();
                FontFace* face = nullptr;
                if (webFont.fromLocal()) {
                    face = loadLocalWebFont(webFont.localFontName(), style,
                                            weight);
                } else {
                    FontResource* resource = webFont.fontResource();
                    if (!ranges) {
                        requestWebFont(document(), resource);
                    }
                    face = resource->fontFace();
                    if (!face && !resource->isFaildToFetchResource()) {
                        usable = true;
                        if (ranges && !resource->isRequested()) {
                            result->m_fontFaceList->addPendingRangedFont(
                                ranges, resource);
                        } else if (!ranges &&
                                   result->m_fontFaceList
                                           ->m_seenUnresolvedWebFontIndex ==
                                       SIZE_MAX) {
                            result->m_fontFaceList
                                ->m_seenUnresolvedWebFontIndex =
                                result->m_fontFaceList->size();
                        }
                    }
                }
                if (face) {
                    result->m_fontFaceList->pushRangedFace(face, ranges);
                    usable = true;
                }
            }
            if (usable) {
                continue;
            }
        } else if (selectedWebFont) {
            if (!selectedWebFont->fromLocal()) {
                // download from web
                requestWebFont(document(), selectedWebFont->fontResource());
                if (selectedWebFont->fontResource()->fontFace()) {
                    // fontface loaded!
                    FontFace* face =
                        selectedWebFont->fontResource()->fontFace();
                    result->m_fontFaceList->push_back(face);
                    continue;
                } else if (!selectedWebFont->fontResource()
                                ->isFaildToFetchResource()) {
                    if (result->m_fontFaceList->m_seenUnresolvedWebFontIndex ==
                        SIZE_MAX) {
                        result->m_fontFaceList->m_seenUnresolvedWebFontIndex =
                            result->m_fontFaceList->size();
                    }
                    continue;
                }
            } else {
                // local font
                FontFace* face = loadLocalWebFont(
                    selectedWebFont->localFontName(), style, weight);
                if (face) {
                    result->m_fontFaceList->push_back(face);
                    continue;
                }
            }
        }

        // search system font Path if no matching web font (or web font failed)
        auto fm = familyNameArray[i]->toUTF8NonGCString();
        std::transform(fm.begin(), fm.end(), fm.begin(), tolower);
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
    }

    if (result->m_fontFaceList->hasPendingRangedFonts()) {
        result->m_fontFaceList->rebuildPendingCoverage();
    }

    UTF8StringDataNonGCStd familyName = document()
                                            ->webView()
                                            ->initialFontFamilyDatas()[1]
                                            .m_familyName.string()
                                            ->toUTF8NonGCString();
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
    std::transform(fm.begin(), fm.end(), fm.begin(), tolower);
    cacheDeleter(m_fontFaceListCache, fm);
    cacheDeleter(m_fontCache, fm);
    cacheDeleter(m_webFontLocalSrcCache, fm);
}
}; // namespace Starfish
