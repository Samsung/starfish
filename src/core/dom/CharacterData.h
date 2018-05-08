/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCharacterData__
#define __StarFishCharacterData__

#include "core/dom/Node.h"

namespace StarFish {

class Text;

class CharacterData : public Node {
public:
    CharacterData(Document* document, String* data)
        : Node(document)
    {
        STARFISH_ASSERT(data);
        m_data = data;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCharacterData() const override;

    /* 4.9. Interface CharacterData */

    String* data() const
    {
        return m_data;
    }

    void parserSetData(String* data)
    {
        STARFISH_ASSERT(data);
        m_data = data;
    }

    void setData(String* data);

    uint32_t length() const
    {
        return m_data->length();
    }

    bool isContainerNode() override
    {
        return false;
    }

    String* substringData(unsigned long offset, unsigned long count);
    void appendData(String* data);
    void insertData(unsigned long offset, String* data);
    void deleteData(unsigned long offset, unsigned long count);
    void replaceData(unsigned long offset, unsigned long count, String* data);

    /* Other methods (not in DOM API) */

    static std::string replaceAll(const std::string& str,
                                  const std::string& pattern,
                                  const std::string& replace)
    {
        std::string result = str;
        std::string::size_type pos = 0;
        std::string::size_type offset = 0;

        while ((pos = result.find(pattern, offset)) != std::string::npos) {
            result.replace(result.begin() + pos,
                           result.begin() + pos + pattern.size(), replace);
            offset = pos + replace.size();
        }

        return result;
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump() override
    {
        Node::dump();

        auto utf8String = m_data->toUTF8NonGCString();
        utf8String = replaceAll(utf8String, "\n", "\\n");
        printf("data:%s ", utf8String.data());
    }
#endif

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        Node::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(CharacterData, m_data));
    }

private:
    String* m_data;
};
}

#endif
