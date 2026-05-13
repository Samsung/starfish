/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCSSStyleSheetInit__
#define __StarfishCSSStyleSheetInit__

#include "binding/generated/MediaListOrDOMStringUnion.h"

namespace Starfish {

class String;

struct CSSStyleSheetInit {
    CSSStyleSheetInit()
    {
    }

    void setBaseURL(Optional<String*> baseURL)
    {
        m_baseURL = baseURL;
    }

    Optional<String*> baseURL() const
    {
        return m_baseURL;
    }

    void setMedia(const MediaListOrDOMString& media)
    {
        m_media = media;
    }

    const MediaListOrDOMString& media() const
    {
        return m_media;
    }

    void setDisabled(bool disabled)
    {
        m_disabled = disabled;
    }

    bool disabled() const
    {
        return m_disabled;
    }

    Optional<String*> m_baseURL;
    MediaListOrDOMString m_media;
    bool m_disabled = false;
};

} // namespace Starfish

#endif
