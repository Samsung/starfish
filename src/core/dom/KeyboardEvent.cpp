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
#include "KeyboardEvent.h"
#include "platform/event/PlatformKeyEventData.h"

namespace Starfish {

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
#ifdef STARFISH_TIZEN_TV
    } else if (v == EscapeKey) {
        return String::createASCIIString("XF86Back");
    } else if (v == TVHomeKey) {
        return String::createASCIIString("XF86Home");
    } else if (v == TVExitKey) {
        return String::createASCIIString("XF86Exit");
    } else if (v == TVPreviousChannel) {
        return String::createASCIIString("XF86PreviousChannel");
    } else if (v == TVVolumeUpKey) {
        return String::createASCIIString("F10");
    } else if (v == TVVolumeDownKey) {
        return String::createASCIIString("F9");
    } else if (v == TVChannelUpKey) {
        return String::createASCIIString("XF86RaiseChannel");
    } else if (v == TVChannelDownKey) {
        return String::createASCIIString("XF86LowerChannel");
    } else if (v == TVMuteKey) {
        return String::createASCIIString("F8");
    } else if (v == TVChannelList) {
        return String::createASCIIString("XF86ChannelList");
    } else if (v == TVMenuKey) {
        return String::createASCIIString("XF86SysMenu");
    } else if (v == TVChannelGuide) {
        return String::createASCIIString("XF86ChannelGuide");
    } else if (v == TVSimpleMenu) {
        return String::createASCIIString("XF86SimpleMenu");
    } else if (v == TVInfoKey) {
        return String::createASCIIString("XF86Info");
    } else if (v == TVRedKey) {
        return String::createASCIIString("XF86Red");
    } else if (v == TVGreenKey) {
        return String::createASCIIString("XF86Green");
    } else if (v == TVYellowKey) {
        return String::createASCIIString("XF86Yellow");
    } else if (v == TVBlueKey) {
        return String::createASCIIString("XF86Blue");
    } else if (v == TVEManual) {
        return String::createASCIIString("XF86EManual");
    } else if (v == TVExtraApp) {
        return String::createASCIIString("XF86ExtraApp");
    } else if (v == TVSearch) {
        return String::createASCIIString("XF86Search");
    } else if (v == TVPictureSize) {
        return String::createASCIIString("XF86PictureSize");
    } else if (v == TVSleep) {
        return String::createASCIIString("XF86Sleep");
    } else if (v == TVCaption) {
        return String::createASCIIString("XF86Caption");
    } else if (v == MediaTrackPreviousKey) {
        return String::createASCIIString("XF86AudioRewind");
    } else if (v == MediaTrackNextKey) {
        return String::createASCIIString("XF86AudioNext");
    } else if (v == MediaPauseKey) {
        return String::createASCIIString("XF86AudioPause");
    } else if (v == MediaRecordKey) {
        return String::createASCIIString("XF86AudioRecord");
    } else if (v == MediaPlayKey) {
        return String::createASCIIString("XF86AudioPlay");
    } else if (v == MediaStopKey) {
        return String::createASCIIString("XF86AudioStop");
    } else if (v == TVMore) {
        return String::createASCIIString("XF86More");
    } else if (v == TVBTVoice) {
        return String::createASCIIString("XF86BTVoice");
    } else if (v == TVColor) {
        return String::createASCIIString("XF86Color");
    } else if (v == TVPlayBack) {
        return String::createASCIIString("XF86PlayBack");
#else
    } else if (v == EscapeKey) {
        return String::createASCIIString("Escape");
#endif
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
        char buf[5] = "Key";
        buf[3] = ch;
        buf[4] = 0;
        return String::createASCIIString(buf);
    } else if (v >= LowerAKey && v <= LowerZKey) {
        char ch = v - LowerAKey + 'A';
        char buf[5] = "Key";
        buf[3] = ch;
        buf[4] = 0;
        return String::createASCIIString(buf);
    } else if (v >= Digit0Key && v <= Digit9Key) {
        char ch = v - Digit0Key + '0';
        char buf[7] = "Digit";
        buf[5] = ch;
        buf[6] = 0;
        return String::createASCIIString(buf);
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
#ifdef STARFISH_TIZEN_TV
    } else if (v == TVHomeKey) {
        return String::createASCIIString("F5");
    } else if (v == TVExitKey) {
        return String::createASCIIString("");
    } else if (v == TVPreviousChannel) {
        return String::createASCIIString("");
    } else if (v == TVVolumeUpKey) {
        return String::createASCIIString("F10");
    } else if (v == TVVolumeDownKey) {
        return String::createASCIIString("F9");
    } else if (v == TVChannelUpKey) {
        return String::createASCIIString("F12");
    } else if (v == TVChannelDownKey) {
        return String::createASCIIString("F11");
    } else if (v == TVMuteKey) {
        return String::createASCIIString("F8");
    } else if (v == TVChannelList) {
        return String::createASCIIString("F7");
    } else if (v == TVMenuKey) {
        return String::createASCIIString("MetaLeft");
    } else if (v == TVChannelGuide) {
        return String::createASCIIString("");
    } else if (v == TVSimpleMenu) {
        return String::createASCIIString("ContextMenu");
    } else if (v == TVInfoKey) {
        return String::createASCIIString("F18");
    } else if (v == TVRedKey) {
        return String::createASCIIString("F1");
    } else if (v == TVGreenKey) {
        return String::createASCIIString("F2");
    } else if (v == TVYellowKey) {
        return String::createASCIIString("F3");
    } else if (v == TVBlueKey) {
        return String::createASCIIString("F4");
    } else if (v == TVEManual) {
        return String::createASCIIString("Help");
    } else if (v == TVExtraApp) {
        return String::createASCIIString("");
    } else if (v == TVSearch) {
        return String::createASCIIString("BrowserSearch");
    } else if (v == TVPictureSize) {
        return String::createASCIIString("Select");
    } else if (v == TVSleep) {
        return String::createASCIIString("Sleep");
    } else if (v == TVCaption) {
        return String::createASCIIString("");
    } else if (v == MediaTrackPreviousKey) {
        return String::createASCIIString("MediaRewind");
    } else if (v == MediaTrackNextKey) {
        return String::createASCIIString("MediaFastForward");
    } else if (v == MediaPauseKey) {
        return String::createASCIIString("");
    } else if (v == MediaRecordKey) {
        return String::createASCIIString("MediaRecord");
    } else if (v == MediaPlayKey) {
        return String::createASCIIString("");
    } else if (v == MediaStopKey) {
        return String::createASCIIString("MediaStop");
    } else if (v == TVMore) {
        return String::createASCIIString("LaunchApp2");
    } else if (v == TVBTVoice) {
        return String::createASCIIString("");
    } else if (v == TVColor) {
        return String::createASCIIString("");
    } else if (v == TVPlayBack) {
        return String::createASCIIString("");
#endif
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
#ifdef STARFISH_TIZEN_TV
    } else if (v == TVVolumeUpKey) {
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
        return 10133;
    } else if (v == TVHomeKey) {
        return 10071;
    } else if (v == TVExitKey) {
        return 10182;
    } else if (v == EscapeKey) {
        return 10009;
    } else if (v == TVPreviousChannel) {
        return 10190;
    } else if (v == TVChannelList) {
        return 10073;
    } else if (v == TVChannelGuide) {
        return 458;
    } else if (v == TVSimpleMenu) {
        return 10135;
    } else if (v == TVEManual) {
        return 10146;
    } else if (v == TVExtraApp) {
        return 10253;
    } else if (v == TVSearch) {
        return 10225;
    } else if (v == TVPictureSize) {
        return 10140;
    } else if (v == TVSleep) {
        return 10150;
    } else if (v == TVCaption) {
        return 10221;
    } else if (v == TVMore) {
        return 10148;
    } else if (v == TVBTVoice) {
        return 10224;
    } else if (v == TVColor) {
        return 10385;
    } else if (v == TVPlayBack) {
        return 10252;
#else
    } else if (v == EscapeKey) {
        return 27;
#endif
    } else {
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

KeyboardEventInit::KeyboardEventInit(PlatformKeyEventData& kdata)
    : EventModifierInit(kdata.m_eventModifierData)
    , m_keyboardEventData(kdata.m_keyboardEventData)
{
}
} // namespace Starfish
