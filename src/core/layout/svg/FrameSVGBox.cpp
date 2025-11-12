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
#include "core/dom/svg/SVGSVGElement.h"
#include "core/dom/svg/SVGClipPathElement.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGMaskElement.h"
#include "core/dom/svg/SVGLinearGradientElement.h"
#include "core/dom/svg/SVGRadialGradientElement.h"
#include "core/dom/svg/SVGAnimatedTransformList.h"
#include "platform/loader/ResourceURL.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterPrimitive.h"
#include "core/dom/svg/SVGFilterElement.h"

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

Optional<LayoutUnit> FrameSVGBox::resolveStyleLength(
    const Length& length, const LayoutUnit& viewportLength)
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

    auto mt = motionTransformedPoint();
    if (UNLIKELY(mt)) {
        result.setX(mt.value().x());
        result.setY(mt.value().y());
    }

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

Optional<Unit::FloatPoint> FrameSVGBox::motionTransformedPoint()
{
    auto dx = node()->asSVGElement()->animatedLengthAttribute(
        node()->starfish()->staticStrings()->m_dx.localNameAtomic());
    auto dy = node()->asSVGElement()->animatedLengthAttribute(
        node()->starfish()->staticStrings()->m_dy.localNameAtomic());

    if (dx && dy) {
        return Unit::FloatPoint(dx.value().fixed(), dy.value().fixed());
    }
    return NullOption;
}

float FrameSVGBox::computeSVGLength(SVGLength* length, float fullValue,
                                    bool isObjectBoundingBoxMode)
{
    if (length->unitType() == SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
        return length->valueInSpecifiedUnits(false) / 100 * fullValue;
    } else if (length->unitType() == SVGLength::SVG_LENGTHTYPE_NUMBER) {
        if (isObjectBoundingBoxMode) {
            return length->valueInSpecifiedUnits(false) * fullValue;
        } else {
            return length->valueInSpecifiedUnits(false);
        }
    } else {
        return length->valueInSpecifiedUnits(false);
    }
}

static void adjustFrameRectByFilter(FrameSVGBox* self,
                                    SVGFilterElement* filterElement,
                                    const SkMatrix& matrix)
{
    Filter* fe = filterElement->filter();

    auto unadjustedFrameRectByFilter = *self->unadjustedFrameRectByFilter() =
        self->frameRect();

    auto vp = self->viewport();
    FrameSVGSVGBox* viewportBox = self->outmostSVGViewportBox();
    auto transScale = viewportBox->computeTranlateScaleOnPaint();

    auto eX = filterElement->x();
    auto eY = filterElement->y();
    auto eWidth = filterElement->width();
    auto eHeight = filterElement->height();

    bool isObjectBoundingBoxMode =
        filterElement->filterUnits()->animVal() ==
        SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX;

    float fullWidth = self->width();
    float fullHeight = self->height();
    if (!isObjectBoundingBoxMode) {
        fullWidth = vp.width();
        fullHeight = vp.height();
    }

    float x = FrameSVGBox::computeSVGLength(eX->animVal(), fullWidth,
                                            isObjectBoundingBoxMode);
    float y = FrameSVGBox::computeSVGLength(eY->animVal(), fullHeight,
                                            isObjectBoundingBoxMode);
    float width = FrameSVGBox::computeSVGLength(eWidth->animVal(), fullWidth,
                                                isObjectBoundingBoxMode);
    float height = FrameSVGBox::computeSVGLength(eHeight->animVal(), fullHeight,
                                                 isObjectBoundingBoxMode);

    LayoutRect newFrameRect = unadjustedFrameRectByFilter;
    if (isObjectBoundingBoxMode) {
        newFrameRect.setX(self->x() + x);
        newFrameRect.setY(self->y() + y);
        newFrameRect.setWidth(width);
        newFrameRect.setHeight(height);
    } else {
        newFrameRect.setX(x);
        newFrameRect.setY(y);
        newFrameRect.setWidth(width);
        newFrameRect.setHeight(height);
        newFrameRect = computeBoxExtent(newFrameRect, matrix);
    }

    auto bias =
        fe->computeBias(self, unadjustedFrameRectByFilter,
                        Unit::Rect(newFrameRect.x(), newFrameRect.y(),
                                   newFrameRect.width(), newFrameRect.height()),
                        std::make_pair(transScale.second.getScaleX(),
                                       transScale.second.getScaleY()));
    LayoutRect maximumBiasRect = unadjustedFrameRectByFilter;
    if (bias.maximumBias) {
        Unit::Rect rt = bias.maximumBias.value();
        maximumBiasRect = LayoutRect(rt.x(), rt.y(), rt.width(), rt.height());
    }

    LayoutRect result =
        LayoutRect::overlappedRect(maximumBiasRect, newFrameRect);
    if (bias.minimumBias) {
        Unit::Rect rt = bias.minimumBias.value();
        result.unite(LayoutRect(rt.x(), rt.y(), rt.width(), rt.height()));
    }

    const auto& primitives = fe->filterPrimitives();
    for (auto* primitive : primitives) {
        if (primitive->canSubRegionExpandFrameRect()) {
            auto subRegion = Filter::computeSubRegion(
                primitive, filterElement, self,
                std::make_pair(transScale.second.getScaleX(),
                               transScale.second.getScaleY()));
            LayoutRect subRegionRect(
                unadjustedFrameRectByFilter.x() +
                    (unadjustedFrameRectByFilter.width()) * subRegion.x(),
                unadjustedFrameRectByFilter.y() +
                    (unadjustedFrameRectByFilter.height()) * subRegion.y(),
                (unadjustedFrameRectByFilter.width()) * subRegion.width(),
                (unadjustedFrameRectByFilter.height()) * subRegion.height());
            subRegionRect =
                LayoutRect::overlappedRect(subRegionRect, newFrameRect);
            result.unite(subRegionRect);
        }
    }

    self->setFrameRect(result);
}

static float resolveBoundingBoxUnitSVGLength(SVGLength* l, float parentLength)
{
    if (l->unitType() == SVGLength::UnitType::SVG_LENGTHTYPE_NUMBER) {
        return l->valueInSpecifiedUnits(false);
    } else if (l->unitType() ==
               SVGLength::UnitType::SVG_LENGTHTYPE_PERCENTAGE) {
        return l->valueInSpecifiedUnits(false) / 100;
    } else {
        return l->value() / parentLength;
    }
}

static float resolveUserspaceUnitSVGLength(SVGLength* l, float viewportScale,
                                           const LayoutUnit& viewportLength)
{
    if (l->unitType() == SVGLength::UnitType::SVG_LENGTHTYPE_NUMBER) {
        return l->valueInSpecifiedUnits(false) * viewportScale;
    } else if (l->unitType() ==
               SVGLength::UnitType::SVG_LENGTHTYPE_PERCENTAGE) {
        float p = l->valueInSpecifiedUnits(false) / 100;
        p *= viewportLength;
        return p;
    } else {
        return l->value(false);
    }
}

static Unit::Rect getMaskRegionScale(FrameSVGBox::SVGLayoutContext& ctx,
                                     SVGMaskElement* maskElement,
                                     FrameSVGBox* targetBox,
                                     SkMatrix transScale)
{
    auto x = maskElement->x()->animVal();
    auto y = maskElement->y()->animVal();
    auto width = maskElement->width()->animVal();
    auto height = maskElement->height()->animVal();

    bool isObjectBoundingBox = maskElement->maskUnits()->animVal() ==
                               SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX;

    FrameSVGSVGBox* viewportBox = targetBox->outmostSVGViewportBox();

    auto unAdjustedFrameRect = targetBox->frameRect();
    Unit::Rect maskRegionInFloat(0, 0, 1, 1);
    if (isObjectBoundingBox) {
        maskRegionInFloat.setX(resolveBoundingBoxUnitSVGLength(
            x, unAdjustedFrameRect.width().toFloat()));
        maskRegionInFloat.setY(resolveBoundingBoxUnitSVGLength(
            y, unAdjustedFrameRect.height().toFloat()));
        maskRegionInFloat.setWidth(resolveBoundingBoxUnitSVGLength(
            width, unAdjustedFrameRect.width().toFloat()));
        maskRegionInFloat.setHeight(resolveBoundingBoxUnitSVGLength(
            height, unAdjustedFrameRect.height().toFloat()));
    } else {
        auto vp = targetBox->viewport();
        LayoutRect viewportRect(0, 0, transScale.getScaleX() * vp.width(),
                                transScale.getScaleY() * vp.height());
        LayoutRect absoluteRect(
            resolveUserspaceUnitSVGLength(x, transScale.getScaleX(),
                                          vp.width()),
            resolveUserspaceUnitSVGLength(y, transScale.getScaleY(),
                                          vp.height()),
            resolveUserspaceUnitSVGLength(width, transScale.getScaleX(),
                                          vp.width()),
            resolveUserspaceUnitSVGLength(height, transScale.getScaleY(),
                                          vp.height()));

        if (absoluteRect.containsInVisual(viewportRect)) {
            maskRegionInFloat = Unit::Rect(0, 0, 1, 1);
        } else {
            LayoutRect rt = targetBox->parent()->asFrameBox()->absoluteRect(
                targetBox->outmostSVGViewportBox());
            rt.setX(rt.x() + unAdjustedFrameRect.x());
            rt.setY(rt.y() + unAdjustedFrameRect.y());
            rt.setWidth(unAdjustedFrameRect.width());
            rt.setHeight(unAdjustedFrameRect.height());
            LayoutRect ort = LayoutRect::overlappedRect(absoluteRect, rt);
            maskRegionInFloat = Unit::Rect((ort.x() - rt.x()) / rt.width(),
                                           (ort.y() - rt.y()) / rt.height(),
                                           ort.width() / rt.width(),
                                           ort.height() / rt.height());
        }
    }
    return maskRegionInFloat;
}

LayoutRect getMaskRect(FrameSVGBox::SVGLayoutContext& ctx,
                       LayoutRect targetMaskRect, FrameSVGBox* targetBox,
                       SVGMaskElement* maskElement)
{
    auto transScale =
        targetBox->outmostSVGViewportBox()->computeTranlateScaleOnPaint();

    Unit::Rect maskRegionInFloat =
        getMaskRegionScale(ctx, maskElement, targetBox, transScale.second);
    targetMaskRect.setX(targetMaskRect.x() +
                        targetMaskRect.width() * maskRegionInFloat.x());
    targetMaskRect.setY(targetMaskRect.y() +
                        targetMaskRect.height() * maskRegionInFloat.y());
    targetMaskRect.setWidth(targetMaskRect.width() * maskRegionInFloat.width());
    targetMaskRect.setHeight(targetMaskRect.height() *
                             maskRegionInFloat.height());
    return targetMaskRect;
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
    bool isStructuralElement = node()->asSVGElement()->isStructuralElement();

    float strokeWidth(style()->strokeWidth().specifiedValue(
        ctx.normalizedDiagonalViewportLength, this));

    if (node()->asSVGElement()->isShapeElement()) {
        auto p = motionTransformedPath();
        if (p) {
            Unit::Rect boundingRect = p->strokeBoundingRect({
                strokeWidth,
                style()->strokeMiterLimit(),
                style()->strokeLineCap(),
                style()->strokeLineJoin(),
                style()->strokeDasharray(),
                style()->strokeDashoffset(),
            });
            m_frameRect =
                LayoutRect(boundingRect.x(), boundingRect.y(),
                           boundingRect.width(), boundingRect.height());

            auto fillPathRect = p->boundingRect();
            if (!fillPathRect.isEmpty()) {
                ctx.m_fillRects.insert(std::make_pair(
                    this,
                    LayoutRect(fillPathRect.x(), fillPathRect.y(),
                               fillPathRect.width(), fillPathRect.height())));
            }

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
                matrix.preTranslate(ox, oy);
                matrix.preConcat(styleMatrix);
                matrix.preTranslate(-ox, -oy);
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

    auto clipPathElement = node()->asSVGElement()->clipPathElement();
    Optional<LayoutRect> clipRect;
    if (clipPathElement) {
        Frame* clipPathFrame = clipPathElement->frame();
        if (clipPathFrame) {
            auto clipPath = clipPathFrame->asFrameSVGClipPathBox()->path();
            if (clipPath) {
                auto floatClipRect = clipPath->fillBoundingRect();
                clipRect = computeBoxExtent(
                    LayoutRect(floatClipRect.x(), floatClipRect.y(),
                               floatClipRect.width(), floatClipRect.height()),
                    matrix);
                ctx.clippedRects.push_back(clipRect.value());
            }
        }
    }

    auto maskElement = node()->asSVGElement()->maskElement();
    bool isDecendentOfInvisibleFrame = false;

    Optional<LayoutRect> maskRect;
    if (maskElement) {
        Frame* maskFrame = maskElement->frame();
        if (maskFrame) {
            LayoutRect targetMaskRect;
            auto iter = ctx.m_fillRects.find(this);
            if (iter == ctx.m_fillRects.end()) {
                targetMaskRect = m_frameRect;
            } else {
                targetMaskRect = iter->second;
            }
            targetMaskRect = computeBoxExtent(targetMaskRect, matrix);

            // only invisible mask content can be used by this case
            for (Frame* f = maskFrame->parent();
                 !f->isFrameSVGSVGBox() && !f->isFrameSVGViewportContextBox();
                 f = f->parent()) {
                if (f->isFrameSVGInvisibleBox()) {
                    isDecendentOfInvisibleFrame = true;
                    break;
                }
            }

            if (isDecendentOfInvisibleFrame) {
                maskFrame->asFrameSVGBox()->layout(ctx, matrix);
                if (!node()->isSVGGElement()) {
                    LayoutRect mrect =
                        getMaskRect(ctx, targetMaskRect, asFrameSVGBox(),
                                    maskElement.getValue());
                    LayoutRect rect;
                    Frame* f = maskFrame->firstChild();
                    while (f) {
                        auto childRect = f->asFrameBox()->frameRect();
                        rect.unite(childRect);
                        f = f->next();
                    }
                    rect = LayoutRect::overlappedRect(rect, mrect);
                    maskRect = rect;
                    ctx.clippedRects.push_back(rect);
                }
            }
        }
    }

    if (!matrix.isIdentity() && needsComputeFrameRect) {
        m_frameRect = computeBoxExtent(m_frameRect, matrix);

        if (!m_computedSVGTransform) {
            m_computedSVGTransform =
                new (GC_MALLOC_ATOMIC(sizeof(SkMatrix))) SkMatrix();
        }
        *m_computedSVGTransform = matrix;
    } else {
        m_computedSVGTransform = nullptr;
    }

    auto filterElement = node()->asSVGElement()->filterElement();
    if (!filterElement) {
        m_unadjustedFrameRectByFilter = nullptr;
    } else {
        if (!m_unadjustedFrameRectByFilter) {
            m_unadjustedFrameRectByFilter =
                new (GC_MALLOC_ATOMIC(sizeof(LayoutRect))) LayoutRect();
        }
    }

    if (needsComputeFrameRect && !isStructuralElement) {
        if (filterElement) {
            adjustFrameRectByFilter(this, filterElement.value(), matrix);
        }
        for (auto rt : ctx.clippedRects) {
            m_frameRect = LayoutRect::overlappedRect(m_frameRect, rt);
        }
    }

    layoutChildren(ctx, matrix);

    if (isStructuralElement) {
        m_frameRect = LayoutRect();
        Frame* f = firstChild();
        while (f) {
            m_frameRect.unite(f->asFrameBox()->frameRect());
            f = f->next();
        }

        if (filterElement) {
            adjustFrameRectByFilter(this, filterElement.value(), matrix);
        }

        f = firstChild();
        while (f) {
            LayoutRect childRect = f->asFrameBox()->frameRect();
            f->asFrameBox()->setX(childRect.x() - m_frameRect.x());
            f->asFrameBox()->setY(childRect.y() - m_frameRect.y());
            f = f->next();
        }
    }

    if (maskElement) {
        Frame* maskFrame = maskElement->frame();
        if (maskFrame && node()->isSVGGElement()) {
            LayoutRect targetMaskRect = asFrameSVGBox()->boundingRect();

            // only invisible mask content can be used by this case
            if (isDecendentOfInvisibleFrame) {
                maskFrame->asFrameSVGBox()->layout(ctx, matrix);
                LayoutRect mrect =
                    getMaskRect(ctx, targetMaskRect, asFrameSVGBox(),
                                maskElement.getValue());
                LayoutRect rect;
                Frame* f = maskFrame->firstChild();
                while (f) {
                    rect.unite(f->asFrameBox()->frameRect());
                    f = f->next();
                }
                rect = LayoutRect::overlappedRect(rect, mrect);

                float oldFrameRectX = (float)m_frameRect.x();
                float oldFrameRectY = (float)m_frameRect.y();
                m_frameRect = LayoutRect::overlappedRect(m_frameRect, rect);
                f = firstChild();
                while (f) {
                    LayoutRect childRect = f->asFrameBox()->frameRect();
                    f->asFrameBox()->moveX(oldFrameRectX - m_frameRect.x());
                    f->asFrameBox()->moveY(oldFrameRectY - m_frameRect.y());
                    f = f->next();
                }
            }
        }
    }

    postLayoutSVG(ctx);

    if (maskRect) {
        ctx.clippedRects.pop_back();
    }

    if (clipRect) {
        ctx.clippedRects.pop_back();
    }
}

void FrameSVGBox::layoutChildren(SVGLayoutContext& ctx, SkMatrix matrix)
{
    Frame* f = firstChild();
    while (f) {
        if (f->isFrameSVGBox()) {
            f->asFrameSVGBox()->layout(ctx, matrix);
        } else {
            f->layout(ctx.layoutContext,
                      Frame::LayoutWantToResolve::ResolveAll);
        }
        f = f->next();
    }
}

Optional<Path*> FrameSVGBox::motionTransformedPath()
{
    auto p = path();
    if (p) {
        auto mt = motionTransformedPoint();
        if (UNLIKELY(mt)) {
            p->translate(mt.value().x(), mt.value().y());
        }
    }
    return p;
}

LayoutRect FrameSVGBox::boundingRect()
{
    LayoutRect result;
    Frame* child = firstChild();
    while (child) {
        if (child) {
            if (child->isFrameSVGBox()) {
                FrameSVGBox* childBox = child->asFrameSVGBox();
                auto childRect = childBox->boundingRect();
                if (childBox->computedSVGTransform().hasValue()) {
                    auto svgMatrix =
                        *childBox->computedSVGTransform().getValue();
                    childRect = computeBoxExtent(childRect, svgMatrix);
                }
                result.unite(childRect);
            }
        }
        child = child->next();
    }
    return result;
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
            return f->asFrameSVGViewportContextBox()
                ->normalizedDiagonalViewportLength();
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

    if (style()->mixBlendMode() != BlendMode::Normal) {
        ctx.m_canvas->setCompositeOperator(CanvasCompositeOperator::SourceOver,
                                           style()->mixBlendMode());
    }

    node()->document()->removeSVGPaintClientElement(node()->asSVGElement());

    auto vp = viewport();
    bool needsGeometryAttributes = needsSVGGeometryAttributes();

    if (!applyTransformTo(ctx.m_canvas, vp)) {
        // ignore invalid matrix
        ctx.m_canvas->restore();
        return;
    }

    float opacity = style()->opacity();
    auto filterElement = node()->asSVGElement()->filterElement();
    bool needsCanvasLayer = opacity != 1 || filterElement;
    if (needsCanvasLayer) {
        FrameSVGSVGBox* viewportBox = outmostSVGViewportBox();
        LayoutRect absRect = viewportBox->computeCanvasLayerRect(this);
        Unit::Rect rt(absRect.x(), absRect.y(), absRect.width(),
                      absRect.height());
        auto ctm = ctx.m_canvas->currentTransformMatrix();
        ctx.m_canvas->setMatrix(viewportBox->svgPaintingMatrix());
        ctx.m_canvas->beginLayer(rt, opacity, CanvasLayerMode::SubLayer);
        ctx.m_canvas->setMatrix(ctm);
    }

    auto clipPathElement = node()->asSVGElement()->clipPathElement();
    if (clipPathElement) {
        Frame* clipPathFrame = clipPathElement->frame();
        if (clipPathFrame) {
            auto clipPath = clipPathFrame->asFrameSVGClipPathBox()->path();
            if (clipPath) {
                ctx.m_canvas->clipPath(clipPath.value());
            }
        }
    }

    auto maskElement = node()->asSVGElement()->maskElement();
    if (maskElement) {
        Frame* maskFrame = maskElement->frame();
        if (maskFrame) {
            maskFrame->asFrameSVGMaskBox()->applyMask(ctx, this);
        }
    }

    paintSVG(ctx);

    if (!prepareChildPainting(ctx.m_canvas)) {
        return;
    }

    Frame* child = firstChild();
    while (child) {
        child->asFrameBox()->paintContent(ctx);
        child = child->next();
    }

    if (needsCanvasLayer) {
        Canvas::LayerPixelModifyFunction fn;
        if (filterElement) {
            fn = [this, &ctx, filterElement](uint8_t* ptr, size_t w, size_t s,
                                             size_t h) -> void {
                Filter* filter = filterElement->asSVGFilterElement()->filter();
                FrameSVGSVGBox* viewportBox = outmostSVGViewportBox();
                auto transScale = viewportBox->computeTranlateScaleOnPaint();
                Filter::FilterApplyContext ctx(
                    this, w, s, h, ptr, transScale.second.getScaleX(),
                    transScale.second.getScaleY(), false);

                filter->applyFilter(ctx);

                // copy if needs
                if (ctx.output->data() != ptr) {
                    STARFISH_ASSERT(ctx.output->size() == s * h);
                    memcpy(ptr, ctx.output->data(), s * h);
                }
            };
        }
        ctx.m_canvas->endLayer(fn);
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

#define READ_NUMBER(n)                                                    \
    if (!CSSPropertyParser::parseNumber(token.data(), token.length(),     \
                                        CSSPropertyParser::AllowNegative, \
                                        &n)) {                            \
        break;                                                            \
    }                                                                     \
    if (gotMinus) {                                                       \
        n = -n;                                                           \
    }                                                                     \
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

namespace {

    struct SVGCoordinate {
        double value;
        bool isPercent;
    };

    SVGCoordinate extractCoordinate(SVGLength* length, double defaultValue)
    {
        if (length->hasSpecificValue()) {
            if (length->unitType() == SVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
                return { length->valueInSpecifiedUnits() / 100, true };
            }
            return { length->value(), false };
        }
        return { defaultValue, false };
    }

    CanvasGradient* createLinearGradient(
        FrameSVGBox* self, SVGLinearGradientElement* gradientElement,
        const Unit::Rect& rect, bool isUserSpaceOnUseMode, const SkMatrix& mat)
    {
        auto x1Coord = extractCoordinate(gradientElement->x1()->animVal(), 0);
        auto y1Coord = extractCoordinate(gradientElement->y1()->animVal(), 1);
        auto x2Coord = extractCoordinate(gradientElement->x2()->animVal(), 0);
        auto y2Coord = extractCoordinate(gradientElement->y2()->animVal(), 0);

        double x1 = x1Coord.value;
        double y1 = y1Coord.value;
        double x2 = x2Coord.value;
        double y2 = y2Coord.value;

        bool hasPercentValue = x1Coord.isPercent || y1Coord.isPercent ||
                               x2Coord.isPercent || y2Coord.isPercent;

        CanvasGradient* gradient = nullptr;
        if (isUserSpaceOnUseMode && !hasPercentValue) {
            double xx1 = x1 * mat[0] + y1 * mat[1] + rect.width() * mat[2];
            double yy1 = x1 * mat[3] + y1 * mat[4] + rect.height() * mat[5];
            double xx2 = x2 * mat[0] + y2 * mat[1] + rect.width() * mat[2];
            double yy2 = x2 * mat[3] + y2 * mat[4] + rect.height() * mat[5];

            gradient = new CanvasGradient(gradientElement->executionContext(),
                                          xx1, yy1, xx2, yy2);
            GradientData* gradientData = new LinearGradientData();
            gradientData->colorStopList() = gradientElement->colorStops();
            gradient->nativeGradient()->setGradientDrawingInfo(
                gradientData->makeGradientDrawingInfo(rect, self));
        } else {
            SkMatrix objectBoundingMatrix = SkMatrix::I();
            objectBoundingMatrix.preTranslate(rect.x(), rect.y());
            objectBoundingMatrix.preScale(rect.width(), rect.height());
            objectBoundingMatrix.preConcat(mat);

            GradientData* gradientData = new LinearGradientData();
            gradientData->colorStopList() = gradientElement->colorStops();

            GradientDrawingInfo* gradientDrawinginfo =
                gradientData->makeGradientDrawingInfo(rect, self);

            gradientDrawinginfo->x1 = x1;
            gradientDrawinginfo->y1 = y1;
            gradientDrawinginfo->x2 = x2;
            gradientDrawinginfo->y2 = y2;
            gradientDrawinginfo->matrix = objectBoundingMatrix;

            gradient = new CanvasGradient(gradientElement->executionContext(),
                                          gradientDrawinginfo);
            gradient->nativeGradient()->setGradientDrawingInfo(
                gradientDrawinginfo);
        }
        return gradient;
    }

    CanvasGradient* createRadialGradient(
        FrameSVGBox* self, SVGRadialGradientElement* gradientElement,
        const Unit::Rect& rect, bool isUserSpaceOnUseMode, const SkMatrix& mat)
    {
        double cx =
            extractCoordinate(gradientElement->cx()->animVal(), 0.5).value;
        double cy =
            extractCoordinate(gradientElement->cy()->animVal(), 0.5).value;
        double r =
            extractCoordinate(gradientElement->r()->animVal(), 0.5).value;
        double fx =
            gradientElement->fx()->animVal()->hasSpecificValue()
                ? extractCoordinate(gradientElement->fx()->animVal(), 0.0).value
                : cx;
        double fy =
            gradientElement->fy()->animVal()->hasSpecificValue()
                ? extractCoordinate(gradientElement->fy()->animVal(), 0.0).value
                : cy;
        double fr =
            gradientElement->fr()->animVal()->hasSpecificValue()
                ? extractCoordinate(gradientElement->fr()->animVal(), 0.0).value
                : 0.0;

        CanvasGradient* gradient = nullptr;
        if (isUserSpaceOnUseMode) {
            double xx1 = fx * mat[0] + fy * mat[1] + rect.width() * mat[2];
            double yy1 = fx * mat[3] + fy * mat[4] + rect.height() * mat[5];
            double xx2 = cx * mat[0] + cy * mat[1] + rect.width() * mat[2];
            double yy2 = cx * mat[3] + cy * mat[4] + rect.height() * mat[5];

            gradient = new CanvasGradient(gradientElement->executionContext(),
                                          xx1, yy1, fr, xx2, yy2, r);

            RadialGradientData* radialGradient = new RadialGradientData();
            radialGradient->setHorizontalSide(SideValue::LeftSideValue);
            radialGradient->setVerticalSide(SideValue::TopSideValue);
            radialGradient->setHorizontalSideOffset(
                Length(Length::Type::Fixed, cx));
            radialGradient->setVerticalSideOffset(
                Length(Length::Type::Fixed, cy));
            radialGradient->setFirstRadius(Length(Length::Type::Fixed, r));
            radialGradient->setSecondRadius(Length(Length::Type::Fixed, r));
            radialGradient->colorStopList() = gradientElement->colorStops();

            Optional<GradientDrawingInfo*> gradientDrawingInfo =
                radialGradient->makeGradientDrawingInfo(
                    Unit::Rect(xx1, yy1, std::abs(xx2 - xx1),
                               std::abs(yy2 - yy1)),
                    self);
            gradient->nativeGradient()->setGradientDrawingInfo(
                gradientDrawingInfo.getValue());
        } else {
            SkMatrix objectBoundingMatrix = SkMatrix::I();
            objectBoundingMatrix.preTranslate(rect.x(), rect.y());
            objectBoundingMatrix.preScale(rect.width(), rect.height());
            objectBoundingMatrix.preConcat(mat);

            GradientData* gradientData = new RadialGradientData();
            RadialGradientData* radialGradient =
                gradientData->asRadialGradientData();
            radialGradient->colorStopList() = gradientElement->colorStops();

            Optional<GradientDrawingInfo*> gradientDrawingInfo =
                radialGradient->makeGradientDrawingInfo(Unit::Rect(0, 0, 0, 0),
                                                        self);
            gradientDrawingInfo->x1 = fx;
            gradientDrawingInfo->y1 = fy;
            gradientDrawingInfo->x2 = cx;
            gradientDrawingInfo->y2 = cy;
            gradientDrawingInfo->r1 = fr;
            gradientDrawingInfo->r2 = r;
            gradientDrawingInfo->matrix = objectBoundingMatrix;

            gradient = new CanvasGradient(gradientElement->executionContext(),
                                          gradientDrawingInfo.getValue());
            gradient->nativeGradient()->setGradientDrawingInfo(
                gradientDrawingInfo.getValue());
        }
        return gradient;
    }

} // anonymous namespace

Optional<CanvasFillStrokeSource*> FrameSVGBox::makeCanvasFillStrokeSource(
    const AtomicString& id, const Unit::Rect& svgRect)
{
    Unit::Rect rect = svgRect;

    auto owner = node()->asSVGElement()->ownerSVGElement();
    STARFISH_ASSERT(owner);

    document()->registerSVGPaintClientElements(id, node()->asSVGElement());

    auto matchingSvg = owner->getSVGElementById(id);
    if (!matchingSvg || !matchingSvg->isSVGGradientElement()) {
        return nullptr;
    }

    SVGGradientElement* gradientElement = matchingSvg->asSVGGradientElement();

    CanvasGradient* canvasGradient = nullptr;
    bool isUserSpaceOnUseMode = false;
    auto vp = viewport();
    Unit::Rect vpRect = Unit::Rect(0, 0, vp.width(), vp.height());

    if (gradientElement->gradientUnits()->animVal() ==
        SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
        rect = vpRect;
        isUserSpaceOnUseMode = true;
    }

    // Handle gradient transform
    SVGTransformList* gradientTransform =
        gradientElement->gradientTransform()->animVal();
    SkMatrix mat = SkMatrix::I();
    for (size_t i = 0; i < gradientTransform->length(); ++i) {
        mat = mat * gradientTransform->getItem(i)->matrix()->matrix();
    }

    if (matchingSvg->isSVGLinearGradientElement()) {
        auto linearElement = matchingSvg->asSVGLinearGradientElement();
        canvasGradient = createLinearGradient(this, linearElement, rect,
                                              isUserSpaceOnUseMode, mat);
        // Add color stops
        const auto& colorStops = linearElement->colorStops();
        for (const auto& stop : colorStops) {
            canvasGradient->addColorStop(stop->offset().numberData(),
                                         stop->color());
        }
    } else if (matchingSvg->isSVGRadialGradientElement()) {
        auto radialElement = matchingSvg->asSVGRadialGradientElement();
        canvasGradient = createRadialGradient(this, radialElement, rect,
                                              isUserSpaceOnUseMode, mat);
        // Add color stops
        const auto& colorStops = radialElement->colorStops();
        for (const auto& stop : colorStops) {
            canvasGradient->addColorStop(stop->offset().numberData(),
                                         stop->color());
        }
    } else {
        STARFISH_UNSUPPORTED("SVG Gradient type");
        return nullptr;
    }

    if (canvasGradient) {
        auto canvasStyle = CanvasStyle::createCanvasGradient(canvasGradient);
        if (!canvasStyle.isNoneValue()) {
            return new CanvasFillStrokeSource(canvasStyle);
        }
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

    ComputedStyle* cs = style();
    Optional<CanvasFillStrokeSource*> fillInfo;
    Optional<CanvasFillStrokeSource*> strokeInfo;

    auto path = this->motionTransformedPath();
    if (path) {
        auto strokeWidth = cs->strokeWidth().specifiedValue(
            normalizedDiagonalViewportLength(), this);
        Path::StrokeStyle ss({
            strokeWidth,
            cs->strokeMiterLimit(),
            cs->strokeLineCap(),
            cs->strokeLineJoin(),
            cs->strokeDasharray(),
            cs->strokeDashoffset(),
        });
        // fill and stroke need same boundingRect for cover this case
        // <path stroke="url(#linear0)" fill="url(#linear0)" ... />
        Unit::Rect rect = path->strokeBoundingRect(ss);

        if (cs->hasFillPaintData() && cs->fill()->hasId()) {
            fillInfo = makeCanvasFillStrokeSource(cs->fill()->id(), rect);
        }

        if (cs->hasStrokePaintData() && cs->stroke()->hasId()) {
            strokeInfo = makeCanvasFillStrokeSource(cs->stroke()->id(), rect);
        }

        float fillOpacity = cs->fillOpacity();
        float strokeOpacity = cs->strokeOpacity();

        bool shouldPaintFill =
            (fillOpacity != 0) &&
            (fillInfo.hasValue() ||
             (cs->hasFillPaintData() && !cs->fill()->color().isTransparent()));
        bool shouldPaintStroke =
            strokeWidth && (strokeOpacity != 0) &&
            (strokeInfo.hasValue() || (cs->hasStrokePaintData() &&
                                       !cs->stroke()->color().isTransparent()));

        if (shouldPaintFill || shouldPaintStroke) {
            ctx.m_canvas->save();

            if (shouldPaintFill) {
                if (fillOpacity != 1 && fillInfo.hasValue()) {
                    ctx.m_canvas->beginOpacityLayer(fillOpacity, rect);
                }
                if (fillInfo.hasValue()) {
                    ctx.m_canvas->setFillSource(fillInfo.value());
                } else {
                    Unit::Color fillColor = cs->fill()->color();
                    fillColor.m_a = fillColor.a() * fillOpacity;
                    STARFISH_ASSERT(!fillColor.isTransparent());
                    ctx.m_canvas->setFillColor(fillColor);
                }

                auto rule = cs->fillRule();
                if (rule == FillRuleValue::FillRuleNonZero) {
                    ctx.m_canvas->setFillRule(true);
                } else {
                    STARFISH_ASSERT(rule == FillRuleValue::FillRuleEvenOdd);
                    ctx.m_canvas->setFillRule(false);
                }

                ctx.m_canvas->referencePath(path.value());

                if (shouldPaintStroke) {
                    ctx.m_canvas->fillPreserve();
                } else {
                    ctx.m_canvas->fill();
                }

                if (fillOpacity != 1 && fillInfo.hasValue()) {
                    ctx.m_canvas->endOpacityLayer();
                }
            }

            // paint stroke
            if (shouldPaintStroke) {
                if (!shouldPaintFill) {
                    ctx.m_canvas->referencePath(path.value());
                }

                bool shouldUseOpacityLayer =
                    strokeInfo.hasValue() && strokeOpacity != 1;
                if (shouldUseOpacityLayer) {
                    ctx.m_canvas->beginOpacityLayer(strokeOpacity, rect);
                }

                ctx.m_canvas->setLineWidth(strokeWidth);
                ctx.m_canvas->setLineCap(ss.strokeLineCap);
                ctx.m_canvas->setLineJoin(ss.strokeLineJoin);
                ctx.m_canvas->setMiterLimit(ss.strokeMiterLimit);
                ctx.m_canvas->setDash(ss.strokeDasharray);
                ctx.m_canvas->setDashOffset(ss.strokeDashoffset);
                if (strokeInfo.hasValue()) {
                    if (shouldPaintFill) {
                        // NOTE we need to referencePath again
                        // it is limitation of cairo
                        ctx.m_canvas->referencePath(path.value());
                    }
                    ctx.m_canvas->setStrokeSource(strokeInfo.value());
                } else {
                    Unit::Color strokeColor = cs->stroke()->color();
                    STARFISH_ASSERT(!strokeColor.isTransparent());
                    ctx.m_canvas->setStrokeColor(Unit::Color(
                        strokeColor.r(), strokeColor.g(), strokeColor.b(),
                        strokeColor.a() * strokeOpacity));
                }
                ctx.m_canvas->stroke();

                if (shouldUseOpacityLayer) {
                    ctx.m_canvas->endOpacityLayer();
                }
            }

            ctx.m_canvas->restore();
        }
    }
}

} // namespace Starfish
