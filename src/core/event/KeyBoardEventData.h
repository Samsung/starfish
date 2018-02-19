/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishKeyBoardEventData__
#define __StarFishKeyBoardEventData__

namespace StarFish {

// This table has same chars with
// ASCII printable char(32-126)
enum KeyValue {
    UnidentifiedKey,
    AltLeftKey,
    AltRightKey,
    ControlLeftKey,
    ControlRightKey,
    CapsLockKey,
    FnKey,
    FnLockKey,
    HyperKey,
    MetaKey,
    NumLockKey,
    ScrollLockKey,
    ShiftLeftKey,
    ShiftRightKey,
    SuperKey,
    SymbolKey,
    SymbolLockKey,
    EnterKey,
    TabKey,
    ArrowDownKey,
    ArrowUpKey,
    ArrowLeftKey,
    ArrowRightKey,
    EndKey,
    HomeKey,
    PageDownKey,
    PageUpKey,
    BackspaceKey,
    DeleteKey,
    InsertKey,
    ContextMenuKey,
    EscapeKey,
    SpaceKey = 32,
    ExclamationMarkKey = 33,
    DoubleQuoteMarkKey = 34,
    SharpMarkKey = 35,
    DollarMarkKey = 36,
    PercentMarkKey = 37,
    AmpersandMarkKey = 38,
    SingleQuoteMarkKey = 39,
    LeftParenthesisMarkKey = 40,
    RightParenthesisMarkKey = 41,
    AsteriskMarkKey = 42,
    PlusMarkKey = 43,
    CommaMarkKey = 44,
    MinusMarkKey = 45,
    PeriodKey = 46,
    SlashKey = 47,
    Digit0Key = 48,
    Digit1Key,
    Digit2Key,
    Digit3Key,
    Digit4Key,
    Digit5Key,
    Digit6Key,
    Digit7Key,
    Digit8Key,
    Digit9Key,
    ColonMarkKey = 58,
    SemiColonMarkKey = 59,
    LessThanMarkKey = 60,
    EqualitySignKey = 61,
    GreaterThanSignKey = 62,
    QuestionMarkKey = 63,
    AtMarkKey = 64,
    AKey = 65,
    BKey,
    CKey,
    DKey,
    EKey,
    FKey,
    GKey,
    HKey,
    IKey,
    JKey,
    KKey,
    LKey,
    MKey,
    NKey,
    OKey,
    PKey,
    QKey,
    RKey,
    SKey,
    TKey,
    UKey,
    VKey,
    WKey,
    XKey,
    YKey,
    ZKey,
    LeftSquareBracketKey = 91,
    BackslashKey = 92,
    RightSquareBracketKey = 93,
    CaretMarkKey = 94,
    UnderScoreMarkKey = 95,
    AccentMarkKey = 96,
    LowerAKey = 97,
    LowerBKey,
    LowerCKey,
    LowerDKey,
    LowerEKey,
    LowerFKey,
    LowerGKey,
    LowerHKey,
    LowerIKey,
    LowerJKey,
    LowerKKey,
    LowerLKey,
    LowerMKey,
    LowerNKey,
    LowerOKey,
    LowerPKey,
    LowerQKey,
    LowerRKey,
    LowerSKey,
    LowerTKey,
    LowerUKey,
    LowerVKey,
    LowerWKey,
    LowerXKey,
    LowerYKey,
    LowerZKey,
    LeftCurlyBracketMarkKey = 123,
    VerticalBarMarkKey = 124,
    RightCurlyBracketMarkKey = 125,
    TildeMarkKey = 126,
    F1Key,
    F2Key,
    F3Key,
    F4Key,
    F5Key,
    F6Key,
    F7Key,
    F8Key,
    F9Key,
    F10Key,
    F11Key,
    F12Key,
    F13Key,
    F14Key,
    F15Key,
    F16Key,
    F17Key,
    F18Key,
    F19Key,
    F20Key,
    TVChannelDownKey,
    TVChannelUpKey,
    TVVolumeUpKey,
    TVVolumeDownKey,
    TVMuteKey,
    TVReturnKey,
    TVExitKey,
    TVInfoKey,
    TVRedKey,
    TVGreenKey,
    TVYellowKey,
    TVBlueKey,
    TVMenuKey,
    MediaFastForwardKey,
    MediaPauseKey,
    MediaPlayKey,
    MediaPlayPauseKey,
    MediaRecordKey,
    MediaRewindKey,
    MediaStopKey,
    MediaTrackNextKey,
    MediaTrackPreviousKey,
    TVKey,
    TV3DModeKey,
    TVAntennaCableKey,
    TVAudioDescriptionKey,
    TVAudioDescriptionMixDownKey,
    TVAudioDescriptionMixUpKey,
    TVContentsMenuKey,
    TVDataServiceKey,
    TVInputKey,
    TVInputComponent1Key,
    TVInputComponent2Key,
    TVInputComposite1Key,
    TVInputComposite2Key,
    TVInputHDMI1Key,
    TVInputHDMI2Key,
    TVInputHDMI3Key,
    TVInputHDMI4Key,
    TVInputVGA1Key,
    TVMediaContextKey,
    TVNetworkKey,
    TVNumberEntryKey,
    TVPowerKey,
    TVRadioServiceKey,
    TVSatelliteKey,
    TVSatelliteBSKey,
    TVSatelliteCSKey,
    TVSatelliteToggleKey,
    TVTerrestrialAnalogKey,
    TVTerrestrialDigitalKey,
    TVTimerKey,
    MediaAppsKey,
    MediaAudioTrackKey,
    MediaLastKey,
    MediaSkipBackwardKey,
    MediaSkipForwardKey,
    MediaStepBackwardKey,
    MediaStepForwardKey,
    MediaTopMenuKey,
    BrowserBackKey,
    BrowserFavoritesKey,
    BrowserForwardKey,
    BrowserHomeKey,
    BrowserRefreshKey,
    BrowserSearchKey,
    BrowserStopKey,
};

String* keyValueToKey(KeyValue v);
String* keyValueToCode(KeyValue v);
uint32_t keyValueToKeyCode(KeyValue v, bool isForVirtualKeyCode = false);
uint32_t keyValueToCharCode(KeyValue v);

class KeyboardEventData {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    KeyboardEventData(KeyValue value = KeyValue::UnidentifiedKey)
        : m_keyValue(value)
        , m_key(keyValueToKey(value))
        , m_code(keyValueToCode(value))
        , m_location(0)
        , m_repeat(false)
        , m_isComposing(false)
        , m_keyCode(keyValueToKeyCode(value))
        , m_charCode(keyValueToCharCode(value))
        , m_virtualKeyCode(keyValueToKeyCode(value, true))
    {
    }

    KeyValue keyValue() const
    {
        return m_keyValue;
    }

    String* key() const
    {
        return m_key;
    }

    void setKey(String* key)
    {
        m_key = key;
    }

    String* code() const
    {
        return m_code;
    }

    void setCode(String* code)
    {
        m_code = code;
    }

    uint32_t location() const
    {
        return m_location;
    }

    void setLocation(uint32_t location)
    {
        m_location = location;
    }

    bool repeat() const
    {
        return m_repeat;
    }

    void setRepeat(bool repeat)
    {
        m_repeat = repeat;
    }

    bool isComposing() const
    {
        return m_isComposing;
    }

    void setIsComposing(bool isComposing)
    {
        m_isComposing = isComposing;
    }

    uint32_t keyCode() const
    {
        return m_keyCode;
    }

    void setKeyCode(uint32_t keyCode)
    {
        m_keyCode = keyCode;
    }

    uint32_t charCode() const
    {
        return m_charCode;
    }

    void setCharCode(uint32_t charCode)
    {
        m_charCode = charCode;
    }

    uint32_t virtualKeyCode() const
    {
        return m_virtualKeyCode;
    }

private:
    KeyValue m_keyValue;
    String* m_key;
    String* m_code;
    uint32_t m_location;
    bool m_repeat;
    bool m_isComposing;
    uint32_t m_keyCode;
    uint32_t m_charCode;
    uint32_t m_virtualKeyCode;
};
} // namespace StarFish
#endif
