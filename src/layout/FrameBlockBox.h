#ifndef __StarFishBlockBox__
#define __StarFishBlockBox__

#include "layout/FrameBox.h"
#include "layout/FrameReplaced.h"
#include "layout/FrameInline.h"
#include "layout/FrameText.h"

namespace StarFish {

class FrameBlockBox;
class LineFormattingContext;
class InlineTextBox; // TextNode
class InlineNonReplacedBox; // non-replaced element, display: inline

class InlineBox : public FrameBox {
public:
    InlineBox(Node* node, ComputedStyle* style, Frame* parent)
        : FrameBox(node, style)
    {
        setParent(parent);
    }

    Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (stage == HitTestStage::HitTestNormalFlowInline) {
            return FrameBox::hitTest(x, y, stage);
        }
        return nullptr;
    }

    virtual bool isInlineBox() { return true; }
    virtual bool isInlineTextBox() const { return false; }
    virtual bool isInlineNonReplacedBox() const { return false; }

    InlineTextBox* asInlineTextBox()
    {
        STARFISH_ASSERT(isInlineTextBox());
        return (InlineTextBox*)this;
    }

    InlineNonReplacedBox* asInlineNonReplacedBox()
    {
        STARFISH_ASSERT(isInlineNonReplacedBox());
        return (InlineNonReplacedBox*)this;
    }
};

class InlineTextBox : public InlineBox {
public:
    enum CharDirection {
        Ltr,
        Rtl,
        Mixed,
        Netural,
    };

    InlineTextBox(Node* node, ComputedStyle* style, Frame* parent, String* str, FrameText* origin, CharDirection charDirection)
        : InlineBox(node, style, parent)
    {
        m_charDirection = charDirection;
        m_text = str;
        m_origin = origin;
    }

    virtual bool isInlineTextBox() const { return true; }

    virtual void paint(Canvas* canvas, PaintingStage stage);
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        InlineBox::dump(depth);
        printf(" [(%s), dir: %d] ", m_text->utf8Data(), (int)m_charDirection);
    }
#endif
    virtual const char* name()
    {
        return "InlineTextBox";
    }

    String* text()
    {
        return m_text;
    }

    void setText(String* t)
    {
        m_text = t;
    }

    CharDirection charDirection()
    {
        return m_charDirection;
    }

    void setCharDirection(CharDirection dir)
    {
        m_charDirection = dir;
    }

    FrameText* origin()
    {
        return m_origin;
    }

protected:
    CharDirection m_charDirection;
    String* m_text;
    FrameText* m_origin;
};

class InlineNonReplacedBox : public InlineBox {
    friend FrameBlockBox;
    friend void splitInlineBoxesAndMarkDirectionForResolveBidi(LineFormattingContext& ctx, DirectionValue parentDir, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes);
    friend void reassignLeftRightMBPOfInlineNonReplacedBox(LineFormattingContext& ctx, std::vector<FrameBox*, gc_allocator<FrameBox*>>& boxes);
public:
    InlineNonReplacedBox(Node* node, ComputedStyle* style, Frame* parent, FrameInline* origin)
        : InlineBox(node, style, parent)
    {
        if (origin->isLeftMBPCleared())
            setLeftMBPCleared();
        if (origin->isRightMBPCleared())
            setRightMBPCleared();
        m_origin = origin;
        m_descender = m_ascender = 0;

        // recompute style flags
        // we should re compute flgas here
        // because, when ctor of Frame is Executed, vtable is not setted correctly
        // so we could not consider that what kind of frame is this
        computeStyleFlags();
    }

    virtual bool isInlineNonReplacedBox() const { return true; }
    virtual const char* name()
    {
        return "InlineNonReplacedBox";
    }
    static InlineNonReplacedBox* layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx, FrameBlockBox* blockBox,
        LineFormattingContext* lineFormattingContext, InlineNonReplacedBox* layoutParentBox, bool freshStart);
    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual void paintChildrenWith(Canvas* canvas, PaintingStage stage)
    {
        auto iter = boxes().begin();
        while (iter != boxes().end()) {
            FrameBox* child = *iter;
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, stage);
            canvas->restore();
            iter++;
        }
    }
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void iterateChildBoxes(void (*fn)(FrameBox*))
    {
        fn(this);
        for (size_t i = 0; i < m_boxes.size(); i ++) {
            m_boxes[i]->iterateChildBoxes(fn);
        }
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    LayoutUnit ascender()
    {
        return m_ascender;
    }

    LayoutUnit decender()
    {
        return m_descender;
    }

    void setAscender(const LayoutUnit& a)
    {
        m_ascender = a;
    }

    void setDecender(const LayoutUnit& a)
    {
        m_descender = a;
    }

    FrameInline* origin()
    {
        return m_origin;
    }

    std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes()
    {
        return m_boxes;
    }

protected:
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
    FrameInline* m_origin;
    LayoutBoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;
    std::vector<FrameBox*, gc_allocator<FrameBox*> > m_boxes;
};

class LineBox : public FrameBox {
    friend class LineFormattingContext;
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;

public:
    LineBox(Frame* parent)
        : FrameBox(nullptr, nullptr)
    {
        setParent(parent);
        m_descender = m_ascender = 0;
    }

    virtual bool isLineBox()
    {
        return true;
    }

    LayoutUnit ascender()
    {
        return m_ascender;
    }

    LayoutUnit decender()
    {
        return m_descender;
    }

    std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes()
    {
        return m_boxes;
    }

    virtual void iterateChildBoxes(void (*fn)(FrameBox*))
    {
        fn(this);

        for (size_t i = 0; i < m_boxes.size(); i ++) {
            m_boxes[i]->iterateChildBoxes(fn);
        }
    }

protected:
    std::vector<FrameBox*, gc_allocator<FrameBox*> > m_boxes;
    // FIXME
    // we use these value only for vertical-align of inline-block
    // in layout, we use only 'ascender'
    // should we delete m_decender?
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
};

class MarginInfo {
public:
    MarginInfo(LayoutUnit topBorderPadding, LayoutUnit bottomBorderPadding, bool isNewContext, Length height)
    {
        m_canCollapseWithChildren = !isNewContext;
        m_canCollapseTopWithChildren = m_canCollapseWithChildren && !topBorderPadding;
        m_canCollapseBottomWithChildren = m_canCollapseWithChildren && !bottomBorderPadding && height.isAuto();
        m_atTopSideOfBlock = true;
    }
    void setPositiveMargin(LayoutUnit m)
    {
        m_positiveMargin = m;
    }
    LayoutUnit positiveMargin()
    {
        return m_positiveMargin;
    }
    void setNegativeMargin(LayoutUnit m)
    {
        m_negativeMargin = m;
    }
    LayoutUnit negativeMargin()
    {
        return m_negativeMargin;
    }
    void setMargin(LayoutUnit pos, LayoutUnit neg)
    {
        STARFISH_ASSERT(pos >= 0 && neg >= 0);
        m_positiveMargin = pos;
        m_negativeMargin = neg;
    }
    void setMargin(LayoutUnit val)
    {
        if (val >= 0) {
            setMargin(val, 0);
        } else {
            setMargin(0, -val);
        }
    }
    bool canCollapseTopWithChildren()
    {
        return m_canCollapseTopWithChildren;
    }
    void setAtTopSideOfBlock(bool b) { m_atTopSideOfBlock = b; }
    bool atTopSideOfBlock() { return m_atTopSideOfBlock; }
    bool canCollapseWithMarginTop()
    {
        return m_atTopSideOfBlock && m_canCollapseTopWithChildren;
    }
    bool canCollapseWithMarginBottom()
    {
        return m_canCollapseBottomWithChildren;
    }
    bool canCollapseBottomWithChildren()
    {
        return m_canCollapseBottomWithChildren;
    }
    void setCanCollapseBottomWithChildren(bool v)
    {
        m_canCollapseBottomWithChildren = v;
    }
    bool m_canCollapseWithChildren;
    bool m_canCollapseTopWithChildren;
    bool m_canCollapseBottomWithChildren;
    bool m_atTopSideOfBlock;
    LayoutUnit m_positiveMargin;
    LayoutUnit m_negativeMargin;
};

class FrameBlockBox : public FrameBox {
    friend class LineFormattingContext;
    friend class InlineNonReplacedBox;
    friend class FrameDocument;
public:
    FrameBlockBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
        , m_visibleRect(0, 0, 0, 0)
    {
        STARFISH_ASSERT((node == nullptr && style != nullptr) || (node != nullptr && style == nullptr));
    }

    virtual bool isFrameBlockBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameBlockBox";
    }

    bool isNecessaryBlockBox()
    {
        if (!node()) {
            if (firstChild() && firstChild()->isFrameInline()) {
                FrameInline* fi = firstChild()->asFrameInline();
                if (fi->isLeftMBPCleared() && fi->isRightMBPCleared()) {
                    if (!fi->firstChild())
                        return false;
                    if (fi->firstChild()->isFrameText()) {
                        if (fi->firstChild()->asFrameText()->text()->containsOnlyWhitespace())
                            return false;
                    }
                }
            }
        }
        return true;
    }

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual void paintChildrenWith(Canvas* canvas, PaintingStage stage);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y, HitTestStage stage);

    virtual void iterateChildBoxes(void (*fn)(FrameBox*))
    {
        if (hasBlockFlow()) {
            FrameBox::iterateChildBoxes(fn);
            return;
        }

        fn(this);

        for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
            m_lineBoxes[i]->iterateChildBoxes(fn);
        }
    }

    bool hasBlockFlow()
    {
        if (!firstChild())
            return true;

        Frame* child = firstChild();
        return child->style()->originalDisplay() == BlockDisplayValue;
    }

    bool isSelfCollapsingBlock(LayoutContext& ctx)
    {
        if (isEstablishesBlockFormattingContext())
            return false;
        if (asFrameBox()->height() > 0)
            return false;
        if (!isNecessaryBlockBox())
            return true;
        Length heightLength = style()->height();
        // NOTE: In case of percentage height,
        // if containing blocks' height is fixed, the block is not self-collaping block.
        if (heightLength.isPercent() && !heightLength.isZero() && ctx.parentHasFixedHeight(this)) {
            return false;
        }

        if (heightLength.isAuto() || heightLength.isZero()) {
            // If the block is inline flow and has any line boxes,
            // this is not self-collapsing.
            if (!hasBlockFlow()) {
                for (unsigned i = 0; i < m_lineBoxes.size(); i++) {
                    for (unsigned j = 0; j < m_lineBoxes[i]->boxes().size(); j++) {
                        if (!m_lineBoxes[i]->boxes()[j]->isNormalFlow())
                            continue;
                        if (!m_lineBoxes[i]->boxes()[j]->isSelfCollapsingBlock(ctx))
                            return false;
                    }
                }
                return true;
            }
            Frame* child = firstChild();
            while (child) {
                if (!child->isNormalFlow()) {
                    child = child->next();
                    continue;
                }
                if (!child->asFrameBox()->isSelfCollapsingBlock(ctx))
                    return false;
                child = child->next();
            }
            return true;
        }
        return false;
    }

    virtual LayoutRect visibleRect()
    {
        return m_visibleRect;
    }

protected:
    std::pair<LayoutUnit, LayoutRect> layoutBlock(LayoutContext& ctx);
    std::pair<LayoutUnit, LayoutRect> layoutInline(LayoutContext& ctx);
    std::vector<LineBox*, gc_allocator<LineBox*> > m_lineBoxes;

    LayoutRect m_visibleRect;
};

struct DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine {
    DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine()
    {
        m_isFirstEdgeProcessed = false;
    }
    LayoutBoxSurroundData m_margin;
    LayoutBoxSurroundData m_border;
    LayoutBoxSurroundData m_padding;
    LayoutBoxSurroundData m_orgMargin;
    LayoutBoxSurroundData m_orgBorder;
    LayoutBoxSurroundData m_orgPadding;
    bool m_isFirstEdgeProcessed;
};

class LineFormattingContext {
public:
    LineFormattingContext(FrameBlockBox& block, LayoutContext& ctx, const LayoutUnit& lineBoxWidth)
        : m_block(block)
        , m_layoutContext(ctx)
    {
        m_lineBoxWidth = lineBoxWidth;
        m_block.m_lineBoxes.clear();
        // m_block.m_lineBoxes.shrink_to_fit();
        LineBox* lineBox = new LineBox(&m_block);
        lineBox->setWidth(m_lineBoxWidth);
        m_block.m_lineBoxes.push_back(lineBox);
        m_currentLine = 0;
        m_currentLineWidth = 0;
    }

    void breakLine(bool dueToBr = false);
    void completeLastLine();

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

    void registerInlineBlockAscender(LayoutUnit ascender, FrameBlockBox* box)
    {
        m_inlineBlockAscender[box] = ascender;
    }

    LayoutUnit inlineBlockAscender(FrameBlockBox* box)
    {
        STARFISH_ASSERT(m_inlineBlockAscender.find(box) != m_inlineBlockAscender.end());
        return m_inlineBlockAscender[box];
    }

    LayoutUnit m_currentLineWidth;
    LayoutUnit m_lineBoxWidth;
    size_t m_currentLine;
    FrameBlockBox& m_block;
    LayoutContext& m_layoutContext;

    std::set<size_t> m_breakedLinesSet;
    // we dont need gc_allocater here
    // frame tree has strong reference already
    std::unordered_map<FrameBlockBox*, LayoutUnit> m_inlineBlockAscender;
    std::unordered_map<FrameInline*, DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine> m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine;
    std::unordered_map<FrameInline*, InlineNonReplacedBox*> m_checkLastInlineNonReplacedPerLine;
};
}

#endif
