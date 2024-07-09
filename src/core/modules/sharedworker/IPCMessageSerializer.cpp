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

#if defined(STARFISH_ENABLE_SHARED_WORKER)
#include "StarfishConfig.h"
#include "core/serialize/MemorySerializer.h"
#include "core/util/debug/Trace.h"
#include "core/modules/sharedworker/IPCMessageSerializer.h"

namespace Starfish {

IPCMessageSerializer::IPCMessageSerializer(const std::string& messageID)
    : m_writer(new MemorySerializeWriter())
{
    writeString(messageID);
}

void IPCMessageSerializer::writeBool(const bool value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kBoolean);
    m_writer->write<bool>(value);
}

void IPCMessageSerializer::writeUInt32(const uint32_t value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kUInt32);
    m_writer->write<uint32_t>(value);
}

void IPCMessageSerializer::writeSize(const size_t value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kSizeNumber);
    m_writer->write<size_t>(value);
}

void IPCMessageSerializer::writeString(const std::string& value)
{
    m_writer->write<IPCMessageTag>(IPCMessageTag::kString);
    m_writer->write<size_t>(value.length());
    m_writer->write<char>(value.data(), value.length());
}

void IPCMessageSerializer::writeTerminator()
{
    m_writer->writeTerminator();
}

const char* IPCMessageSerializer::data() const
{
    return m_writer->buffer()->data();
}

size_t IPCMessageSerializer::size() const
{
    return m_writer->buffer()->size();
}

bool IPCMessageSerializer::isError() const
{
    return m_writer->isError();
}

IPCMessageDeserializer::IPCMessageDeserializer(const char* data,
                                               const size_t length)
    : m_reader(new MemorySerializeReader(data, length))
{
    m_messageID = readString();
    TRACE(IPC, "received message:", m_messageID.c_str());
}

bool IPCMessageDeserializer::checkTag(IPCMessageTag tag)
{
    if (m_reader->checkValue(static_cast<char>(tag))) {
        char tag;
        m_reader->read<char>(tag);
        return true;
    }

    return false;
}

bool IPCMessageDeserializer::readBool()
{
    bool value = false;
    if (checkTag(IPCMessageTag::kBoolean)) {
        m_reader->read<bool>(value);
    }

    return value;
}

uint32_t IPCMessageDeserializer::readUInt32()
{
    uint32_t value = 0;
    if (checkTag(IPCMessageTag::kUInt32)) {
        m_reader->read<uint32_t>(value);
    }

    return value;
}

size_t IPCMessageDeserializer::readSize()
{
    size_t value = 0;
    if (checkTag(IPCMessageTag::kSizeNumber)) {
        m_reader->read<size_t>(value);
    }

    return value;
}

std::string IPCMessageDeserializer::readString()
{
    std::string value;
    if (checkTag(IPCMessageTag::kString)) {
        size_t length = 0;
        m_reader->read<size_t>(length);

        char* data = nullptr;
        m_reader->readRawBytes(length, data);

        value = std::string(data, length);
    }

    return value;
}

bool IPCMessageDeserializer::isError() const
{
    return m_reader->isError();
}

} // namespace Starfish

#endif
