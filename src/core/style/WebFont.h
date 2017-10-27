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

#ifndef __StarFishWebFont__
#define __StarFishWebFont__

#include "core/style/Style.h"
#include "platform/loader/FontResource.h"

namespace StarFish {

class WebFont : public gc {
public:
    WebFont(bool isFontStyleSpecified, bool isFontWeightSpecified,
            String* familyName, FontStyleValue fontStyleValue,
            FontWeightValue fontWeightValue, FontResource* fontResource)
    {
        m_fromLocal = false;
        m_isFontStyleSpecified = isFontStyleSpecified;
        m_isFontWeightSpecified = isFontWeightSpecified;
        m_familyName = familyName;
        m_fontStyleValue = fontStyleValue;
        m_fontWeightValue = fontWeightValue;
        m_fontResource = fontResource;
    }

    WebFont(bool isFontStyleSpecified, bool isFontWeightSpecified,
            String* familyName, FontStyleValue fontStyleValue,
            FontWeightValue fontWeightValue, String* localFontName)
    {
        m_fromLocal = true;
        m_isFontStyleSpecified = isFontStyleSpecified;
        m_isFontWeightSpecified = isFontWeightSpecified;
        m_familyName = familyName;
        m_fontStyleValue = fontStyleValue;
        m_fontWeightValue = fontWeightValue;
        m_localFontName = localFontName;
    }

    bool isFontStyleSpecified()
    {
        return m_isFontStyleSpecified;
    }

    bool isFontWeightSpecified()
    {
        return m_isFontWeightSpecified;
    }

    bool fromLocal()
    {
        return m_fromLocal;
    }

    String* familyName()
    {
        return m_familyName;
    }

    FontStyleValue fontStyleValue()
    {
        return m_fontStyleValue;
    }

    FontWeightValue fontWeightValue()
    {
        return m_fontWeightValue;
    }

    FontResource* fontResource()
    {
        STARFISH_ASSERT(!m_fromLocal);
        return m_fontResource;
    }

    String* localFontName()
    {
        STARFISH_ASSERT(m_fromLocal);
        return m_localFontName;
    }

private:
    bool m_fromLocal;
    bool m_isFontStyleSpecified;
    bool m_isFontWeightSpecified;
    FontStyleValue m_fontStyleValue;
    FontWeightValue m_fontWeightValue;

    String* m_familyName;
    union {
        FontResource* m_fontResource;
        String* m_localFontName;
    };
};

} /* namespace StarFish */

#endif /* __StarFishWebFont__ */
