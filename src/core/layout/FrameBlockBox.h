/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishFrameBlockBox__
#define __StarFishFrameBlockBox__

#include "core/layout/FrameBox.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameLineBreak.h"
#include "core/layout/FrameText.h"
#include "core/util/RefPtr.h"

namespace StarFish {

class FrameBlockBox;
class LineFormattingContext;

class InlineTextBox : public FrameBox {
public:
    InlineTextBox(InlineTextBox* box)
        : FrameBox(box->node(), box->style())
        , m_text(nullptr)
        , m_start(0)
        , m_end(std::numeric_limits<uint16_t>::max())
    {
        m_flags.m_isFirstLine = box->isFirstLine();
        m_flags.m_direction = box->charDirection();
    }

    InlineTextBox(FrameText* frame, const TextRun& run, bool isFirstLine)
        : FrameBox(frame->node(), frame->style())
    {
        setText(run.m_stringView.string(), run.m_stringView.start(),
                run.m_stringView.end());
        m_flags.m_isFirstLine = isFirstLine;
        m_flags.m_direction = run.m_direction;
    }

    virtual bool isInlineBox() const
    {
        return true;
    }

    virtual bool isInlineTextBox() const
    {
        return true;
    }

    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        FrameBox::dump(depth);
        StringView tv = text();
        auto s = tv.substring()->toUTF8NonGCString();
        printf(" [(%s), dir: %d, start: %d, end %d] ", s.data(),
               (int)charDirection(), (int)tv.start(), (int)tv.end());
    }
#endif
    virtual const char* name()
    {
        return "InlineTextBox";
    }

    void setText(String* t)
    {
        setText(t, 0, t->length());
    }

    CharDirection charDirection()
    {
        return m_flags.m_direction;
    }

    void setCharDirection(CharDirection dir)
    {
        m_flags.m_direction = dir;
    }

    FrameText* origin();

    TextRun textRun()
    {
        StringView tv = text();
        return TextRun(tv.string(), tv.start(), tv.end(), charDirection());
    }

    StringView text()
    {
        if (UNLIKELY(m_flags.m_gotLongString)) {
            STARFISH_ASSERT(m_text->isStringView());
            return *((StringView*)m_text);
        }
        return StringView(m_text, m_start, m_end);
    }

    void unmarkFirstLine()
    {
        m_flags.m_isFirstLine = false;
        setWidth(style()->font()->measureText(textRun().m_stringView));
        setHeight(style()->font()->metrics().m_fontHeight);
    }

    bool isFirstLine()
    {
        return m_flags.m_isFirstLine;
    }

    ComputedStyle* style()
    {
        Frame* parent = layoutParent();
        while (parent->isLineBox()) {
            parent = parent->layoutParent();
        }

        return Frame::style(parent, parent->style(), m_flags.m_isFirstLine);
    }

    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    void setText(String* str, size_t start, size_t end)
    {
        STARFISH_ASSERT(str);
        if (start < std::numeric_limits<uint16_t>::max() &&
            end < std::numeric_limits<uint16_t>::max()) {
            m_text = str;
            m_start = start;
            m_end = end;
            m_flags.m_gotLongString = false;
        } else {
            m_text = new StringView(str, start, end);
            m_flags.m_gotLongString = true;
        }
    }
    String* m_text;
    uint16_t m_start;
    uint16_t m_end;
};

class InlineBoxLayoutParentBox : public FrameBox {
public:
    InlineBoxLayoutParentBox()
        : InlineBoxLayoutParentBox(nullptr, nullptr)
    {
    }

    InlineBoxLayoutParentBox(Frame* frame)
        : InlineBoxLayoutParentBox(frame->node(), frame->style())
    {
    }

    virtual bool isInlineBoxLayoutParentBox() const
    {
        return true;
    }

    GCVector<FrameBox*>& boxes()
    {
        return m_boxes;
    }

    virtual void establishesStackingContextIfNeeds()
    {
        for (size_t i = 0; i < m_boxes.size(); i++) {
            m_boxes[i]->establishesStackingContextIfNeeds();
        }
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        InlineNonReplacedBox* ret = nullptr;
        for (size_t i = 0; i < m_boxes.size(); i++) {
            if ((ret = m_boxes[i]->firstInlineNonReplacedBox(f))) {
                return ret;
            }
        }

        return ret;
    }

    virtual void computeVisibleRect(Frame::ComputeVisibleRectContext& ctx);

    FrameBox* firstInlineBox();
    FrameBox* lastInlineBox();
    void removeDanglingSpace(LineFormattingContext* ctx);
    bool containOnlyEmptyInlineNonReplacedBoxes(LineFormattingContext* ctx);
    bool isAbsolutePositionedBoxLayoutParent(LineFormattingContext* ctx);

    void insertInlineBox(FrameBox* box)
    {
        m_boxes.push_back(box);
        box->setLayoutParent(this);
    }
    void layoutInlineBoxes(LineFormattingContext* ctx, LayoutUnit start);
    virtual void coordinateVerticalProperties(LineFormattingContext* ctx,
                                              LayoutUnit yOffset);
    void registerRelativePositionedBoxesAndMarkPaintFlag(LayoutContext& ctx);

    void moveToNewLineBox(LineFormattingContext* ctx, FrameBox* box,
                          LineBox* lineBox);

    void setLeftMBPs(LineFormattingContext* ctx);
    void setRightMBPs(LineFormattingContext* ctx);

    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);
        for (size_t i = 0; i < m_boxes.size(); i++) {
            m_boxes[i]->iterateChildFrameBox(fn);
        }
    }

    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage)
    {
        Frame* result = nullptr;
        for (size_t i = 0; i < m_boxes.size(); i++) {
            Frame* child = m_boxes[m_boxes.size() - 1 - i];
            LayoutUnit cx = x - child->asFrameBox()->x();
            LayoutUnit cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx, cy, stage);
            if (result) {
                return result;
            }
        }
        return result;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage);

protected:
    GCVector<FrameBox*> m_boxes;

    InlineBoxLayoutParentBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
    {
    }
};

struct InlineNonReplacedBoxRareData : public gc {
    InlineNonReplacedBoxRareData(FrameInline* origin)
        : m_tag(0x3)
        , m_origin(origin)
    {
    }
    void* operator new(size_t size)
    {
        // We can use atomic malloc
        // because FrameTree already has reference of m_origin
        return GC_MALLOC_ATOMIC(size);
    }
    void* operator new[](size_t size) = delete;

    size_t m_tag;
    FrameInline* m_origin;
    LayoutBoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;
};

enum InlineNonReplacedBoxMBPStatus {
    MBPStatusNone = 0,
    ProcessedStaringMBP = 1,
    ProcessedEndingMBP = 2,
    SetLeftMBP = 4,
    SetRightMBP = 8,
};

class InlineNonReplacedBoxMBPStatusHolder
    : public RefCounted<InlineNonReplacedBoxMBPStatusHolder> {
public:
    InlineNonReplacedBoxMBPStatusHolder()
    {
        m_status = 0;
    }
    int m_status;
};

class InlineNonReplacedBox : public InlineBoxLayoutParentBox {
    friend class FrameBlockBox;
    friend class LineFormattingContext;

public:
    InlineNonReplacedBox(LineFormattingContext* ctx,
                         InlineNonReplacedBox* inlineBox, bool isFirstLine);
    InlineNonReplacedBox(LineFormattingContext* ctx, FrameInline* frame,
                         bool isFirstLine);

    virtual bool isInlineBox() const
    {
        return true;
    }

    virtual bool isInlineNonReplacedBox() const
    {
        return true;
    }

    virtual const char* name()
    {
        return "InlineNonReplacedBox";
    }

    virtual void layoutInline(LineFormattingContext& lineFormattingContext);
    virtual void paintStackingContextContent(Canvas* canvas);
    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage);
    virtual void paintChildrenWith(PaintingContext& ctx);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);

    ComputedStyle* style()
    {
        return Frame::style(this, Frame::style(), m_flags.m_isFirstLine);
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void establishesStackingContextIfNeeds()
    {
        FrameBox::establishesStackingContextIfNeeds();

        InlineBoxLayoutParentBox::establishesStackingContextIfNeeds();
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        if (origin() == f) {
            return this;
        }

        return InlineBoxLayoutParentBox::firstInlineNonReplacedBox(f);
    }

    virtual void computeVisibleRect(Frame::ComputeVisibleRectContext& ctx)
    {
        if (!tryUniteVisibleRect(ctx)) {
            return;
        }

        InlineBoxLayoutParentBox::computeVisibleRect(ctx);
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    FrameInline* origin()
    {
        if (rareData()) {
            return rareData()->m_origin;
        }
        return m_origin;
    }

    bool isCollapsed() const
    {
        return m_flags.m_isCollapsed;
    }

    void markCollapsed()
    {
        m_flags.m_isCollapsed = true;
    }

    void processStartingMBP(LineFormattingContext* lineFormattingContext);
    void processEndingMBP(LineFormattingContext* lineFormattingContext);
    void setOrgLeftMBP(LineFormattingContext* lineFormattingContext);
    void setOrgRightMBP(LineFormattingContext* lineFormattingContext);

    void unsetLeftMBP()
    {
        setMarginLeft(0);
        setBorderLeft(0);
        setPaddingLeft(0);
    }

    void unsetRightMBP()
    {
        setMarginRight(0);
        setBorderRight(0);
        setPaddingRight(0);
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    union {
        FrameInline* m_origin;
        InlineNonReplacedBoxRareData* m_rareData;
    };

    InlineNonReplacedBoxRareData* rareData()
    {
        if (m_origin == nullptr || 0x3 != *((size_t*)m_origin)) {
            return nullptr;
        }
        return m_rareData;
    }

    InlineNonReplacedBox(Frame* frame, FrameInline* origin, bool isFirstLine)
        : InlineBoxLayoutParentBox(frame)
        , m_origin(origin)
    {
        m_flags.m_isCollapsed = false;
        m_flags.m_isFirstLine = isFirstLine;
        if (origin->isLeftMBPCleared()) {
            setLeftMBPCleared();
        }

        if (origin->isRightMBPCleared()) {
            setRightMBPCleared();
        }

        // recompute style flags
        // we should re compute flags here
        // because, when ctor of Frame is executed, vtable is not set correctly
        // so we could not consider that what kind of frame is this
        computeStyleFlags();
    }

    void setTopBottomOrgMBP()
    {
        if (rareData() || marginTop() || marginBottom() || borderTop() ||
            borderBottom() || paddingTop() || paddingBottom()) {
            if (!rareData()) {
                m_rareData = new InlineNonReplacedBoxRareData(m_origin);
            }
            m_rareData->m_orgMargin.setTop(marginTop());
            m_rareData->m_orgMargin.setBottom(marginBottom());
            m_rareData->m_orgBorder.setTop(borderTop());
            m_rareData->m_orgBorder.setBottom(borderBottom());
            m_rareData->m_orgPadding.setTop(paddingTop());
            m_rareData->m_orgPadding.setBottom(paddingBottom());
        }
    }
};

class LineBox : public InlineBoxLayoutParentBox {
    friend class LineFormattingContext;
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;

public:
    LineBox(Frame* parent)
        : InlineBoxLayoutParentBox()
    {
        setLayoutParent(parent);
    }

    virtual bool isLineBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "LineBox";
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
};

struct FrameBlockBoxRareData : public FrameBoxRareData {
    FrameBlockBoxRareData(Frame* layoutParent)
        : FrameBoxRareData(layoutParent)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    LayoutUnit m_scrollWidth;
    LayoutUnit m_scrollHeight;
};

class FrameBlockBox : public FrameBox {
    friend class LineFormattingContext;
    friend class InlineNonReplacedBox;
    friend class FrameDocument;

public:
    FrameBlockBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
    {
        STARFISH_ASSERT((node == nullptr && style != nullptr) ||
                        (node != nullptr && style == nullptr));
    }

    // Do not access this property before finish layout!
    LayoutUnit scrollWidth()
    {
        if (m_flags.m_hasBiggerContentThanFrameWidth) {
            return frameBlockBoxRareData()->m_scrollWidth;
        } else {
            return width();
        }
    }

    // Do not access this property before finish layout!
    LayoutUnit scrollHeight()
    {
        if (m_flags.m_hasBiggerContentThanFrameHeight) {
            return frameBlockBoxRareData()->m_scrollHeight;
        } else {
            return height();
        }
    }

    virtual LayoutUnit scrollLeft();
    virtual LayoutUnit scrollTop();

    virtual bool isFrameBlockBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameBlockBox";
    }

    FrameDocument* asFrameDocument()
    {
        return (FrameDocument*)this;
    }

    bool isNecessaryBlockBox()
    {
        if (isAnonymous()) {
            if (firstChild() && firstChild()->isFrameInline()) {
                FrameInline* fi = firstChild()->asFrameInline();
                if (fi->isLeftMBPCleared() && fi->isRightMBPCleared()) {
                    if (!fi->firstChild()) {
                        return false;
                    }
                    if (fi->firstChild() == fi->lastChild() &&
                        fi->firstChild()->isFrameText()) {
                        if (fi->firstChild()
                                ->asFrameText()
                                ->text()
                                ->containsOnlyWhitespace()) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    void markHeightComputed(bool b)
    {
        m_flags.m_heightComputed = b;
    }

    bool heightComputed() const
    {
        return m_flags.m_heightComputed;
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void computePreferredWidth(PreferredWidthContext& ctx);
    virtual void layoutInline(LineFormattingContext& ctx);
    void computeContentWidth(LayoutContext& ctx,
                             LayoutUnit containgBlockContentWidth);
    void computeContentHeight(LayoutContext& ctx, LayoutUnit contentHeight);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void paintContent(PaintingContext& ctx) override;
    virtual void paintChildrenWith(PaintingContext& ctx) override;
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage);

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f);

    virtual void establishesStackingContextIfNeeds();
    virtual void computeVisibleRect(FrameBox::ComputeVisibleRectContext& ctx);

    virtual bool hasBlockFlow()
    {
        if (!firstChild()) {
            return true;
        }
        Frame* child = firstChild();
        return (child->isBlockLevel() && child->isNormalFlow());
    }

    virtual bool isSelfCollapsingBlock(LayoutContext& ctx);
    GCVector<LineBox*>& lineBoxes()
    {
        return m_lineBoxes;
    }

    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);
        if (hasBlockFlow()) {
            Frame* box = firstChild();
            while (box) {
                box->asFrameBox()->iterateChildFrameBox(fn);
                box = box->next();
            }
        } else {
            for (size_t i = 0; i < m_lineBoxes.size(); i++) {
                m_lineBoxes[i]->iterateChildFrameBox(fn);
            }
        }
    }

    bool hasBiggerContentThanFrameWidth()
    {
        return m_flags.m_hasBiggerContentThanFrameWidth;
    }

    bool hasBiggerContentThanFrameHeight()
    {
        return m_flags.m_hasBiggerContentThanFrameHeight;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    void paintInlineContent(Canvas* canvas);
    LayoutUnit layoutBlock(LayoutContext& ctx);
    LayoutUnit layoutInline(LayoutContext& ctx);
    void computeContentHeight(LayoutContext& ctx, FrameBox* cb);

    virtual bool hasFrameTreeItemModel()
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel()
    {
        return &m_treeItemModel;
    }

    virtual FrameBlockBoxRareData* createRareData()
    {
        return new FrameBlockBoxRareData(m_layoutParent);
    }

    FrameBlockBoxRareData* frameBlockBoxRareData() const
    {
        STARFISH_ASSERT(hasRareData());
        return (FrameBlockBoxRareData*)m_layoutParent;
    }

    FrameTreeItemModel m_treeItemModel;
    GCVector<LineBox*> m_lineBoxes;
};

struct FloatingBoxLayoutContext {
    int m_hasFloat;
    LayoutUnit m_y;
    LayoutUnit m_accumulatedLeftFloatingBoxWidth;
    LayoutUnit m_accumulatedRightFloatingBoxWidth;
    LayoutUnit m_originalLineBoxX;
    LayoutUnit m_originalLineBoxWidth;

    FloatingBoxLayoutContext(int hasFloat, LayoutUnit y,
                             LayoutUnit originalLineBoxX,
                             LayoutUnit originalLineBoxWidth)
        : m_hasFloat(hasFloat)
        , m_y(y)
        , m_originalLineBoxX(originalLineBoxX)
        , m_originalLineBoxWidth(originalLineBoxWidth)
    {
    }
};

class Word {
public:
    void concat(FrameBox* box)
    {
        m_boxes.push_back(box);
    }

    void clear()
    {
        m_boxes.clear();
    }

    bool isEmpty() const
    {
        return m_boxes.size() == 0;
    }

    LayoutUnit width() const
    {
        // TODO: consider starting mbp of InlineNonReplacedBox
        return std::accumulate(std::next(m_boxes.begin()), m_boxes.end(),
                               (*m_boxes.begin())->outerWidth(),
                               [](LayoutUnit w, FrameBox* box) {
                                   if (box->isNormalFlow()) {
                                       return w + box->outerWidth();
                                   } else {
                                       return w;
                                   }
                               });
    }

    void unmarkFirstLine()
    {
        unmarkFirstLine(m_boxes);
    }

    GCVector<FrameBox*>& boxes()
    {
        return m_boxes;
    }

private:
    GCVector<FrameBox*> m_boxes;

    void unmarkFirstLine(GCVector<FrameBox*>& boxes)
    {
        std::for_each(boxes.begin(), boxes.end(), [this](FrameBox* box) {
            if (box->isNormalFlow()) {
                if (box->isInlineNonReplacedBox()) {
                    unmarkFirstLine(box->asInlineNonReplacedBox()->boxes());
                } else {
                    box->asInlineTextBox()->unmarkFirstLine();
                }
            }
        });
    }
};

struct LanguageDirection {
    LanguageDirection()
        : m_direction(None)
        , m_parentDirectionValue(DirectionValue::LtrDirectionValue)
    {
    }

    void markDirection(DirectionValue dir)
    {
        m_parentDirectionValue = dir;
    }

    void markDirection(CharDirection dir)
    {
        switch (dir) {
        case CharDirection::Ltr:
            markLtrDirection();
            break;
        case CharDirection::Rtl:
            markRtlDirection();
            break;
        case CharDirection::Mixed:
            markLtrDirection();
            markRtlDirection();
            break;
        default:
            break;
        }
    }

    void markLtrDirection()
    {
        m_direction |= LtrDirection;
    }

    void markRtlDirection()
    {
        m_direction |= RtlDirection;
    }

    bool isLtrOnly()
    {
        return m_direction == LtrDirection;
    }

    bool isRtlOnly()
    {
        return m_direction == RtlDirection;
    }

    bool isMixed()
    {
        return (m_parentDirectionValue == DirectionValue::RtlDirectionValue) ||
               ((m_direction & RtlDirection) != 0);
    }

private:
    enum Direction { None, LtrDirection, RtlDirection };

    size_t m_direction;
    DirectionValue m_parentDirectionValue;
};

class LineFormattingContext {
private:
    void resetLineBox();
    void registerInlineContent(FrameLineBreak* br);
    LayoutUnit inlineBlockAscender(FrameBlockBox* box)
    {
        STARFISH_ASSERT(m_inlineBlockAscender.find(box) !=
                        m_inlineBlockAscender.end());
        return m_inlineBlockAscender[box];
    }

    LayoutUnit offsetApplyingTextAlign();
    void computeHorizontalProperties();
    LayoutUnit distanceToNextLineBox(FrameLineBreak* br,
                                     bool hasMoreInlineBoxes);

    void insertPendingFloatingBoxes();
    void insertPendingInlineBoxes();
    void layoutLineBox(LayoutUnit yDiff, LayoutUnit height);

    void generateInlineTextBox(TextToken& token);
    void insertFloatingBoxAndReLayoutLineBoxIfNeeds(FrameBox* box);
    InlineTextBox* splitInlineTextBox(InlineTextBox* textBox);
    void insertInlineBox(FrameBox* box);
    void insertAbsolutePositionedBoxes();

    void registerAbsolutePositionedBox(FrameBox* box);

    template <typename Box>
    LayoutUnit layoutInlineBoxes(Box* parent, LayoutUnit start);
    template <typename Iter>
    void sortInlineBoxes(Iter& iter);

    void removeAllInlineBoxes();

    bool isAnyOfInlineBoxesCollidedWithFloatingBoxes();
    void reCacheFloatingBoxes(LayoutUnit xDiff);
    void makeFloatingBoxLayoutContext(LayoutUnit yDiff);

    void breakLineForLineBox(FrameLineBreak* br, bool isLastLine,
                             bool skipFinishLine);
    void breakLineForInlineNonReplacedBox(FrameLineBreak* br);

    CharDirection contentDir(FrameBox* box);
    void resolveBidi(DirectionValue parentDir, GCVector<FrameBox*>& boxes);
    void splitInlineBoxes(GCVector<FrameBox*>& boxes);

    bool canInsertToLineBox(Frame* f, LayoutUnit width);
    bool hasFloatingBoxAlreadyInLineBox(Frame* f);
    bool canInsertFloatingBox(FrameBox* f, bool allowPendingFloatingBox);

    void collectComputeDirectionsCandidate(Frame* parent,
                                           std::vector<Frame*>& frames);

public:
    LineFormattingContext(FrameBlockBox* block, LayoutContext& ctx);

    void updateCurrentLayoutParent(Frame* parent);
    void computeVerticalProperties(FrameBox* parentBox, bool dueToBr);

    void layoutInline(Frame* origin);

    void finishLineForLineBox(FrameLineBreak* br, bool isLastLine);
    void finishLineForInlineNonReplacedBox(FrameLineBreak* br, bool isLastNode);
    void breakLine(FrameLineBreak* br);
    void makeFloatingBoxLayoutContextDueToClearIfNeeds(FrameBox* box);

    void tryInsertInlineBox(FrameBox* box);
    void tryInsertFloatingBox(FrameBox* box);
    void insertWord(Frame* next);
    void markInlineBoxIndex(FrameBox* box);

    bool removeLastLineBoxIfNeeds();
    void registerRelativePositionedBoxesAndMarkPaintFlag();
    LayoutUnit contentHeightForBlock();

    LineBox* currentLine()
    {
        return m_block->m_lineBoxes.back();
    }

    bool isFirstLineBox() const
    {
        return m_block->m_lineBoxes.size() == 1 && !m_isPendingBreakLine;
    }

    bool isWordProcessing() const
    {
        return !m_word.isEmpty();
    }

    void handleAbsoluteBox(FrameBox* box, bool canInsert, bool canRegister);
    void handleTextToken(TextToken& token);
    void handleSoftHyphenate(bool hyphenateOnLine);

    bool isWhiteSpaceAtLast()
    {
        return m_isWhiteSpaceAtLast;
    }

    void setIsWhiteSpaceAtLast(bool isWhiteSpaceAtLast)
    {
        m_isWhiteSpaceAtLast = isWhiteSpaceAtLast;
    }

    LayoutUnit currentLineWidth()
    {
        return m_currentLineWidth;
    }

    void registerInlineBlockAscender(LayoutUnit ascender, FrameBlockBox* box)
    {
        m_inlineBlockAscender[box] = ascender;
    }

    size_t absolutePositionedBoxLayoutParentCnt(
        InlineBoxLayoutParentBox* box) const
    {
        auto iter = m_absolutePositionedLayoutParentCnt.find(box);
        if (iter != m_absolutePositionedLayoutParentCnt.end()) {
            return iter->second;
        }
        return 0;
    }

    void markAbsolutePositionedBoxLayoutParent(InlineBoxLayoutParentBox* box)
    {
        auto iter = m_absolutePositionedLayoutParentCnt.find(box);
        if (iter == m_absolutePositionedLayoutParentCnt.end()) {
            m_absolutePositionedLayoutParentCnt[box] = 1;
        } else {
            iter->second++;
        }
    }

    void unMarkAbsolutePositionedBoxLayoutParent(InlineBoxLayoutParentBox* box)
    {
        STARFISH_ASSERT(absolutePositionedBoxLayoutParentCnt(box) != 0);
        m_absolutePositionedLayoutParentCnt[box]--;
    }

    bool dontBreakLine(FrameBox* box, LayoutUnit width);

    void computeDirection(Frame* parent, DirectionValue direction);

    LayoutUnit ascender(InlineBoxLayoutParentBox* box)
    {
        return m_ascenderDescenderOfInlineBoxLayoutParentBox[box].first;
    }

    LayoutUnit descender(InlineBoxLayoutParentBox* box)
    {
        return m_ascenderDescenderOfInlineBoxLayoutParentBox[box].second;
    }

    void setAscDescender(InlineBoxLayoutParentBox* box, LayoutUnit ascender,
                         LayoutUnit descender)
    {
        m_ascenderDescenderOfInlineBoxLayoutParentBox[box] =
            std::make_pair(ascender, descender);
        box->setHeight(ascender - descender);
    }

    bool isProcessedStartingMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & ProcessedStaringMBP) != 0;
    }

    void markProcessedStartingMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isProcessedStartingMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= ProcessedStaringMBP;
    }

    bool isProcessedEndingMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & ProcessedEndingMBP) != 0;
    }

    void markProcessedEndingMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isProcessedEndingMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= ProcessedEndingMBP;
    }

    bool isSetLeftMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & SetLeftMBP) != 0;
    }

    void markSetLeftMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isSetLeftMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= SetLeftMBP;
    }

    bool isSetRightMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & SetRightMBP) != 0;
    }

    void markSetRightMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isSetRightMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= SetRightMBP;
    }

    void setInlineBoxIndex(FrameBox* f, size_t inlineBoxIdx)
    {
        m_inlineBoxIndexes[f] = inlineBoxIdx;
    }

    size_t inlineBoxIndex(FrameBox* f)
    {
        auto iter = m_inlineBoxIndexes.find(f);
        if (iter == m_inlineBoxIndexes.end())
            return SIZE_MAX;
        return iter->second;
    }

    LayoutUnit m_leftBoundary;
    LayoutUnit m_rightBoundary;
    LayoutLocation m_absPosition;
    LayoutUnit m_lineBoxX;
    LayoutUnit m_lineBoxY;
    LayoutUnit m_currentLineWidth;
    LayoutUnit m_textIndentWidth;
    LayoutUnit m_lineBoxWidth;
    LayoutUnit m_unprocessedStartingMBPWidth;
    FrameBlockBox* m_block;
    LayoutContext& m_layoutContext;
    InlineBoxLayoutParentBox* m_currentLayoutParent;
    bool m_isPendingBreakLine;
    bool m_isWhiteSpaceAtLast;
    bool m_isSoftHyphenAtLast;
    bool m_canConcatWord;
    size_t m_inlineBoxIndex;
    size_t m_pendingFloatingBoxNumsBeforeCurrentLine;
    size_t m_floatingBoxesSizeBeforeCurrentLine;

    // we don't need gc_allocater here
    // frame tree has strong reference already
    std::unordered_map<FrameBlockBox*, LayoutUnit> m_inlineBlockAscender;

    std::vector<FloatingBoxLayoutContext> m_floatingBoxLayoutContexts;
    std::vector<FrameBox*> m_absolutePositionedBoxes;
    std::vector<FrameBox*> m_pendingFloatingBoxes;
    GCVector<FrameBox*> m_pendingInlineBoxes;
    Word m_word;

    std::unordered_map<Frame*, DirectionValue> m_computedDirectionValuePerFrame;
    std::unordered_map<FrameText*, std::vector<TextRun>> m_textRunsPerFrameText;
    std::unordered_map<FrameBox*, size_t> m_inlineBoxIndexes;

    std::unordered_map<InlineBoxLayoutParentBox*, size_t>
        m_absolutePositionedLayoutParentCnt;

    std::unordered_map<InlineNonReplacedBox*,
                       RefPtr<InlineNonReplacedBoxMBPStatusHolder>>
        m_inlineNonReplacedBoxMBPStatus;

    std::unordered_map<InlineBoxLayoutParentBox*,
                       std::pair<LayoutUnit, LayoutUnit>>
        m_ascenderDescenderOfInlineBoxLayoutParentBox;

    LanguageDirection m_currentLanguageDirection;
    std::unordered_map<Frame*, LanguageDirection> m_languageDirections;
};
}

#endif
