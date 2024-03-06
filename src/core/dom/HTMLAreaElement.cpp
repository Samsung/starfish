/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"

#include "core/dom/HTMLAreaElement.h"

#include "core/dom/Document.h"
#include "core/dom/DOMTokenList.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLImageElement.h"
#include "core/dom/HTMLMapElement.h"
#include "core/dom/Traverse.h"
#include "core/dom/canvas/CanvasFillRule.h"
#include "core/dom/parser/HTMLParserIdioms.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameBlockBox.h"
#include "core/modules/canvas/Path.h"
#include "core/page/Location.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/DOMException.h"

namespace Starfish {

template <typename CharType, bool characterPredicate(CharType)>
void skipUntil(const CharType*& position, const CharType* end)
{
    while (position < end && !characterPredicate(*position)) {
        ++position;
    }
}

template <typename CharType, bool characterPredicate(CharType)>
void skipWhile(const CharType*& position, const CharType* end)
{
    while (position < end && characterPredicate(*position)) {
        ++position;
    }
}

static float clampCoord(double value)
{
    return LayoutUnit(value).toFloat();
}

void* HTMLAreaElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLAreaElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLAreaElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLAreaElement, m_relList));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLAreaElement, m_path));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLAreaElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

DOMTokenList* HTMLAreaElement::relList()
{
    if (!m_relList) {
        m_relList = new DOMTokenList(this, starfish()->staticStrings()->m_rel);
    }
    return m_relList;
}

String* HTMLAreaElement::referrerPolicy()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_referrerpolicy);
}

void HTMLAreaElement::setReferrerPolicy(String* policy)
{
    if (ReferrerURL::isValidPolicy(policy)) {
        setAttribute(starfish()->staticStrings()->m_referrerpolicy, policy);
    }
}

bool HTMLAreaElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (event->type()->equals("click")) {
        const auto& href = starfish()->staticStrings()->m_href;
        Nullable<String*> hrefAttr = getAttribute(href);
        if (hrefAttr.hasValue()) {
            GET_EFFECTIVE_REFERRERPOLICY();
            ReferrerURL* rUrl =
                new ReferrerURL(document()->documentURI(), policy);
            String* hrefStr = hrefAttr.getValue()->trim();
            if (hrefStr->length()) {
                if (hrefStr->startsWith("#")) {
                    window()->location()->setHash(hrefStr);
                } else if (hrefStr->startsWith("javascript:", false)) {
                    String* ret2 = toBrowserString(
                        window()->scriptBindingInstance(),
                        evaluateString(
                            window()->scriptBindingInstance(),
                            hrefStr->substring(11, hrefStr->length() - 11)));
                    if (!ret2->equalsIgnoreCase("undefined")) {
                        try {
                            GCVector<String*> value0;
                            value0.push_back(ret2);
                            document()->write(document(), value0);
                        } catch (DOMException* e) {
                            // TODO: should throw the exception into onError
                            // event handler
                            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                        }
                    }
                } else {
                    window()->location()->setLocation(hrefStr, rUrl);
                }
            } else {
                window()->location()->setLocation(
                    document()->documentURI()->urlString(), rUrl);
            }
            return true;
        }
    }
    return false;
}

void HTMLAreaElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starfish()->staticStrings()->m_shape) {
        invalidatePath();

        String* shape = value->toLower();
        if (shape->equals("default")) {
            m_shape = Shape::Default;
        } else if (shape->equals("circle") || shape->equals("circ")) {
            m_shape = Shape::Circle;
        } else if (shape->equals("polygon") || shape->equals("poly")) {
            m_shape = Shape::Poly;
        } else {
            m_shape = Shape::Rect;
        }
    } else if (name == starfish()->staticStrings()->m_coords) {
        invalidatePath();

        if (value->isEmpty()) {
            return;
        }
        std::vector<double> numbers;
        value->peekUTF8Buffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                std::vector<double>* numbers = (std::vector<double>*)data;

                const char* end = buf + len;
                skipWhile<char, isSpaceOrDelimiter>(buf, end);

                while (buf < end) {
                    skipWhile<char, isNotSpaceDelimiterOrNumberStart>(buf, end);

                    const char* numStart = buf;
                    skipUntil<char, isSpaceOrDelimiter>(buf, end);

                    std::string num(numStart, buf - numStart);
                    double d = std::atof(num.c_str());
                    numbers->push_back(d);

                    skipWhile<char, isSpaceOrDelimiter>(buf, end);
                }
                return 0;
            },
            &numbers);

        m_coords.clear();
        m_coords.assign(numbers.begin(), numbers.end());
    }
}

void HTMLAreaElement::invalidatePath()
{
    m_path = nullptr;
}

HTMLImageElement* HTMLAreaElement::imageElement()
{
    Node* map = Traverse::firstAncestor(this, [&](Node* ancestor) {
        if (ancestor->isHTMLMapElement()) {
            return true;
        }
        return false;
    });
    if (map) {
        return map->asHTMLMapElement()->imageElement();
    }
    return nullptr;
}

bool HTMLAreaElement::includePoint(Frame* cb, float x, float y)
{
    return areaPath(cb)->isPointInPath(x, y, CanvasFillRule::EvenOdd);
}

Path* HTMLAreaElement::areaPath(Frame* cb)
{
    Path* path = Path::create();
    if (!cb) {
        return path;
    }

    if (m_shape == Shape::Default) {
        if (cb->isFrameBox()) {
            LayoutRect rect = cb->asFrameBox()->frameRect();
            path->rect(rect.x().toFloat(), rect.y().toFloat(),
                       rect.width().toFloat(), rect.height().toFloat());
        }
        m_path = nullptr;
        return path;
    }

    if (m_path) {
        return m_path;
    }

    if (m_coords.empty()) {
        return path;
    }

    switch (m_shape) {
    case Shape::Poly:
        if (m_coords.size() >= 6) {
            int numPoints = m_coords.size() / 2;
            path->moveTo(clampCoord(m_coords[0]), clampCoord(m_coords[1]));
            for (int i = 1; i < numPoints; ++i) {
                path->lineTo(clampCoord(m_coords[i * 2]),
                             clampCoord(m_coords[i * 2 + 1]));
            }
        }
        break;
    case Shape::Circle:
        if (m_coords.size() >= 3 && m_coords[2] > 0) {
            float r = clampCoord(m_coords[2]);
            path->arc(clampCoord(m_coords[0]), clampCoord(m_coords[1]), r, 0.0,
                      2 * M_PI);
        }
        break;
    case Shape::Rect:
        if (m_coords.size() >= 4) {
            float x0 = clampCoord(m_coords[0]);
            float y0 = clampCoord(m_coords[1]);
            float x1 = clampCoord(m_coords[2]);
            float y1 = clampCoord(m_coords[3]);
            path->rect(x0, y0, x1 - x0, y1 - y0);
        }
        break;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        break;
    }
    m_path = path;

    return path;
}

bool HTMLAreaElement::supportsFocus()
{
    const auto& href = starfish()->staticStrings()->m_href;
    HTMLImageElement* image = imageElement();
    if (!image) {
        return false;
    }

    return const_cast<HTMLAreaElement*>(this)->hasAttribute(href) != SIZE_MAX
               ? true
               : false;
}

} // namespace Starfish
