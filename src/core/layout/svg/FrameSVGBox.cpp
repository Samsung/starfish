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
#include "FrameSVGSVGBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/canvas/CanvasGradient.h"
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

bool FrameSVGBox::needsSVGGeometryAttributes()
{
    return node()->asSVGElement()->needsGeometryAttributes();
}

bool FrameSVGBox::isAlwaysInvisible()
{
    return node()->asSVGElement()->isStructuralElement();
}

LayoutLocation FrameSVGBox::resolveStylePosition(FrameBox* box,
                                                 const LayoutSize& viewport)
{
    STARFISH_ASSERT(box->needsSVGGeometryAttributes());
    if (box->isFrameSVGBox()) {
        return box->asFrameSVGBox()->resolveStylePosition(viewport);
    } else {
        return box->frameRect().location();
    }
}

Optional<LayoutUnit> FrameSVGBox::resolveStyleLength(const Length& length, const LayoutUnit& viewportLength)
{
    Optional<LayoutUnit> result;
    if (length.isSpecified()) {
        result = LayoutUnit(length.specifiedValue(viewportLength, this));
    }
    return result;
}

LayoutLocation FrameSVGBox::resolveStylePosition(const LayoutSize& viewport)
{
    STARFISH_ASSERT(node()->asSVGElement()->needsGeometryAttributes());
    LayoutLocation result;
    auto styleX = style()->x();
    if (styleX.isSpecified()) {
        result.setX(styleX.specifiedValue(viewport.width(), this));
    }
    auto styleY = style()->y();
    LayoutUnit yResult;
    if (styleY.isSpecified()) {
        result.setY(styleY.specifiedValue(viewport.height(), this));
    }
    return result;
}

LayoutSize FrameSVGBox::resolveStyleSize(const LayoutSize& viewport)
{
    STARFISH_ASSERT(node()->asSVGElement()->needsSizingAttributes());
    LayoutSize result;
    auto styleWidth = style()->width();
    LayoutUnit width;
    if (!styleWidth.isAuto()) {
        width = styleWidth.specifiedValue(viewport.width(), this);
    }

    result.setWidth(width);

    auto styleHeight = style()->height();
    LayoutUnit height;
    if (!styleHeight.isAuto()) {
        height = styleHeight.specifiedValue(viewport.height(), this);
    }
    result.setHeight(height);
    return result;
}

void FrameSVGBox::layout(SVGLayoutContext& ctx, SkMatrix matrix)
{
    if (node()->asSVGElement()->needsSizingAttributes()) {
        m_frameRect.setSize(resolveStyleSize(ctx.viewport));
    }

    bool needsGeometryAttributes = needsSVGGeometryAttributes();
    LayoutLocation stylePos;
    bool needsComputeFrameRect =
        needsGeometryAttributes || node()->asSVGElement()->isShapeElement();
    if (needsGeometryAttributes) {
        stylePos = resolveStylePosition(ctx.viewport);
        m_frameRect.setLocation(stylePos);
    } else if (node()->asSVGElement()->isShapeElement()) {
        auto p = path();
        if (p) {
            Unit::Rect boundingRect = p->boundingRect(true);
            m_frameRect =
                LayoutRect(boundingRect.x(), boundingRect.y(),
                           boundingRect.width(), boundingRect.height());
        } else {
            m_frameRect = LayoutRect();
        }
    }

    layoutSVG(ctx);

    // expand frameRect with stroke width
    if (!needsGeometryAttributes &&
        (!style()->stroke()->color().isTransparent() ||
         style()->stroke()->hasUrl()) &&
        !m_frameRect.isEmpty()) {
        LayoutUnit strokeWidth(style()->strokeWidth().specifiedValue(
            ctx.normalizedDiagonalViewportLength, this));
        if (strokeWidth > 1) {
            LayoutUnit halfStrokeWidth = strokeWidth / 2;

            m_frameRect.setX(m_frameRect.x() - halfStrokeWidth);
            m_frameRect.setY(m_frameRect.y() - halfStrokeWidth);
            m_frameRect.setWidth(m_frameRect.width() + strokeWidth);
            m_frameRect.setHeight(m_frameRect.height() + strokeWidth);
        }
    }

    // update frameRect with transform
    if (UNLIKELY(style()->hasTransforms())) {
        auto styleMatrix = style()->transformsToMatrix(
            ctx.viewport.width(), ctx.viewport.height(), this, true);
        if (!styleMatrix.isIdentity()) {
            if (style()->hasTransformOrigin()) {
                auto to = style()->transformOrigin()->originValue();
                auto ox =
                    to->getXAxis().specifiedValue(ctx.viewport.width(), this);
                auto oy =
                    to->getYAxis().specifiedValue(ctx.viewport.height(), this);
                matrix.postTranslate(ox, oy);
                matrix.preConcat(styleMatrix);
                matrix.postTranslate(-ox, -oy);
            } else {
                if (needsGeometryAttributes) {
                    matrix.postTranslate(-stylePos.x().toFloat(),
                                         -stylePos.y().toFloat());
                }
                matrix.preConcat(styleMatrix);
                if (needsGeometryAttributes) {
                    matrix.postTranslate(stylePos.x().toFloat(),
                                         stylePos.y().toFloat());
                }
            }
        }
    }

    if (!matrix.isIdentity() && needsComputeFrameRect) {
        m_frameRect = computeBoxExtent(m_frameRect, matrix);
    }

    Frame* f = firstChild();
    while (f) {
        if (f->isFrameSVGBox()) {
            f->asFrameSVGBox()->layout(ctx, matrix);
        } else if (f->isFrameSVGSVGBox()) {
            f->layout(ctx.layoutContext,
                      Frame::LayoutWantToResolve::ResolveAll);
        }

        f = f->next();
    }

    if (node()->asSVGElement()->isStructuralElement()) {
        m_frameRect = LayoutRect();
        f = firstChild();
        while (f) {
            m_frameRect.unite(f->asFrameBox()->frameRect());
            f = f->next();
        }

        f = firstChild();
        while (f) {
            LayoutRect childRect = f->asFrameBox()->frameRect();
            f->asFrameBox()->setX(childRect.x() - m_frameRect.x());
            f->asFrameBox()->setY(childRect.y() - m_frameRect.y());
            f = f->next();
        }
    }

    postLayoutSVG(ctx);
}

void FrameSVGBox::layout(LayoutContext& ctx,
                         Frame::LayoutWantToResolve resolveWhat)
{
    STARFISH_ASSERT_NOT_REACHED();
}

LayoutSize FrameSVGBox::viewport()
{
    Frame* f = this;
    while (true) {
        if (f->isFrameSVGSVGBox()) {
            return f->asFrameSVGSVGBox()->viewport();
        }
        f = f->layoutParent();
    }
}

LayoutUnit FrameSVGBox::normalizedDiagonalViewportLength()
{
    Frame* f = this;
    while (true) {
        if (f->isFrameSVGSVGBox()) {
            return f->asFrameSVGSVGBox()->normalizedDiagonalViewportLength();
        }
        f = f->layoutParent();
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

    auto vp = viewport();
    bool needsGeometryAttributes = needsSVGGeometryAttributes();

    if (style()->hasTransforms()) {
        auto matrix =
            style()->transformsToMatrix(vp.width(), vp.height(), this, true);
        if (!matrix.isIdentity()) {
            SkMatrix test;
            bool testResult = matrix.invert(&test);
            if (!testResult) {
                ctx.m_canvas->restore();
                // invalid matrix to transform svg
                return;
            }

            if (style()->hasTransformOrigin()) {
                auto to = style()->transformOrigin()->originValue();
                auto vp = viewport();
                auto ox = to->getXAxis().specifiedValue(vp.width(), this);
                auto oy = to->getYAxis().specifiedValue(vp.height(), this);
                if (needsGeometryAttributes) {
                    ctx.m_canvas->translate(ox, oy);
                }
                ctx.m_canvas->postMatrix(matrix);
                if (needsGeometryAttributes) {
                    ctx.m_canvas->translate(-ox, -oy);
                }
            } else {
                ctx.m_canvas->postMatrix(matrix);
            }
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
            auto clipPath = clipPathFrame->asFrameSVGClipPathBox()->path();
            if (clipPath) {
                ctx.m_canvas->clipPath(clipPath.value());
            }
        }
    }

    if (m_hasMask && node()->isSVGElement() &&
        node()->asSVGElement()->maskElement()) {
        Frame* maskFrame = node()->asSVGElement()->maskElement()->frame();
        if (maskFrame && maskFrame->isFrameSVGMaskBox()) {
            maskFrame->asFrameSVGMaskBox()->applyMask(ctx);
        }
    }

    ctx.m_canvas->save();
    paintSVG(ctx);
    ctx.m_canvas->restore();

    prepareChildPainting(ctx.m_canvas);

    Frame* child = firstChild();
    while (child) {
        child->asFrameBox()->paintContent(ctx);
        child = child->next();
    }

    if (opacity != 1) {
        ctx.m_canvas->endOpacityLayer();
    }

    ctx.m_canvas->restore();
}

#define IS_SVGLENGTH_UNIT_TYPE_NUMBER(gradient, name)         \
    (svg##gradient##Element->name()->baseVal()->unitType() == \
     SVGLength::SVG_LENGTHTYPE_NUMBER)

static Optional<GradientDrawingInfo*> makeLinearGradientDrawingInfo(
    SVGLinearGradientElement* svgLinearGradientElement, FrameSVGBox* frameBox,
    const Unit::Rect& rect)
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

    Optional<GradientDrawingInfo*> gradientDrawingInfo =
        gradientData->asLinearGradientData()->makeGradientDrawingInfo(rect,
                                                                      frameBox);
    gradientDrawingInfo->x1 =
        x1.specifiedValue(rect.width(), frameBox) + rect.x();
    gradientDrawingInfo->y1 =
        y1.specifiedValue(rect.height(), frameBox) + rect.y();
    gradientDrawingInfo->x2 =
        x2.specifiedValue(rect.width(), frameBox) + rect.x();
    gradientDrawingInfo->y2 =
        y2.specifiedValue(rect.height(), frameBox) + rect.y();

    return gradientDrawingInfo;
}

static Optional<GradientDrawingInfo*> makeRadialGradientDrawingInfo(
    SVGRadialGradientElement* svgRadialGradientElement, FrameSVGBox* frameBox,
    const Unit::Rect& rect)
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
    cx = Length(Length::Fixed,
                cx.specifiedValue(rect.width(), frameBox) + rect.x());

    if (cy.isAuto()) {
        cy = Length(Length::Percent, 0.5);
    } else if (IS_SVGLENGTH_UNIT_TYPE_NUMBER(RadialGradient, cy)) {
        cy = Length(Length::Percent, cy.fixed());
    }
    cy = Length(Length::Fixed,
                cy.specifiedValue(rect.height(), frameBox) + rect.y());

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

    Optional<GradientDrawingInfo*> gradientDrawingInfo =
        radialGradient->makeGradientDrawingInfo(rect, frameBox);

    return gradientDrawingInfo;
}
#undef IS_SVGLENGTH_UNIT_TYPE_NUMBER

Optional<GradientDrawingInfo*> FrameSVGBox::makeGradientDrawingInfo(
    String* url, const Unit::Rect& rect)
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
    auto owner = node()->asSVGElement()->ownerSVGElement();
    if (!owner) {
        return nullptr;
    }
    auto matchingSvg = owner->getSVGElementById(id);
    if (!matchingSvg) {
        return nullptr;
    }

    Optional<GradientDrawingInfo*> gradientDrawingInfo = nullptr;
    if (matchingSvg->isSVGLinearGradientElement()) {
        // NOTE : There is a problem that width and height are different and the
        // gradient direction is not normally drawn in cases other than 0, 90,
        // 180, 270 degrees by a given x1, x2, y1, y2 value.
        gradientDrawingInfo = makeLinearGradientDrawingInfo(
            matchingSvg->asSVGLinearGradientElement(), this, rect);
    } else if (matchingSvg->isSVGRadialGradientElement()) {
        gradientDrawingInfo = makeRadialGradientDrawingInfo(
            matchingSvg->asSVGRadialGradientElement(), this, rect);
    } else {
        STARFISH_UNSUPPORTED("SVG Gradient type");
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

Optional<CanvasFillStrokeSource*> FrameSVGBox::makeCanvasFillStrokeSource(
    String* url, const Unit::Rect& svgRect)
{
    Unit::Rect rect = svgRect;

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
        auto owner = node()->asSVGElement()->ownerSVGElement();
        if (!owner) {
            return nullptr;
        }
        auto matchingSvg = owner->getSVGElementById(id);
        if (!matchingSvg) {
            return nullptr;
        }

        // Use extents of the path
        CanvasGradient* gradient = nullptr;
        bool isUserSpaceOnUseMode = false;
        auto vp = viewport();
        Unit::Rect vpRect = Unit::Rect(0, 0, vp.width(), vp.height());

        if (matchingSvg->isSVGLinearGradientElement()) {
            SVGLinearGradientElement* gradientElement =
                matchingSvg->asSVGLinearGradientElement();

            if (gradientElement->gradientUnits()->baseVal() ==
                SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
                rect = vpRect;
                isUserSpaceOnUseMode = true;
            }

            double x1 = rect.x();
            if (gradientElement->x1()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                x1 +=
                    gradientElement->x1()->baseVal()->valueInSpecifiedUnits() *
                    rect.width() / 100;
            } else {
                x1 += gradientElement->x1()->baseVal()->value();
            }

            double y1 = rect.y();
            if (gradientElement->y1()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                y1 +=
                    gradientElement->y1()->baseVal()->valueInSpecifiedUnits() *
                    rect.height() / 100;
            } else {
                y1 += gradientElement->y1()->baseVal()->value();
            }

            double x2 = rect.x();
            if (gradientElement->x2()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                x2 +=
                    gradientElement->x2()->baseVal()->valueInSpecifiedUnits() *
                    rect.width() / 100;
            } else {
                x2 += gradientElement->x2()->baseVal()->value();
            }

            double y2 = rect.y();
            if (gradientElement->y2()->baseVal()->unitType() ==
                SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                y2 +=
                    gradientElement->y2()->baseVal()->valueInSpecifiedUnits() *
                    rect.height() / 100;
            } else {
                y2 += gradientElement->y2()->baseVal()->value();
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
            // TODO: RadialGradient works partially, 'fx', 'fy', 'fr' need to be
            // implemented.
            STARFISH_UNIMPLEMENTED();
            SVGRadialGradientElement* gradientElement =
                matchingSvg->asSVGRadialGradientElement();

            if (gradientElement->gradientUnits()->baseVal() ==
                SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
                rect = vpRect;
                isUserSpaceOnUseMode = true;
            }

            double cx = rect.x();
            if (gradientElement->cx()->baseVal()->hasSpecificValue()) {
                if (gradientElement->cx()->baseVal()->unitType() ==
                    SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                    cx += gradientElement->cx()
                              ->baseVal()
                              ->valueInSpecifiedUnits() *
                          rect.width() / 100;
                } else {
                    if (isUserSpaceOnUseMode) {
                        cx += gradientElement->cx()->baseVal()->value();
                    } else {
                        cx += gradientElement->cx()->baseVal()->value() *
                              rect.width();
                    }
                }
            } else {
                // Default value: 50%
                cx += 0.5 * rect.width();
            }

            double cy = rect.y();
            if (gradientElement->cy()->baseVal()->hasSpecificValue()) {
                if (gradientElement->cy()->baseVal()->unitType() ==
                    SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                    cy += gradientElement->cy()
                              ->baseVal()
                              ->valueInSpecifiedUnits() *
                          rect.height() / 100;
                } else {
                    if (isUserSpaceOnUseMode) {
                        cy += gradientElement->cy()->baseVal()->value();
                    } else {
                        cy += gradientElement->cy()->baseVal()->value() *
                              rect.height();
                    }
                }
            } else {
                // Default value: 50%
                cy += 0.5 * rect.height();
            }

            // Default value: 50%
            double r = 0.5;
            if (gradientElement->r()->baseVal()->hasSpecificValue()) {
                if (gradientElement->r()->baseVal()->unitType() ==
                    SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                    if (isUserSpaceOnUseMode) {
                        r = gradientElement->r()
                                ->baseVal()
                                ->valueInSpecifiedUnits() /
                            100 * normalizedDiagonalViewportLength().toFloat();
                    } else {
                        r = gradientElement->r()
                                ->baseVal()
                                ->valueInSpecifiedUnits() /
                            100;
                    }
                } else {
                    r = gradientElement->r()->baseVal()->value();
                }
            }

            // Default value: cx
            double fx = cx;
            if (gradientElement->fx()->baseVal()->hasSpecificValue()) {
                if (gradientElement->fx()->baseVal()->unitType() ==
                    SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                    fx += gradientElement->fx()
                              ->baseVal()
                              ->valueInSpecifiedUnits() *
                          rect.width() / 100;
                } else {
                    if (isUserSpaceOnUseMode) {
                        fx += gradientElement->fx()->baseVal()->value();
                    } else {
                        fx += gradientElement->fx()->baseVal()->value() *
                              rect.width();
                    }
                }
            } else {
                fx = cx;
            }

            // Default value: cy
            double fy = cy;
            if (gradientElement->fy()->baseVal()->hasSpecificValue()) {
                if (gradientElement->fy()->baseVal()->unitType() ==
                    SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                    fy += gradientElement->fy()
                              ->baseVal()
                              ->valueInSpecifiedUnits() *
                          rect.height() / 100;
                } else {
                    if (isUserSpaceOnUseMode) {
                        fy += gradientElement->fy()->baseVal()->value();
                    } else {
                        fy += gradientElement->fy()->baseVal()->value() *
                              rect.height();
                    }
                }
            } else {
                fy = cy;
            }

            // Default value: 0
            double fr = 0;
            if (gradientElement->fr()->baseVal()->hasSpecificValue()) {
                if (gradientElement->fr()->baseVal()->unitType() ==
                    SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                    if (isUserSpaceOnUseMode) {
                        fr = gradientElement->fr()
                                 ->baseVal()
                                 ->valueInSpecifiedUnits() /
                             100 * normalizedDiagonalViewportLength().toFloat();
                    } else {
                        fr = gradientElement->fr()
                                 ->baseVal()
                                 ->valueInSpecifiedUnits() /
                             100;
                    }
                } else {
                    fr = gradientElement->fr()->baseVal()->value();
                }
            }

            SVGTransformList* gradientTransform =
                gradientElement->gradientTransform()->baseVal();
            SkMatrix mat = SkMatrix::I();
            for (size_t i = 0; i < gradientTransform->length(); ++i) {
                mat = mat * gradientTransform->getItem(i)->matrix()->matrix();
            }

            if (isUserSpaceOnUseMode) {
                double xx1 = fx * mat[0] + fy * mat[1] + rect.width() * mat[2];
                double yy1 = fx * mat[3] + fy * mat[4] + rect.height() * mat[5];
                double xx2 = cx * mat[0] + cy * mat[1] + rect.width() * mat[2];
                double yy2 = cx * mat[3] + cy * mat[4] + rect.height() * mat[5];

                gradient = new CanvasGradient(matchingSvg->executionContext(),
                                              xx1, yy1, fr, xx2, yy2, r);

            } else {
                GradientData* gradientData = new RadialGradientData();
                gradientData->setHorizontalSide(SideValue::LeftSideValue);
                gradientData->setVerticalSide(SideValue::TopSideValue);

                RadialGradientData* radialGradient =
                    gradientData->asRadialGradientData();
                radialGradient->setHorizontalSideOffset(
                    Length(Length::Type::Fixed, cx));
                radialGradient->setVerticalSideOffset(
                    Length(Length::Type::Fixed, cy));
                radialGradient->setFirstRadius(
                    Length(Length::Type::Percent, r));
                radialGradient->setSecondRadius(
                    Length(Length::Type::Percent, r));
                radialGradient->colorStopList() = gradientElement->colorStops();

                double xx1 = rect.x() * mat[0] + rect.y() * mat[1] +
                             rect.width() * mat[2];
                double yy1 = rect.x() * mat[3] + rect.y() * mat[4] +
                             rect.height() * mat[5];

                Optional<GradientDrawingInfo*> gradientDrawingInfo =
                    radialGradient->makeGradientDrawingInfo(
                        Unit::Rect(xx1, yy1, rect.width(), rect.height()),
                        this);

                fx = gradientDrawingInfo.getValue()->x1;
                fy = gradientDrawingInfo.getValue()->y1;
                cx = gradientDrawingInfo.getValue()->x2;
                cy = gradientDrawingInfo.getValue()->y2;
                fr = gradientDrawingInfo.getValue()->r1;
                r = gradientDrawingInfo.getValue()->r2;

                gradient = new CanvasGradient(matchingSvg->executionContext(),
                                              fx, fy, fr, cx, cy, r);
                gradient->nativeGradient()->setRadialGradientScale(
                    gradientDrawingInfo.getValue()->firstRadius,
                    gradientDrawingInfo.getValue()->secondRadius);
            }

            const auto colorStops = gradientElement->colorStops();
            size_t size = colorStops.size();
            for (size_t i = 0; i < size; ++i) {
                gradient->addColorStop(colorStops[i]->offset().numberData(),
                                       colorStops[i]->color());
            }

        } else {
            STARFISH_UNSUPPORTED("SVG Gradient type");
            return nullptr;
        }

        auto canvasStyle = CanvasStyle::createCanvasGradient(gradient);
        return new CanvasFillStrokeSource(canvasStyle);
    }
    return nullptr;
}

void FrameSVGBox::paintSVG(PaintingContext& ctx)
{
    if (!node()->asSVGElement()->isShapeElement()) {
        return;
    }

    auto vp = viewport();

    bool fillHasUrl = style()->fill()->hasUrl();
    bool strokeHasUrl = style()->stroke()->hasUrl();
    Optional<CanvasFillStrokeSource*> fillInfo;
    Optional<CanvasFillStrokeSource*> strokeInfo;

    auto newPath = path();
    if (newPath) {
        Unit::Rect rect = newPath->boundingRect(true);
        if (fillHasUrl) {
            fillInfo = makeCanvasFillStrokeSource(style()->fill()->url(), rect);
            if (!fillInfo) {
                Optional<GradientDrawingInfo*> radialGradientInfo =
                    makeGradientDrawingInfo(style()->fill()->url(), rect);
                if (radialGradientInfo.hasValue()) {
                    ctx.m_canvas->save();
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

        if (strokeHasUrl) {
            // TODO: Only support linear gradient
            strokeInfo =
                makeCanvasFillStrokeSource(style()->stroke()->url(), rect);
        }

        ctx.m_canvas->save();
        if (fillInfo.hasValue()) {
            ctx.m_canvas->setFillSource(fillInfo.value());
        } else {
            Unit::Color fillColor = style()->fill()->color();
            ctx.m_canvas->setFillColor(
                Unit::Color(fillColor.r(), fillColor.g(), fillColor.b(),
                            fillColor.a() * style()->fillOpacity()));
            ctx.m_canvas->setFillRule(style()->fillRule());
        }

        if (strokeInfo.hasValue()) {
            ctx.m_canvas->setStrokeSource(strokeInfo.value());
        } else {
            Unit::Color strokeColor = style()->stroke()->color();
            ctx.m_canvas->setStrokeColor(
                Unit::Color(strokeColor.r(), strokeColor.g(), strokeColor.b(),
                            strokeColor.a() * style()->strokeOpacity()));
        }

        ctx.m_canvas->fillPath(newPath.value());
        ctx.m_canvas->setLineWidth(style()->strokeWidth().specifiedValue(
            normalizedDiagonalViewportLength(), this));
        ctx.m_canvas->strokePath(newPath.value());
        ctx.m_canvas->restore();
    }
}

} // namespace Starfish
