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

#ifndef __StarFishRepaintRegionTracker__
#define __StarFishRepaintRegionTracker__

#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameReplacedIFrame.h"
#include "core/layout/StackingContext.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"

namespace StarFish {

class RepaintRegionTracker {
public:
    RepaintRegionTracker(
        FrameBox* rootFrame, LayoutRect initialRepaintRegion,
        PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap,
        LayoutUnit sx, LayoutUnit sy)
        : m_repaintRegion(initialRepaintRegion)
        , m_prevDrawnStackingContextInfoMap(prevDrawnStackingContextInfoMap)
    {
        if (rootFrame->needsPainting()) {
            LayoutRect r = rootFrame->frameVisibleRect();
            r.setX(r.x() + sx);
            r.setY(r.y() + sy);

            m_repaintRegion.unite(r);
        }
        trackRepaintRegion(rootFrame, SkMatrix::I());

        auto iter = m_prevDrawnStackingContextInfoMap.begin();

        while (iter != m_prevDrawnStackingContextInfoMap.end()) {
            if (!iter->second.hasThisLayerThisTime) {
                // layer disappear
                m_repaintRegion.unite(iter->second.screenExtent);
            } else if (!iter->second.isEqualsWithPrevDrawing) {
                m_repaintRegion.unite(iter->second.screenExtent);
            }
            iter++;
        }
    }

    const LayoutRect& repaintRegion()
    {
        return m_repaintRegion;
    }

    ~RepaintRegionTracker()
    {
    }

protected:
    LayoutRect m_repaintRegion;
    PrevDrawnStackingContextInfoMap& m_prevDrawnStackingContextInfoMap;

    void trackRepaintRegion(FrameBox* frame, SkMatrix currentMatrix)
    {
        bool needsRepainting = frame->needsPainting();

        if (frame->isInlineBoxLayoutParentBox() &&
            frame->isInlineNonReplacedBox()) {
            needsRepainting |=
                frame->asInlineNonReplacedBox()->origin()->needsPainting();
            frame->asInlineNonReplacedBox()->origin()->setNeedsPainting(false);
        }

        StackingContext* sc = frame->stackingContext();
        if (sc) {
            auto iter = m_prevDrawnStackingContextInfoMap.find(frame->node());
            if (iter == m_prevDrawnStackingContextInfoMap.end()) {
                // if cannot find prevDrawingStackingContextInfo, force do
                // repaint self & containing block
                needsRepainting = true;
                FrameBox* cb = containingBlock(frame);
                if (cb) {
                    m_repaintRegion.unite(cb->computeScreenExtent());
                }
            } else {
                iter->second.hasThisLayerThisTime = true;
                if (iter->second.screenExtent == sc->screenExtent()) {
                    iter->second.isEqualsWithPrevDrawing = true;
                }
            }

            if (frame->style() &&
                frame->style()->position() == FixedPositionValue) {
                currentMatrix = frame->computeScreenMatrix();
            }
        }

        if (needsRepainting) {
            LayoutRect r = frame->frameVisibleRect();
            if (currentMatrix.rectStaysRect()) {
                SkRect skRect =
                    SkRect::MakeXYWH((float)r.x(), (float)r.y(),
                                     (float)r.width(), (float)r.height());

                currentMatrix.mapRect(&skRect);
                skRect.sort();
                LayoutRect tmp = LayoutRect(skRect.x(), skRect.y(),
                                            skRect.width(), skRect.height());
                m_repaintRegion.unite(tmp);
            } else {
                SkPoint pt[4];

                pt[0].fX = r.x();
                pt[0].fX = r.y();

                pt[1].fX = r.maxX();
                pt[1].fX = r.y();

                pt[2].fX = r.x();
                pt[2].fX = r.maxY();

                pt[3].fX = r.maxX();
                pt[3].fX = r.maxY();

                currentMatrix.mapPoints(pt, 4);

                LayoutUnit minX = pt[0].x();
                LayoutUnit minY = pt[0].y();
                LayoutUnit maxX = pt[0].x();
                LayoutUnit maxY = pt[0].y();

                for (size_t i = 1; i < 4; i++) {
                    minX = std::min((float)pt[i].x(), (float)minX);
                    minY = std::min((float)pt[i].y(), (float)minY);

                    maxX = std::max((float)pt[i].x(), (float)maxX);
                    maxY = std::max((float)pt[i].y(), (float)maxY);
                }

                LayoutRect tmp(minX, minY, (maxX - minX).abs(),
                               (maxY - minY).abs());
                m_repaintRegion.unite(tmp);
            }
        }

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
        }

        auto iter = frame->childFrameBoxiterator();
        while (iter->hasNext()) {
            auto box = iter->next();
            SkMatrix childMatrix = currentMatrix;

            if (box->stackingContext()) {
                SkMatrix m2 = box->stackingContext()->transformMatrix();
                if (!m2.isIdentity()) {
                    LayoutLocation to =
                        box->stackingContext()->transformOrigin();
                    childMatrix.postTranslate((float)to.x(), (float)to.y());
                    childMatrix.preConcat(m2);
                    childMatrix.postTranslate(-(float)to.x(), -(float)to.y());
                }
            }

            auto pos = box->absolutePointIncludingScroll(frame);
            childMatrix.postTranslate((float)pos.x(), (float)pos.y());
            trackRepaintRegion(box, childMatrix);
        }

        frame->setNeedsPainting(false);
    }
};
}

#endif
