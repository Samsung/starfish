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

class ResourceErrorImpl : public ResourceError {
public:
    ResourceErrorImpl(int code, const std::string& description,
                      const std::string& url);
    ResourceErrorImpl(const ResourceError& other);

    int GetErrorCode() override;
    std::string GetDescription() override;
    std::string GetUrl() override;

private:
    int m_errorCode;
    std::string m_description;
    std::string m_url;
};

ResourceErrorImpl::ResourceErrorImpl(int code, const std::string& description,
                                     const std::string& url)
    : m_errorCode(code)
    , m_description(description)
    , m_url(url)
{
}

int ResourceErrorImpl::GetErrorCode()
{
    return m_errorCode;
}

std::string ResourceErrorImpl::GetDescription()
{
    return m_description;
}

std::string ResourceErrorImpl::GetUrl()
{
    return m_url;
}

ResourceError* ResourceError::Create(int code, const std::string& description,
                                     const std::string& url)
{
    return new ResourceErrorImpl(code, description, url);
}

} // namespace LWEDelegate

extern "C" {
uintptr_t EXPORT_UNMANAGED_API LWEDelegate_ResourceError_Create(
    int code, const char* description, const char* url)
{
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::ResourceError::Create(code, description, url));
}
}
