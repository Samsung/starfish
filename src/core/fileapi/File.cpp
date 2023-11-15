/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "core/fileapi/File.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

File::File(ExecutionContext* executionContext,
           const GCVector<BufferSourceOrBlobOrDOMString>& fileBits,
           String* fileName, const FilePropertyBag& options)
    : Blob(executionContext)
    , m_name(fileName)
    , m_lastModified(options.hasLastModified() ? options.lastModified() : 0)
{
    initialize(fileBits, static_cast<const BlobPropertyBag&>(options));
}

File::File(ExecutionContext* executionContext, BlobData blobData)
    : Blob(executionContext, blobData)
    , m_name(String::emptyString)
    , m_lastModified(0)
{
}

ScriptBindingInstance* File::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
