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
#ifndef __StarfishIPCMessageSerializer__
#define __StarfishIPCMessageSerializer__

namespace Starfish {

class MemorySerializeWriter;
class MemorySerializeReader;

enum class IPCMessageTag : char {
    kUndefine = 0,
    kBoolean,
    kUInt32,
    kSizeNumber,
    kString,
};

class IPCMessageSerializer : public gc {
public:
    IPCMessageSerializer(const std::string& messageID);

    void writeBool(const bool value);
    void writeUInt32(const uint32_t value);
    void writeSize(const size_t value);
    void writeString(const std::string& value);
    void writeTerminator();

    const char* data() const;

    size_t size() const;

    bool isError() const;

private:
    MemorySerializeWriter* m_writer;
};

class IPCMessageDeserializer : public gc {
public:
    IPCMessageDeserializer(const char* data, const size_t length);

    std::string messageID() const
    {
        return m_messageID;
    }

    bool checkTag(const IPCMessageTag tag);

    bool readBool();
    uint32_t readUInt32();
    size_t readSize();
    std::string readString();

    bool isError() const;

private:
    MemorySerializeReader* m_reader;
    std::string m_messageID;
};

} // namespace Starfish

#endif
#endif
