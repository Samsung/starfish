/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFontFaceSrcData__
#define __StarFishFontFaceSrcData__

#include "core/util/String.h"
#include "core/style/Length.h"
#include "core/style/Style.h"

namespace StarFish {

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

    String* toString()
    {
        StringBuilder builder;

        for (size_t i = 0; i < m_data.size(); i++) {
            auto loadFrom = std::get<1>(m_data[i]);
            if (loadFrom == URL) {
                builder.appendString("url(\"");
            } else {
                builder.appendString("local(\"");
            }
            builder.appendString(std::get<0>(m_data[i]));
            builder.appendString("\")");

            auto format = std::get<2>(m_data[i]);
            if (format >= SVG) {
                builder.appendString(" format(\"");

                if (format == TrueType) {
                    builder.appendString("truetype");
                } else if (format == WOFF) {
                    builder.appendString("woff");
                } else if (format == WOFF2) {
                    builder.appendString("woff2");
                } else if (format == OpenType) {
                    builder.appendString("opentype");
                } else if (format == EmbeddedOpenType) {
                    builder.appendString("embedded-opentype");
                } else if (format == SVG) {
                    builder.appendString("svg");
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                builder.appendString("\")");
            }

            if (i + 1 != m_data.size()) {
                builder.appendString(", ");
            }
        }

        return builder.finalize();
    }

protected:
    FontFaceSrcDataList m_data;
};
}

#endif
