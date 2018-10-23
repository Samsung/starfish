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
            frame->asInlineNonReplacedBox()->origin()->clearNeedsPainting();
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
            r = computeBoxExtent(r, currentMatrix);
            m_repaintRegion.unite(r);
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
                    s.preTranslate(frame->borderLeft() + frame->paddingLeft(),
                                   frame->borderTop() + frame->paddingTop());
                    trackRepaintRegion(documentFrame->asFrameBox(), s);
                }
            }
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
