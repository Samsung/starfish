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
#include "KeyBoardEventData.h"

namespace StarFish {

String* keyValueToKey(KeyValue v)
{
    if (v >= AKey && v <= ZKey) {
        char ch = v - AKey + 'A';
        return String::createASCIIString(ch);
    } else if (v >= LowerAKey && v <= LowerZKey) {
        char ch = v - LowerAKey + 'a';
        return String::createASCIIString(ch);
    } else if (v >= Digit0Key && v <= Digit9Key) {
        char ch = v - Digit0Key + '0';
        return String::createASCIIString(ch);
    } else if (v == EnterKey) {
        return String::createASCIIString("Enter");
    } else if (v == EscapeKey) {
        return String::createASCIIString("Escape");
    } else if (v == SpaceKey) {
        return String::createASCIIString(" ");
    } else if (v == AtMarkKey) {
        return String::createASCIIString("@");
    } else if (v == PeriodKey) {
        return String::createASCIIString(".");
    } else if (v == ArrowUpKey) {
        return String::createASCIIString("ArrowUp");
    } else if (v == ArrowDownKey) {
        return String::createASCIIString("ArrowDown");
    } else if (v == ArrowLeftKey) {
        return String::createASCIIString("ArrowLeft");
    } else if (v == ArrowRightKey) {
        return String::createASCIIString("ArrowRight");
    } else if (v == MinusMarkKey) {
        return String::createASCIIString("-");
    } else if (v == DeleteKey) {
        return String::createASCIIString("Delete");
    } else {
        if (v == UnidentifiedKey) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        return String::createASCIIString("undefined");
    }
}

String* keyValueToCode(KeyValue v)
{
    if (v >= AKey && v <= ZKey) {
        char ch = v - AKey + 'A';
        char buf[16] = "Key";
        buf[3] = ch;
        buf[4] = 0;
        return String::createASCIIString(buf);
    } else if (v >= LowerAKey && v <= LowerZKey) {
        char ch = v - LowerAKey + 'A';
        char buf[16] = "Key";
        buf[3] = ch;
        buf[4] = 0;
        return String::createASCIIString(buf);
    } else if (v >= Digit0Key && v <= Digit9Key) {
        char ch = v - Digit0Key + '0';
        char buf[16] = "Digit";
        buf[5] = ch;
        buf[6] = 0;
        return String::createASCIIString(ch);
    } else if (v == EnterKey) {
        return String::createASCIIString("Enter");
    } else if (v == EscapeKey) {
        return String::createASCIIString("Escape");
    } else if (v == SpaceKey) {
        return String::createASCIIString("Space");
    } else if (v == AtMarkKey) {
        return String::createASCIIString("Digit2");
    } else if (v == PeriodKey) {
        return String::createASCIIString("Period");
    } else if (v == ArrowUpKey) {
        return String::createASCIIString("ArrowUp");
    } else if (v == ArrowDownKey) {
        return String::createASCIIString("ArrowDown");
    } else if (v == ArrowLeftKey) {
        return String::createASCIIString("ArrowLeft");
    } else if (v == ArrowRightKey) {
        return String::createASCIIString("ArrowRight");
    } else if (v == MinusMarkKey) {
        return String::createASCIIString("Minus");
    } else if (v == DeleteKey) {
        return String::createASCIIString("Delete");
    } else {
        if (v == UnidentifiedKey) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        return String::createASCIIString("undefined");
    }
}

uint32_t keyValueToKeyCode(KeyValue v, bool isForVirtualKeyCode)
{
    if (v >= AKey && v <= ZKey) {
        char ch = v - AKey + 'A';
        return ch;
    } else if (v >= LowerAKey && v <= LowerZKey) {
        if (isForVirtualKeyCode) {
            return (v - LowerAKey + 'A');
        } else {
            return (v - LowerAKey + 'a');
        }
    } else if (v >= Digit0Key && v <= Digit9Key) {
        char ch = v - Digit0Key + '0';
        return ch;
    } else if (v == SpaceKey) {
        return 32;
    } else if (v == AtMarkKey) {
        return '@';
    } else if (v == PeriodKey) {
        return '.';
    } else if (v == ArrowLeftKey) {
        return 37;
    } else if (v == ArrowRightKey) {
        return 39;
    } else if (v == ArrowUpKey) {
        return 38;
    } else if (v == ArrowDownKey) {
        return 40;
    } else if (v == EnterKey) {
        return 13;
    } else if (v == EscapeKey) {
        return 27;
    } else if (v == BackspaceKey) {
        return 8;
    } else if (v == MinusMarkKey) {
        if (isForVirtualKeyCode) {
            return 189;
        } else {
            return 45;
        }
    } else if (v == DeleteKey) {
        return 46;
    }
#ifdef STARFISH_TIZEN_TV
    else if (v == TVVolumeUpKey) {
        return 447;
    } else if (v == TVVolumeDownKey) {
        return 448;
    } else if (v == TVMuteKey) {
        return 449;
    } else if (v == TVChannelUpKey) {
        return 427;
    } else if (v == TVChannelDownKey) {
        return 428;
    } else if (v == MediaTrackPreviousKey) {
        return 412;
    } else if (v == MediaTrackNextKey) {
        return 417;
    } else if (v == MediaPauseKey) {
        return 19;
    } else if (v == MediaRecordKey) {
        return 416;
    } else if (v == MediaPlayKey) {
        return 415;
    } else if (v == MediaStopKey) {
        return 413;
    } else if (v == TVInfoKey) {
        return 457;
    } else if (v == TVReturnKey) {
        return 0;
    } else if (v == TVRedKey) {
        return 403;
    } else if (v == TVGreenKey) {
        return 404;
    } else if (v == TVYellowKey) {
        return 405;
    } else if (v == TVBlueKey) {
        return 406;
    } else if (v == TVMenuKey) {
        return 18;
    }
#endif
    else {
        return 0;
    }
}

// Return the Unicode reference number
// This implementation for charCode, but it is deprecated. So we implemented it
// to a minimum.
uint32_t keyValueToCharCode(KeyValue v)
{
    // 32 ~ 126 are equal to ASCII values.
    if (32 <= v && v <= 126) {
        return keyValueToKeyCode(v);
    } else {
        return 0;
    }
}
} // namespace StarFish
