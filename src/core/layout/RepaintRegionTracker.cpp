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

#include "StarfishConfig.h"
#include "RepaintRegionTracker.h"
#include "core/dom/Node.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameReplacedIFrame.h"
#include "core/layout/StackingContext.h"
#include "core/layout/ComputeOverflow.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/layout/svg/FrameSVGBox.h"
namespace Starfish {

RepaintRegionTracker::ComputeOverflow::ComputeOverflow(
    RepaintRegionTracker& tracker, FrameBox* frame, const SkMatrix& matrix)
    : tracker(tracker)
    , frame(frame)
{
    if (frame->shouldApplyOverflow()) {
        if (tracker.m_willCompositing) {
            // buffer owner
            auto nearOwner = tracker.findNearestStackingContextOwner(frame);
            if (nearOwner == frame) {
                LayoutRect scrollRect = frame->overflowRepaintRect();
                if (frame->isFrameBlockBox()) {
                    scrollRect.unite(LayoutRect(
                        0, 0, frame->asFrameBlockBox()->scrollWidth(),
                        frame->asFrameBlockBox()->scrollHeight()));
                }
                tracker.m_boundMaxExtentDueToOverflow.push_back(std::make_tuple(
                    scrollRect, frame->stackingContext(), frame));
            } else {
                tracker.m_boundMaxExtentDueToOverflow.push_back(std::make_tuple(
                    computeBoxExtent(
                        frame->overflowRepaintRect(),
                        frame->computeMatrixOnGraphicsBuffer(false)),
                    nearOwner->stackingContext(), frame));
            }
        } else {
            tracker.m_boundMaxExtentDueToOverflow.push_back(std::make_tuple(
                computeBoxExtent(frame->overflowRepaintRect(), matrix), nullptr,
                frame));
        }
    }
}

RepaintRegionTracker::ComputeOverflow::~ComputeOverflow()
{
    if (frame->shouldApplyOverflow()) {
        tracker.m_boundMaxExtentDueToOverflow.pop_back();
    }
}

RepaintRegionTracker::ClearNeedsPainting::ClearNeedsPainting(FrameBox* frame)
    : frame(frame)
{
}

RepaintRegionTracker::ClearNeedsPainting::~ClearNeedsPainting()
{
    frame->clearNeedsPainting();
}

RepaintRegionTracker::RepaintRegionTracker(
    const RepaintRegionTrackerContext& oldContext,
    RepaintRegionTrackerContext& newContext, FrameBox* rootFrame,
    bool needsFullPainting,
    PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap,
    LayoutUnit sx, LayoutUnit sy, bool wc)
    : m_willCompositing(wc)
    , m_needsFullPainting(needsFullPainting)
    , m_oldContext(oldContext)
    , m_newContext(newContext)
    , m_prevDrawnStackingContextInfoMap(prevDrawnStackingContextInfoMap)
{
    m_screenRect = LayoutRect(0, 0, rootFrame->node()->window()->innerWidth(),
                              rootFrame->node()->window()->innerHeight());

    if (m_needsFullPainting) {
        m_repaintRegionPerGraphicsLayer[nullptr].unite(m_screenRect);
    }

    if (rootFrame->needsPainting()) {
        m_repaintRegionPerGraphicsLayer[nullptr].unite(
            rootFrame->frameVisibleRect());
        m_needsFullPainting = true;
        rootFrame->clearNeedsPainting();
    }
    trackRepaintRegion(rootFrame, SkMatrix::I());

    auto iter = m_prevDrawnStackingContextInfoMap.begin();
    while (iter != m_prevDrawnStackingContextInfoMap.end()) {
        bool needsRepainting = false;
        if (!iter.value().hasThisLayerThisTime) {
            needsRepainting = true;
            // layer disappear
        } else if (!iter.value().isEqualsWithPrevDrawing) {
            needsRepainting = true;
        }

        if (needsRepainting) {
            m_repaintRegionPerGraphicsLayer[nullptr].unite(
                iter->second.screenExtent);
            if (m_willCompositing) {
                if (iter->second.graphicsLayerOwner) {
                    if (iter->second.graphicsLayerOwner->frame() &&
                        iter->second.graphicsLayerOwner->frame()
                            ->isFrameBox() &&
                        iter->second.graphicsLayerOwner->frame()
                            ->asFrameBox()
                            ->stackingContext() &&
                        iter->second.graphicsLayerOwner->frame()
                            ->asFrameBox()
                            ->stackingContext()
                            ->needsGraphicsBuffer()) {
                        m_repaintRegionPerGraphicsLayer[iter->second
                                                            .graphicsLayerOwner]
                            .unite(iter->second.extentOnGraphicsLayer);
                    }
                }
            }
        }
        iter++;
    }

    // extend repaint area for removing glitch
    {
        auto iter = m_repaintRegionPerGraphicsLayer.begin();
        while (iter != m_repaintRegionPerGraphicsLayer.end()) {
            if (!iter->second.isEmpty()) {
                iter->second.setX(iter->second.x() - 1);
                iter->second.setY(iter->second.y() - 1);
                iter->second.setWidth(iter->second.width() + 2);
                iter->second.setHeight(iter->second.height() + 2);
            }
            iter++;
        }
    }
}

const RepaintRegion& RepaintRegionTracker::repaintRegion()
{
    return m_repaintRegionPerGraphicsLayer;
}

RepaintRegionTracker::~RepaintRegionTracker()
{
}

void RepaintRegionTracker::notifyDirty(FrameBox* frame, StackingContext* sc,
                                       const SkMatrix& currentMatrix,
                                       LayoutRect r)
{
    LayoutRect tmp = computeBoxExtent(r, currentMatrix);
    ::Starfish::ComputeOverflow<JustCheckOveflow> co(frame);

    for (size_t i = 0; i < m_boundMaxExtentDueToOverflow.size(); i++) {
        if (co.canApplyOverflow(
                std::get<2>(m_boundMaxExtentDueToOverflow[i]))) {
            tmp = LayoutRect::overlappedRect(
                tmp, std::get<0>(m_boundMaxExtentDueToOverflow[i]));
        }
    }
    m_repaintRegionPerGraphicsLayer[nullptr].unite(tmp);
    if (m_willCompositing) {
        if (sc && sc->needsGraphicsBuffer()) {
            r = computeBoxExtent(r,
                                 frame->computeMatrixOnGraphicsBuffer(false));
            m_repaintRegionPerGraphicsLayer[frame->node()].unite(r);
        } else {
            if (frame->isFrameDocument() && frame->parent() == nullptr) {
                auto root = frame->node()->webView()->rootStackingContext();
                notifyDirty(root->owner(), root,
                            root->owner()->computeScreenMatrix(),
                            root->visibleRect());
            } else {
                r = computeBoxExtent(
                    r, frame->computeMatrixOnGraphicsBuffer(false));
                StackingContext* s =
                    findNearestStackingContextOwner(frame)->stackingContext();

                for (size_t i = 0; i < m_boundMaxExtentDueToOverflow.size();
                     i++) {
                    if ((std::get<1>(m_boundMaxExtentDueToOverflow[i]) == s) &&
                        co.canApplyOverflow(
                            std::get<2>(m_boundMaxExtentDueToOverflow[i]))) {
                        r = LayoutRect::overlappedRect(
                            r, std::get<0>(m_boundMaxExtentDueToOverflow[i]));
                    }
                }

                m_repaintRegionPerGraphicsLayer
                    [findNearestStackingContextOwner(frame)->node()]
                        .unite(r);
            }
        }
    }
}

FrameBox* RepaintRegionTracker::findNearestStackingContextOwner(FrameBox* frame)
{
    while (frame) {
        if (frame->stackingContext() &&
            frame->stackingContext()->needsGraphicsBuffer()) {
            return frame;
        }
        frame = frame->layoutParent()->asFrameBox();
    }
    return nullptr;
}

void RepaintRegionTracker::trackRepaintRegion(FrameBox* frame,
                                              SkMatrix currentMatrix)
{
    bool needsRepainting = frame->needsPainting();
    ClearNeedsPainting clearNeedsPainting(frame);

    if (frame->isInlineBoxLayoutParentBox() &&
        frame->isInlineNonReplacedBox()) {
        needsRepainting |=
            frame->asInlineNonReplacedBox()->origin()->needsPainting();
        frame->asInlineNonReplacedBox()->origin()->clearNeedsPainting();
    } else if (frame->isInlineTextBox()) {
        needsRepainting |= frame->asInlineTextBox()->origin()->needsPainting();
        frame->asInlineTextBox()->origin()->clearNeedsPainting();
    }

    bool orgNeedsPainting = needsRepainting;

    bool isInvisibleFromHere = false;

    StackingContext* sc = frame->stackingContext();
    if (sc) {
        if (frame->isBoxesInvisibleFromHere()) {
            bool needToSkip = true;
            auto iter = m_prevDrawnStackingContextInfoMap.find(frame->node());
            if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                iter.value().hasThisLayerThisTime = true;
                iter.value().isEqualsWithPrevDrawing = true;
                if (!iter->second.isVisibleBefore) {
                    needToSkip = false;
                }
            } else {
                needToSkip = false;
            }
            if (needToSkip) {
                return;
            }
        }

        auto iter = m_prevDrawnStackingContextInfoMap.find(frame->node());
        if (iter == m_prevDrawnStackingContextInfoMap.end()) {
            // if cannot find prevDrawingStackingContextInfo, force do
            // repaint self & containing block
            needsRepainting = true;
            FrameBox* cb = containingBlock(frame);
            if (cb) {
                notifyDirty(cb, cb->stackingContext(),
                            cb->computeScreenMatrix(), cb->frameVisibleRect());
            }
        } else {
            if (!frame->isBoxesInvisibleFromHere()) {
                iter.value().isVisibleBefore = true;
            }
            iter.value().hasThisLayerThisTime = true;

            bool compositedBefore = iter->second.needsGraphicsBuffer;
            bool willBeComposited = sc->needsGraphicsBuffer();

            if (compositedBefore != willBeComposited) {
                if (!compositedBefore) {
                    StackingContext* s = sc->parent();
                    while (s) {
                        if (s->needsGraphicsBuffer()) {
                            LayoutRect r2 = computeBoxExtent(
                                sc->visibleRect(),
                                frame
                                    ->computeMatrixOnGraphicsBufferOnGraphicsBuffer());
                            m_repaintRegionPerGraphicsLayer[s->owner()->node()]
                                .unite(r2);
                            break;
                        }
                        s = s->parent();
                    }
                } else {
                    if (frame->isRootElement()) {
                        if (!frame->node()
                                 ->document()
                                 ->browsingContext()
                                 ->isTopLevelBrowsingContext()) {
                            FrameBox* holder = frame->node()
                                                   ->document()
                                                   ->browsingContext()
                                                   ->sourceElement()
                                                   ->frame()
                                                   ->asFrameBox();
                            notifyDirty(holder, holder->stackingContext(),
                                        holder->computeScreenMatrix(),
                                        holder->frameVisibleRect());
                        }
                    }
                }

                if (compositedBefore) {
                    if (frame->isRootElement()) {
                        if (!frame->node()
                                 ->document()
                                 ->browsingContext()
                                 ->isTopLevelBrowsingContext()) {
                            FrameBox* holder = frame->node()
                                                   ->document()
                                                   ->browsingContext()
                                                   ->sourceElement()
                                                   ->frame()
                                                   ->asFrameBox();
                            notifyDirty(holder, holder->stackingContext(),
                                        holder->computeScreenMatrix(),
                                        holder->frameVisibleRect());
                        }
                    }
                }

                needsRepainting = true;
            } else if (compositedBefore &&
                       compositedBefore == willBeComposited) {
                if (compositedBefore) {
                    if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                        if (iter->second.additionalPixelRatio !=
                            sc->additionalPixelRatio()) {
                            needsRepainting = true;
                        }
                    }
                } else {
                    if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                        if (iter->second.graphicsBufferVisibleRect !=
                                sc->visibleRect() ||
                            iter->second.additionalPixelRatio !=
                                sc->additionalPixelRatio()) {
                            // visible rect changed
                            needsRepainting = true;
                        }
                    }
                }

            } else if (!compositedBefore && !willBeComposited) {
                if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                    if ((iter->second.opacity != frame->style()->opacity()) ||
                        (iter->second.transformMatrix !=
                         sc->transformMatrix())) {
                        needsRepainting = true;
                    }
                }
            }

            if (m_willCompositing) {
                if (sc->needsGraphicsBuffer() &&
                    iter->second.needsGraphicsBuffer) {
                    iter.value().isEqualsWithPrevDrawing = true;
                } else if (!sc->needsGraphicsBuffer() &&
                           !iter->second.needsGraphicsBuffer) {
                    LayoutRect extentThisTime = computeBoxExtent(
                        LayoutRect(0, 0, frame->width(), frame->height()),
                        frame->computeMatrixOnGraphicsBuffer(false));
                    if (iter->second.extentOnGraphicsLayer == extentThisTime) {
                        iter.value().isEqualsWithPrevDrawing = true;
                    }
                }

            } else {
                if (sc->isIFrameStackingContext()) {
                    if (iter->second.screenExtent ==
                        sc->parent()->screenExtent()) {
                        iter.value().isEqualsWithPrevDrawing = true;
                    }
                } else {
                    if (iter->second.screenExtent == sc->screenExtent()) {
                        iter.value().isEqualsWithPrevDrawing = true;
                    }
                }
            }
        }

        currentMatrix = frame->computeScreenMatrix();

        auto& layoutRepaintTracker = frame->node()
                                         ->document()
                                         ->browsingContext()
                                         ->layoutRepaintTracker();
        auto iter2 =
            layoutRepaintTracker.dirtyAreaPerStackingContextOwners().find(
                frame->node());
        if (iter2 !=
            layoutRepaintTracker.dirtyAreaPerStackingContextOwners().end()) {
            LayoutRect r = iter2->second;
            notifyDirty(frame, sc, currentMatrix, r);
        }
    } else {
        if (frame->node()) {
            auto iter = m_prevDrawnStackingContextInfoMap.find(frame->node());
            if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                // stacking context is disappear
                // invalidate new region
                needsRepainting = true;
            }
        }
    }

    LayoutRect currentVisibleRect = frame->frameVisibleRect();

    if (currentVisibleRect != frame->frameRect()) {
        m_newContext.m_visibleRectOfFrameRectIsOverflowedBoxes.insert(
            std::make_pair(frame, currentVisibleRect));
    }

    if (needsRepainting) {
        if (sc && sc->inScrollWithGraphicsBufferActive()) {
            currentVisibleRect = sc->visibleRect();
        }
        notifyDirty(frame, sc, currentMatrix, currentVisibleRect);
        auto iter =
            m_oldContext.m_visibleRectOfFrameRectIsOverflowedBoxes.find(frame);
        if (iter !=
            m_oldContext.m_visibleRectOfFrameRectIsOverflowedBoxes.end()) {
            notifyDirty(frame, sc, currentMatrix, iter->second);
        }

        if (frame->isRootElement() && sc && sc->needsGraphicsBuffer()) {
            LayoutRect r(0, 0, frame->node()->window()->scrollWidth(false),
                         frame->node()->window()->scrollHeight(false));

            notifyDirty(frame, sc, currentMatrix, r);
        }
    }

    ComputeOverflow o(*this, frame, currentMatrix);

    if (frame->isFrameReplaced() &&
        frame->asFrameReplaced()->isFrameReplacedIFrame()) {
        auto iframe = frame->asFrameReplaced()
                          ->asFrameReplacedIFrame()
                          ->node()
                          ->asHTMLIFrameElement();
        if (iframe->browsingContext()) {
            if (iframe->browsingContext()->window()) {
                auto documentFrame =
                    iframe->browsingContext()->window()->document()->frame();
                SkMatrix s = currentMatrix;
                s.postTranslate(frame->borderLeft() + frame->paddingLeft(),
                                frame->borderTop() + frame->paddingTop());
                trackRepaintRegion(documentFrame->asFrameBox(), s);
            }
        }
        return;
    }

    auto iter = frame->childFrameBoxIterator(
        alloca(FrameBox::maxChildFrameBoxIteratorSize));
    while (iter->hasNext()) {
        auto box = iter->next();
        SkMatrix childMatrix = currentMatrix;

        if (box->stackingContext() &&
            !box->stackingContext()->transformMatrix().isIdentity()) {
            childMatrix = box->computeScreenMatrix();
        } else {
            auto pos = box->absolutePointIncludingScroll(frame);
            childMatrix.preTranslate((float)pos.x(), (float)pos.y());
        }

        trackRepaintRegion(box, childMatrix);
    }
}
} // namespace Starfish
