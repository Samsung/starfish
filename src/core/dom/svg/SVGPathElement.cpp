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
#include "Starfish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGPathElement.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSParser.h"
#include "core/modules/canvas/Path.h"

namespace Starfish {

void* SVGPathElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGPathElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGPathElement)] = { 0 };
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGPathElement, m_path));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGPathElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

ALWAYS_INLINE static void paintArgSegment(Path* path, double xc, double yc,
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

    path->bezierCurveTo(xc + cosf * x1 - sinf * y1, yc + sinf * x1 + cosf * y1,
                        xc + cosf * x2 - sinf * y2, yc + sinf * x2 + cosf * y2,
                        xc + cosf * x3 - sinf * y3, yc + sinf * x3 + cosf * y3);
}

// http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
void paintPathArcCommand(Path* path, double x1, double y1, double rx, double ry,
                         double xAxisRotation, bool isLargeArc,
                         bool isPositiveSweep, double x2, double y2)
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
        path->lineTo(x2, y2);
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
        paintArgSegment(path, cx, cy, theta1 + i * deltaTheta / segmentsCount,
                        theta1 + (i + 1) * deltaTheta / segmentsCount, rx, ry,
                        xAxisRotation);
    }
}

// PathToken has two mode
// first one is plainMode, and it holds start and end position of input string
// second one is bufferMode,
// it holds std::string comes from combination of input string
class PathToken {
public:
    PathToken(size_t s = 0, size_t e = 0)
        : m_start(s)
        , m_end(e)
    {
    }

    PathToken(const PathToken& t)
    {
        m_start = t.m_start;
        m_end = t.m_end;
        m_buffer = t.m_buffer;
    }

    const PathToken& operator=(const PathToken& t)
    {
        m_start = t.m_start;
        m_end = t.m_end;
        m_buffer = t.m_buffer;
        return *this;
    }

    void setRange(size_t s, size_t e)
    {
        m_start = s;
        m_end = e;
    }

    void shrink()
    {
        STARFISH_ASSERT(!isBufferMode());
        m_end--;
    }

    void expand()
    {
        STARFISH_ASSERT(!isBufferMode());
        m_end++;
    }

    void setString(std::string&& str)
    {
        m_end = SIZE_MAX;
        m_start = SIZE_MAX - str.length();
        m_buffer = std::move(str);
    }

    PathToken substr(size_t s, size_t n) const
    {
        PathToken r;
        if (UNLIKELY(isBufferMode())) {
            r.setString(m_buffer.substr(s, n));
        } else {
            r.setRange(s + m_start, m_start + s + n);
        }
        return r;
    }

    size_t length() const
    {
        return m_end - m_start;
    }

    size_t size() const
    {
        return length();
    }

    bool equals(const StringBufferAccessData& bad, char c) const
    {
        if (UNLIKELY(isBufferMode())) {
            return length() == 1 && m_buffer[0] == c;
        }
        return length() == 1 && static_cast<char>(bad.charAt(m_start)) == c;
    }

    char charAt(const StringBufferAccessData& bad, size_t index) const
    {
        if (UNLIKELY(isBufferMode())) {
            return m_buffer[index];
        }
        STARFISH_ASSERT(m_start + index < m_end);
        return bad.charAt(m_start + index);
    }

    std::string toString(const StringBufferAccessData& bad) const
    {
        if (UNLIKELY(isBufferMode())) {
            return m_buffer;
        }
        std::string ret;
        for (size_t i = m_start; i < m_end; i++) {
            ret += bad.charAt(i);
        }
        return ret;
    }

    bool parseNumber(const StringBufferAccessData& bad, float* n) const
    {
        size_t len = length();
        if (UNLIKELY(isBufferMode())) {
            return CSSPropertyParser::parseNumber(m_buffer.data(), len, 0, n);
        }
        char* buffer = ALLOCA(len + 1, char);
        size_t bufferIndex = 0;
        for (size_t i = m_start; i < m_end; i++) {
            buffer[bufferIndex++] = bad.charAt(i);
        }
        buffer[bufferIndex] = 0;
        return CSSPropertyParser::parseNumber(buffer, len, 0, n);
    }

private:
    bool isBufferMode() const
    {
        return m_end == SIZE_MAX;
    }

    size_t m_start;
    size_t m_end;
    std::string m_buffer;
};

typedef std::vector<PathToken> PathTokenVector;

// this array comes from these separator
// ",mMzZlLhHvVcCsSqQtTaA-e"
static constexpr bool s_sepArray[] = {
    false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false,
    true,  true,  false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, true,
    false, true,  false, false, false, false, true,  false, false, false, true,
    true,  false, false, false, true,  false, true,  true,  false, true,  false,
    false, false, true,  false, false, false, false, false, false, true,  false,
    true,  false, true,  false, false, true,  false, false, false, true,  true,
    false, false, false, true,  false, true,  true,  false, true,  false, false,
    false, true,  false, false, false, false, false
};

static PathTokenVector tokenizePathValue(const StringBufferAccessData& bad)
{
    PathTokenVector tokens;
    tokens.reserve(16);
    PathToken str;
    char prevChar, currentChar;
    for (size_t i = 0; i < bad.length; i++) {
        auto ch = bad.charAt(i);
        if (ch > static_cast<char32_t>(std::numeric_limits<char>::max())) {
            return PathTokenVector();
        }
        currentChar = static_cast<char>(ch);

        if (str.length() == 0) {
            str.setRange(i, i + 1);
        } else {
            str.expand();
        }
        bool hasSepChar = s_sepArray[static_cast<size_t>(currentChar)];
        // below line cover this case "1.37916809e-13"
        if (currentChar == '-' && i >= 1 && prevChar == 'e') {
            hasSepChar = false;
        }

        if ((String::isSpaceOrNewline(currentChar) || hasSepChar)) {
            str.shrink();
            bool onlyWhiteSpace = true;
            for (size_t i = 0; i < str.length(); i++) {
                if (!String::isASCIISpace(str.charAt(bad, i))) {
                    onlyWhiteSpace = false;
                }
            }

            if (!onlyWhiteSpace) {
                tokens.push_back(str);
                str.setRange(0, 0);
            }
            if (hasSepChar) {
                tokens.push_back(PathToken(i, i + 1));
            }
        } else if (i == bad.length - 1 && str.length()) {
            tokens.push_back(str);
        }
        prevChar = currentChar;
    }
    return tokens;
}

Path* SVGPathElement::parsePath(String* d)
{
    if (d->length()) {
        Path* path = Path::create();
        auto bad = d->bufferAccessData();
        auto tokens = tokenizePathValue(bad);

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
        float x = 0, y = 0, x2 = 0, y2 = 0;
        float x3 = 0, y3 = 0, x4 = 0, y4 = 0;
        float lastX = 0, lastY = 0;
        float lastOfLastX = 0, lastOfLastY = 0;
        float lastMoveX = 0, lastMoveY = 0;
        char paintMode = ' ';
        char prevMode = ' ';
#define TO_WAIT_COORDS_MODE(p) \
    prevMode = paintMode;      \
    paintMode = p;             \
    mode = Mode::WaitCoordsX;

#define READ_NUMBER(n)                 \
    if (!token.parseNumber(bad, &n)) { \
        break;                         \
    }                                  \
    if (gotMinus) {                    \
        n = -n;                        \
    }                                  \
    gotMinus = false;

#define REWIND_IF_NEEDED()                                    \
    bool isLookAheadNumber = false;                           \
    for (size_t j = i + 1; j < tokens.size(); j++) {          \
        if (tokens[j].equals(bad, ',')) {                     \
            continue;                                         \
        }                                                     \
        char c = tokens[j].charAt(bad, 0);                    \
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

            if (token.equals(bad, ',')) {
                continue;
            }
            if (token.equals(bad, '-') && (mode != Mode::WaitCommand)) {
                if (gotMinus) {
                    // error
                    break;
                }
                gotMinus = true;
                continue;
            }

            if (mode != Mode::WaitCommand) {
                bool hasMultipleDot = false;
                bool seenDot = false;
                for (size_t k = 0; k < token.size(); k++) {
                    if (token.charAt(bad, k) == '.') {
                        if (!seenDot) {
                            seenDot = true;
                        } else {
                            hasMultipleDot = true;
                            tokens.erase(tokens.begin() + i);
                            PathToken s1 = token.substr(0, k);
                            PathToken s2 = token.substr(k, token.size() - k);
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
                if (i + 1 < tokens.size() && tokens[i + 1].equals(bad, 'e')) {
                    if (i + 2 < tokens.size()) {
                        token.setString(token.toString(bad) +
                                        tokens[i + 1].toString(bad) +
                                        tokens[i + 2].toString(bad));
                        if (tokens[i + 2].equals(bad, '-')) {
                            if (i + 3 < tokens.size()) {
                                token.setString(token.toString(bad) +
                                                tokens[i + 3].toString(bad));
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
                if (token.equals(bad, 'm')) {
                    TO_WAIT_COORDS_MODE('m');
                } else if (token.equals(bad, 'M')) {
                    TO_WAIT_COORDS_MODE('M');
                } else if (token.equals(bad, 'z') || token.equals(bad, 'Z')) {
                    path->closePath();
                    lastX = lastMoveX;
                    lastY = lastMoveY;
                    continue;
                } else if (token.equals(bad, 'l')) {
                    TO_WAIT_COORDS_MODE('l');
                } else if (token.equals(bad, 'L')) {
                    TO_WAIT_COORDS_MODE('L');
                } else if (token.equals(bad, 'c')) {
                    TO_WAIT_COORDS_MODE('c');
                } else if (token.equals(bad, 'C')) {
                    TO_WAIT_COORDS_MODE('C');
                } else if (token.equals(bad, 's')) {
                    TO_WAIT_COORDS_MODE('s');
                } else if (token.equals(bad, 'S')) {
                    TO_WAIT_COORDS_MODE('S');
                } else if (token.equals(bad, 'q')) {
                    TO_WAIT_COORDS_MODE('q');
                } else if (token.equals(bad, 'Q')) {
                    TO_WAIT_COORDS_MODE('Q');
                } else if (token.equals(bad, 't')) {
                    TO_WAIT_COORDS_MODE('t');
                } else if (token.equals(bad, 'T')) {
                    TO_WAIT_COORDS_MODE('T');
                } else if (token.equals(bad, 'a')) {
                    TO_WAIT_COORDS_MODE('a');
                } else if (token.equals(bad, 'A')) {
                    TO_WAIT_COORDS_MODE('A');
                } else if (token.equals(bad, 'h')) {
                    TO_WAIT_COORDS_MODE('h');
                } else if (token.equals(bad, 'H')) {
                    TO_WAIT_COORDS_MODE('H');
                } else if (token.equals(bad, 'v')) {
                    TO_WAIT_COORDS_MODE('v');
                } else if (token.equals(bad, 'V')) {
                    TO_WAIT_COORDS_MODE('V');
                } else {
                    STARFISH_UNSUPPORTED("SVGPath token: %s",
                                         token.toString(bad).data());
                    // error
                    break;
                }
            } else if (mode == Mode::WaitCoordsX) {
                READ_NUMBER(x);

                if (paintMode == 'v') {
                    path->lineTo(lastX, lastY + x);
                    lastY += x;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'V') {
                    path->lineTo(lastX, x);
                    lastY = x;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'h') {
                    path->lineTo(lastX + x, lastY);
                    lastX += x;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'H') {
                    path->lineTo(x, lastY);
                    lastX = x;
                    REWIND_IF_NEEDED();
                } else {
                    mode = Mode::WaitCoordsY;
                }
            } else if (mode == Mode::WaitCoordsY) {
                READ_NUMBER(y);
                if (paintMode == 'm') {
                    path->moveTo(lastX + x, lastY + y);
                    lastX += x;
                    lastY += y;
                    lastMoveX = lastX;
                    lastMoveY = lastY;
                    REWIND_IF_NEEDED();
                    if (mode == Mode::WaitCoordsX) {
                        paintMode = 'l';
                    }
                } else if (paintMode == 'M') {
                    path->moveTo(x, y);
                    lastX = x;
                    lastY = y;
                    lastMoveX = lastX;
                    lastMoveY = lastY;
                    REWIND_IF_NEEDED();
                    if (mode == Mode::WaitCoordsX) {
                        paintMode = 'L';
                    }
                } else if (paintMode == 'l') {
                    path->lineTo(lastX + x, lastY + y);
                    lastX += x;
                    lastY += y;
                    REWIND_IF_NEEDED();
                } else if (paintMode == 'L') {
                    path->lineTo(x, y);
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
                        path->quadraticCurveTo(firstX, firstY, lastX + x,
                                               lastY + y);
                        lastX += x;
                        lastY += y;
                    } else {
                        path->quadraticCurveTo(firstX, firstY, x, y);
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
                        path->bezierCurveTo(firstX, firstY, lastX + x,
                                            lastY + y, lastX + x2, lastY + y2);
                        lastOfLastX = lastX + x;
                        lastOfLastY = lastY + y;
                        lastX = lastX + x2;
                        lastY = lastY + y2;
                    } else if (paintMode == 'S') {
                        path->bezierCurveTo(firstX, firstY, x, y, x2, y2);
                        lastOfLastX = x;
                        lastOfLastY = y;
                        lastX = x2;
                        lastY = y2;
                    }
                    REWIND_IF_NEEDED()
                } else if (paintMode == 'q') {
                    path->quadraticCurveTo(lastX + x, lastY + y, lastX + x2,
                                           lastY + y2);
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
                    path->quadraticCurveTo(x, y, x2, y2);
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
                        path->bezierCurveTo(lastX + x, lastY + y, lastX + x2,
                                            lastY + y2, lastX + x3, lastY + y3);
                        lastOfLastX = lastX + x2;
                        lastOfLastY = lastY + y2;
                        lastX = lastX + x3;
                        lastY = lastY + y3;
                    } else if (paintMode == 'C') {
                        path->bezierCurveTo(x, y, x2, y2, x3, y3);
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
                    paintPathArcCommand(path, lastX, lastY, rx, ry,
                                        xAxisRotation, largeArcFlag, sweepFlag,
                                        targetX, targetY);
                    lastX = targetX;
                    lastY = targetY;
                    REWIND_IF_NEEDED();
                } else {
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            }
        }
        return path;
    }
    return nullptr;
}

void SVGPathElement::didAttributeChanged(QualifiedName name,
                                         Optional<String*> old, String* value,
                                         bool attributeCreated,
                                         bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_d == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsLayout();
        setNeedsPainting();
    }
}

void SVGPathElement::didComputedStyleChanged(ComputedStyle* oldStyle,
                                             ComputedStyle* newStyle,
                                             Optional<StyleResolveContext*> ctx)
{
    m_path = nullptr;
    if (newStyle) {
        m_path = parsePath(newStyle->d());
    }
}

void SVGPathElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);

    String* d = getAttributeOrVarReferencedValue(
        starfish()->staticStrings()->m_d, cssCustomValues);
    if (d->length()) {
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::D);
        pair.setPathFunctionValue(d);
        cssValues.push_back(pair);
    }
}
} // namespace Starfish
