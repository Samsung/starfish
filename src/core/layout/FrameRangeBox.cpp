/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"

#include "core/dom/HTMLInputElement.h"
#include "core/dom/Node.h"
#include "core/layout/FrameRangeBox.h"
#include "core/modules/canvas/Canvas.h"
#include "core/style/ComputedStyle.h"

namespace Starfish {

// The rendering spec leaves a range control's metrics to the user agent; these
// match what other engines settled on for a horizontal slider. The width lives
// in the user agent sheet so that an author declaration can still replace it.
static const int RANGE_DEFAULT_HEIGHT = 16;
static const int RANGE_TRACK_THICKNESS = 4;
static const int RANGE_THUMB_SIZE = 12;

static const Unit::Color RANGE_TRACK_COLOR(183, 183, 183, 255);
static const Unit::Color RANGE_TRACK_DISABLED_COLOR(214, 214, 214, 255);
static const Unit::Color RANGE_THUMB_COLOR(89, 89, 89, 255);
static const Unit::Color RANGE_THUMB_DISABLED_COLOR(156, 156, 156, 255);

void* FrameRangeBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameRangeBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameRangeBox)] = { 0 };
        FrameRangeBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameRangeBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameRangeBox::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    // Skip FrameInputBox::layout(): there is no value text and no caret.
    FrameBlockBox::layout(ctx, resolveWhat);

    // The control has no child content, so an auto height resolves to zero.
    // A height that a flex container or a pair of opposing insets already
    // resolved is left alone -- that stretching is what makes the control
    // semi-replaced rather than replaced.
    if (style()->height().isAuto() && contentHeight() == 0) {
        setContentHeight(RANGE_DEFAULT_HEIGHT);
    }
}

void FrameRangeBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Unlike the other input types, appearance: none leaves the CSS box of a
    // range control alone and only drops the slider, so the background and
    // borders are painted either way.
    FrameBox::paintBackgroundAndBorders(canvas);
}

void FrameRangeBox::paintInlineContentBlock(Canvas* canvas,
                                            PaintPassMemos* memos)
{
    paintSlider(canvas);
}

void FrameRangeBox::paintContent(PaintingContext& ctx)
{
    if (canSkipPaintingStage(ctx)) {
        return;
    }

    if (ctx.m_paintingStage == PaintingStage::PaintingNormalFlowInline) {
        paintSlider(ctx.m_canvas);
    }
}

void FrameRangeBox::paintSlider(Canvas* canvas)
{
    // https://www.w3.org/TR/css-ui-4/#appearance-switching
    // appearance: none drops the native control; the element keeps the CSS box
    // that has already been painted.
    if (style()->appearance() == NoneAppearanceValue) {
        return;
    }

    LayoutRect content(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                       contentWidth(), contentHeight());
    if (content.width() <= 0 || content.height() <= 0) {
        return;
    }

    bool disabled = node()->asHTMLElement()->disabled();

    canvas->save();

    LayoutUnit trackThickness =
        std::min(LayoutUnit(RANGE_TRACK_THICKNESS), content.height());
    canvas->setFillColor(disabled ? RANGE_TRACK_DISABLED_COLOR
                                  : RANGE_TRACK_COLOR);
    canvas->drawRect(LayoutRect(
        content.x(), content.y() + (content.height() - trackThickness) / 2,
        content.width(), trackThickness));

    double fraction = node()->asHTMLInputElement()->rangeValueFraction();
    if (style()->direction() == DirectionValue::RtlDirectionValue) {
        fraction = 1 - fraction;
    }

    // The thumb travels between the two ends of the track, so its own width
    // comes off the distance it can cover.
    LayoutUnit thumbSize =
        std::min(LayoutUnit(RANGE_THUMB_SIZE), content.height());
    LayoutUnit travel = content.width() - thumbSize;
    canvas->setFillColor(disabled ? RANGE_THUMB_DISABLED_COLOR
                                  : RANGE_THUMB_COLOR);
    canvas->drawRect(
        LayoutRect(content.x() + LayoutUnit(travel.toFloat() * fraction),
                   content.y() + (content.height() - thumbSize) / 2, thumbSize,
                   thumbSize));

    canvas->restore();
}
} // namespace Starfish
