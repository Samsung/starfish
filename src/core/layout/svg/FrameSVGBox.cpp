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
#include "FrameSVGViewportContextBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/canvas/CanvasGradient.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/CalcData.h"
#include "core/style/GradientData.h"
#include "core/page/WebView.h"
#include "core/modules/canvas/NativeGradient.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
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

    float strokeWidth(style()->strokeWidth().specifiedValue(
        ctx.normalizedDiagonalViewportLength, this));

    if (node()->asSVGElement()->isShapeElement()) {
        auto p = path();
        if (p) {
            Unit::Rect boundingRect = p->strokeBoundingRect({
                strokeWidth,
                style()->strokeMiterLimit(),
                style()->strokeLineCap(),
                style()->strokeLineJoin(),
                style()->strokeDashArray(),
                style()->strokeDashOffset()
            });
            m_frameRect =
                LayoutRect(boundingRect.x(), boundingRect.y(),
                           boundingRect.width(), boundingRect.height());
        } else {
            m_frameRect = LayoutRect();
        }
    } else if (needsGeometryAttributes) {
        stylePos = resolveStylePosition(ctx.viewport);
        m_frameRect.setLocation(stylePos);
    }


    layoutSVG(ctx);

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
        if (!m_computedSVGTransform) {
            m_computedSVGTransform = new (GC_MALLOC_ATOMIC(sizeof(SkMatrix))) SkMatrix();
        }
        *m_computedSVGTransform = matrix;
    } else {
        m_computedSVGTransform = nullptr;
    }

    layoutChildren(ctx, matrix);

    if (node()->asSVGElement()->isStructuralElement()) {
        m_frameRect = LayoutRect();
        Frame* f = firstChild();
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

void FrameSVGBox::layoutChildren(SVGLayoutContext& ctx, SkMatrix matrix)
{
    Frame* f = firstChild();
    while (f) {
        if (f->isFrameSVGBox()) {
            f->asFrameSVGBox()->layout(ctx, matrix);
        } else {
            f->layout(ctx.layoutContext, Frame::LayoutWantToResolve::ResolveAll);

        }
        f = f->next();
    }
}

void FrameSVGBox::layout(LayoutContext& ctx,
                         Frame::LayoutWantToResolve resolveWhat)
{
    STARFISH_ASSERT_NOT_REACHED();
}

FrameSVGSVGBox* FrameSVGBox::outmostSVGViewportBox()
{
    Frame* f = this;
    while (true) {
        if (f->isFrameSVGSVGBox()) {
            return f->asFrameSVGSVGBox();
        }
        f = f->layoutParent();
    }
}

LayoutSize FrameSVGBox::viewport()
{
    Frame* f = this;
    while (true) {
        if (f->isFrameSVGSVGBox()) {
            return f->asFrameSVGSVGBox()->viewport();
        } else if (f != this && f->isFrameSVGViewportContextBox()) {
            return f->asFrameSVGViewportContextBox()->viewport();
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
        } else if (f != this && f->isFrameSVGViewportContextBox()) {
            return f->asFrameSVGViewportContextBox()->normalizedDiagonalViewportLength();
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

    if (!applyTransformTo(ctx.m_canvas, vp)) {
        // ignore invalid matrix
        ctx.m_canvas->restore();
        return;
    }

    float opacity = style()->opacity();
    if (opacity != 1) {
        auto svgFrame = outmostSVGViewportBox();
        LayoutRect absRect = absoluteRect(svgFrame);
        Unit::Rect rt(absRect.x(), absRect.y(), absRect.width(), absRect.height());
        auto ctm = ctx.m_canvas->currentTransformMatrix();
        ctx.m_canvas->setMatrix(svgFrame->svgPaintingMatrix());
        ctx.m_canvas->beginOpacityLayer(opacity, rt);
        ctx.m_canvas->setMatrix(ctm);
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
            maskFrame->asFrameSVGMaskBox()->applyMask(ctx, this);
        }
    }

    ctx.m_canvas->save();
    paintSVG(ctx);
    ctx.m_canvas->restore();

    if (!prepareChildPainting(ctx.m_canvas)) {
        return;
    }

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
            token.data(), token.length(), CSSPropertyParser::AllowNegative, &n)) { \
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

            GradientData* gradientData = new LinearGradientData();
            gradientData->colorStopList() = gradientElement->colorStops();
            gradient->nativeGradient()->setGradientDrawingInfo(
                    gradientData->makeGradientDrawingInfo(rect, this));

            const auto& colorStops = gradientElement->colorStops();
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
            double fx = rect.x();
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
            double fy = rect.y();
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

                RadialGradientData* radialGradient = new RadialGradientData();
                radialGradient->setHorizontalSide(SideValue::LeftSideValue);
                radialGradient->setVerticalSide(SideValue::TopSideValue);
                radialGradient->setHorizontalSideOffset(
                    Length(Length::Type::Fixed, cx));
                radialGradient->setVerticalSideOffset(
                    Length(Length::Type::Fixed, cy));
                radialGradient->setFirstRadius(
                    Length(Length::Type::Fixed, r));
                radialGradient->setSecondRadius(
                    Length(Length::Type::Fixed, r));
                radialGradient->colorStopList() = gradientElement->colorStops();

                Optional<GradientDrawingInfo*> gradientDrawingInfo =
                    radialGradient->makeGradientDrawingInfo(Unit::Rect(xx1, yy1,
                            std::abs(xx2 - xx1), std::abs(yy2 - yy1)), this);
                gradient->nativeGradient()->setGradientDrawingInfo(
                    gradientDrawingInfo.getValue());
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

                radialGradient->colorStopList() = gradientElement->colorStops();
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

                gradient->nativeGradient()->setGradientDrawingInfo(
                    gradientDrawingInfo.getValue());
            }

            const auto& colorStops = gradientElement->colorStops();
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

bool FrameSVGBox::applyTransformTo(Canvas* canvas, const LayoutSize& vp)
{
    if (style()->hasTransforms()) {
        auto matrix =
            style()->transformsToMatrix(vp.width(), vp.height(), this, true);
        if (!matrix.isIdentity()) {
            SkMatrix test;
            bool testResult = matrix.invert(&test);
            if (!testResult) {
                // invalid matrix to transform svg
                return false;
            }

            if (style()->hasTransformOrigin()) {
                auto to = style()->transformOrigin()->originValue();
                auto vp = viewport();
                auto ox = to->getXAxis().specifiedValue(vp.width(), this);
                auto oy = to->getYAxis().specifiedValue(vp.height(), this);
                canvas->translate(ox, oy);
                canvas->postMatrix(matrix);
                canvas->translate(-ox, -oy);
            } else {
                canvas->postMatrix(matrix);
            }
        }
    }
    return true;
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

    auto path = this->path();
    if (path) {
        auto strokeWidth = style()->strokeWidth().specifiedValue(
                    normalizedDiagonalViewportLength(), this);
        Path::StrokeStyle ss({
           strokeWidth,
           style()->strokeMiterLimit(),
           style()->strokeLineCap(),
           style()->strokeLineJoin(),
           style()->strokeDashArray(),
           style()->strokeDashOffset()
        });
        // fill and stroke need same boundingRect for cover this case
        // <path stroke="url(#linear0)" fill="url(#linear0)" ... />
        Unit::Rect rect = path->strokeBoundingRect(ss);

        if (fillHasUrl) {
            fillInfo = makeCanvasFillStrokeSource(style()->fill()->url(), rect);
        }

        if (strokeHasUrl) {
            // TODO: Only support linear gradient
            strokeInfo =
                makeCanvasFillStrokeSource(style()->stroke()->url(), rect);
        }

        ctx.m_canvas->save();

        float fillOpacity = style()->fillOpacity();
        FrameSVGSVGBox* viewportBox = outmostSVGViewportBox();
        bool paintingOnSVGViewport = viewportBox->svgMaskPaintingDepth() == 0;
        // fill
        if (fillInfo.hasValue() && paintingOnSVGViewport &&
            fillInfo.value()->isCanvasStyleType() &&
            fillInfo.value()->getCanvasStyleValue().isCanvasGradientValue()) {

            auto pixelSnappedRect = m_frameRect.snapSizeToPixel();

            NativeImageData* bufferImage;
            auto nativeGradient = fillInfo.value()->getCanvasStyleValue().getCanvasGradientValue()->nativeGradient();
            std::shared_ptr<NativeGradient> cachedNativeGradient =
                node()->document()->findInNativeGradientCache(nativeGradient->gradientDrawingInfo());

            auto transScale = viewportBox->computeTranlateScaleOnPaint();
            auto pos = absolutePoint(viewportBox);

            if (!cachedNativeGradient) {
                bufferImage = BufferedNativeImageData::create(
                        node()->webView()->screenInfo().devicePixelRatio,
                        pixelSnappedRect.width().toUnsigned(),
                        pixelSnappedRect.height().toUnsigned());
                Canvas* bufferCanvas = Canvas::create(node()->webView(), bufferImage);
                bufferCanvas->clearColor(Unit::Color(0, 0, 0, 0));

                std::vector<Frame*> tree;
                Frame* f = this;
                while (f != viewportBox) {
                    tree.push_back(f);
                    f = f->parent();
                }

                bufferCanvas->translate(-pos.x() + transScale.second.getTranslateX(),
                        -pos.y() + transScale.second.getTranslateY());
                bufferCanvas->scale(transScale.second.getScaleX(), transScale.second.getScaleY());
                for (auto iter = tree.rbegin(); iter != tree.rend(); iter++) {
                    Frame* f = *iter;
                    f->asFrameSVGBox()->applyTransformTo(bufferCanvas, vp);
                }

                bufferCanvas->setFillSource(fillInfo.value());
                bufferCanvas->rect(rect);
                bufferCanvas->fill();

                delete bufferCanvas;

                nativeGradient->setGradientImageDataCached(bufferImage);
                node()->document()->cacheNativeGradient(nativeGradient->gradientDrawingInfo(), nativeGradient);
            } else {
                bufferImage = cachedNativeGradient->gradientImageDataCached();
                STARFISH_ASSERT(bufferImage);
            }

            ctx.m_canvas->save();
            ctx.m_canvas->clipPath(path.value());
            ctx.m_canvas->setMatrix(viewportBox->svgPaintingMatrix());
            auto targetRect = Unit::Rect(pos.x(), pos.y(),
                    m_frameRect.width(), m_frameRect.height());
            if (fillOpacity != 1) {
                // we should use sqrt(fillOpacity) here
                // since drawImage below uses fillOpacity * fillOpacity for paint
                ctx.m_canvas->beginOpacityLayer(std::sqrt(fillOpacity), targetRect);
            }
            ctx.m_canvas->drawImage(bufferImage, targetRect);
            if (fillOpacity != 1) {
                ctx.m_canvas->endOpacityLayer();
            }
            ctx.m_canvas->restore();
        } else if (fillInfo.hasValue()) {
            if (fillOpacity != 1) {
                ctx.m_canvas->beginOpacityLayer(fillOpacity, rect);
            }
            ctx.m_canvas->referencePath(path.value());
            ctx.m_canvas->setFillSource(fillInfo.value());
            ctx.m_canvas->fill();
            if (fillOpacity != 1) {
                ctx.m_canvas->endOpacityLayer();
            }
        } else {
            Unit::Color fillColor = style()->fill()->color();
            fillColor.m_a = fillColor.a() * fillOpacity;
            if (!fillColor.isTransparent()) {
                ctx.m_canvas->referencePath(path.value());
                ctx.m_canvas->setFillRule(style()->fillRule());
                ctx.m_canvas->setFillColor(fillColor);
                ctx.m_canvas->fill();
            }
        }

        // stroke
        if (strokeWidth) {
            float strokeOpacity = style()->strokeOpacity();
            bool shouldUseOpacityLayer = strokeInfo.hasValue() && strokeOpacity != 1;
            if (shouldUseOpacityLayer) {
                ctx.m_canvas->beginOpacityLayer(strokeOpacity, rect);
            }
            ctx.m_canvas->setLineWidth(strokeWidth);
            ctx.m_canvas->setLineCap(ss.strokeLineCap);
            ctx.m_canvas->setLineJoin(ss.strokeLineJoin);
            ctx.m_canvas->setMiterLimit(ss.strokeMiterLimit);
            ctx.m_canvas->setDash(ss.strokeDashArray);
            ctx.m_canvas->setDashOffset(ss.strokeDashOffset);
            if (strokeInfo.hasValue()) {
                ctx.m_canvas->setStrokeSource(strokeInfo.value());
            } else {
                Unit::Color strokeColor = style()->stroke()->color();
                ctx.m_canvas->setStrokeColor(
                    Unit::Color(strokeColor.r(), strokeColor.g(), strokeColor.b(),
                                strokeColor.a() * style()->strokeOpacity()));
            }
            ctx.m_canvas->referencePath(path.value());
            ctx.m_canvas->stroke();
            if (shouldUseOpacityLayer) {
                ctx.m_canvas->endOpacityLayer();
            }
        }

        ctx.m_canvas->restore();
    }
}

} // namespace Starfish
