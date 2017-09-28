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

ALWAYS_INLINE static void paintArgSegment(Canvas* canvas, double xc, double yc,
                                          double th0, double th1, double rx,
                                          double ry, double xAxisRotation)
{
    double f = xAxisRotation * M_PI / 180.0;
    double sinf = sin(f);
    double cosf = cos(f);

    double thHalf = 0.5 * (th1 - th0);
    double t =
        (8.0 / 3.0) * sin(thHalf * 0.5) * sin(thHalf * 0.5) / sin(thHalf);
    double x1 = rx * (cos(th0) - t * sin(th0));
    double y1 = ry * (sin(th0) + t * cos(th0));
    double x3 = rx * cos(th1);
    double y3 = ry * sin(th1);
    double x2 = x3 + rx * (t * sin(th1));
    double y2 = y3 + ry * (-t * cos(th1));

    canvas->curveTo(xc + cosf * x1 - sinf * y1, yc + sinf * x1 + cosf * y1,
                    xc + cosf * x2 - sinf * y2, yc + sinf * x2 + cosf * y2,
                    xc + cosf * x3 - sinf * y3, yc + sinf * x3 + cosf * y3);
}

// http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
static void paintPathArcCommand(Canvas* canvas, double x1, double y1, double rx,
                                double ry, double xAxisRotation,
                                bool isLargeArc, bool isPositiveSweep,
                                double x2, double y2)
{
    if (x1 == x2 && y1 == y2) {
        return;
    }

    double f = xAxisRotation * M_PI / 180.0;
    double sinf = sin(f);
    double cosf = cos(f);

    rx = std::abs(rx);
    ry = std::abs(ry);

    if ((rx < std::numeric_limits<double>::epsilon()) ||
        (ry < std::numeric_limits<double>::epsilon())) {
        canvas->lineTo(x2, y2);
        return;
    }

    double k1 = (x1 - x2) / 2.0;
    double k2 = (y1 - y2) / 2.0;

    double x1_ = cosf * k1 + sinf * k2;
    double y1_ = -sinf * k1 + cosf * k2;

    double gamma = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
    if (gamma > 1.0) {
        rx *= sqrt(gamma);
        ry *= sqrt(gamma);
    }

    k1 = rx * rx * y1_ * y1_ + ry * ry * x1_ * x1_;
    if (k1 == 0.0) {
        return;
    }

    k1 = std::sqrt(std::abs((rx * rx * ry * ry) / k1 - 1.0));
    if (isPositiveSweep == isLargeArc) {
        k1 = -k1;
    }

    double cx_ = k1 * rx * y1_ / ry;
    double cy_ = -k1 * ry * x1_ / rx;

    double cx = cosf * cx_ - sinf * cy_ + (x1 + x2) / 2.0;
    double cy = sinf * cx_ + cosf * cy_ + (y1 + y2) / 2.0;

    k1 = (x1_ - cx_) / rx;
    k2 = (y1_ - cy_) / ry;
    double k3 = (-x1_ - cx_) / rx;
    double k4 = (-y1_ - cy_) / ry;

    double k5 = std::sqrt(std::abs(k1 * k1 + k2 * k2));
    if (k5 == 0.0) {
        return;
    }

    k5 = k1 / k5;
    if (k5 < -1.0) {
        k5 = -1.0;
    } else if (k5 > 1.0) {
        k5 = 1.0;
    }
    double theta1 = acos(k5);
    if (k2 < 0.0) {
        theta1 = -theta1;
    }

    k5 = std::sqrt(std::abs((k1 * k1 + k2 * k2) * (k3 * k3 + k4 * k4)));
    if (k5 == 0.0) {
        return;
    }

    k5 = (k1 * k3 + k2 * k4) / k5;
    if (k5 < -1.0) {
        k5 = -1.0;
    } else if (k5 > 1.0) {
        k5 = 1.0;
    }
    double deltaTheta = acos(k5);
    if (k1 * k4 - k3 * k2 < 0.0) {
        deltaTheta = -deltaTheta;
    }

    if (isPositiveSweep && deltaTheta < 0.0) {
        deltaTheta += M_PI * 2.0;
    } else if (!isPositiveSweep && deltaTheta > 0.0) {
        deltaTheta -= M_PI * 2.0;
    }

    double segmentsCount =
        std::ceil(std::abs((deltaTheta / (M_PI * 0.5 + 0.001))));

    for (size_t i = 0; i < segmentsCount; i++) {
        paintArgSegment(canvas, cx, cy, theta1 + i * deltaTheta / segmentsCount,
                        theta1 + (i + 1) * deltaTheta / segmentsCount, rx, ry,
                        xAxisRotation);
    }
}

void FrameSVGPathBox::paintSVG(PaintingContext& ctx)
{
    FrameBox* cb = layoutParent()->asFrameBox();
    Element* e = node()->asElement();
    String* d = e->getAttributeOrEmpty(e->starFish()->staticStrings()->m_d);
    if (d->length()) {
        auto utf8Str = d->toUTF8NonGCString();
        CSSTokenVector tokensInput;
        const char* sep = ",mMzZlLhHvVcCsSqQtTaA-e";
        CSSStyleDeclaration::tokenizeCSSValue(tokensInput, utf8Str.data(),
                                              utf8Str.length(), sep, 23, true);
        std::vector<CSSTokenValue> tokens;
        tokens.reserve(tokensInput.size());
        for (size_t i = 0; i < tokensInput.size(); i++) {
            tokens.push_back(std::move(tokensInput[i]));
        }
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

#define READ_NUMBER(n)                                                       \
    if (!CSSPropertyParser::parseNumber(token.data(), token.length(), &n)) { \
        break;                                                               \
    }                                                                        \
    if (gotMinus) {                                                          \
        n = -n;                                                              \
    }                                                                        \
    gotMinus = false;

#define REWIND_IF_NEEDED()                                    \
    bool isLookAheadNumber = false;                           \
    for (size_t j = i + 1; j < tokens.size(); j++) {          \
        if (tokens[j].equals(",")) {                          \
            continue;                                         \
        }                                                     \
        char c = tokens[j][0];                                \
        if (c == '.' || c == '-' || (c >= '0' && c <= '9')) { \
            isLookAheadNumber = true;                         \
        }                                                     \
        break;                                                \
    }                                                         \
    if (isLookAheadNumber) {                                  \
        TO_WAIT_COORDS_MODE(paintMode);                       \
    } else {                                                  \
        mode = Mode::WaitCommand;                             \
    }

        for (size_t i = 0; i < tokens.size(); i++) {
            auto& token = tokens[i];
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

            if (mode != Mode::WaitCommand) {
                auto token = tokens[i];
                bool hasMultipleDot = false;
                bool seenDot = false;
                for (size_t k = 0; k < token.size(); k++) {
                    if (token[k] == '.') {
                        if (!seenDot) {
                            seenDot = true;
                        } else {
                            hasMultipleDot = true;
                            tokens.erase(tokens.begin() + i);
                            CSSTokenValue s1 = token.substr(0, k);
                            CSSTokenValue s2 =
                                token.substr(k, token.size() - k);
                            tokens.insert(tokens.begin() + i, s1);
                            tokens.insert(tokens.begin() + i + 1, s2);
                            i--;
                            break;
                        }
                    }
                }
                if (hasMultipleDot) {
                    continue;
                }
            }
            if (mode != Mode::WaitCommand) {
                if (i + 1 < tokens.size() && tokens[i + 1].size() == 1 &&
                    tokens[i + 1][0] == 'e') {
                    if (i + 2 < tokens.size()) {
                        token = token + tokens[i + 1] + tokens[i + 2];
                        if (tokens[i + 2].size() == 1 &&
                            tokens[i + 2][0] == '-') {
                            if (i + 3 < tokens.size()) {
                                token += tokens[i + 3];
                                i += 3;
                            } else {
                                // error
                                break;
                            }
                        } else {
                            i += 2;
                        }
                    } else {
                        // error
                        break;
                    }
                }
            }

            if (mode == Mode::WaitCommand) {
                if (token.equals("m")) {
                    TO_WAIT_COORDS_MODE('m');
                } else if (token.equals("M")) {
                    TO_WAIT_COORDS_MODE('M');
                } else if (token.equals("z") || token.equals("Z")) {
                    ctx.m_canvas->closePath();
                    continue;
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
                } else if (token.equals("a")) {
                    TO_WAIT_COORDS_MODE('a');
                } else if (token.equals("A")) {
                    TO_WAIT_COORDS_MODE('A');
                } else if (token.equals("h")) {
                    TO_WAIT_COORDS_MODE('h');
                } else if (token.equals("H")) {
                    TO_WAIT_COORDS_MODE('H');
                } else if (token.equals("v")) {
                    TO_WAIT_COORDS_MODE('v');
                } else if (token.equals("V")) {
                    TO_WAIT_COORDS_MODE('V');
                } else {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                    // error
                    break;
                }
            } else if (mode == Mode::WaitCoordsX) {
                READ_NUMBER(x);

                if (paintMode == 'v') {
                    ctx.m_canvas->lineTo(lastX, lastY + x);
                    lastY += x;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'V') {
                    ctx.m_canvas->lineTo(lastX, x);
                    lastY = x;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'h') {
                    ctx.m_canvas->lineTo(lastX + x, lastY);
                    lastX += x;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'H') {
                    ctx.m_canvas->lineTo(x, lastY);
                    lastX = x;
                    REWIND_IF_NEEDED();
                } else {
                    mode = Mode::WaitCoordsY;
                }
            } else if (mode == Mode::WaitCoordsY) {
                READ_NUMBER(y);
                if (paintMode == 'm') {
                    ctx.m_canvas->moveTo(lastX + x, lastY + y);
                    lastX += x;
                    lastY += y;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'M') {
                    ctx.m_canvas->moveTo(x, y);
                    lastX = x;
                    lastY = y;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'l') {
                    ctx.m_canvas->lineTo(lastX + x, lastY + y);
                    lastX += x;
                    lastY += y;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'L') {
                    ctx.m_canvas->lineTo(x, y);
                    lastX = x;
                    lastY = y;
                    REWIND_IF_NEEDED();
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
                        ctx.m_canvas->quadraticCurveTo(firstX, firstY,
                                                       lastX + x, lastY + y);
                        lastX += x;
                        lastY += y;
                    } else {
                        ctx.m_canvas->quadraticCurveTo(firstX, firstY, x, y);
                        lastX = x;
                        lastY = y;
                    }
                    REWIND_IF_NEEDED();
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

                    if (paintMode == 's') {
                        ctx.m_canvas->curveTo(firstX, firstY, lastX + x,
                                              lastY + y, lastX + x2,
                                              lastY + y2);
                        lastOfLastX = lastX + x;
                        lastOfLastY = lastY + y;
                        lastX = lastX + x2;
                        lastY = lastY + y2;
                    } else if (paintMode == 'S') {
                        ctx.m_canvas->curveTo(firstX, firstY, x, y, x2, y2);
                        lastOfLastX = x;
                        lastOfLastY = y;
                        lastX = x2;
                        lastY = y2;
                    }
                    REWIND_IF_NEEDED()
                } else if (paintMode == 'q') {
                    ctx.m_canvas->quadraticCurveTo(lastX + x, lastY + y,
                                                   lastX + x2, lastY + y2);
                    lastOfLastX = lastX + x;
                    lastOfLastY = lastY + y;
                    lastX = lastX + x2;
                    lastY = lastY + y2;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'Q') {
                    lastOfLastX = x;
                    lastOfLastY = y;
                    lastX = x2;
                    lastY = y2;
                    ctx.m_canvas->quadraticCurveTo(x, y, x2, y2);
                    REWIND_IF_NEEDED();
                } else {
                    mode = Mode::WaitCoordsX3;
                }
            } else if (mode == Mode::WaitCoordsX3) {
                READ_NUMBER(x3);
                mode = Mode::WaitCoordsY3;
            } else if (mode == Mode::WaitCoordsY3) {
                READ_NUMBER(y3);
                if (paintMode == 'c' || paintMode == 'C') {
                    if (paintMode == 'c') {
                        ctx.m_canvas->curveTo(lastX + x, lastY + y, lastX + x2,
                                              lastY + y2, lastX + x3,
                                              lastY + y3);
                        lastOfLastX = lastX + x2;
                        lastOfLastY = lastY + y2;
                        lastX = lastX + x3;
                        lastY = lastY + y3;
                    } else if (paintMode == 'C') {
                        ctx.m_canvas->curveTo(x, y, x2, y2, x3, y3);
                        lastOfLastX = x2;
                        lastOfLastY = y2;
                        lastX = x3;
                        lastY = y3;
                    }

                    REWIND_IF_NEEDED()
                } else {
                    STARFISH_ASSERT(paintMode == 'a' || paintMode == 'A');
                    mode = Mode::WaitCoordsX4;
                }
            } else if (mode == Mode::WaitCoordsX4) {
                READ_NUMBER(x4);
                if (paintMode == 'a' || paintMode == 'A') {
                    float rx = x;
                    float ry = y;
                    float xAxisRotation = x2;
                    bool largeArcFlag = y2 == 1;
                    bool sweepFlag = x3 == 1;
                    float targetX = y3;
                    float targetY = x4;
                    if (paintMode == 'a') {
                        targetX += lastX;
                        targetY += lastY;
                    }
                    paintPathArcCommand(ctx.m_canvas, lastX, lastY, rx, ry,
                                        xAxisRotation, largeArcFlag, sweepFlag,
                                        targetX, targetY);
                    lastX = targetX;
                    lastY = targetY;
                    REWIND_IF_NEEDED();
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            }
        }

        ctx.m_canvas->setStrokeWidth(
            style()->strokeWidth().specifiedValue(cb->width(), this));
        ctx.m_canvas->setColor(style()->fill().color());
        ctx.m_canvas->setFillRule(style()->fillRule());
        ctx.m_canvas->fillPreserve();
        ctx.m_canvas->setColor(style()->stroke().color());
        ctx.m_canvas->stroke();
    }
}
}
