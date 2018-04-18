/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "core/dom/Node.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/Document.h"
#ifdef STARFISH_ENABLE_TEST
#include "core/layout/FrameTreeBuilder.h"
#endif
#include "core/layout/FrameReplacedIFrame.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

#ifdef STARFISH_ENABLE_TEST
void FrameReplacedIFrame::dump(int depth)
{
    FrameBox::dump(depth);
    printf("\n");
    HTMLIFrameElement* v = node()->asHTMLIFrameElement();
    if (v->browsingContext()) {
        if (v->browsingContext()->window()) {
            FrameTreeBuilder::dumpFrameTree(v->browsingContext()->document(),
                                            depth + 1);
        }
    }
}
#endif

IntrinsicSize FrameReplacedIFrame::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    result.m_hasAspectRatio = false;
    auto v = node()->asHTMLIFrameElement();
    result.m_intrinsicContentSize =
        LayoutSize(v->frameWidth(), v->frameHeight());
    return result;
}

void FrameReplacedIFrame::layout(LayoutContext& ctx,
                                 Frame::LayoutWantToResolve resolveWhat)
{
    FrameReplaced::layout(ctx, resolveWhat);
    if (resolveWhat & ResolveHeight) {
        HTMLIFrameElement* v = node()->asHTMLIFrameElement();
        if (v->browsingContext()) {
            if (v->browsingContext()->window()) {
                v->browsingContext()->window()->resize(contentWidth(),
                                                       contentHeight());
            }
        }

        clearNeedsLayout();
    }
}

void FrameReplacedIFrame::establishesStackingContextIfNeeds()
{
    FrameReplaced::establishesStackingContextIfNeeds();
    HTMLIFrameElement* v = node()->asHTMLIFrameElement();
    if (v->browsingContext()) {
        if (v->browsingContext()->window()) {
            v->browsingContext()
                ->window()
                ->document()
                ->frame()
                ->establishesStackingContextIfNeeds();
            v->browsingContext()
                ->window()
                ->document()
                ->frame()
                ->asFrameBox()
                ->setLayoutParent(this);
        }
    }
}
}
