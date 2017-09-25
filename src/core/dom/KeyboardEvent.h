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

#ifndef __StarFishKeyboardEvent__
#define __StarFishKeyboardEvent__

#include "UIEvent.h"

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

class KeyboardData {
    STARFISH_MAKE_STACK_ALLOCATED()
    friend KeyboardEvent;

public:
    KeyboardData(KeyValue value = KeyValue::UnidentifiedKey)
        : m_keyValue(value)
        , m_key(keyValueToKey(value))
        , m_code(keyValueToCode(value))
        , m_keyCode(keyValueToKeyCode(value))
        , m_virtualKeyCode(keyValueToKeyCode(value, true))
        , m_ctrlKey(false)
        , m_shiftKey(false)
        , m_altKey(false)
        , m_metaKey(false)
        , m_repeat(false)
    {
    }

    bool isASCIIVisibleChar()
    {
        return keyValue() < 127 &&
               String::isASCIIPrintableKey((char)keyValue());
    }

    KeyValue keyValue()
    {
        return m_keyValue;
    }

    String* key()
    {
        return m_key;
    }
    String* code()
    {
        return m_code;
    }
    uint32_t keyCode()
    {
        return m_keyCode;
    }
    bool ctrlKey()
    {
        return m_ctrlKey;
    }
    bool shiftKey()
    {
        return m_shiftKey;
    }
    bool altKey()
    {
        return m_altKey;
    }
    bool metaKey()
    {
        return m_metaKey;
    }
    bool repeat() const
    {
        return m_repeat;
    }
    void setCtrlKey()
    {
        m_ctrlKey = true;
    }
    void setShiftKey()
    {
        m_shiftKey = true;
    }
    void setAltKey()
    {
        m_altKey = true;
    }
    void setMetaKey()
    {
        m_metaKey = true;
    }
    void setRepeat()
    {
        m_repeat = true;
    }

protected:
    KeyValue m_keyValue;
    String* m_key;
    String* m_code;
    uint32_t m_keyCode;
    uint32_t m_virtualKeyCode;
    bool m_ctrlKey;
    bool m_shiftKey;
    bool m_altKey;
    bool m_metaKey;
    bool m_repeat;
};

class KeyboardEvent : public UIEvent {
public:
    KeyboardEvent(Document* document)
        : UIEvent(document)
        , m_keyboardData()
    {
    }

    KeyboardEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_keyboardData()
    {
    }

    KeyboardEvent(Document* document, String* eventType, KeyboardData& data)
        : UIEvent(document, eventType)
        , m_keyboardData(data)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isKeyboardEvent() const override;

    bool isASCIIVisibleChar()
    {
        return m_keyboardData.isASCIIVisibleChar();
    }
    KeyValue keyValue()
    {
        return m_keyboardData.m_keyValue;
    }
    String* key()
    {
        return m_keyboardData.m_key;
    }
    String* code()
    {
        return m_keyboardData.m_code;
    }
    uint32_t keyCode()
    {
        if (type()->equals(String::createASCIIString("keydown"))) {
            return m_keyboardData.m_virtualKeyCode;
        }
        return m_keyboardData.m_keyCode;
    }
    bool ctrlKey()
    {
        return m_keyboardData.m_ctrlKey;
    }
    bool shiftKey()
    {
        return m_keyboardData.m_shiftKey;
    }
    bool altKey()
    {
        return m_keyboardData.m_altKey;
    }
    bool metaKey()
    {
        return m_keyboardData.m_metaKey;
    }
    bool repeat()
    {
#ifndef NDEBUG
#ifndef PORT_GRAPHIC_BACKEND_EFL
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
#endif
#endif
        return m_keyboardData.m_repeat;
    }

private:
    KeyboardData m_keyboardData;
};
}

#endif
