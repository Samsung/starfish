/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRepaintRegionTracker__
#define __StarfishRepaintRegionTracker__

#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameReplacedIFrame.h"
#include "core/layout/StackingContext.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"

namespace Starfish {

class RepaintRegionTracker {
public:
    class ComputeOverflow {
    public:
        ComputeOverflow(RepaintRegionTracker& tracker, FrameBox* frame,
                        const SkMatrix& matrix)
            : tracker(tracker)
            , frame(frame)
        {
            if (frame->shouldApplyOverflow()) {
                if (tracker.m_willCompositing) {
                    tracker.m_boundMaxExtentDueToOverflow.push_back(
                        std::make_tuple(
                            computeBoxExtent(
                                LayoutRect(0, 0, frame->width(),
                                           frame->height()),
                                frame->computeMatrixOnGraphicsBuffer()),
                            tracker.findNearestStackingContextOwner(frame)
                                ->stackingContext()));
                } else {
                    tracker.m_boundMaxExtentDueToOverflow.push_back(
                        std::make_tuple(
                            computeBoxExtent(LayoutRect(0, 0, frame->width(),
                                                        frame->height()),
                                             matrix),
                            nullptr));
                }
            }
        }

        ~ComputeOverflow()
        {
            if (frame->shouldApplyOverflow()) {
                tracker.m_boundMaxExtentDueToOverflow.pop_back();
            }
        }

    private:
        RepaintRegionTracker& tracker;
        FrameBox* frame;
    };

    RepaintRegionTracker(
        FrameBox* rootFrame, bool needsFullPainting,
        PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap,
        LayoutUnit sx, LayoutUnit sy, bool wc)
        : m_willCompositing(wc)
        , m_needsFullPainting(needsFullPainting)
        , m_prevDrawnStackingContextInfoMap(prevDrawnStackingContextInfoMap)
    {
        m_screenRect =
            LayoutRect(0, 0, rootFrame->node()->window()->innerWidth(),
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
            if (!iter->second.hasThisLayerThisTime) {
                needsRepainting = true;
                // layer disappear
            } else if (!iter->second.isEqualsWithPrevDrawing) {
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
                            m_repaintRegionPerGraphicsLayer
                                [iter->second.graphicsLayerOwner]
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

    const RepaintRegion& repaintRegion()
    {
        return m_repaintRegionPerGraphicsLayer;
    }

    ~RepaintRegionTracker()
    {
    }

    void notifyDirty(FrameBox* frame, StackingContext* sc,
                     const SkMatrix& currentMatrix, LayoutRect r)
    {
        LayoutRect tmp = computeBoxExtent(r, currentMatrix);

        for (size_t i = 0; i < m_boundMaxExtentDueToOverflow.size(); i++) {
            tmp = LayoutRect::overlappedRect(
                tmp, std::get<0>(m_boundMaxExtentDueToOverflow[i]));
        }

        m_repaintRegionPerGraphicsLayer[nullptr].unite(tmp);

        if (m_willCompositing) {
            if (sc && sc->needsGraphicsBuffer()) {
                m_repaintRegionPerGraphicsLayer[frame->node()].unite(
                    sc->visibleRect());
            } else {
                if (frame->isFrameDocument() && frame->parent() == nullptr) {
                    auto root = frame->node()->webView()->rootStackingContext();
                    notifyDirty(root->owner(), root,
                                root->owner()->computeScreenMatrix(),
                                root->visibleRect());
                } else {
                    r = computeBoxExtent(
                        r, frame->computeMatrixOnGraphicsBuffer());

                    if (frame->layoutParent() != nullptr) {
                        StackingContext* s =
                            findNearestStackingContextOwner(frame)
                                ->stackingContext();
                        for (size_t i = 0;
                             i < m_boundMaxExtentDueToOverflow.size(); i++) {
                            if (std::get<1>(m_boundMaxExtentDueToOverflow[i]) ==
                                s) {
                                r = LayoutRect::overlappedRect(
                                    r, std::get<0>(
                                           m_boundMaxExtentDueToOverflow[i]));
                            }
                        }
                    }

                    m_repaintRegionPerGraphicsLayer
                        [findNearestStackingContextOwner(frame)->node()]
                            .unite(r);
                }
            }
        }
    }

    FrameBox* findNearestStackingContextOwner(FrameBox* frame)
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

protected:
    bool m_willCompositing;
    bool m_needsFullPainting;
    std::unordered_map<Node*, LayoutRect> m_repaintRegionPerGraphicsLayer;
    std::vector<std::tuple<LayoutRect, StackingContext*>>
        m_boundMaxExtentDueToOverflow;
    LayoutRect m_screenRect;
    PrevDrawnStackingContextInfoMap& m_prevDrawnStackingContextInfoMap;

    void trackRepaintRegion(FrameBox* frame, SkMatrix currentMatrix)
    {
        bool needsRepainting = frame->needsPainting();

        if (frame->isInlineBoxLayoutParentBox() &&
            frame->isInlineNonReplacedBox()) {
            needsRepainting |=
                frame->asInlineNonReplacedBox()->origin()->needsPainting();
            frame->asInlineNonReplacedBox()->origin()->clearNeedsPainting();
        } else if (frame->isInlineTextBox()) {
            needsRepainting |=
                frame->asInlineTextBox()->origin()->needsPainting();
            frame->asInlineTextBox()->origin()->clearNeedsPainting();
        }

        bool orgNeedsPainting = needsRepainting;

        bool isInvisibleFromHere = false;

        StackingContext* sc = frame->stackingContext();
        if (sc) {
            if (frame->isBoxesInvisibleFromHere()) {
                auto iter =
                    m_prevDrawnStackingContextInfoMap.find(frame->node());
                if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                    iter->second.hasThisLayerThisTime = true;
                    iter->second.isEqualsWithPrevDrawing = true;
                }
                return;
            }

            auto iter = m_prevDrawnStackingContextInfoMap.find(frame->node());
            if (iter == m_prevDrawnStackingContextInfoMap.end()) {
                // if cannot find prevDrawingStackingContextInfo, force do
                // repaint self & containing block
                needsRepainting = true;
                FrameBox* cb = containingBlock(frame);
                if (cb) {
                    notifyDirty(cb, cb->stackingContext(),
                                cb->computeScreenMatrix(),
                                cb->frameVisibleRect());
                }
            } else {
                iter->second.hasThisLayerThisTime = true;

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
                                m_repaintRegionPerGraphicsLayer[s->owner()
                                                                    ->node()]
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
                    if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                        if (iter->second.graphicsBufferVisibleRect !=
                            sc->visibleRect()) {
                            // visible rect changed
                            needsRepainting = true;
                        }
                    }

                } else if (!compositedBefore && !willBeComposited) {
                    if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                        if ((iter->second.opacity !=
                             frame->style()->opacity()) ||
                            (iter->second.transformMatrix !=
                             sc->transformMatrix())) {
                            needsRepainting = true;
                        }
                    } else {
                        if (sc->transformMatrix() != SkMatrix::I()) {
                            needsRepainting = true;
                        }
                    }
                }

                if (m_willCompositing) {
                    if (sc->needsGraphicsBuffer() &&
                        iter->second.needsGraphicsBuffer) {
                        iter->second.isEqualsWithPrevDrawing = true;
                    } else if (!sc->needsGraphicsBuffer() &&
                               !iter->second.needsGraphicsBuffer) {
                        LayoutRect extentThisTime = computeBoxExtent(
                            LayoutRect(0, 0, frame->width(), frame->height()),
                            frame->computeMatrixOnGraphicsBuffer());
                        if (iter->second.extentOnGraphicsLayer ==
                            extentThisTime) {
                            iter->second.isEqualsWithPrevDrawing = true;
                        }
                    }

                } else {
                    if (sc->isIFrameStackingContext()) {
                        if (iter->second.screenExtent ==
                            sc->parent()->screenExtent()) {
                            iter->second.isEqualsWithPrevDrawing = true;
                        }
                    } else {
                        if (iter->second.screenExtent == sc->screenExtent()) {
                            iter->second.isEqualsWithPrevDrawing = true;
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
                layoutRepaintTracker.dirtyAreaPerStackingContextOwners()
                    .end()) {
                LayoutRect r = iter2->second;
                notifyDirty(frame, sc, currentMatrix, r);
            }
        } else {
            if (frame->node()) {
                auto iter =
                    m_prevDrawnStackingContextInfoMap.find(frame->node());
                if (iter != m_prevDrawnStackingContextInfoMap.end()) {
                    // stacking context is disappear
                    // invalidate new region
                    needsRepainting = true;
                }
            }
        }

        if (needsRepainting) {
            LayoutRect r = frame->frameVisibleRect();
            notifyDirty(frame, sc, currentMatrix, r);

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
                    auto documentFrame = iframe->browsingContext()
                                             ->window()
                                             ->document()
                                             ->frame();
                    SkMatrix s = currentMatrix;
                    s.postTranslate(frame->borderLeft() + frame->paddingLeft(),
                                    frame->borderTop() + frame->paddingTop());
                    trackRepaintRegion(documentFrame->asFrameBox(), s);
                }
            }
            return;
        }

        auto iter = frame->childFrameBoxiterator();
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

        frame->clearNeedsPainting();
    }
};
}

#endif
