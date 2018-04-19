/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "core/style/ImageValue.h"
#include "core/style/GradientData.h"

namespace StarFish {

String* ImageValue::urlValue() const
{
    STARFISH_ASSERT(m_valueType.isURL());
    return m_valueData.m_url;
}

GradientData* ImageValue::gradientValue() const
{
    STARFISH_ASSERT(m_valueType.isGradient());
    return m_valueData.m_gradient;
}

void ImageValue::applyOriginToURL(const ResourceURL* origin)
{
    STARFISH_ASSERT(m_valueType.isURL());
    m_valueData.m_url = ResourceURL::mergeDocumentURIWithURIString(
        origin->baseURI(), m_valueData.m_url);
}

bool ImageValue::operator==(const ImageValue& other) const
{
    if (m_valueType != other.m_valueType) {
        return false;
    }

    if (m_valueType == other.m_valueType &&
        m_valueType == ImageValueType::ValueType::None) {
        return true;
    }

    switch (m_valueType.m_type) {
    case ImageValueType::ValueType::URL:
        return m_valueData.m_url->equals(other.m_valueData.m_url);
        break;
    case ImageValueType::ValueType::Gradient:
        return m_valueData.m_gradient->equals(other.m_valueData.m_gradient);
        break;
    default:
        return false;
        break;
    }
    return false;
}
} // namespace
