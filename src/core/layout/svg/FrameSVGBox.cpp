/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "FrameSVGBox.h"
#include "FrameSVGClipPathBox.h"
#include "FrameSVGMaskBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/CalcData.h"
#include "core/style/GradientData.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/dom/svg/SVGLinearGradientElement.h"
#include "core/dom/svg/SVGRadialGradientElement.h"
#include "core/dom/svg/SVGAnimatedTransformList.h"
#include "platform/loader/ResourceURL.h"

namespace Starfish {

void* FrameSVGBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGBox)] = { 0 };
        FrameSVGBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGBox::layout(LayoutContext& ctx,
                         Frame::LayoutWantToResolve resolveWhat)
{
    FrameBox* cb = layoutParent()->asFrameBox();
    if (node()->asSVGElement()->needsGeometryAttributes()) {
        if (resolveWhat & Frame::ResolveWidth) {
            auto styleWidth = style()->width();
            LayoutUnit width;
            if (!styleWidth.isAuto()) {
                width = styleWidth.specifiedValue(cb->width(), this);
            }
            setWidth(width);
        }

        if (resolveWhat & Frame::ResolveHeight) {
            auto styleHeight = style()->height();
            LayoutUnit height;
            if (!styleHeight.isAuto()) {
                height = styleHeight.specifiedValue(cb->height(), this);
            }
            setHeight(height);
        }
    } else {
        if (resolveWhat & Frame::ResolveWidth) {
            setWidth(cb->width());
        }

        if (resolveWhat & Frame::ResolveHeight) {
            setHeight(cb->height());
        }
    }

    layoutSVG();

    Frame* f = firstChild();
    while (f) {
        if (f->isFrameSVGBox()) {
            f->asFrameSVGBox()->resolvePosition(ctx);
            f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
        }
        f = f->next();
    }
}

void FrameSVGBox::resolvePosition(LayoutContext& ctx)
{
    if (node()->asSVGElement()->needsGeometryAttributes()) {
        FrameBox* cb = layoutParent()->asFrameBox();
        auto styleX = style()->x();
        LayoutUnit xResult;
        if (styleX.isSpecified())
            xResult = styleX.specifiedValue(cb->width(), this);
        auto styleY = style()->y();
        LayoutUnit yResult;
        if (styleY.isSpecified())
            yResult = styleY.specifiedValue(cb->height(), this);
        setX(xResult);
        setY(yResult);
    } else {
        setX(0);
        setY(0);
    }
}

void FrameSVGBox::paintContent(PaintingContext& ctx)
{
    ctx.m_canvas->save();

    if (style()->visibility() == VisibilityValue::HiddenVisibilityValue) {
        ctx.m_canvas->setVisible(false);
    } else {
        ctx.m_canvas->setVisible(true);
    }

    if (style()->hasTransforms()) {
        FrameBox* cb = layoutParent()->asFrameBox();
        auto matrix =
            style()->transformsToMatrix(cb->width(), cb->height(), this, true);
        if (!matrix.isIdentity()) {
            ctx.m_canvas->translate(-x(), -y());
            ctx.m_canvas->postMatrix(matrix);
            ctx.m_canvas->translate(x(), y());
        }
    }

    float opacity = style()->opacity();
    if (opacity != 1) {
        ctx.m_canvas->beginOpacityLayer(opacity);
    }

    if (m_hasClipPath && node()->isSVGElement() &&
        node()->asSVGElement()->clipPathElement()) {
        Frame* clipPathFrame =
            node()->asSVGElement()->clipPathElement()->frame();
        if (clipPathFrame && clipPathFrame->isFrameSVGClipPathBox()) {
            Path* clipPath = clipPathFrame->asFrameSVGClipPathBox()->path();
            if (clipPath) {
                ctx.m_canvas->translate(-x(), -y());
                ctx.m_canvas->clipPath(clipPath);
                ctx.m_canvas->translate(x(), y());
            }
        }
    }

    if (m_hasMask && node()->isSVGElement() &&
        node()->asSVGElement()->maskElement()) {
        Frame* maskFrame = node()->asSVGElement()->maskElement()->frame();
        if (maskFrame && maskFrame->isFrameSVGMaskBox()) {
            maskFrame->asFrameSVGMaskBox()->applyMask(ctx, x(), y());
        }
    }

    ctx.m_canvas->save();
    paintSVG(ctx);
    ctx.m_canvas->restore();
    paintChildrenWith(ctx);

    if (opacity != 1) {
        ctx.m_canvas->endOpacityLayer();
    }

    ctx.m_canvas->restore();
}

#define IS_SVGLENGTH_UNIT_TYPE_NUMBER(gradient, name)         \
    (svg##gradient##Element->name()->baseVal()->unitType() == \
     SVGLength::SVG_LENGTHTYPE_NUMBER)

static Nullable<GradientDrawingInfo*> makeLinearGradientDrawingInfo(
    SVGLinearGradientElement* svgLinearGradientElement, LayoutRect layoutRect,
    FrameBox* frameBox)
{
    ComputedStyle* computedStyle = svgLinearGradientElement->style();
    Length x1 = computedStyle->x1();
    Length y1 = computedStyle->y1();
    Length x2 = computedStyle->x2();
    Length y2 = computedStyle->y2();

    if (x1.isAuto()) {
        x1 = Length(Length::Percent, 0);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(LinearGradient, x1)) {
        x1 = Length(Length::Percent, x1.fixed());
    }
    if (y1.isAuto()) {
        y1 = Length(Length::Percent, 0);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(LinearGradient, y1)) {
        y1 = Length(Length::Percent, y1.fixed());
    }
    if (x2.isAuto()) {
        x2 = Length(Length::Percent, 0);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(LinearGradient, x2)) {
        x2 = Length(Length::Percent, x2.fixed());
    }
    if (y2.isAuto()) {
        y2 = Length(Length::Percent, 0);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(LinearGradient, y2)) {
        y2 = Length(Length::Percent, y2.fixed());
    }

    GradientData* gradientData = new LinearGradientData();
    gradientData->colorStopList() =
        svgLinearGradientElement->asSVGLinearGradientElement()->colorStops();

    Unit::Rect unitRect = Unit::Rect(layoutRect.x(), layoutRect.y(),
                                     layoutRect.width(), layoutRect.height());
    Nullable<GradientDrawingInfo*> gradientDrawingInfo =
        gradientData->asLinearGradientData()->makeGradientDrawingInfo(unitRect,
                                                                      frameBox);
    gradientDrawingInfo->x1 = x1.specifiedValue(unitRect.width(), frameBox);
    gradientDrawingInfo->y1 = y1.specifiedValue(unitRect.height(), frameBox);
    gradientDrawingInfo->x2 = x2.specifiedValue(unitRect.width(), frameBox);
    gradientDrawingInfo->y2 = y2.specifiedValue(unitRect.height(), frameBox);

    return gradientDrawingInfo;
}

static Nullable<GradientDrawingInfo*> makeRadialGradientDrawingInfo(
    SVGRadialGradientElement* svgRadialGradientElement, LayoutRect layoutRect,
    FrameBox* frameBox)
{
    ComputedStyle* computedStyle = svgRadialGradientElement->style();
    Length cx = computedStyle->cx();
    Length cy = computedStyle->cy();
    Length r = computedStyle->r();

    if (cx.isAuto()) {
        cx = Length(Length::Percent, 0.5);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(RadialGradient, cx)) {
        cx = Length(Length::Percent, cx.fixed());
    }
    if (cy.isAuto()) {
        cy = Length(Length::Percent, 0.5);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(RadialGradient, cy)) {
        cy = Length(Length::Percent, cy.fixed());
    }
    if (r.isAuto()) {
        r = Length(Length::Percent, 0.5);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(RadialGradient, r)) {
        r = Length(Length::Percent, r.fixed());
    }

    GradientData* gradientData = new RadialGradientData();
    gradientData->setHorizontalSide(SideValue::LeftSideValue);
    gradientData->setVerticalSide(SideValue::TopSideValue);

    RadialGradientData* radialGradient = gradientData->asRadialGradientData();
    radialGradient->setHorizontalSideOffset(cx);
    radialGradient->setVerticalSideOffset(cy);
    radialGradient->setFirstRadius(r);
    radialGradient->setSecondRadius(r);
    radialGradient->colorStopList() =
        svgRadialGradientElement->asSVGRadialGradientElement()->colorStops();

    Unit::Rect unitRect = Unit::Rect(layoutRect.x(), layoutRect.y(),
                                     layoutRect.width(), layoutRect.height());
    Nullable<GradientDrawingInfo*> gradientDrawingInfo =
        radialGradient->makeGradientDrawingInfo(unitRect, frameBox);

    return gradientDrawingInfo;
}
#undef IS_SVGLENGTH_UNIT_TYPE_NUMBER

Nullable<GradientDrawingInfo*> FrameSVGBox::makeGradientDrawingInfo(String* url)
{
    ResourceURL* resourceUrl = new ResourceURL(url);
    if (!resourceUrl->isValid()) {
        return nullptr;
    }

    // NOTE: Consider obtaining a reusable SVG node that is locally available
    // under the same root SVGElement.
    String* urlString = resourceUrl->string();
    if (!urlString->startsWith("#")) {
        return nullptr;
    }
    if (!node()) {
        return nullptr;
    }

    String* id = urlString->substring(1, urlString->length() - 1);
    SVGElement* owner = node()->asSVGElement()->ownerSVGElement();
    SVGElement* matchingSvg = owner->getSVGElementById(id);
    if (!matchingSvg) {
        return nullptr;
    }

    LayoutRect layoutRect = frameRect();
    Nullable<GradientDrawingInfo*> gradientDrawingInfo = nullptr;
    if (matchingSvg->isSVGLinearGradientElement()) {
        // NOTE : There is a problem that width and height are different and the
        // gradient direction is not normally drawn in cases other than 0, 90,
        // 180, 270 degrees by a given x1, x2, y1, y2 value.
        gradientDrawingInfo = makeLinearGradientDrawingInfo(
            matchingSvg->asSVGLinearGradientElement(), layoutRect, this);
    } else if (matchingSvg->isSVGRadialGradientElement()) {
        gradientDrawingInfo = makeRadialGradientDrawingInfo(
            matchingSvg->asSVGRadialGradientElement(), layoutRect, this);
    } else {
        STARFISH_UNIMPLEMENTED();
    }

    return gradientDrawingInfo;
}

std::vector<std::pair<double, double>> FrameSVGBox::parsePointsFromString(
    String* str)
{
    std::vector<std::pair<double, double>> result;
    auto utf8Str = str->toUTF8NonGCString();
    CSSTokenVector tokensInput;
    const char* sep = ",-";
    CSSStyleDeclaration::tokenizeCSSValue(tokensInput, utf8Str.data(),
                                          utf8Str.length(), sep, 2, true);
    std::vector<CSSTokenValue> tokens;
    tokens.reserve(tokensInput.size());
    for (size_t i = 0; i < tokensInput.size(); i++) {
        tokens.push_back(std::move(tokensInput[i]));
    }
    enum Mode {
        WaitCoordsX,
        WaitCoordsY,
    };
    Mode mode = Mode::WaitCoordsX;
    bool gotMinus = false;
    float x, y;

#define READ_NUMBER(n)                                             \
    if (!CSSPropertyParser::parseNumber(                           \
            token.data(), CSSPropertyParser::AllowNegative, &n)) { \
        break;                                                     \
    }                                                              \
    if (gotMinus) {                                                \
        n = -n;                                                    \
    }                                                              \
    gotMinus = false;

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& token = tokens[i];
        if (token.equals(",")) {
            continue;
        }

        if (token.equals("-")) {
            if (gotMinus) {
                // error
                break;
            }
            gotMinus = true;
            continue;
        }

        {
            auto token = tokens[i];
            bool hasMultipleDot = false;
            bool seenDot = false;
            for (size_t k = 0; k < token.size(); k++) {
                if (token[k] == '.') {
                    if (!seenDot) {
                        seenDot = true;
                    } else {
                        hasMultipleDot = true;
                        tokens.erase(tokens.begin() + i);
                        CSSTokenValue s1 = token.substr(0, k);
                        CSSTokenValue s2 = token.substr(k, token.size() - k);
                        tokens.insert(tokens.begin() + i, s1);
                        tokens.insert(tokens.begin() + i + 1, s2);
                        i--;
                        break;
                    }
                }
            }
            if (hasMultipleDot) {
                continue;
            }
        }

        if (mode == Mode::WaitCoordsX) {
            READ_NUMBER(x);
            mode = Mode::WaitCoordsY;
        } else {
            READ_NUMBER(y);
            mode = Mode::WaitCoordsX;

            result.push_back(std::make_pair(x, y));
        }
    }

    return result;
}

CanvasFillStrokeSource* FrameSVGBox::makeCanvasFillStrokeSource(String* url)
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

        // Use extents of the path
        Unit::Rect rect = path()->boundingRect(true).snapSizeToPixel();
        CanvasGradient* gradient = nullptr;
        if (matchingSvg->isSVGLinearGradientElement()) {
            SVGLinearGradientElement* gradientElement =
                matchingSvg->asSVGLinearGradientElement();

            double x1 = 0;
            if (gradientElement->x1()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                x1 = rect.x() +
                     gradientElement->x1()->baseVal()->valueInSpecifiedUnits() *
                         rect.width() / 100;
            } else {
                x1 = gradientElement->x1()->baseVal()->value();
            }

            double y1 = 0;
            if (gradientElement->y1()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                y1 = rect.y() +
                     gradientElement->y1()->baseVal()->valueInSpecifiedUnits() *
                         rect.height() / 100;
            } else {
                y1 = gradientElement->y1()->baseVal()->value();
            }

            double x2 = 0;
            if (gradientElement->x2()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                x2 = rect.x() +
                     gradientElement->x2()->baseVal()->valueInSpecifiedUnits() *
                         rect.width() / 100;
            } else {
                x2 = gradientElement->x2()->baseVal()->value();
            }

            double y2 = 0;
            if (gradientElement->y2()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                y2 = rect.y() +
                     gradientElement->y2()->baseVal()->valueInSpecifiedUnits() *
                         rect.height() / 100;
            } else {
                y2 = gradientElement->y2()->baseVal()->value();
            }

            SVGTransformList* gradientTransform =
                gradientElement->gradientTransform()->baseVal();
            SkMatrix mat = SkMatrix::I();
            for (size_t i = 0; i < gradientTransform->length(); ++i) {
                mat = mat * gradientTransform->getItem(i)->matrix()->matrix();
            }

            double xx1 = x1 * mat[0] + y1 * mat[1] + rect.width() * mat[2];
            double yy1 = x1 * mat[3] + y1 * mat[4] + rect.height() * mat[5];
            double xx2 = x2 * mat[0] + y2 * mat[1] + rect.width() * mat[2];
            double yy2 = x2 * mat[3] + y2 * mat[4] + rect.height() * mat[5];

            gradient = new CanvasGradient(matchingSvg->executionContext(), xx1,
                                          yy1, xx2, yy2);

            const auto colorStops = gradientElement->colorStops();
            size_t size = colorStops.size();
            for (size_t i = 0; i < size; ++i) {
                gradient->addColorStop(colorStops[i]->offset().numberData(),
                                       colorStops[i]->color());
            }
        } else if (matchingSvg->isSVGRadialGradientElement()) {
            // RadialGradient implementation required.
            STARFISH_UNIMPLEMENTED();
            return nullptr;
        } else {
            STARFISH_UNIMPLEMENTED();
            return nullptr;
        }

        auto canvasStyle = CanvasStyle::createCanvasGradient(gradient);
        return new CanvasFillStrokeSource(canvasStyle);
    }
    return nullptr;
}

void FrameSVGBox::paintSVG(PaintingContext& ctx)
{
    FrameBox* cb = layoutParent()->asFrameBox();

    CanvasFillStrokeSource* info = nullptr;

    Path* newPath = path();
    if (newPath) {
        if (style()->fill()->hasUrl()) {
            // TODO: Only support linear gradient
            info = makeCanvasFillStrokeSource(style()->fill()->url());

            // TODO: Remove ME!
            // Temporarily use the existing path until implementing radial
            // gradient at makeCanvasFillStrokeSource.
            if (!info) {
                Nullable<GradientDrawingInfo*> radialGradientInfo =
                    makeGradientDrawingInfo(style()->fill()->url());
                if (radialGradientInfo.hasValue()) {
                    ctx.m_canvas->save();
                    Unit::Rect rect =
                        newPath->boundingRect(true).snapSizeToPixel();
                    std::shared_ptr<NativeGradient> gradient =
                        NativeGradient::create(radialGradientInfo.getValue());
                    if (radialGradientInfo->type ==
                        GradientType::RadialGradient) {
                        ctx.m_canvas->drawRadialGradient(
                            rect, radialGradientInfo.getValue(),
                            gradient.get());
                    }
                    ctx.m_canvas->restore();
                }
            }
        }
        ctx.m_canvas->save();
        if (info != nullptr) {
            ctx.m_canvas->setFillSource(info);
        } else {
            Unit::Color fillColor = style()->fill()->color();
            ctx.m_canvas->setFillColor(
                Unit::Color(fillColor.r(), fillColor.g(), fillColor.b(),
                            fillColor.a() * style()->fillOpacity()));
            ctx.m_canvas->setFillRule(style()->fillRule());
        }
        ctx.m_canvas->fillPath(newPath);
        Unit::Color strokeColor = style()->stroke()->color();
        ctx.m_canvas->setStrokeColor(
            Unit::Color(strokeColor.r(), strokeColor.g(), strokeColor.b(),
                        strokeColor.a() * style()->strokeOpacity()));
        ctx.m_canvas->setLineWidth(
            style()->strokeWidth().specifiedValue(cb->width(), this));
        ctx.m_canvas->strokePath(newPath);
        ctx.m_canvas->restore();
    }
}

} // namespace Starfish
