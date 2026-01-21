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

#include "StarfishConfig.h"

#include "LayoutRepaintTracker.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/layout/StackingContext.h"
#include "core/layout/FrameDocument.h"
#include "core/page/WebView.h"

namespace Starfish {

LayoutRepaintTracker::ComputeOverflow::ComputeOverflow(
    LayoutRepaintTracker& tracker, Frame* frame, FrameBox* stackingContextOwner)
    : tracker(tracker)
    , frame(frame)
{
    if (frame->shouldApplyOverflow()) {
        LayoutRect repaintRect = frame->asFrameBox()->overflowRepaintRect();
        repaintRect.setX(repaintRect.x() + frame->asFrameBox()->x());
        repaintRect.setY(repaintRect.y() + frame->asFrameBox()->y());
        LayoutLocation pos = frame->asFrameBox()->absolutePointIncludingScroll(
            stackingContextOwner);
        LayoutRect overflowRect(LayoutLocation(pos.x() + repaintRect.x(),
                                               pos.y() + repaintRect.y()),
                                repaintRect.size());
        tracker.m_boundMaxExtentDueToOverflow.push_back(
            std::make_tuple(overflowRect, stackingContextOwner));
    }
}

LayoutRepaintTracker::ComputeOverflow::~ComputeOverflow()
{
    if (frame->shouldApplyOverflow()) {
        tracker.m_boundMaxExtentDueToOverflow.pop_back();
    }
}

void LayoutRepaintTracker::ComputeOverflow::reduceRect(
    LayoutRepaintTracker& tracker, LayoutRect& rt,
    FrameBox* stackingContextOwner)
{
    for (size_t i = 0; i < tracker.m_boundMaxExtentDueToOverflow.size(); i++) {
        if (std::get<1>(tracker.m_boundMaxExtentDueToOverflow[i]) ==
            stackingContextOwner) {
            rt = LayoutRect::overlappedRect(
                rt, std::get<0>(tracker.m_boundMaxExtentDueToOverflow[i]));
        }
    }
}

static void collectInlineBoxes(
    InlineBoxLayoutParentBox* parent,
    LayoutRepaintTracker::InlineLayoutResult* oldResult,
    LayoutRepaintTracker::InlineLayoutResult* result,
    FrameBox* stackingContextOwner,
    Optional<FrameBox*> lastGraphicsBufferContext)
{
    auto& b = parent->boxes();
    auto* parentStackingContextBox = lastGraphicsBufferContext
                                         ? lastGraphicsBufferContext.value()
                                         : stackingContextOwner;
    bool includeTopMostScroll = lastGraphicsBufferContext ? false : true;
    for (size_t i = 0; i < b.size(); i++) {
        Frame* f = b[i];
        bool inserted = false;
        if (f->isInlineTextBox()) {
            LayoutRepaintTracker::InlineLayoutResultItem r;
            r.m_frameRect = f->asFrameBox()->absoluteRectIncludingScroll(
                parentStackingContextBox, includeTopMostScroll);

            float diff =
                FONT_WIDTH_DIFFERENCE_GLYPH_ADVANCE_AND_ACTUAL_WIDTH_OF_GLYPH(
                    f->style()->font()->size());
            r.m_frameRect.setWidth(r.m_frameRect.width() + diff * 2);
            r.m_frameRect.setX(r.m_frameRect.x() - diff);

            auto ydiff = f->style()->font()->maxVerticalGlyphSize();
            r.m_frameRect.setY(r.m_frameRect.y() - ydiff.first);
            r.m_frameRect.setHeight(r.m_frameRect.height() + ydiff.first +
                                    ydiff.second);

            // we can store just text start & end
            // because text chaning triggier frametree-rebuild
            // new frames are have needsPainting flag already
            r.m_textStartEndValue = f->asInlineTextBox()->textSignatureValue();
            inserted = true;
            result->push_back(r);
        } else if (f->isInlineNonReplacedBox()) {
            LayoutRepaintTracker::InlineLayoutResultItem r;
            r.m_frameRect = f->asFrameBox()->absoluteRectIncludingScroll(
                parentStackingContextBox, includeTopMostScroll);
            r.m_textStartEndValue = 0;
            inserted = true;
            result->push_back(r);
        }

        if (inserted && oldResult) {
            size_t idx = result->size() - 1;
            if (idx < oldResult->size()) {
                if (oldResult->at(idx) == result->at(idx)) {
                    f->clearNeedsPainting();
                }
            }
        }

        if (f->isInlineBoxLayoutParentBox()) {
            collectInlineBoxes(f->asInlineBoxLayoutParentBox(), oldResult,
                               result, stackingContextOwner,
                               lastGraphicsBufferContext);
        }
    }
}

static LayoutRect computeLayoutRect(
    FrameBox* currentFrameBox, FrameBox* lastStackingContextOwner,
    Optional<FrameBox*> lastGraphicsBufferContext)
{
    LayoutRect newLayoutResultRect;

    if (lastGraphicsBufferContext) {
        newLayoutResultRect = currentFrameBox->absoluteRectIncludingScroll(
            lastGraphicsBufferContext.value(), false);
    } else {
        newLayoutResultRect = currentFrameBox->absoluteRectIncludingScroll(
            lastStackingContextOwner, true);
    }
    return newLayoutResultRect;
}

static void traceRepaintRegionJob(
    LayoutRepaintTracker& tracker, Frame* currentFrame,
    FrameBox* lastStackingContextOwner,
    Optional<FrameBox*> lastGraphicsBufferContext,
    std::unordered_map<Node*, std::pair<LayoutRect, Node*>>& oldResultMap,
    std::unordered_map<Node*, std::pair<LayoutRect, Node*>>& newLayoutResultMap,
    GCVector<std::tuple<FrameBlockBox*, FrameBox*,
                        LayoutRepaintTracker::InlineLayoutResult*>>&
        oldInlineResultMap,
    GCVector<std::tuple<FrameBlockBox*, FrameBox*,
                        LayoutRepaintTracker::InlineLayoutResult*>>&
        newInlineResultMap,
    std::unordered_map<Node*, LayoutRect>& dirtyAreaMapPerStackingContext,
    GCUnorderedSet<Node*, std::hash<Node*>, std::equal_to<Node*>,
                   GCUtil::gc_malloc_allocator<Node*>>& rootedNodeSet,
    bool& gotPaintingDirty, bool inCompositeMode)
{
    // if box is invisible from here, ignore from currentBox
    if (currentFrame->isFrameBox() &&
        currentFrame->asFrameBox()->isBoxesInvisibleFromHere()) {
        return;
    }

    // collect results related with box
    if (!currentFrame->isAlwaysInvisible() && !currentFrame->isAnonymous() &&
        currentFrame->isFrameBox()) {
        FrameBox* currentFrameBox = currentFrame->asFrameBox();
        Node* node = currentFrame->node();

        rootedNodeSet.insert(node);
        rootedNodeSet.insert(lastStackingContextOwner->node());

        // if frame box establish StackingContext, this box cared by
        // RepaintTracker
        bool needToEstablishStackingContext =
            currentFrame->needToEstablishStackingContext();
        if (needToEstablishStackingContext) {
            // compute new result
            if (lastGraphicsBufferContext) {
                if (currentFrameBox->stackingContext() &&
                    currentFrameBox->stackingContext()->needsGraphicsBuffer()) {
                    lastGraphicsBufferContext = currentFrameBox;
                }
            }
            LayoutRect newLayoutResultRect =
                computeLayoutRect(currentFrameBox, lastStackingContextOwner,
                                  lastGraphicsBufferContext);

            // check last result
            auto iter = oldResultMap.find(node);
            bool gotNewNode = iter == oldResultMap.end();
            bool frameRectChanged =
                !gotNewNode && (iter->second.first != newLayoutResultRect);

            if (gotNewNode || frameRectChanged) {
                // got new node || frameRectChanged -> dirty
                gotPaintingDirty = true;
                if (frameRectChanged) {
                    LayoutRect dirtyRect = currentFrameBox->frameRect();
                    dirtyRect.unite(iter->second.first);
                    dirtyRect.setX(0);
                    dirtyRect.setY(0);

                    auto iter2 = dirtyAreaMapPerStackingContext.find(node);
                    if (iter2 == dirtyAreaMapPerStackingContext.end()) {
                        dirtyAreaMapPerStackingContext.insert(
                            std::make_pair(node, dirtyRect));
                    } else {
                        iter2->second.unite(dirtyRect);
                    }

                    iter->second.first.setX(LayoutUnit::min());
                    iter->second.first.setY(LayoutUnit::min());
                }

                if (currentFrameBox->isVisible()) {
                    if (!lastStackingContextOwner->stackingContext() ||
                        !lastStackingContextOwner->stackingContext()
                             ->needsGraphicsBufferReason() ||
                        !currentFrame->asFrameBox()->stackingContext() ||
                        !currentFrame->asFrameBox()
                             ->stackingContext()
                             ->needsGraphicsBufferReason()) {
                        node->setNeedsPainting();
                    }
                }
            } else {
                // finded & result are same
                iter->second.first.setX(LayoutUnit::min());
                iter->second.first.setY(LayoutUnit::min());
            }

            newLayoutResultMap.insert(std::make_pair(
                node, std::make_pair(newLayoutResultRect,
                                     lastStackingContextOwner->node())));

            lastStackingContextOwner = currentFrameBox;
        } else {
            // compute new result
            LayoutRect newLayoutResultRect =
                currentFrameBox->absoluteRectIncludingScroll(
                    lastStackingContextOwner);

            // check last result
            LayoutRepaintTracker::ComputeOverflow::reduceRect(
                tracker, newLayoutResultRect, lastStackingContextOwner);

            auto iter = oldResultMap.find(node);
            bool gotNewNode = iter == oldResultMap.end();
            bool frameRectChanged =
                !gotNewNode && (iter->second.first != newLayoutResultRect);
            if (gotNewNode || frameRectChanged) {
                // got new node || frameRectChanged -> dirty
                gotPaintingDirty = true;
                LayoutRect rt = newLayoutResultRect;
                if (iter != oldResultMap.end()) {
                    rt.unite(iter->second.first);
                }

                if (frameRectChanged) {
                    iter->second.first.setX(LayoutUnit::min());
                    iter->second.first.setY(LayoutUnit::min());
                }

                if (currentFrameBox->isVisible()) {
                    Node* stackingContextOwner =
                        lastStackingContextOwner->node();
                    auto iter2 = dirtyAreaMapPerStackingContext.find(
                        stackingContextOwner);
                    if (iter2 == dirtyAreaMapPerStackingContext.end()) {
                        dirtyAreaMapPerStackingContext.insert(
                            std::make_pair(stackingContextOwner, rt));
                    } else {
                        iter2->second.unite(rt);
                    }
                }
            } else {
                // finded & result are same
                iter->second.first.setX(LayoutUnit::min());
                iter->second.first.setY(LayoutUnit::min());
            }

            newLayoutResultMap.insert(std::make_pair(
                node, std::make_pair(newLayoutResultRect,
                                     lastStackingContextOwner->node())));
        }
    }

    // collect inline layout result(not on frame tree)
    if (currentFrame->isFrameBlockBox() &&
        !currentFrame->asFrameBlockBox()->hasBlockFlow()) {
        // make current result set
        LayoutRepaintTracker::InlineLayoutResult* inlineResult =
            new LayoutRepaintTracker::InlineLayoutResult();
        LayoutRepaintTracker::InlineLayoutResult* oldInlineResult = nullptr;
        size_t oldInlineResultIndexInMap = SIZE_MAX;
        FrameBlockBox* inlineFlowHolder = currentFrame->asFrameBlockBox();
        for (size_t i = 0; i < oldInlineResultMap.size(); i++) {
            if (std::get<0>(oldInlineResultMap[i]) == inlineFlowHolder) {
                oldInlineResult = std::get<2>(oldInlineResultMap[i]);
                oldInlineResultIndexInMap = i;
                break;
            }
        }
        auto& lineBoxes = currentFrame->asFrameBlockBox()->lineBoxes();
        for (size_t i = 0; i < lineBoxes.size(); i++) {
            collectInlineBoxes(lineBoxes[i], oldInlineResult, inlineResult,
                               lastStackingContextOwner,
                               lastGraphicsBufferContext);
        }

        if (oldInlineResult) {
            auto newInlineResult = inlineResult;

            size_t iterEnd =
                std::max(oldInlineResult->size(), newInlineResult->size());
            LayoutRect dirtyRect;
            for (size_t j = 0; j < iterEnd; j++) {
                if (oldInlineResult->size() <= j) {
                    dirtyRect.unite(newInlineResult->at(j).m_frameRect);
                } else if (newInlineResult->size() <= j) {
                    dirtyRect.unite(oldInlineResult->at(j).m_frameRect);
                } else {
                    if (oldInlineResult->at(j) != newInlineResult->at(j)) {
                        dirtyRect.unite(oldInlineResult->at(j).m_frameRect);
                        dirtyRect.unite(newInlineResult->at(j).m_frameRect);
                    }
                }
            }

            LayoutRepaintTracker::ComputeOverflow::reduceRect(
                tracker, dirtyRect, lastStackingContextOwner);

            if (dirtyRect.size().width()) {
                gotPaintingDirty = true;
                Node* stackingContextOwner = lastStackingContextOwner->node();
                auto iter2 =
                    dirtyAreaMapPerStackingContext.find(stackingContextOwner);
                if (iter2 == dirtyAreaMapPerStackingContext.end()) {
                    dirtyAreaMapPerStackingContext.insert(
                        std::make_pair(stackingContextOwner, dirtyRect));
                } else {
                    iter2->second.unite(dirtyRect);
                }
            }

            oldInlineResultMap[oldInlineResultIndexInMap] = std::make_tuple(
                std::get<0>(oldInlineResultMap[oldInlineResultIndexInMap]),
                std::get<0>(oldInlineResultMap[oldInlineResultIndexInMap]),
                nullptr);
        }

        newInlineResultMap.push_back(
            std::make_tuple(currentFrame->asFrameBlockBox(),
                            lastStackingContextOwner, std::move(inlineResult)));
    }

    LayoutRepaintTracker::ComputeOverflow o(tracker, currentFrame,
                                            lastStackingContextOwner);

    Frame* f = currentFrame->firstChild();
    while (f) {
        traceRepaintRegionJob(
            tracker, f, lastStackingContextOwner, lastGraphicsBufferContext,
            oldResultMap, newLayoutResultMap, oldInlineResultMap,
            newInlineResultMap, dirtyAreaMapPerStackingContext, rootedNodeSet,
            gotPaintingDirty, inCompositeMode);
        f = f->next();
    }
}

bool LayoutRepaintTracker::traceRepaintRegion(FrameDocument* fd)
{
    std::unordered_map<Node*, std::pair<LayoutRect, Node*>> newResult;
    GCVector<std::tuple<FrameBlockBox*, FrameBox*, InlineLayoutResult*>>
        newInlineLayoutResult;
    LayoutRect dirtyArea;
    bool gotPaintingDirty = false;
    bool needsComposite = fd->node()->webView()->needsComposite();
    Optional<FrameBox*> lastGraphicsContext;
    auto oldRootedNodeSet = std::move(m_rootedNodeSet);

    if (needsComposite) {
        lastGraphicsContext = fd;
    }

    traceRepaintRegionJob(
        *this, fd, fd, lastGraphicsContext, m_lastLayoutResult, newResult,
        m_lastInlineTextLayoutResult, newInlineLayoutResult,
        m_dirtyAreaPerStackingContextOwners, m_rootedNodeSet, gotPaintingDirty,
        fd->node()->webView()->needsComposite());

    auto iter = m_lastLayoutResult.begin();
    while (iter != m_lastLayoutResult.end()) {
        if (iter->second.first.location() !=
            LayoutLocation(LayoutUnit::min(), LayoutUnit::min())) {
            // box is disappear
            gotPaintingDirty = true;
            Node* stackingContextOwner = iter->second.second;
            LayoutRect rt = iter->second.first;
            auto iter2 =
                m_dirtyAreaPerStackingContextOwners.find(stackingContextOwner);
            if (iter2 == m_dirtyAreaPerStackingContextOwners.end()) {
                m_dirtyAreaPerStackingContextOwners.insert(
                    std::make_pair(stackingContextOwner, rt));
            } else {
                iter2->second.unite(rt);
            }
        }
        iter++;
    }

    auto iter2 = m_lastInlineTextLayoutResult.begin();
    while (iter2 != m_lastInlineTextLayoutResult.end()) {
        if (std::get<2>(*iter2)) {
            // FrameBlockBox is disappear
            gotPaintingDirty = true;
            auto oldInlineResult = std::get<2>(*iter2);

            LayoutRect dirtyRect;
            for (size_t j = 0; j < oldInlineResult->size(); j++) {
                dirtyRect.unite(oldInlineResult->at(j).m_frameRect);
            }

            if (dirtyRect.size().width()) {
                auto iter3 = m_dirtyAreaPerStackingContextOwners.find(
                    std::get<1>(*iter2)->node());
                if (iter3 == m_dirtyAreaPerStackingContextOwners.end()) {
                    m_dirtyAreaPerStackingContextOwners.insert(
                        std::make_pair(std::get<1>(*iter2)->node(), dirtyRect));
                } else {
                    iter3->second.unite(dirtyRect);
                }
            }
        }
        iter2++;
    }

    m_lastLayoutResult = std::move(newResult);
    m_lastInlineTextLayoutResult = std::move(newInlineLayoutResult);

    return gotPaintingDirty;
}
} // namespace Starfish
