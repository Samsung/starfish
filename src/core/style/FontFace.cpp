/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"
#include "core/style/FontFace.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

ScriptBindingInstance* FontFace::scriptBindingInstance()
{
    return m_document->executionContext()->scriptBindingInstance();
}

String* FontFace::status()
{
    if (m_font.fromLocal()) {
        switch (m_localStatus) {
        case LocalStatus::Loaded:
            return String::createASCIIString("loaded");
        case LocalStatus::Error:
            return String::createASCIIString("error");
        case LocalStatus::Unloaded:
            return String::createASCIIString("unloaded");
        }
    }

    auto* resource = m_font.fontResource();
    switch (resource->state()) {
    case Resource::Finished:
        return String::createASCIIString("loaded");
    case Resource::Failed:
    case Resource::Canceled:
        return String::createASCIIString("error");
    default:
        return resource->isRequested() ? String::createASCIIString("loading")
                                       : String::createASCIIString("unloaded");
    }
}

bool FontFace::loadLocal()
{
    if (m_localStatus == LocalStatus::Unloaded) {
        m_localStatus = m_document->fontSelector()->loadLocalWebFont(
                            m_font.localFontName(), m_font.fontStyleValue(),
                            m_font.fontWeightValue())
                            ? LocalStatus::Loaded
                            : LocalStatus::Error;
    }
    return m_localStatus == LocalStatus::Loaded;
}

} // namespace Starfish
