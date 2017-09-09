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
#include "FrameSVGPathBox.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace StarFish {

void FrameSVGPathBox::paintSVG(PaintingContext& ctx)
{
    FrameBox* cb = layoutParent()->asFrameBox();
    LayoutUnit viewportWidth =
        node()->document()->frame()->style()->width().fixed();

    Element* e = node()->asElement();
    String* d = e->getAttributeOrEmpty(e->starFish()->staticStrings()->m_d);
    if (d->length()) {
        auto utf8Str = d->toUTF8NonGCString();
        CSSTokenVector tokens;
        const char* sep = ",mMzZlLhHvVcCsSqQtTaA-";
        CSSStyleDeclaration::tokenizeCSSValue(tokens, utf8Str.data(),
                                              utf8Str.length(), sep, 22, true);

        enum Mode {
            WaitCommand,
            WaitCoordsX,
            WaitCoordsY,
            WaitCoordsX2,
            WaitCoordsY2,
            WaitCoordsX3,
            WaitCoordsY3,
            WaitCoordsX4,
            WaitCoordsY4,
        };
        Mode mode = Mode::WaitCommand;
        bool gotMinus = false;
        bool isClosed = false;
        float x, y, x2, y2;
        float x3, y3, x4, y4;
        float lastX = 0, lastY = 0;
        float lastOfLastX = 0, lastOfLastY = 0;
        char paintMode = ' ';
        char prevMode = ' ';
#define TO_WAIT_COORDS_MODE(p) \
    prevMode = paintMode;      \
    paintMode = p;             \
    mode = Mode::WaitCoordsX;
#define TO_WAIT_COMMAND_MODE() mode = Mode::WaitCommand;
#define READ_NUMBER(n)                                                       \
    if (!CSSPropertyParser::parseNumber(token.data(), token.length(), &n)) { \
        break;                                                               \
    }                                                                        \
    if (gotMinus) {                                                          \
        n = -n;                                                              \
    }                                                                        \
    gotMinus = false;

        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];
            if (token.equals(",")) {
                continue;
            }
            if (token.equals("-") && (mode != Mode::WaitCommand)) {
                if (gotMinus) {
                    // error
                    break;
                }
                gotMinus = true;
                continue;
            }
            if (mode == Mode::WaitCommand) {
                if (token.equals("m")) {
                    TO_WAIT_COORDS_MODE('m');
                } else if (token.equals("M")) {
                    TO_WAIT_COORDS_MODE('M');
                } else if (token.equals("z") || token.equals("Z")) {
                    break;
                } else if (token.equals("l")) {
                    TO_WAIT_COORDS_MODE('l');
                } else if (token.equals("L")) {
                    TO_WAIT_COORDS_MODE('L');
                } else if (token.equals("c")) {
                    TO_WAIT_COORDS_MODE('c');
                } else if (token.equals("C")) {
                    TO_WAIT_COORDS_MODE('C');
                } else if (token.equals("s")) {
                    TO_WAIT_COORDS_MODE('s');
                } else if (token.equals("S")) {
                    TO_WAIT_COORDS_MODE('S');
                } else if (token.equals("q")) {
                    TO_WAIT_COORDS_MODE('q');
                } else if (token.equals("Q")) {
                    TO_WAIT_COORDS_MODE('Q');
                } else if (token.equals("t")) {
                    TO_WAIT_COORDS_MODE('t');
                } else if (token.equals("T")) {
                    TO_WAIT_COORDS_MODE('T');
                } else {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                    // error
                    break;
                }
            } else if (mode == Mode::WaitCoordsX) {
                READ_NUMBER(x);
                mode = Mode::WaitCoordsY;
            } else if (mode == Mode::WaitCoordsY) {
                READ_NUMBER(y);
                if (paintMode == 'm') {
                    ctx.m_canvas->moveToRel(x, y);
                    lastX = x;
                    lastY = y;
                    TO_WAIT_COMMAND_MODE()
                } else if (paintMode == 'M') {
                    ctx.m_canvas->moveTo(x, y);
                    lastX = x;
                    lastY = y;
                    TO_WAIT_COMMAND_MODE()
                } else if (paintMode == 'l') {
                    ctx.m_canvas->lineToRel(x, y);
                    lastX = x;
                    lastY = y;
                    TO_WAIT_COMMAND_MODE()
                } else if (paintMode == 'L') {
                    ctx.m_canvas->lineTo(x, y);
                    lastX = x;
                    lastY = y;
                    TO_WAIT_COMMAND_MODE()
                } else if (paintMode == 't' || paintMode == 'T') {
                    float firstX, firstY;
                    if (prevMode != 'Q' && prevMode != 'q') {
                        firstX = lastX;
                        firstY = lastY;
                    } else {
                        firstX = lastX + lastX - lastOfLastX;
                        firstY = lastY + lastY - lastOfLastY;
                    }
                    if (paintMode == 't') {
                        ctx.m_canvas->quadraticCurveToRel(firstX, firstY, x, y);
                    } else {
                        ctx.m_canvas->quadraticCurveTo(firstX, firstY, x, y);
                    }
                    lastX = x;
                    lastY = y;
                    TO_WAIT_COMMAND_MODE()
                } else if (paintMode == 'T') {
                    lastX = x;
                    lastY = y;
                    TO_WAIT_COMMAND_MODE()
                } else {
                    mode = Mode::WaitCoordsX2;
                }
            } else if (mode == Mode::WaitCoordsX2) {
                READ_NUMBER(x2);
                mode = Mode::WaitCoordsY2;
            } else if (mode == Mode::WaitCoordsY2) {
                READ_NUMBER(y2);

                if (paintMode == 's' || paintMode == 'S') {
                    float firstX;
                    float firstY;
                    // The first control point is assumed to be the reflection
                    // of the second control point on the previous command
                    // relative to the current point.
                    // (If there is no previous command or if the previous
                    // command was not an C, c, S or s,
                    // assume the first control point is coincident with the
                    // current point.)
                    if (prevMode == ' ' ||
                        (prevMode != 'C' && prevMode != 'c' &&
                         prevMode != 'S' && prevMode != 's')) {
                        firstX = lastX;
                        firstY = lastY;
                    } else {
                        firstX = lastX + lastX - lastOfLastX;
                        firstY = lastY + lastY - lastOfLastY;
                    }

                    lastOfLastX = x;
                    lastOfLastY = y;
                    lastX = x2;
                    lastY = y2;
                    if (paintMode == 's') {
                        ctx.m_canvas->curveToRel(firstX, firstY, x, y, x2, y2);
                        TO_WAIT_COMMAND_MODE()
                    } else if (paintMode == 'S') {
                        ctx.m_canvas->curveTo(firstX, firstY, x, y, x2, y2);
                        TO_WAIT_COMMAND_MODE()
                    }
                } else if (paintMode == 'q') {
                    lastOfLastX = x;
                    lastOfLastY = y;
                    lastX = x2;
                    lastY = y2;
                    ctx.m_canvas->quadraticCurveToRel(x, y, x2, y2);
                    TO_WAIT_COMMAND_MODE()
                } else if (paintMode == 'Q') {
                    lastOfLastX = x;
                    lastOfLastY = y;
                    lastX = x2;
                    lastY = y2;
                    ctx.m_canvas->quadraticCurveTo(x, y, x2, y2);
                    TO_WAIT_COMMAND_MODE()
                } else {
                    mode = Mode::WaitCoordsX3;
                }
            } else if (mode == Mode::WaitCoordsX3) {
                READ_NUMBER(x3);
                mode = Mode::WaitCoordsY3;
            } else if (mode == Mode::WaitCoordsY3) {
                READ_NUMBER(y3);
                if (paintMode == 'c') {
                    ctx.m_canvas->curveToRel(x, y, x2, y2, x3, y3);
                } else if (paintMode == 'C') {
                    ctx.m_canvas->curveTo(x, y, x2, y2, x3, y3);
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                lastOfLastX = x2;
                lastOfLastY = y2;
                lastX = x3;
                lastY = y3;

                bool isLookAheadNumber = false;
                for (size_t j = i + 1; j < tokens.size(); j++) {
                    if (tokens[j].equals(",")) {
                        continue;
                    }
                    char c = tokens[j][0];
                    if (c == '.' || c == '-' || (c >= '0' && c <= '9')) {
                        isLookAheadNumber = true;
                    }
                    break;
                }

                if (isLookAheadNumber) {
                    TO_WAIT_COORDS_MODE(paintMode);
                } else {
                    TO_WAIT_COMMAND_MODE();
                }
            }
        }

        ctx.m_canvas->setStrokeWidth(
            style()->strokeWidth().specifiedValue(cb->width(), viewportWidth));
        ctx.m_canvas->setColor(style()->fill().color());
        ctx.m_canvas->setFillRule(style()->fillRule());
        ctx.m_canvas->fillPreserve();
        ctx.m_canvas->setColor(style()->stroke().color());
        ctx.m_canvas->stroke();
    }
}
}
