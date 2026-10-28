/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 *               2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 *               2001-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2008 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "StarfishConfig.h"
#include "core/style/FontFaceSrcData.h"
#include "core/style/StyleUtil.h"

namespace Starfish {

String* FontFaceSrcData::toString()
{
    StringBuilder builder;

    for (size_t i = 0; i < m_data.size(); i++) {
        auto loadFrom = std::get<1>(m_data[i]);
        if (loadFrom == URL) {
            builder.appendString("url(");
        } else {
            builder.appendString("local(");
        }
        StyleUtil::appendCSSString(builder, std::get<0>(m_data[i]));
        builder.appendChar(')');

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
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
            builder.appendString("\")");
        }

        if (i + 1 != m_data.size()) {
            builder.appendString(", ");
        }
    }

    return builder.finalize();
}

} // namespace Starfish
