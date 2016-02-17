#ifndef __StarFishBlockBox__
#define __StarFishBlockBox__

#include "layout/FrameBox.h"
#include "layout/FrameReplaced.h"

namespace StarFish {

class FrameBlockBox;
class LineFormattingContext;
class InlineTextBox; // TextNode
class InlineBlockBox; // display-inline:block
class InlineReplacedBox; // replaced element, display: inline
class InlineNonReplacedBox; // non-replaced element, display: inline

class InlineBox : public FrameBox {
public:
    InlineBox(Node* node, ComputedStyle* style, Frame* parent)
        : FrameBox(node, style)
    {
        setParent(parent);
    }


    Frame* hitTest(float x, float y, HitTestStage stage)
    {
        if (stage == HitTestStage::HitTestNormalFlowInline) {
            return FrameBox::hitTest(x, y, stage);
        }
        return nullptr;
    }

    virtual bool isInlineBox() { return true; }
    virtual bool isInlineTextBox() const { return false; }
    virtual bool isInlineReplacedBox() const { return false; }
    virtual bool isInlineBlockBox() const { return false; }
    virtual bool isInlineNonReplacedBox() const { return false; }

    InlineTextBox* asInlineTextBox()
    {
        STARFISH_ASSERT(isInlineTextBox());
        return (InlineTextBox*)this;
    }

    InlineBlockBox* asInlineBlockBox()
    {
        STARFISH_ASSERT(isInlineBlockBox());
        return (InlineBlockBox*)this;
    }

    InlineReplacedBox* asInlineReplacedBox()
    {
        STARFISH_ASSERT(isInlineReplacedBox());
        return (InlineReplacedBox*)this;
    }

    InlineNonReplacedBox* asInlineNonReplacedBox()
    {
        STARFISH_ASSERT(isInlineNonReplacedBox());
        return (InlineNonReplacedBox*)this;
    }
};

class InlineTextBox : public InlineBox {
public:
    InlineTextBox(Node* node, ComputedStyle* style, Frame* parent, String* str)
        : InlineBox(node, style, parent)
    {
        m_text = str;
    }

    virtual bool isInlineTextBox() const { return true; }

    virtual void paint(Canvas* canvas, PaintingStage stage)
    {
        if (stage == PaintingNormalFlowInline) {
            canvas->setFont(style()->font());
            canvas->setColor(style()->color());
            canvas->drawText(0, 0, m_text);
        }
    }

    virtual void dump(int depth)
    {
        InlineBox::dump(depth);
        printf(" [%s] ", m_text->utf8Data());
    }

    virtual const char* name()
    {
        return "InlineTextBox";
    }

    String* text()
    {
        return m_text;
    }

protected:
    String* m_text;
};

class InlineReplacedBox : public InlineBox {
public:
    InlineReplacedBox(Node* node, ComputedStyle* style, Frame* parent, FrameReplaced* f)
        : InlineBox(node, style, parent)
    {
        m_frameReplaced = f;
    }

    virtual bool isInlineReplacedBox() const { return true; }

    virtual void paint(Canvas* canvas, PaintingStage stage)
    {
        m_frameReplaced->paint(canvas, stage);
    }

    virtual const char* name()
    {
        return "InlineReplacedBox";
    }

    FrameReplaced* replacedBox()
    {
        return m_frameReplaced;
    }

protected:
    FrameReplaced* m_frameReplaced;
};

class InlineBlockBox : public InlineBox {
    friend FrameBlockBox;
public:
    InlineBlockBox(Node* node, ComputedStyle* style, Frame* parent, FrameBlockBox* f, float ascender)
        : InlineBox(node, style, parent)
    {
        m_frameBlockBox = f;
        ((FrameBox*)m_frameBlockBox)->setLayoutParent(this);
        m_ascender = ascender;
    }

    virtual bool isInlineBlockBox() const { return true; }

    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual Frame* hitTest(float x, float y, HitTestStage stage);
    virtual const char* name()
    {
        return "InlineBlockBox";
    }

    float ascender()
    {
        return m_ascender;
    }

protected:
    float m_ascender;
    FrameBlockBox* m_frameBlockBox;
};


class InlineNonReplacedBox : public InlineBox {
    friend FrameBlockBox;
public:
    InlineNonReplacedBox(Node* node, ComputedStyle* style, Frame* parent, FrameInline* origin)
        : InlineBox(node, style, parent)
    {
        m_origin = origin;
        m_descender= m_ascender = 0;

    }

    virtual bool isInlineNonReplacedBox() const { return true; }
    virtual const char* name()
    {
        return "InlineNonReplacedBox";
    }
    static InlineNonReplacedBox* layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx, FrameBlockBox* blockBox,
            LineFormattingContext* lineFormattingContext, FrameBox* layoutParentBox, bool freshStart);
    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual Frame* hitTest(float x, float y, HitTestStage stage);
    virtual void dump(int depth);

    float ascender()
    {
        return m_ascender;
    }

    float decender()
    {
        return m_descender;
    }
protected:
    float m_ascender;
    float m_descender;
    FrameInline* m_origin;
    BoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;
};


class LineBox : public FrameBox {
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;
public:
    LineBox(Frame* parent)
        : FrameBox(nullptr, nullptr)
    {
        setParent(parent);
        m_descender= m_ascender = 0;
    }

    virtual bool isLineBox()
    {
        return true;
    }

    float ascender()
    {
        return m_ascender;
    }

    float decender()
    {
        return m_descender;
    }

protected:
    // FIXME
    // we use these value only for vertical-align of inline-block
    // in layout, we use only 'ascender'
    // should we delete m_decender?
    float m_ascender;
    float m_descender;
};

class FrameBlockBox : public FrameBox {
    friend class LineFormattingContext;
    friend class InlineNonReplacedBox;
public:
    FrameBlockBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
    {

    }

    virtual bool isFrameBlockBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameBlockBox";
    }

    virtual void layout(LayoutContext& ctx);
    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx);

    virtual void dump(int depth);
    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual Frame* hitTest(float x, float y,HitTestStage stage);

    bool hasBlockFlow()
    {
        if (!firstChild())
            return true;

        Frame* child = firstChild();

        while (child && !child->isNormalFlow()) {
            child = child->next();
        }

        if (!child)
            return true;

        return child->style()->display() == BlockDisplayValue;
    }
protected:
    float layoutBlock(LayoutContext& ctx);
    float layoutInline(LayoutContext& ctx);

    std::vector<LineBox*, gc_allocator<LineBox*>> m_lineBoxes;

    static void paintChildrenWith(FrameBlockBox* block, Canvas* canvas, PaintingStage stage);
};

class LineFormattingContext {
public:
    LineFormattingContext(FrameBlockBox& block, LayoutContext& ctx)
        : m_block(block)
        , m_layoutContext(ctx)
    {
        m_block.m_lineBoxes.clear();
        // m_block.m_lineBoxes.shrink_to_fit();
        m_block.m_lineBoxes.push_back(new LineBox(&m_block));
        m_currentLine = 0;
        m_currentLineWidth = 0;
    }


    void breakLine(bool dueToBr = false)
    {
        if (dueToBr == false)
            m_breakedLinesSet.insert(m_block.m_lineBoxes.size() - 1);
        m_block.m_lineBoxes.push_back(new LineBox(&m_block));
        m_currentLine++;
        m_currentLineWidth = 0;
    }

    bool isBreakedLineWithoutBR(size_t idx)
    {
        return m_breakedLinesSet.find(idx) != m_breakedLinesSet.end();
    }

    void registerInlineContent()
    {
        m_layoutContext.setLastLineBox(m_block.m_lineBoxes.back());
    }

    LineBox* currentLine()
    {
        return m_block.m_lineBoxes.back();
    }

    std::set<size_t> m_breakedLinesSet;
    float m_currentLineWidth;
    size_t m_currentLine;
    FrameBlockBox& m_block;
    LayoutContext& m_layoutContext;
};

}

#endif
