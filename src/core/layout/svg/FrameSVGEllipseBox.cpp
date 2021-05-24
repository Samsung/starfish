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
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameSVGEllipseBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

#include "core/dom/svg/SVGLinearGradientElement.h"
#include "core/dom/svg/SVGAnimatedTransformList.h"
#include "core/style/GradientData.h"
#include "core/style/CSSGradientValue.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/canvas/NativeGradient.h"

namespace Starfish {

void* FrameSVGEllipseBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGEllipseBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGEllipseBox)] = { 0 };
        FrameSVGEllipseBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGEllipseBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGEllipseBox::paintSVG(PaintingContext& ctx)
{
    FrameBox* cb = layoutParent()->asFrameBox();

    CanvasFillStrokeSource* info = nullptr;

    Path* newPath = path();
    if (newPath) {
        if (style()->fill()->hasUrl()) {
            // TODO: Only support linear gradient
            info = makeCanvasFillStrokeSource(style()->fill()->url());
        }

        ctx.m_canvas->save();
        float opacity = style()->opacity();
        if (info != nullptr) {
            ctx.m_canvas->setFillSource(info);
            ctx.m_canvas->fillPath(newPath);
        } else {
            Unit::Color fillColor = style()->fill()->color();
            ctx.m_canvas->setFillColor(
                Unit::Color(fillColor.r(), fillColor.g(), fillColor.b(),
                            fillColor.a() * style()->fillOpacity() * opacity));
            Unit::Color strokeColor = style()->stroke()->color();
            ctx.m_canvas->setStrokeColor(Unit::Color(
                strokeColor.r(), strokeColor.g(), strokeColor.b(),
                strokeColor.a() * style()->strokeOpacity() * opacity));
            ctx.m_canvas->setFillRule(style()->fillRule());
            ctx.m_canvas->fillPath(newPath);
            ctx.m_canvas->setLineWidth(
                style()->strokeWidth().specifiedValue(cb->width(), this));
            ctx.m_canvas->strokePath(newPath);

            ctx.m_canvas->restore();
        }
    }
}

Path* FrameSVGEllipseBox::path()
{
    Path* path = Path::create();
    FrameBox* cb = layoutParent()->asFrameBox();
    double cx = 0;
    if (style()->cx().isSpecified()) {
        cx = style()->cx().specifiedValue(cb->width(), this);
    }
    double cy = 0;
    if (style()->cy().isSpecified()) {
        cy = style()->cy().specifiedValue(cb->height(), this);
    }
    double rx = 0;
    if (style()->rx().isSpecified()) {
        rx = style()->rx().specifiedValue(cb->width(), this);
    }
    double ry = 0;
    if (style()->ry().isSpecified()) {
        ry = style()->ry().specifiedValue(cb->height(), this);
    }
    if (rx && ry) {
        path->ellipse(cx, cy, rx, ry, 0, 0, 2 * M_PI);
    } else {
        return nullptr;
    }

    return path;
}

CanvasFillStrokeSource* FrameSVGEllipseBox::makeCanvasFillStrokeSource(
    String* url)
{
    ResourceURL* resourceUrl = new ResourceURL(url);
    if (!resourceUrl->isValid()) {
        return nullptr;
    }

    String* urlString = resourceUrl->string();
    if (urlString->startsWith("#")) {
        if (!node()) {
            return nullptr;
        }

        String* id = urlString->substring(1, urlString->length() - 1);
        SVGElement* owner = node()->asSVGElement()->ownerSVGElement();
        SVGElement* matchingSvg = owner->getSVGElementById(id);
        if (!matchingSvg) {
            return nullptr;
        }

        LayoutRect fRect = frameRect();
        Unit::Rect rect =
            Unit::Rect(fRect.x(), fRect.y(), fRect.width(), fRect.height());
        CanvasGradient* gradient = nullptr;
        if (matchingSvg->isSVGLinearGradientElement()) {
            double x1 = 0;
            auto linearGradient = matchingSvg->asSVGLinearGradientElement();
            auto gradientTransform =
                linearGradient->gradientTransform()->baseVal();
            if (linearGradient->x1()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                x1 = linearGradient->x1()->baseVal()->valueInSpecifiedUnits() *
                     rect.width() / 100;
            } else {
                x1 = linearGradient->x1()->baseVal()->value();
            }
            double y1 = 0;
            if (linearGradient->y1()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                y1 = linearGradient->y1()->baseVal()->valueInSpecifiedUnits() *
                     rect.height() / 100;
            } else {
                y1 = linearGradient->y1()->baseVal()->value();
            }
            double x2 = 0;
            if (linearGradient->x2()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                x2 = linearGradient->x2()->baseVal()->valueInSpecifiedUnits() *
                     rect.width() / 100;
            } else {
                x2 = linearGradient->x2()->baseVal()->value();
            }
            double y2 = 0;
            if (linearGradient->y2()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                y2 = linearGradient->y2()->baseVal()->valueInSpecifiedUnits() *
                     rect.height() / 100;
            } else {
                y2 = linearGradient->y2()->baseVal()->value();
            }

            SkMatrix mat = SkMatrix::I();
            for (size_t i = 0; i < gradientTransform->length(); ++i) {
                mat = mat * gradientTransform->getItem(i)->matrix()->matrix();
            }

            double xx1 = x1 * mat[0] + y1 * mat[1] + fRect.width() * mat[2];
            double yy1 = x1 * mat[3] + y1 * mat[4] + fRect.height() * mat[5];
            double xx2 = x2 * mat[0] + y2 * mat[1] + fRect.width() * mat[2];
            double yy2 = x2 * mat[3] + y2 * mat[4] + fRect.height() * mat[5];

            gradient = new CanvasGradient(matchingSvg->executionContext(), xx1,
                                          yy1, xx2, yy2);

            const auto colorStops =
                matchingSvg->asSVGLinearGradientElement()->colorStops();
            size_t size = colorStops.size();

            for (size_t i = 0; i < size; ++i) {
                const auto& color = colorStops[i]->color().toString();
                const auto& offset = colorStops[i]->offset().numberData();
                gradient->addColorStop(offset, color);
            }
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
        auto canvasStyle = CanvasStyle::createCanvasGradient(gradient);
        return new CanvasFillStrokeSource(canvasStyle);
    }

    return nullptr;
}
} // namespace Starfish
