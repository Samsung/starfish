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

#ifndef __StarfishFontFaceSrcData__
#define __StarfishFontFaceSrcData__

#include "core/util/String.h"
#include "core/style/Length.h"
#include "core/style/Style.h"

namespace Starfish {

class FontFaceSrcData : public gc {
    friend class CSSStyleValuePair;

public:
    enum LoadFrom { Local, URL };
    // This order is related with FreeType font support
    // don't change
    enum Format {
        Unknown,
        NotSpecified,
        SVG,
        WOFF2,
        EmbeddedOpenType,
        OpenType,
        TrueType,
        WOFF,
    };

    typedef GCVector<std::tuple<String*, LoadFrom, Format>> FontFaceSrcDataList;

    const FontFaceSrcDataList& data()
    {
        return m_data;
    }

    bool equals(FontFaceSrcData* src)
    {
        if (m_data.size() != src->m_data.size()) {
            return false;
        }

        for (size_t i = 0; i < m_data.size(); i++) {
            if (!std::get<0>(m_data[i])->equals(std::get<0>(src->m_data[i]))) {
                return false;
            }

            if (std::get<1>(m_data[i]) != std::get<1>(src->m_data[i])) {
                return false;
            }

            if (std::get<2>(m_data[i]) != std::get<2>(src->m_data[i])) {
                return false;
            }
        }

        return true;
    }

    String* toString();

protected:
    FontFaceSrcDataList m_data;
};
} // namespace Starfish

#endif
