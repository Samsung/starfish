/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishComputeOverflow__
#define __StarfishComputeOverflow__

#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/StackingContext.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"

namespace Starfish {

struct OverflowStatus {
    Frame* m_child;
    FrameBox* m_absChild;
    bool m_seenContainingBlockForAbsBlock;
    bool m_seenAbsBlock; // FIXME: dup with m_absChild
    bool m_seenFixedBlock;
    OverflowStatus(Frame* child)
    {
        reset(child);
    }

    bool canApplyOverflow(Frame* parent, bool considerIFrame = true)
    {
        if (!parent) {
            return false;
        }

        if (!parent->style()) {
            STARFISH_ASSERT(parent->isLineBox());
            return false;
        }

        if (considerIFrame && parent->isFrameReplaced() &&
            parent->asFrameReplaced()->isFrameReplacedIFrame()) {
            return true;
        }

        if (!m_seenAbsBlock && parent->isAbsolutePositioned()) {
            m_seenAbsBlock = true;
            m_absChild = parent->asFrameBox();
            return parent->shouldApplyOverflow();
        }

        if (m_seenAbsBlock) {
            if (m_seenFixedBlock) {
                return false;
            }
            if (parent->style()->position() ==
                PositionValue::FixedPositionValue) {
                m_seenFixedBlock = true;
                if (m_child && m_child->style() &&
                    m_child->style()->position() ==
                        PositionValue::FixedPositionValue) {
                    return false;
                }
                return parent->shouldApplyOverflow();
            } else {
                bool b = parent->canBeContainingBlockOfAbsolutePositionedBox(
                    m_absChild);
                if (!m_seenContainingBlockForAbsBlock && b) {
                    if (parent->style()->position() == RelativePositionValue) {
                        m_seenAbsBlock = false;
                        return parent->shouldApplyOverflow();
                    }
                }
                m_seenContainingBlockForAbsBlock =
                    b || m_seenContainingBlockForAbsBlock;
                return b && parent->shouldApplyOverflow();
            }
        }

        return parent->shouldApplyOverflow();
    }

    static bool isScrollableFrame(Frame* f)
    {
        if (!f || !f->style()) {
            STARFISH_LOG_WARN("Wrong frame is used for checking scrollable");
            return false;
        }

        if (f->style()->position() == FixedPositionValue) {
            return false;
        }

        if (!f->isInlineLevel()) {
            OverflowStatus status(f);
            status.canApplyOverflow(f->parent());
            if (status.m_seenAbsBlock &&
                !status.m_seenContainingBlockForAbsBlock) {
                return false;
            }
        }

        return true;
    }

    void reset(Frame* f)
    {
        m_child = f;
        if (m_child->isAbsolutePositioned()) {
            m_absChild = f->asFrameBox();
            m_seenAbsBlock = true;
        } else {
            m_absChild = nullptr;
            m_seenAbsBlock = false;
        }
        m_seenContainingBlockForAbsBlock = false;
        m_seenFixedBlock = false;
    }
};

class JustCheckOveflow {
public:
    void restore()
    {
    }
    void endOpacityLayer()
    {
    }
};

template <typename T, const bool forDrawScrollBar = false>
class ComputeOverflow {
private:
    std::vector<std::pair<Frame*, std::pair<bool, bool>>>
        m_canApplyOverflowOrScrolls;

    void insertIntoCanApplyOverflowOrScrolls(Frame* f, std::pair<bool, bool> v)
    {
        auto len = m_canApplyOverflowOrScrolls.size();
        for (size_t i = 0; i < len; i++) {
            if (m_canApplyOverflowOrScrolls[i].first == f) {
                m_canApplyOverflowOrScrolls[i].second = v;
                return;
            }
        }
        m_canApplyOverflowOrScrolls.push_back(std::make_pair(f, v));
    }

    std::pair<bool, bool> readFromCanApplyOverflowOrScrolls(Frame* f)
    {
        auto len = m_canApplyOverflowOrScrolls.size();
        for (size_t i = 0; i < len; i++) {
            if (m_canApplyOverflowOrScrolls[i].first == f) {
                return m_canApplyOverflowOrScrolls[i].second;
            }
        }
        return std::make_pair(false, false);
    }

    bool needToRestore(StackingContext* stackingContext)
    {
        Frame* parentFrame = stackingContext->owner()->layoutParent();

        while (parentFrame) {
            if ((parentFrame->shouldApplyOverflow() &&
                 (!parentFrame->needToEstablishStackingContext() ||
                  (parentFrame->isFrameBox() &&
                   !parentFrame->asFrameBox()->canOwnsStackingContext()))) ||
                (parentFrame->style() &&
                 parentFrame->style()->position() == FixedPositionValue)) {
                return true;
            }
            parentFrame = parentFrame->layoutParent();
        }

        return false;
    }

    bool canBeNearestBufferedFrame(Frame* frame,
                                   StackingContext* childStackingContext)
    {
        return frame && frame->asFrameBox()->stackingContext() &&
               frame->asFrameBox()->stackingContext()->needsGraphicsBuffer() &&
               frame->asFrameBox()->stackingContext()->isAncestorOf(
                   childStackingContext);
    }

    void insertOverflowOrScroll(Frame* frame, OverflowStatus& status,
                                bool& canScroll)
    {
        bool applyOverflow = status.canApplyOverflow(frame);
        if (forDrawScrollBar && !applyOverflow) {
            applyOverflow =
                frame && frame->style() && frame->style()->hasBorderRadius();
        }
        if (applyOverflow) {
            status.reset(frame);
            canScroll =
                status.m_child->style()->position() != FixedPositionValue;
            insertIntoCanApplyOverflowOrScrolls(
                frame, std::make_pair(true, canScroll && frame &&
                                                frame->isFrameBlockBox()));
        } else {
            if (status.m_seenAbsBlock &&
                !status.m_seenContainingBlockForAbsBlock) {
                canScroll = false;
            }
            insertIntoCanApplyOverflowOrScrolls(
                frame, std::make_pair(false, canScroll && frame &&
                                                 frame->isFrameBlockBox()));
        }
    }

    bool isFixedPosition(Frame* frame)
    {
        return frame && frame->style() &&
               frame->style()->position() == FixedPositionValue;
    }

    void translateIFrame(StackingContext* stackingContext)
    {
        FrameBox* iframeBox = stackingContext->owner()
                                  ->node()
                                  ->document()
                                  ->browsingContext()
                                  ->sourceElement()
                                  ->frame()
                                  ->asFrameBox();
        m_canvasOrCompositor->translate(
            iframeBox->borderLeft() + iframeBox->paddingLeft(),
            iframeBox->borderTop() + iframeBox->paddingTop());
    }

    void applyStyleClip(ComputedStyle* style)
    {
        RectData* rect = style->clip();
        if (rect) {
            m_canvasOrCompositor->clip(Unit::Rect(
                rect->left().numberData(), rect->top().numberData(),
                rect->right().numberData(), rect->bottom().numberData()));
        }
    }

    void clipIfNeedsGraphicsBuffer(
        StackingContext* stackingContext,
        const StackingContext::PaintingStackingContextContext& ctx)
    {
        StackingContext* parentStackingContext = stackingContext->parent();
        while (parentStackingContext) {
            if (parentStackingContext->needsGraphicsBuffer()) {
                LayoutRect visibleRect = parentStackingContext->visibleRect();
                LayoutUnit minX = visibleRect.x();
                LayoutUnit minY = visibleRect.y();

                if (ctx.willCompositing) {
                    m_canvasOrCompositor->pixelSnappedClip(ctx.layerClipRect);
                    m_canvasOrCompositor->translate(
                        -ctx.layerBaseX - ctx.layerScrollX,
                        -ctx.layerBaseY - ctx.layerScrollY);
                }
                m_canvasOrCompositor->translate(-minX, -minY);
                break;
            }
            parentStackingContext = parentStackingContext->parent();
        }
    }

    void postMatrixIfNeeds(StackingContext* stackingContext)
    {
        if (stackingContext) {
            SkMatrix m = stackingContext->transformMatrix();
            if (!m.isIdentity()) {
                auto o = stackingContext->transformOrigin();
                m_canvasOrCompositor->translate(o.x(), o.y());
                m_canvasOrCompositor->postMatrix(m);
                m_canvasOrCompositor->translate(-o.x(), -o.y());
            }
        }
    }

    void clipFrameBoxRect(FrameBox* frameBox)
    {
        Unit::Rect rect(frameBox->borderLeft(), frameBox->borderTop(),
                        frameBox->width() - frameBox->borderWidth(),
                        frameBox->height() - frameBox->borderHeight());
        m_canvasOrCompositor->clip(rect);
    }

    void clipBorderRadiusIfNeeds(FrameBox* frameBox)
    {
        if (frameBox->hasFrameBorderRadius()) {
            const LayoutRect rect(0, 0, frameBox->width(), frameBox->height());
            frameBox->applyBorderRadiusClippingIfNeeds(m_canvasOrCompositor,
                                                       rect);
        }
    }

    void translatePosition(const LayoutUnit& x, const LayoutUnit& y)
    {
        m_canvasOrCompositor->translate(x, y);
    }

    void saveState()
    {
        m_canvasOrCompositor->save();
    }

public:
    template <typename U = T, typename = typename std::enable_if<std::is_same<
                                  JustCheckOveflow, U>::value>::type>
    ComputeOverflow(FrameBox* frame)
        : m_canvasOrCompositor(nullptr)
        , m_opacity(1)
    {
        m_canApplyOverflowOrScrolls.reserve(32);

        OverflowStatus status(frame);
        bool canScroll = OverflowStatus::isScrollableFrame(frame);

        Frame* f = frame;
        while (f) {
            f = f->layoutParent();

            insertOverflowOrScroll(f, status, canScroll);

            if (canScroll && isFixedPosition(f)) {
                canScroll = false;
            }
        }
    }

    bool canApplyOverflow(Frame* f)
    {
        auto len = m_canApplyOverflowOrScrolls.size();
        for (size_t i = 0; i < len; i++) {
            if (m_canApplyOverflowOrScrolls[i].first == f) {
                return m_canApplyOverflowOrScrolls[i].second.first;
            }
        }
        return false;
    }

    template <typename U = T, typename = typename std::enable_if<
                                  std::is_same<Canvas, U>::value>::type>
    ComputeOverflow(
        U* canvas, StackingContext* childStackingContext,
        FrameBox* parentFrameBox,
        const StackingContext::PaintingStackingContextContext& paintingContext)
        : m_canvasOrCompositor(canvas)
        , m_opacity(1)
    {
        saveState();
        if (!parentFrameBox) {
            return;
        }

        FrameBox* childFrameBox = childStackingContext->owner();
        if (!isFixedPosition(childFrameBox)) {
            if (!needToRestore(childStackingContext)) {
                auto o = childFrameBox->absolutePointIncludingScroll(
                    parentFrameBox, false);
                translatePosition(o.x(), o.y());
                return;
            }
        }

        VectorWithInlineStorage<32, FrameBox*, std::allocator<FrameBox*>>
            frameList;
        m_canApplyOverflowOrScrolls.reserve(32);
        Frame* nearestBufferedFrame = nullptr;
        bool needToShareBuffer = true;

        {
            Frame* frame = childFrameBox;
            OverflowStatus status(frame);
            bool canScroll = OverflowStatus::isScrollableFrame(frame);

            while (frame) {
                frameList.push_back(frame->asFrameBox());
                frame = frame->layoutParent();

                if (needToShareBuffer &&
                    canBeNearestBufferedFrame(frame, childStackingContext)) {
                    nearestBufferedFrame = frame;
                    needToShareBuffer = false;
                }

                if (needToShareBuffer) {
                    insertOverflowOrScroll(frame, status, canScroll);
                }

                if (canScroll && isFixedPosition(frame)) {
                    canScroll = false;
                }
            }
        }

        canvas->resetMatrixAndClip();
        canvas->resetTextDecorationData();

        if (!paintingContext.willCompositing) {
            canvas->pixelSnappedClip(paintingContext.screenClipRect);
        }
        clipIfNeedsGraphicsBuffer(childStackingContext, paintingContext);

        auto iter = frameList.rbegin();
        needToShareBuffer = nearestBufferedFrame ? false : true;
        while (iter != frameList.rend()) {
            FrameBox* frameBox = *iter;

            ComputedStyle* style = frameBox->style();
            if (style) {
                if (frameBox != childFrameBox) {
                    if (frameBox->shouldResetTextDecoration()) {
                        canvas->resetTextDecorationData();
                    } else {
                        canvas->mergeTextDecorationData(frameBox->style());
                    }
                }
            }

            if (nearestBufferedFrame && nearestBufferedFrame == frameBox) {
                needToShareBuffer = true;
            }

            if (!needToShareBuffer) {
                iter++;
                continue;
            }

            if (!(nearestBufferedFrame && nearestBufferedFrame == frameBox)) {
                translatePosition(frameBox->x(), frameBox->y());
            }

            if (style) {
                auto overflowOrScroll =
                    readFromCanApplyOverflowOrScrolls(frameBox);

                StackingContext* stackingContext = frameBox->stackingContext();
                if (frameBox != childFrameBox) {
                    if (frameBox != nearestBufferedFrame) {
                        postMatrixIfNeeds(stackingContext);
                    }

                    if (overflowOrScroll.first && childFrameBox != frameBox) {
                        clipFrameBoxRect(frameBox);
                        clipBorderRadiusIfNeeds(frameBox);
                    }

                    if (style->isAbsolutePositioned()) {
                        applyStyleClip(style);
                    }

                    if (overflowOrScroll.second) {
                        translatePosition(
                            -frameBox->asFrameBlockBox()->scrollLeft(),
                            -frameBox->asFrameBlockBox()->scrollTop());
                    }
                }

                if (stackingContext &&
                    stackingContext->isIFrameStackingContext() &&
                    nearestBufferedFrame != frameBox) {
                    translateIFrame(stackingContext);
                }
            }
            iter++;
        }
    }

    template <typename U = T, typename = typename std::enable_if<
                                  std::is_same<Compositor, U>::value>::type>
    ComputeOverflow(U* compositor, StackingContext* childStackingContext,
                    FrameBox* parentFrameBox)
        : m_canvasOrCompositor(compositor)
        , m_opacity(1)
    {
        saveState();
        if (!parentFrameBox) {
            return;
        }

        FrameBox* childFrameBox = childStackingContext->owner();
        VectorWithInlineStorage<32, FrameBox*, std::allocator<FrameBox*>>
            frameList;
        m_canApplyOverflowOrScrolls.reserve(32);

        {
            Frame* frame = childFrameBox;
            OverflowStatus status(frame);
            bool canScroll = OverflowStatus::isScrollableFrame(frame);

            while (frame) {
                frameList.push_back(frame->asFrameBox());
                frame = frame->layoutParent();

                insertOverflowOrScroll(frame, status, canScroll);

                if (canScroll && isFixedPosition(frame)) {
                    canScroll = false;
                }
            }
        }

        compositor->resetMatrixAndClip();
        float opacity = 1;

        auto iter = frameList.rbegin();
        while (iter != frameList.rend()) {
            FrameBox* frameBox = *iter;
            translatePosition(frameBox->x(), frameBox->y());

            ComputedStyle* style = frameBox->style();
            if (style) {
                auto overflowOrScroll =
                    readFromCanApplyOverflowOrScrolls(frameBox);

                StackingContext* stackingContext = frameBox->stackingContext();
                postMatrixIfNeeds(stackingContext);

                if (stackingContext) {
                    float n = style->opacity();
                    if (n != 1) {
                        opacity = opacity * n;
                    }

                    SkMatrix test;
                    if (!compositor->currentTransformMatrix().invert(&test)) {
                        compositor->postMatrix(SkMatrix::InvalidMatrix());
                        return;
                    }

                    if (style->mixBlendMode() != BlendMode::Normal) {
                        compositor->setBlendMode(style->mixBlendMode());
                    }
                }

                if (overflowOrScroll.first && childFrameBox != frameBox) {
                    clipFrameBoxRect(frameBox);
                    clipBorderRadiusIfNeeds(frameBox);
                }

                if (childStackingContext->owner() == frameBox &&
                    childStackingContext->inScrollActive() &&
                    !childStackingContext->needsRepaintingWhenScrolling()) {
                    clipBorderRadiusIfNeeds(frameBox);
                }

                if (style->isAbsolutePositioned()) {
                    applyStyleClip(style);
                }

                if (overflowOrScroll.second &&
                    childStackingContext->owner() != frameBox) {
                    translatePosition(
                        -frameBox->asFrameBlockBox()->scrollLeft(),
                        -frameBox->asFrameBlockBox()->scrollTop());
                }

                if (stackingContext &&
                    stackingContext->isIFrameStackingContext()) {
                    translateIFrame(stackingContext);
                }
            }
            iter++;
        }

        m_opacity = opacity;
        if (m_opacity != 1) {
            compositor->beginOpacityLayer(m_opacity,
                                          childStackingContext->visibleRect());
        }
    }

    ~ComputeOverflow()
    {
        if (m_opacity != 1) {
            m_canvasOrCompositor->endOpacityLayer();
        }
        m_canvasOrCompositor->restore();
    }

    T* canvasOrCompositor()
    {
        return m_canvasOrCompositor;
    }

private:
    T* m_canvasOrCompositor;
    float m_opacity;
};

} // namespace Starfish

#endif
