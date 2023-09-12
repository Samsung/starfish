/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWindowKeyType__
#define __StarfishWindowKeyType__

namespace LWE {

enum class INPUT : unsigned long {
    // clang-format off
    NONE = 0,
    // ACTION
    RELEASE,
    PRESS,
    ACTION_END,
    // TYPE
    MOUSE_LBUTTON,
    TYPE_END,
    // CODE
    LEFT       = 0xff51,
    UP         = 0xff52,
    RIGHT      = 0xff53,
    DOWN       = 0xff54,
    CODE_END,
    // clang-format on
};

enum class MOD : unsigned int {
    SHIFT = (1 << 0),
    CONTROL = (1 << 2),
};

enum class ASCII : unsigned int {
    NUL = 0, // Null character
    SOH,     // Start of Header
    STX,     // Start of Text
    ETX,     // End of Text
    EOT,     // End of Transmission
    ENQ,     // Enquiry
    ACK,     // Acknowledgment
    BEL,     // Bell
    BS,      // Backspace
    HT,      // Horizontal Tab
    LF,      // Line Feed
    VT,      // Vertical Tab
    FF,      // Form Feed
    CR,      // Carriage Return
    SO,      // Shift Out
    SI,      // Shift In
    DLE,     // Data Link Escape
    DC1,     // Device Control 1
    DC2,     // Device Control 2
    DC3,     // Device Control 3
    DC4,     // Device Control 4
    NAK,     // Negative Acknowledgment
    SYN,     // Synchronous Idle
    ETB,     // End of Transmission Block
    CAN,     // Cancel
    EM,      // End of Medium
    SUB,     // Substitute
    ESC,     // Escape
    FS,      // File Separator
    GS,      // Group Separator
    RS,      // Record Separator
    US,      // Unit Separator
    SPACE = 32,
    EXCLAMATION_MARK,
    DOUBLE_QUOTE,
    HASH,
    DOLLAR,
    PERCENT,
    AMPERSAND,
    SINGLE_QUOTE,
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    ASTERISK,
    PLUS,
    COMMA,
    MINUS,
    PERIOD,
    SLASH,
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    COLON,
    SEMICOLON,
    LESS_THAN,
    EQUAL,
    GREATER_THAN,
    QUESTION_MARK,
    AT,
    A = 65,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LEFT_BRACKET,
    BACKSLASH,
    RIGHT_BRACKET,
    CARET,
    UNDERSCORE,
    GRAVE_ACCENT,
    a = 97,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,
    LEFT_CURLY_BRACE,
    PIPE,
    RIGHT_CURLY_BRACE,
    TILDE,
    DEL
};

} // namespace LWE

#endif
