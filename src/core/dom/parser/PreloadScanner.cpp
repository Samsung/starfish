/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "PreloadScanner.h"
#include "core/dom/Document.h"

namespace StarFish {

PreloadScanner::PreloadScanner(Document* document, String* source)
    : m_document(document)
{
    auto buf = source->bufferAccessData();

    size_t len = buf.length;
    for (size_t i = 0; i < len; i++) {
        char32_t ch = buf.charAt(i);

        if (ch == '<') {
            bool shouldSearchAttr = false;
            bool isScript = false;
            bool isCSS = false;
            bool foundRelStyleSheet = false;
            bool foundAsync = false;
            bool foundDefer = false;
            size_t attributeStart = 0;
            if (len > i + 7 && buf.charAt(i + 1) == 's' &&
                buf.charAt(i + 2) == 'c' && buf.charAt(i + 3) == 'r' &&
                buf.charAt(i + 4) == 'i' && buf.charAt(i + 5) == 'p' &&
                buf.charAt(i + 6) == 't') {
                isScript = true;
                attributeStart = i + 7;
                shouldSearchAttr = true;
            } else if (len > i + 5 && buf.charAt(i + 1) == 'l' &&
                       buf.charAt(i + 2) == 'i' && buf.charAt(i + 3) == 'n' &&
                       buf.charAt(i + 4) == 'k') {
                isCSS = true;
                attributeStart = i + 5;
                shouldSearchAttr = true;
            }

            UTF32StringDataNonGCStd encoding;
            UTF32StringDataNonGCStd src;

            if (shouldSearchAttr) {
                bool closeFinded = false;
                for (size_t k = i + 7; k < len; k++) {
                    if (buf.charAt(k) == '>') {
                        i = k;
                        closeFinded = true;
                        break;
                    }
                }

                enum Mode {
                    ModeAttr,
                    ModeValueWaitFirstQuotationMark,
                    ModeValueWaitValue
                };
                Mode mode = ModeAttr;
                UTF32StringDataNonGCStd mayBeAttrName;
                UTF32StringDataNonGCStd mayBeValue;

                for (size_t k = attributeStart; k < i; k++) {
                    char32_t c = buf.charAt(k);
                    if (mode == ModeAttr) {
                        if (c == '=') {
                            mode = ModeValueWaitFirstQuotationMark;
                        } else {
                            if (!String::isSpaceOrNewline(c)) {
                                mayBeAttrName += c;
                            }
                        }
                    } else if (mode == ModeValueWaitFirstQuotationMark) {
                        if (String::isSpaceOrNewline(c)) {
                            continue;
                        } else if (c == '\'' || c == '"') {
                            mode = ModeValueWaitValue;
                        } else if (String::isASCIIPrintableKey(c)) {
                            mode = ModeValueWaitValue;
                            mayBeValue += c;
                        }
                    } else {
                        if (String::isSpaceOrNewline(c) || c == '\'' ||
                            c == '"') {
                            if (mayBeAttrName == U"src") {
                                src = mayBeValue;
                            } else if (mayBeAttrName == U"charset") {
                                encoding = mayBeValue;
                            }

                            if (isCSS) {
                                if (mayBeAttrName == U"rel" &&
                                    mayBeValue == U"stylesheet") {
                                } else {
                                    isCSS = false;
                                }
                            } else {
                                if (mayBeAttrName == U"async" ||
                                    mayBeAttrName == U"defer") {
                                    isScript = false;
                                }
                            }
                            mode = ModeAttr;
                            mayBeAttrName.clear();
                            mayBeValue.clear();
                        } else {
                            mayBeValue += c;
                        }
                    }
                }
            }

            if (src.length()) {
                if (isScript) {
                    auto url = new ResourceURL(
                        String::createASCIIStringFromUTF32SourceIfPossible(src),
                        m_document->baseURL()->baseURI());
                    m_preloadedJS.push_back(
                        m_document->resourceLoader().fetchText(
                            url,
                            String::createASCIIStringFromUTF32SourceIfPossible(
                                encoding)));
                    m_preloadedJS.back()->request(
                        Resource::ResourceRequestSyncLevel::NeverSync,
                        m_document->documentURI(), true);
                } else if (isCSS) {
                    auto url = new ResourceURL(
                        String::createASCIIStringFromUTF32SourceIfPossible(src),
                        m_document->baseURL()->baseURI());
                    m_preloadedCSS.push_back(
                        m_document->resourceLoader().fetchText(
                            url,
                            String::createASCIIStringFromUTF32SourceIfPossible(
                                encoding)));
                    m_preloadedCSS.back()->request(
                        Resource::ResourceRequestSyncLevel::NeverSync,
                        m_document->documentURI(), true);
                }
            }
        }
    }
}
}
