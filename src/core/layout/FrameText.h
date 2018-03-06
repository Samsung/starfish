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

#ifndef __StarFishFrameText__
#define __StarFishFrameText__

#include "core/layout/Frame.h"
#include "core/dom/CharacterData.h"

namespace StarFish {

class FrameTextRareData : public gc {
public:
    bool m_isFrameTextRareData;
    Node* m_node;
    String* m_text;

    FrameTextRareData(Node* n)
        : m_isFrameTextRareData(true)
        , m_node(n)
        , m_text(String::emptyString)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
};

class FrameText : public Frame {
public:
    FrameText(Node* node, ComputedStyle* style);

    virtual const char* name()
    {
        return "FrameText";
    }

    bool hasRareData() const
    {
        size_t* ptr = (size_t*)m_node;
        if (ptr && *ptr == true) {
            return true;
        }
        return false;
    }

    FrameTextRareData* frameTextRareData() const
    {
        STARFISH_ASSERT(hasRareData());
        return ((FrameTextRareData*)m_node);
    }

    String* text()
    {
        if (hasRareData()) {
            return frameTextRareData()->m_text;
        }
        return node()->asCharacterData()->data();
    }
    void setText(String* text);

    void transformText(String* text);
    String* makeCapitalized(String* text, char32_t prev);
    char32_t previousChar();
    bool isFrameInlineOrEmptyText(Frame* f);

    virtual bool isSelfCollapsingBlock(LayoutContext& ctx);
#ifndef NDEBUG
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
#endif
    virtual void computePreferredWidth(PreferredWidthContext& ctx);
    virtual void layoutInline(LineFormattingContext& ctx);

#ifdef STARFISH_ENABLE_TEST
#ifndef NDEBUG
    virtual void dump(int depth)
    {
        UTF8StringDataNonGCStd str = text()->toUTF8NonGCString();
        str = replaceAll(str, "\n", "\\n");
        printf("text-> %s", str.data());
    }
#endif
#endif

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word obj_bitmap[GC_BITMAP_SIZE(FrameText)] = { 0 };
            GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameText, m_node));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameText, m_treeItemModel.m_firstChild));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameText, m_treeItemModel.m_lastChild));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameText, m_treeItemModel.m_parent));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameText, m_treeItemModel.m_previous));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameText, m_treeItemModel.m_next));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameText, m_treeItemModel.m_firstChild));
            GC_set_bit(obj_bitmap,
                       GC_WORD_OFFSET(FrameText, m_treeItemModel.m_lastChild));

            descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameText));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    virtual bool hasFrameTreeItemModel()
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel()
    {
        return &m_treeItemModel;
    }

    FrameTreeItemModel m_treeItemModel;
};
}

#endif
