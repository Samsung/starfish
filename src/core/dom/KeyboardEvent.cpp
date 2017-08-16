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
#include "KeyboardEvent.h"

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
    } else {
        if (v == UnidentifiedKey) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        return String::createASCIIString("");
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
    } else {
        if (v == UnidentifiedKey) {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        return String::createASCIIString("");
    }
}

uint32_t keyValueToKeyCode(KeyValue v)
{
    if (v >= AKey && v <= ZKey) {
        char ch = v - AKey + 'A';
        return ch;
    } else if (v >= LowerAKey && v <= LowerZKey) {
        char ch = v - LowerAKey + 'a';
        return ch;
    } else if (v >= Digit0Key && v <= Digit9Key) {
        char ch = v - Digit0Key + '0';
        return ch;
    } else if (v == SpaceKey) {
        return ' ';
    } else if (v == AtMarkKey) {
        return '@';
    } else if (v == PeriodKey) {
        return '.';
    } else {
        return 0;
    }
}
}
