/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "ResourceErrorDelegate.h"

namespace LWEDelegate {

ResourceError::ResourceError(int code, const std::string& description,
                             const std::string& url)
    : m_errorCode(code)
    , m_description(description)
    , m_url(url)
{
}

int ResourceError::GetErrorCode()
{
    return m_errorCode;
}

std::string ResourceError::GetDescription()
{
    return m_description;
}

std::string ResourceError::GetUrl()
{
    return m_url;
}

} // namespace LWEDelegate
