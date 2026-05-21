/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "core/layout/StackingContext.h"

#include "Starfish.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/Scrolling.h"
#include "core/dom/svg/SVGElement.h"
#include "core/animation/AnimationTask.h"
#include "core/animation/AnimationExecutor.h"
#include "core/style/FilterFunctions.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/ComputeOverflow.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/GradientData.h"
#include "core/modules/canvas/NativeGradient.h"
#include "platform/loader/ImageResource.h"

namespace Starfish {

inline void computeBufferSizeFromVisibleRect(LayoutUnit minX, LayoutUnit minY,
                                             LayoutUnit maxX, LayoutUnit maxY,
                                             size_t& bufferWidth,
                                             size_t& bufferHeight)
{
#if defined(STARFISH_ENABLE_TEST)
    bufferWidth = (int)(maxX - minX);
    bufferHeight = (int)(maxY - minY);
#else
    bufferWidth = (maxX - minX).round();
    bufferHeight = (maxY - minY).round();
#endif
}

struct StackingContext::ComputeStackingContextContext {
    bool needsToAllocateGraphicsBufferForFixedElement;
    bool seenPositionFixed;
    std::unordered_map<StackingContext*, LayoutRect> extentPerLayer;
    std::unordered_map<StackingContext*, LayoutRect> clippedExtentPerLayer;
    std::unordered_map<StackingContext*, bool> compositeFlagInfo;
    std::unordered_map<StackingContext*, bool> compositeFlagInfoBecauseSelf;
    std::vector<StackingContext*> compositedLayers;
    std::set<Document*> compositedDocuments;
    std::vector<StackingContext*> documentOwners;
    StackingContext* rootLayer;

    ComputeStackingContextContext(StackingContext* rootLayer)
        : rootLayer(rootLayer)
    {
        seenPositionFixed = false;
        needsToAllocateGraphicsBufferForFixedElement = false;
    }

    LayoutRect screenExtentPerLayer(StackingContext* c)
    {
        {
            auto iter = extentPerLayer.find(c);
            if (iter != extentPerLayer.end()) {
                return iter->second;
            }
        }

        LayoutRect rt = c->owner()->computeScreenExtent();
        extentPerLayer.insert(std::make_pair(c, rt));

        return rt;
    }

    // screenExtent after overflow applies
    LayoutRect clippedScreenExtentPerLayer(StackingContext* c)
    {
        STARFISH_ASSERT(c != nullptr);

        {
            auto iter = clippedExtentPerLayer.find(c);
            if (iter != clippedExtentPerLayer.end()) {
                return iter->second;
            }
        }

        LayoutRect rt = screenExtentPerLayer(c);

        if (c->owner()->style()->position() == FixedPositionValue ||
            c->isIFrameStackingContext()) {
            return rt;
        }

        Frame* f = c->owner()->layoutParent();

        while (f != nullptr) {
            if (f->isFrameBox() && f->asFrameBox()->stackingContext() &&
                f->asFrameBox()->stackingContext()->isIFrameStackingContext()) {
                break;
            }

            if (f->shouldApplyOverflow()) {
                LayoutRect parentExtent =
                    f->asFrameBox()->computeScreenExtent();
                rt = LayoutRect::overlappedRect(parentExtent, rt);

                if (rt.isEmpty() ||
                    f->style()->position() == FixedPositionValue) {
                    break;
                }
            }
            f = f->layoutParent();
        }

        clippedExtentPerLayer.insert(std::make_pair(c, rt));
        return rt;
    }

    void pushCompsitedLayer(StackingContext* c)
    {
        STARFISH_ASSERT(c != nullptr);
        STARFISH_ASSERT(!isCompsitedLayer(c));

        if (seenPositionFixed) {
            throw RecomputeStackContextReason::PositionFixed;
        }
        compositedLayers.push_back(c);
        compositedDocuments.insert(c->m_owner->node()->document());
    }

    bool isCompsitedLayer(StackingContext* c, size_t* idx = nullptr)
    {
        STARFISH_ASSERT(c != nullptr);
        for (size_t i = 0; i < compositedLayers.size(); i++) {
            if (compositedLayers.at(i) == c) {
                if (idx) {
                    *idx = i;
                }
                return true;
            }
        }
        return false;
    }

    bool seenCompsitedLayer()
    {
        return compositedLayers.size();
    }
};

static bool canSplitBuffer(StackingContext* sc)
{
    bool canSplitGraphicsBufferCond = true;
    if (sc->owner()->style()->hasFilter() || sc->owner()->isFrameSVGSVGBox()) {
        canSplitGraphicsBufferCond = false;
    } else {
        sc->owner()->iterateChildFrameBox([&](FrameBox* fb) {
            if (fb->stackingContext() &&
                fb->stackingContext()->needsGraphicsBuffer()) {
                return false;
            }

            if (fb->isFrameSVGBox()) {
                auto filterElement =
                    fb->node()->asSVGElement()->filterElement();
                if (filterElement) {
                    canSplitGraphicsBufferCond = false;
                    return false;
                }
            }

            return true;
        });
    }

    return canSplitGraphicsBufferCond;
}

GraphicsBufferHolder::GraphicsBufferHolder(size_t bufferWidth,
                                           size_t bufferHeight,
                                           size_t screenWidth,
                                           size_t screenHeight,
                                           StackingContext* sc)
    : m_bufferWidth(bufferWidth)
    , m_bufferHeight(bufferHeight)
    , m_tileDataWidth(bufferWidth)
    , m_tileDataHeight(bufferHeight)
    , m_horizontalTileCount(1)
    , m_verticalTileCount(1)
    , m_additionalPixelRatio(sc->additionalPixelRatio())
{
    bool canSplitGraphicsBufferCond = canSplitBuffer(sc);

    LayoutRect screenRect(0, 0, screenWidth, screenHeight);
    // if buffer is smaller than screen && whole content will be shown on
    // screen
    // we don't need to divide buffer
    if (screenRect.containsInVisual(sc->screenExtent()) &&
        bufferWidth <= screenWidth && bufferHeight <= screenHeight) {
        canSplitGraphicsBufferCond = false;
    }

    // FIXME non-integer pixel ratio makes glitch between tiles
    if (m_additionalPixelRatio != 1) {
        canSplitGraphicsBufferCond = false;
    }

    if (!canSplitGraphicsBufferCond) {
        m_tileDataWidth = ceil(bufferWidth * m_additionalPixelRatio);
        m_horizontalTileCount = 1;
        m_tileDataHeight = ceil(bufferHeight * m_additionalPixelRatio);
        m_verticalTileCount = 1;

        m_surfaces.resize(1);
        m_surfaces[0] = nullptr;
    } else {
        size_t tileSize =
            ceil(CanvasSurface::g_canvasSurfaceTileSize /
                 sc->owner()->node()->webView()->screenInfo().devicePixelRatio);

        float effectiveWidth = bufferWidth * m_additionalPixelRatio;
        float effectiveHeight = bufferHeight * m_additionalPixelRatio;

        size_t wTextureCount = 1;
        while (effectiveWidth / wTextureCount > tileSize) {
            wTextureCount++;
        }
        m_tileDataWidth = ceil(effectiveWidth / (float)wTextureCount);
        m_horizontalTileCount = wTextureCount;

        size_t hTextureCount = 1;
        while (effectiveHeight / hTextureCount > tileSize) {
            hTextureCount++;
        }
        m_tileDataHeight = ceil(effectiveHeight / (float)hTextureCount);
        m_verticalTileCount = hTextureCount;

        m_surfaces.resize(wTextureCount * hTextureCount);
        for (size_t i = 0; i < m_surfaces.size(); i++) {
            m_surfaces[i] = nullptr;
        }
    }
}

void GraphicsBufferHolder::flushSurfaces()
{
    for (size_t i = 0; i < m_surfaces.size(); i++) {
        if (m_surfaces[i]) {
            m_surfaces[i]->detachNativeBuffer();
            m_surfaces[i] = nullptr;
        }
    }
}

void GraphicsBufferHolder::detachNativeBuffers()
{
    for (size_t i = 0; i < m_surfaces.size(); i++) {
        if (m_surfaces[i]) {
            m_surfaces[i]->detachNativeBuffer();
        }
        m_surfaces[i] = nullptr;
    }
    m_surfaces.clear();

    m_tileDataWidth = 0;
    m_tileDataHeight = 0;
    m_horizontalTileCount = 0;
    m_verticalTileCount = 0;
}

void* GraphicsBufferHolder::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(GraphicsBufferHolder));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(GraphicsBufferHolder)] = { 0 };
        GraphicsBufferHolder::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(GraphicsBufferHolder));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

StackingContextRareData::StackingContextRareData()
    : m_visibleRect(0, 0, 0, 0)
    , m_additionalPixelRatio(1)
    , m_graphicsBufferHolder(nullptr)
    , m_matrix(SkMatrix::I())
{
}

void* StackingContextRareData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(StackingContextRareData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(StackingContextRareData)] = { 0 };
        StackingContextRareData::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(StackingContextRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

StackingContext::StackingContext(FrameBox* owner, StackingContext* parent)
    : m_needsGraphicsBuffer(false)
    , m_hasNon2DRectTransform(false)
    , m_isVisibleRectComputedForNonGraphicsLayer(false)
    , m_hasFilterEffect(false)
    , m_needsGraphicsBufferReason(
          NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonNone)
    , m_owner(owner)
    , m_parent(parent)
    , m_rareData(nullptr)
{
    if (m_parent) {
        int32_t num = zIndex();
        auto iter = m_parent->m_childContexts.rbegin();
        size_t idx = m_parent->m_childContexts.size();
        StackingContextChild* target = nullptr;
        while (iter != m_parent->m_childContexts.rend()) {
            StackingContextChild* child = *iter;

            if (child->at(0)->zIndex() == num) {
                target = child;
                break;
            } else if (child->at(0)->zIndex() < num) {
                target = new StackingContextChild();
                m_parent->m_childContexts.insert(idx, target);
                break;
            }

            idx--;
            iter++;
        }
        if (!target) {
            target = new StackingContextChild();
            m_parent->m_childContexts.insert(m_parent->m_childContexts.begin(),
                                             target);
        }
        target->push_back(this);

        StackingContext* ancestor = m_parent;
        while (ancestor) {
            if (ancestor->owner()->style()->hasAvailableFilter()) {
                ancestorsThatHasFilters().push_back(ancestor);
            }
            ancestor = ancestor->parent();
        }
    }
}

void* StackingContext::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(StackingContext));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(StackingContext)] = { 0 };
        StackingContext::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(StackingContext));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

StackingContextRareData* StackingContext::ensureRareData()
{
    if (!m_rareData) {
        m_rareData = new StackingContextRareData();
    }
    return m_rareData;
}

int32_t StackingContext::zIndex()
{
    if (m_owner->isPositioned() ||
        (m_owner->isFlexItem() && m_owner->isSpecifiedZIndex())) {
        return m_owner->style()->zIndex();
    } else {
        return 0;
    }
}

void StackingContext::clearGraphicsBuffer()
{
    if (m_rareData && m_rareData->m_graphicsBufferHolder) {
        m_rareData->m_graphicsBufferHolder->detachNativeBuffers();
        m_rareData->m_graphicsBufferHolder = nullptr;
    }
}

bool StackingContext::needsRepaintingWhenScrolling()
{
    // can't scroll
    if (!m_owner->isFrameBlockBox()) {
        return false;
    }

    if (!needsGraphicsBuffer()) {
        return true;
    }

    // check border
    auto paddingBox = owner()->makeRect(BoxValue::PaddingBoxBoxValue);
    if ((paddingBox.width() != owner()->width().toFloat()) ||
        (paddingBox.height() != owner()->height().toFloat())) {
        return true;
    }

    if (owner()->style()->boxShadow()) {
        return true;
    }

    auto outline = owner()->style()->outline();
    if (outline && outline->isVisible()) {
        return true;
    }

    if (owner()->style()->backgroundLayerSize()) {
        return true;
    }

    return false;
}

bool StackingContext::needsToDrawScrollbar()
{
    // can't scroll
    if (!m_owner->isFrameBlockBox()) {
        return false;
    }

    if (!m_owner->node() || !m_owner->node()->isElement()) {
        return false;
    }

    auto frame = m_owner->asFrameBlockBox();
    bool hasVerticalScroll =
        frame->hasBiggerContentThanFrameHeight() &&
        frame->appliedOverflowY() >= OverflowValue::AutoOverflow &&
        frame->height();
    bool hasHorizontalScroll =
        frame->hasBiggerContentThanFrameWidth() &&
        frame->appliedOverflowX() >= OverflowValue::AutoOverflow &&
        frame->width();
    return hasVerticalScroll || hasHorizontalScroll;
}

bool StackingContext::inScrollActive()
{
    auto ao = m_owner->appliedOverflow();
    auto ox = ao.first;
    auto oy = ao.second;
    bool nonVisibleOverflowValueApplied =
        ox != OverflowValue::VisibleOverflow ||
        oy != OverflowValue::VisibleOverflow;
    if (nonVisibleOverflowValueApplied && m_owner->node() &&
        m_owner->node()->isElement()) {
        if (m_owner->node()->asElement()->hasRareMembers() &&
            (m_owner->node()->asElement()->rareMembers()->m_scrollLeft ||
             m_owner->node()->asElement()->rareMembers()->m_scrollTop)) {
            return true;
        }
    }
    if (m_owner->isFrameBlockBox()) {
        if (ox == OverflowValue::AutoOverflow ||
            ox == OverflowValue::ScrollOverflow) {
            if (m_owner->asFrameBlockBox()->hasBiggerContentThanFrameWidth()) {
                return true;
            }
        }
        if (oy == OverflowValue::AutoOverflow ||
            oy == OverflowValue::ScrollOverflow) {
            if (m_owner->asFrameBlockBox()->hasBiggerContentThanFrameHeight()) {
                return true;
            }
        }
    }
    return false;
}

bool StackingContext::isIFrameStackingContext()
{
    if (m_owner->layoutParent() && m_owner->layoutParent()->isFrameDocument()) {
        if (!m_owner->node()
                 ->document()
                 ->browsingContext()
                 ->isTopLevelBrowsingContext()) {
            return true;
        }
    }
    return false;
}

bool StackingContext::isIFrameStackingContextOwner()
{
    if (m_owner->node()->isHTMLIFrameElement()) {
        return true;
    }
    return false;
}

LayoutLocation StackingContext::transformOrigin()
{
    LayoutUnit ox = m_owner->width() / 2;
    LayoutUnit oy = m_owner->height() / 2;
    ComputedStyle* cs = m_owner->style();
    if (cs->hasTransformOrigin()) {
        auto od = cs->transformOrigin()->originValue();
        ox = od->getXAxis().specifiedValue(m_owner->width(), m_owner);
        oy = od->getYAxis().specifiedValue(m_owner->height(), m_owner);
    }
    return LayoutLocation(ox, oy);
}

void StackingContext::computeTransformMatrix()
{
    ComputedStyle* cs = m_owner->style();
    if (cs->hasTransforms(m_owner)) {
        ensureRareData();
        m_rareData->m_matrix = cs->transformsToMatrix(
            m_owner->width(), m_owner->height(), m_owner, true);

        m_hasNon2DRectTransform = m_owner->style()->has3DTransforms(m_owner) ||
                                  !m_rareData->m_matrix.rectStaysRect();

        if (!m_rareData->m_matrix.isIdentity()) {
            /*
            STARFISH_LOG_INFO("matrix [%f %f %f][%f %f %f][%f %f %f]",
                               m_rareData->m_matrix.getScaleX(),
                               m_rareData->m_matrix.getSkewX(),
                               m_rareData->m_matrix.getTranslateX(),
                               m_rareData->m_matrix.getSkewY(),
                               m_rareData->m_matrix.getScaleY(),
                               m_rareData->m_matrix.getTranslateY(),
                               m_rareData->m_matrix.getPerspX(),
                               m_rareData->m_matrix.getPerspY(),
                               m_rareData->m_matrix.get(8));*/
            SkMatrix test;
            bool testResult = m_rareData->m_matrix.invert(&test);
            if (testResult) {
                for (size_t i = 0; i < 9; i++) {
                    // prevent applying too big matrix
                    // because cairo can't deal well with huge matrix
                    if (m_rareData->m_matrix.get(i) >
                        STARFISH_CANVAS_LENGTH_MAX) {
                        testResult = false;
                        break;
                    }
                }
            }
            if (!testResult) {
                m_rareData->m_matrix = SkMatrix::InvalidMatrix();
            }
        }
    } else {
        if (m_rareData) {
            m_rareData->m_matrix = SkMatrix::I();
        }
    }
}

static void extractMaxScaleFactorFromAnimation(AnimatedValue* v,
                                               float& transformScaleMaxValue,
                                               ActiveAnimationTask* task)
{
    if (v->isTransformData()) {
        auto transformData =
            ((ActiveTransformAnimationTask*)task)->toTransformValue();
        if (transformData) {
            for (size_t i = 0; i < transformData->size(); i++) {
                if (transformData->at(i).type() == StyleTransformData::Scale) {
                    transformScaleMaxValue =
                        std::max(transformScaleMaxValue,
                                 (float)transformData->at(i).scale()->x());
                    transformScaleMaxValue =
                        std::max(transformScaleMaxValue,
                                 (float)transformData->at(i).scale()->y());
                }
            }
        }

    } else {
        auto m = v->getMatrix();
        transformScaleMaxValue =
            std::max(transformScaleMaxValue, m.getScaleX());
        transformScaleMaxValue =
            std::max(transformScaleMaxValue, m.getScaleY());
    }
}

static void findAnimationTaskRelatedWithTransformScale(
    ActiveAnimationTask* task, float& transformScaleMaxValue)
{
    if (task->property() == CSSStyleValuePair::Transform) {
        const auto& v = task->values();
        for (size_t i = 0; i < v.size(); i++) {
            extractMaxScaleFactorFromAnimation(v.at(i), transformScaleMaxValue,
                                               task);
        }
    }
}

void StackingContext::computeStackingContextProperties()
{
    STARFISH_ASSERT(isRootContext());

    ComputeStackingContextContext ctx(this);
    try {
        computeStackingContextProperties(ctx);
    } catch (RecomputeStackContextReason e) {
        if (e == RecomputeStackContextReason::PositionFixed) {
            ctx = ComputeStackingContextContext(this);
            ctx.needsToAllocateGraphicsBufferForFixedElement = true;
            computeStackingContextProperties(ctx);
        }
    }
    applyStackingContextProperties(ctx);

    ApplyPropertiesPostProcessingContext postCtx;
    applyStackingContextPropertiesPostProcessing(postCtx);

    m_owner->node()
        ->window()
        ->webView()
        ->m_stackingContextsNeedsGraphicsBuffer =
        std::move(postCtx.stackingContextsNeedsGraphicsBuffer);
}

void StackingContext::computeStackingContextProperties(
    ComputeStackingContextContext& compositingState)
{
    computeTransformMatrix();

    m_hasFilterEffect = false;
    if ((owner()->style()->hasAvailableFilter() ||
         m_ancestorsThatHasFilters.size() != 0)) {
        m_hasFilterEffect = true;
    }

    if (m_owner->isRootElement()) {
        compositingState.documentOwners.push_back(this);
    }

    NeedsGraphicsLayerReason reason =
        NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonNone;

    bool selfNeedsGraphicsBuffer = m_owner->needsGraphicsBuffer();

    if (m_owner->style()->position() == PositionValue::FixedPositionValue) {
        if (!compositingState.needsToAllocateGraphicsBufferForFixedElement &&
            compositingState.seenCompsitedLayer()) {
            throw RecomputeStackContextReason::PositionFixed;
        } else if (compositingState
                       .needsToAllocateGraphicsBufferForFixedElement) {
            selfNeedsGraphicsBuffer = true;
        } else {
            compositingState.seenPositionFixed = true;
        }
    }

    // check self visibility
    if (selfNeedsGraphicsBuffer && m_owner->isBoxesInvisibleFromHere()) {
        selfNeedsGraphicsBuffer = false;
    }
    if (selfNeedsGraphicsBuffer &&
        m_owner->style()->visibility() == HiddenVisibilityValue) {
        bool everyDesendentBoxIsHidden = true;
        m_owner->iterateChildFrameBox(
            [&everyDesendentBoxIsHidden](FrameBox* fb) {
                if (!fb->isAnonymous() &&
                    fb->style()->visibility() != HiddenVisibilityValue) {
                    everyDesendentBoxIsHidden = false;
                }
            });
        if (everyDesendentBoxIsHidden) {
            selfNeedsGraphicsBuffer = false;
        }
    }

    auto selfExtent = compositingState.screenExtentPerLayer(this);
    m_screenExtent = selfExtent;

    bool compositedBySelf = selfNeedsGraphicsBuffer;

    if (m_owner->node() && m_owner->node()->isElement()) {
        compositedBySelf |=
            m_owner->node()->window()->webView()->hasActiveAnimationExecutor(
                m_owner->node()->asElement());
    }

#if !defined(STARFISH_ENABLE_TEST)
    if (m_owner->isRootElement()) {
        FrameBlockBox* fb = m_owner->asFrameBlockBox();
        if (isRootContext()) {
            fb = m_owner->node()
                     ->document()
                     ->frame()
                     ->asFrameBox()
                     ->asFrameBlockBox();
        }
        if ((fb->hasBiggerContentThanFrameWidth() ||
             fb->hasBiggerContentThanFrameHeight())) {
            compositedBySelf = true;
        }
    }
#endif

    if (compositingState.seenCompsitedLayer() && !isRootContext() &&
        m_owner->isAbsolutePositioned()) {
        SkMatrix windowMatrix = m_owner->computeMatrixOnWindow();
        auto windowRect = computeBoxExtent(
            LayoutRect(0, 0, m_owner->width(), m_owner->height()),
            windowMatrix);
        if (windowRect.maxX() < 0 || windowRect.maxY() < 0) {
            compositedBySelf = true;
        }
    }

    if (Compositor::supportsFilterEffect(m_owner->document()->starfish(), 1,
                                         1) ==
            true /* test whatever compostior supports filter */
        && m_hasFilterEffect) {
        compositedBySelf = true;
    }

    if (compositedBySelf) {
        reason = NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonBySelf;
        compositingState.compositeFlagInfoBecauseSelf[this] = true;
    } else {
        if (inScrollActive()) {
            compositedBySelf = true;
            reason =
                NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonNeedsScroll;
        }
        compositingState.compositeFlagInfoBecauseSelf[this] = compositedBySelf;
    }
    bool willBeComposited = compositedBySelf;

    if (!willBeComposited && compositingState.seenCompsitedLayer() &&
        compositingState.compositedDocuments.find(
            m_owner->node()->document()) !=
            compositingState.compositedDocuments.end()) {
        // find most nearest Composited ancestor index
        size_t ancestorIndex = 0;

        StackingContext* p = parent();
        StackingContext* compositedAncestor = nullptr;
        bool hasScrollBetweenThisStackigContextAndGraphicsBuffer = false;
        while (p) {
            if (p->inScrollActive()) {
                hasScrollBetweenThisStackigContextAndGraphicsBuffer = true;
            }
            if (compositingState.isCompsitedLayer(p, &ancestorIndex)) {
                compositedAncestor = p;
                break;
            }
            p = p->parent();
        }
        STARFISH_ASSERT(compositedAncestor);
        bool canConveredByParentCompositedLayer = false;
        bool isCollapsedWithSilbingLayer = false;
        bool parentIsRootElementLayer = false; // parent layer is html element

        auto parentExtent =
            compositingState.screenExtentPerLayer(compositedAncestor);

        if (compositedAncestor->owner()->isRootElement()) {
            parentIsRootElementLayer = true;
        }

        bool thereIsOverflowHiddenBetweenSelfAndCompositedAncestor = false;
        {
            Frame* f = m_owner;
            while (f != nullptr) {
                if (f->shouldApplyOverflow()) {
                    thereIsOverflowHiddenBetweenSelfAndCompositedAncestor =
                        true;
                }
                if (f == compositedAncestor->owner()) {
                    break;
                }
                f = f->layoutParent();
            }
        }

        if (m_owner->isAbsolutePositioned() &&
            hasScrollBetweenThisStackigContextAndGraphicsBuffer) {
            reason = NeedsGraphicsLayerReason::
                NeedsGraphicsLayerReasonNotCoveredByParent;
        } else if (parentIsRootElementLayer ||
                   (parentExtent.containsInVisual(selfExtent.x(),
                                                  selfExtent.y()) == true &&
                    parentExtent.containsInVisual(selfExtent.maxX(),
                                                  selfExtent.y()) == true &&
                    parentExtent.containsInVisual(selfExtent.x(),
                                                  selfExtent.maxY()) == true &&
                    parentExtent.containsInVisual(selfExtent.maxX(),
                                                  selfExtent.maxY())) ||
                   thereIsOverflowHiddenBetweenSelfAndCompositedAncestor) {
            canConveredByParentCompositedLayer = true;
        } else {
            reason = NeedsGraphicsLayerReason::
                NeedsGraphicsLayerReasonNotCoveredByParent;
        }

        if (canConveredByParentCompositedLayer) {
            auto& cv = compositingState.compositedLayers;
            for (size_t i = ancestorIndex + 1; i < cv.size(); i++) {
                auto extent =
                    compositingState.clippedScreenExtentPerLayer(cv[i]);
                if (extent.intersects(selfExtent)) {
                    isCollapsedWithSilbingLayer = true;
                    reason = NeedsGraphicsLayerReason::
                        NeedsGraphicsLayerReasonCollapsedWithSiblingLayer;
                    break;
                }

                if (cv[i]->owner()->isRunningTransformAnimation()) {
                    // find never collapsed case by overflow: hidden;
                    Frame* f = cv[i]->owner();
                    bool foundOverflow = false;
                    LayoutRect clippedExtentRect;
                    while (f != nullptr) {
                        if (f->isAncestorOf(owner())) {
                            break;
                        }
                        if (f->shouldApplyOverflow()) {
                            foundOverflow = true;
                            clippedExtentRect =
                                compositingState.clippedScreenExtentPerLayer(
                                    cv[i]);
                            break;
                        }
                        f = f->layoutParent();
                    }

                    if (foundOverflow) {
                        if (clippedExtentRect.intersects(selfExtent)) {
                            isCollapsedWithSilbingLayer = true;
                            reason = NeedsGraphicsLayerReason::
                                NeedsGraphicsLayerReasonSiblingLayerNeedsAnimation;
                        }
                    } else {
                        isCollapsedWithSilbingLayer = true;
                        reason = NeedsGraphicsLayerReason::
                            NeedsGraphicsLayerReasonSiblingLayerNeedsAnimation;
                    }

                    if (isCollapsedWithSilbingLayer) {
                        break;
                    }
                }
            }
        }

        if (canConveredByParentCompositedLayer &&
            !isCollapsedWithSilbingLayer) {
        } else {
            willBeComposited = true;
        }
    }

    if (willBeComposited) {
        for (size_t i = 0; i < compositingState.documentOwners.size(); i++) {
            if ((compositingState.documentOwners[i]->isRootContext()) ||
                (compositingState.documentOwners[i]
                     ->owner()
                     ->asFrameBlockBox()
                     ->hasBiggerContentThanFrameWidth()) ||
                (compositingState.documentOwners[i]
                     ->owner()
                     ->asFrameBlockBox()
                     ->hasBiggerContentThanFrameHeight())) {
                if (compositingState.compositedLayers.size() == 0) {
                    STARFISH_ASSERT(
                        compositingState.documentOwners[i]->isRootContext());
                }
                if (!compositingState.isCompsitedLayer(
                        compositingState.documentOwners[i])) {
                    compositingState.pushCompsitedLayer(
                        compositingState.documentOwners[i]);
                }
            }
        }
        if (!compositingState.isCompsitedLayer(this)) {
            compositingState.pushCompsitedLayer(this);
        }
    }

    m_needsGraphicsBufferReason = reason;

    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        int32_t num = child->at(0)->zIndex();
        if (num >= 0) {
            break;
        }
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            (*iter2)->computeStackingContextProperties(compositingState);
            iter2++;
        }
        iter++;
    }

    iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        int32_t num = child->at(0)->zIndex();
        if (num >= 0) {
            auto iter2 = child->begin();
            while (iter2 != child->end()) {
                (*iter2)->computeStackingContextProperties(compositingState);
                iter2++;
            }
        }
        iter++;
    }

    if (m_owner->isRootElement()) {
        if (compositingState.isCompsitedLayer(this)) {
            willBeComposited = true;
            m_needsGraphicsBufferReason = NeedsGraphicsLayerReason::
                NeedsGraphicsLayerReasonCollapsedWithSiblingLayer;
        }
    }

    compositingState.compositeFlagInfo.insert(
        std::make_pair(this, willBeComposited));
}

static void computeVisibleRectPedigreeWorker(
    StackingContext* c, std::vector<FrameBox*>& pedigree,
    std::vector<FrameBox*>::reverse_iterator iter,
    Frame::ComputeVisibleRectContext& ctx)
{
    if (pedigree.rend() == iter) {
        if (ctx.visbleRectComputedBox.find(c->owner()) ==
            ctx.visbleRectComputedBox.end()) {
            ctx.visbleRectComputedBox.insert(c->owner());
            c->owner()->computeVisibleRect(ctx);
        }
    } else {
        Frame::ComputeVisibleRectContextFragment f(ctx, *iter);
        computeVisibleRectPedigreeWorker(c, pedigree, iter + 1, ctx);
    }
}

static void computeVisibleRect(StackingContext* source, StackingContext* c,
                               Frame::ComputeVisibleRectContext& ctx)
{
    if (c != source && c->needsGraphicsBuffer()) {
        return;
    }

    if (c->owner()->isBoxesInvisibleFromHere()) {
        return;
    }

    if (c != source) {
        std::vector<FrameBox*> pedigree;
        FrameBox* box = c->owner()->layoutParent()->asFrameBox();
        while (source->owner() != box) {
            pedigree.push_back(box);
            box = box->layoutParent()->asFrameBox();
        }

        computeVisibleRectPedigreeWorker(c, pedigree, pedigree.rbegin(), ctx);
    } else {
        if (ctx.visbleRectComputedBox.find(c->owner()) ==
            ctx.visbleRectComputedBox.end()) {
            ctx.visbleRectComputedBox.insert(c->owner());
            c->owner()->computeVisibleRect(ctx);
        }
    }

    auto iter = c->childContexts().begin();
    while (iter != c->childContexts().end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            computeVisibleRect(source, (*iter2), ctx);
            iter2++;
        }
        iter++;
    }
}

static CanvasSurface::CanvasSurfaceFlag computeSurfaceFlag(StackingContext* sc)
{
    if (sc->owner()->isFrameSVGSVGBox()) {
        return static_cast<CanvasSurface::CanvasSurfaceFlag>(
            CanvasSurface::PreferEGLImage |
            CanvasSurface::PreferRetainCPUBufferWhenUnmap |
            CanvasSurface::PreferUnitedTexture);
    }
    if (sc->hasFilterEffect()) {
        return CanvasSurface::PreferUnitedTexture;
    }
    return CanvasSurface::PlainElement;
}

void StackingContext::applyStackingContextProperties(
    ComputeStackingContextContext& ctx)
{
    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->rbegin();
        while (iter2 != child->rend()) {
            (*iter2)->applyStackingContextProperties(ctx);
            iter2++;
        }
        iter++;
    }

    bool inAnimation = m_owner->node()->isRunningOpacityAnimation() ||
                       m_owner->node()->isRunningTransformAnimation();
    auto& prevDrawnMap =
        m_owner->node()->webView()->prevDrawnStackingContextInfo();
    bool compositedBefore = false;
    auto prevDrawnMapIter = prevDrawnMap.find(m_owner->node());
    if (prevDrawnMap.end() != prevDrawnMapIter) {
        compositedBefore = prevDrawnMapIter->second.needsGraphicsBuffer;
        if (compositedBefore) {
            ensureRareData()->m_graphicsBufferHolder =
                prevDrawnMapIter->second.graphicsBufferHolder;
        }
    }
    bool willBeComposited = ctx.compositeFlagInfo[this];
    bool willBeCompositedDueToSelf = ctx.compositeFlagInfoBecauseSelf[this];

    if (inAnimation || (compositedBefore && !willBeComposited)) {
        willBeComposited = true;
    }

    if (m_rareData) {
        m_rareData->m_visibleRect = LayoutRect(0, 0, 0, 0);
    }

    if (willBeComposited) {
        m_needsGraphicsBuffer = true;
        ensureRareData();
        bool shouldPaintWindowBackgroundImage = false;
        if (m_owner->isRootElement()) {
            BrowsingContext* bc =
                m_owner->node()->document()->browsingContext();
            HTMLElement* e = nullptr;
            if (bc->hasRootElementBackground()) {
                HTMLHtmlElement* root = bc->document()->rootElement();
                e = root;
            } else if (bc->hasBodyElementBackground()) {
                HTMLBodyElement* body = bc->document()->rootElement()->body();
                e = body;
            }

            if (e) {
                if (e->style()->backgroundLayerSize()) {
                    shouldPaintWindowBackgroundImage = true;
                }
            }
        }

        SkMatrix l = SkMatrix::I();
        Frame::ComputeVisibleRectContext::ComputePurpose purpose =
            willBeCompositedDueToSelf
                ? Frame::ComputeVisibleRectContext::GraphicsBufferBySelf
                : Frame::ComputeVisibleRectContext::GraphicsBufferByOtherLayer;

        Frame::ComputeVisibleRectContext ctx(purpose, this, l,
                                             m_rareData->m_visibleRect);

        if (shouldPaintWindowBackgroundImage) {
            ctx.isVisibleRectCollapsible = false;
        }

        computeVisibleRect(this, this, ctx);

        if (shouldPaintWindowBackgroundImage) {
            LayoutRect scrollRect(
                0, 0, m_owner->document()->window()->scrollWidth(false),
                m_owner->document()->window()->scrollHeight(false));
            m_rareData->m_visibleRect.unite(scrollRect);
            m_rareData->m_visibleRect.unite(
                LayoutRect(0, 0, m_owner->node()->window()->innerWidth(),
                           m_owner->node()->window()->innerHeight()));
        } else if (inScrollActive()) {
            if (needsRepaintingWhenScrolling()) {
                // expand visibleRect to draw border
                LayoutRect scrollRect(
                    0, 0, m_owner->asFrameBlockBox()->scrollWidth(),
                    m_owner->asFrameBlockBox()->scrollHeight());
                auto paddingBox =
                    owner()->makeRect(BoxValue::PaddingBoxBoxValue);
                auto bw = owner()->width().toFloat() - paddingBox.width();
                auto bh = owner()->height().toFloat() - paddingBox.height();
                scrollRect.setWidth(scrollRect.width() + bw);
                scrollRect.setHeight(scrollRect.height() + bh);
                m_rareData->m_visibleRect.unite(scrollRect);
            }
        }

        if (m_owner->isRootElement()) {
            if (m_rareData->m_visibleRect.x() < 0) {
                if (m_rareData->m_visibleRect.width() +
                        m_rareData->m_visibleRect.x() >
                    0) {
                    m_rareData->m_visibleRect.setWidth(
                        m_rareData->m_visibleRect.width() +
                        m_rareData->m_visibleRect.x());
                    m_rareData->m_visibleRect.setX(0);
                } else {
                    m_rareData->m_visibleRect.setWidth(0);
                }
            }
            if (m_rareData->m_visibleRect.y() < 0) {
                if (m_rareData->m_visibleRect.height() +
                        m_rareData->m_visibleRect.y() >
                    0) {
                    m_rareData->m_visibleRect.setHeight(
                        m_rareData->m_visibleRect.height() +
                        m_rareData->m_visibleRect.y());
                    m_rareData->m_visibleRect.setY(0);
                } else {
                    m_rareData->m_visibleRect.setHeight(0);
                }
            }
        }

        m_isVisibleRectComputedForNonGraphicsLayer = true;
    } else {
        m_needsGraphicsBuffer = false;
        m_isVisibleRectComputedForNonGraphicsLayer = false;
    }

    if (compositedBefore != willBeComposited) {
        m_owner->node()->webView()->markNeedsPaintingConsiderInRendering();
    } else if (compositedBefore && compositedBefore == willBeComposited) {
        if (prevDrawnMapIter != prevDrawnMap.end()) {
            if (prevDrawnMapIter->second.graphicsBufferVisibleRect !=
                m_rareData->m_visibleRect) {
                // visible rect changed
                m_owner->node()->setNeedsPainting();
            }
        }
    } else if (!compositedBefore && !willBeComposited) {
        if (prevDrawnMapIter != prevDrawnMap.end()) {
            if ((prevDrawnMapIter->second.opacity !=
                 m_owner->style()->opacity()) ||
                (prevDrawnMapIter->second.transformMatrix !=
                 transformMatrix())) {
                m_owner->node()
                    ->webView()
                    ->markNeedsPaintingConsiderInRendering();
            }
        } else {
            if (transformMatrix() != SkMatrix::I()) {
                m_owner->node()
                    ->webView()
                    ->markNeedsPaintingConsiderInRendering();
            }
        }
    }

    if (isRootContext() && willBeComposited) {
        m_owner->node()->webView()->markNeedsCompositeConsiderInRendering();
    }
}

void StackingContext::applyStackingContextPropertiesPostProcessing(
    ApplyPropertiesPostProcessingContext& ctx)
{
    float orgBaseAdditionalPixelRatio = ctx.baseAdditionalPixelRatio;

    if (needsComposite()) {
        ctx.stackingContextsNeedsGraphicsBuffer.push_back(this);
        ensureRareData();
        STARFISH_ASSERT(m_rareData);
        float oldAdditionalPixelRatio = m_rareData->m_additionalPixelRatio;
        m_rareData->m_additionalPixelRatio = ctx.baseAdditionalPixelRatio;

        const float minScale = 1;
        const float maxScale = 4;

        if (m_owner->node() && m_owner->node()->isRunningTransformAnimation()) {
            float transformScaleMaxValue = 1;

            auto& transitions = m_owner->node()
                                    ->document()
                                    ->animationExecutor()
                                    ->activeTransitions();
            auto iter = transitions.begin();
            while (iter != transitions.end()) {
                if ((*iter)->targetElement() == m_owner->node()) {
                    findAnimationTaskRelatedWithTransformScale(
                        *iter, transformScaleMaxValue);
                }
                iter++;
            }
            auto& animations = m_owner->node()
                                   ->document()
                                   ->animationExecutor()
                                   ->activeAnimations();
            auto iter2 = animations.begin();
            while (iter2 != animations.end()) {
                if (iter2->first->element() == m_owner->node()) {
                    auto& v = iter2->second;
                    auto iter3 = v.begin();
                    while (iter3 != v.end()) {
                        findAnimationTaskRelatedWithTransformScale(
                            *iter3, transformScaleMaxValue);
                        iter3++;
                    }
                }
                iter2++;
            }

            if (transformScaleMaxValue < minScale) {
                transformScaleMaxValue = minScale;
            } else if (transformScaleMaxValue > maxScale) {
                transformScaleMaxValue = maxScale;
            }

            m_rareData->m_additionalPixelRatio =
                std::max(ctx.baseAdditionalPixelRatio, transformScaleMaxValue);
            ctx.baseAdditionalPixelRatio =
                std::max(ctx.baseAdditionalPixelRatio,
                         m_rareData->m_additionalPixelRatio);
        } else {
            auto matrix = m_owner->style()->transformsToMatrix(
                m_owner->width(), m_owner->height(), m_owner,
                m_owner->isTransformable());

            int32_t windowWidth = m_owner->node()->window()->innerWidth();
            int32_t windowHeight = m_owner->node()->window()->innerHeight();
            LayoutUnit visibleWidth = m_rareData->m_visibleRect.width();
            LayoutUnit visibleHeight = m_rareData->m_visibleRect.height();
            // additional
#ifndef STARFISH_GRAPHICS_BUFFER_ADDITIONAL_FACTOR_MAX_SCALE
#define STARFISH_GRAPHICS_BUFFER_ADDITIONAL_FACTOR_MAX_SCALE 6
#endif

#ifdef DISABLE_SCALE_OPTIMIZE
            const int32_t minimumScale =
                STARFISH_GRAPHICS_BUFFER_ADDITIONAL_FACTOR_MAX_SCALE;

            if ((!m_rareData->m_visibleRect.isEmpty() &&
                 !m_owner->node()->window()->isInnerSizeEmpty()) &&
                ((visibleWidth > windowWidth * minimumScale) ||
                 (visibleHeight > windowHeight * minimumScale))) {
                int m = std::max(visibleWidth / windowWidth,
                                 visibleHeight / windowHeight);
                float scale = std::min(matrix.getScaleX(), matrix.getScaleY());
                scale = std::min(1.f / m, scale);
                // respect org scale
                m_rareData->m_additionalPixelRatio =
                    std::min(ctx.baseAdditionalPixelRatio, scale);
                ctx.baseAdditionalPixelRatio =
                    std::min(ctx.baseAdditionalPixelRatio,
                             m_rareData->m_additionalPixelRatio);
            } else {
#endif
                float scale = std::max(matrix.getScaleX(), matrix.getScaleY());
                if (scale < minScale) {
                    scale = minScale;
                } else if (scale > maxScale) {
                    scale = maxScale;
                }

                m_rareData->m_additionalPixelRatio =
                    std::max(ctx.baseAdditionalPixelRatio, scale);
                ctx.baseAdditionalPixelRatio =
                    std::max(ctx.baseAdditionalPixelRatio,
                             m_rareData->m_additionalPixelRatio);
#ifdef DISABLE_SCALE_OPTIMIZE
            }
#endif
        }
        if (oldAdditionalPixelRatio != m_rareData->m_additionalPixelRatio) {
            m_owner->node()->webView()->markNeedsPaintingConsiderInRendering();
        }
    } else {
        if (m_rareData) {
            m_rareData->m_additionalPixelRatio = 1;
        }
    }

    auto iter = childContexts().begin();
    while (iter != childContexts().end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            (*iter2)->applyStackingContextPropertiesPostProcessing(ctx);
            iter2++;
        }
        iter++;
    }

    ctx.baseAdditionalPixelRatio = orgBaseAdditionalPixelRatio;
}

class FilterContext : public gc {
public:
    FilterContext(Canvas** origin, StackingContext* owner,
                  StackingContext::PaintingStackingContextContext& ctx)
        : m_origin(origin)
        , m_originCanvas(*origin)
        , m_ownerStackingContext(owner)
        , m_maxRadiusOffset(0.0f)
        , m_nativeImageToApplyFilter(nullptr)
        , m_canvasToApplyFilter(nullptr)
    {
        auto style = m_ownerStackingContext->owner()->style();

        Length standardDeviation;
        if (style->hasAvailableFilter() &&
            style->filter()->getStandardDeviationOfBlurFilter(
                standardDeviation)) {
            m_maxRadiusOffset = std::max(
                m_maxRadiusOffset, (standardDeviation.numberData() * 2 * 1.8f));
        }

        for (auto ancestor :
             m_ownerStackingContext->ancestorsThatHasFilters()) {
            auto s = ancestor->owner()->style();
            if (s->filter()->getStandardDeviationOfBlurFilter(
                    standardDeviation)) {
                m_maxRadiusOffset =
                    std::max(m_maxRadiusOffset,
                             (standardDeviation.numberData() * 2 * 1.8f));
            }
        }
        m_maxRadiusOffset =
            std::min(ShadowBlur::RADIUS_LIMIT, m_maxRadiusOffset);

        if (!m_ownerStackingContext->needsGraphicsBuffer()) {
            LayoutRect rect = m_ownerStackingContext->screenExtent();
            LayoutUnit minX = rect.x();
            LayoutUnit maxX = rect.maxX();
            LayoutUnit minY = rect.y();
            LayoutUnit maxY = rect.maxY();

            minX = minX.floor();
            maxX = maxX.ceil();
            minY = minY.floor();
            maxY = maxY.ceil();

            size_t bufferWidth;
            size_t bufferHeight;
            computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY,
                                             bufferWidth, bufferHeight);

            if (owner->owner()->node()->webView()->needsComposite()) {
                auto& renderTarget = m_originCanvas->renderTargetInfo();
                bufferWidth = renderTarget.m_width;
                bufferHeight = renderTarget.m_height;
            }

            m_nativeImageToApplyFilter = BufferedNativeImageData::create(
                bufferWidth + ceil(m_maxRadiusOffset),
                bufferHeight + ceil(m_maxRadiusOffset));

            m_canvasToApplyFilter = Canvas::create(
                m_ownerStackingContext->owner()->node()->webView(),
                m_nativeImageToApplyFilter);

            m_canvasToApplyFilter->clearColor(Unit::Color(0, 0, 0, 0));

            m_canvasToApplyFilter->setFont(style->font());
            m_canvasToApplyFilter->setFillColor(style->color());
            m_canvasToApplyFilter->setTextDecorationData(
                m_originCanvas->textDecorationData());

            if (owner->owner()->node()->webView()->needsComposite()) {
                m_canvasToApplyFilter->postMatrix(
                    m_originCanvas->currentTransformMatrix());
            }

            m_canvasToApplyFilter->translate(ceil(m_maxRadiusOffset / 2),
                                             ceil(m_maxRadiusOffset / 2));

            (*m_origin) = m_canvasToApplyFilter;
        }
    }

    ~FilterContext()
    {
    }

    void changeCurrentCanvasToOriginal()
    {
        (*m_origin) = m_originCanvas;
    }

    void changeCurrentCanvasToApplyFilter()
    {
        if (m_canvasToApplyFilter) {
            (*m_origin) = m_canvasToApplyFilter;
        }
    }

    void applyAllFilter(StackingContext::PaintingStackingContextContext& ctx)
    {
        auto webView = m_ownerStackingContext->owner()->node()->webView();

        uint8_t* buffer;
        size_t width, height, stride;

        if (m_ownerStackingContext->needsGraphicsBuffer()) {
            const auto& info = m_originCanvas->renderTargetInfo();
            buffer = info.m_buffer;
            width = info.m_width;
            height = info.m_height;
            stride = info.m_stride;
        } else {
            buffer = m_nativeImageToApplyFilter->data();
            width = m_nativeImageToApplyFilter->width();
            height = m_nativeImageToApplyFilter->height();
            stride = m_nativeImageToApplyFilter->stride();
        }

        size_t imageStartYPosition = height;
        if (!m_ownerStackingContext->needsGraphicsBuffer()) {
            delete m_canvasToApplyFilter;

            LongTaskFinder t("testing painting result in FilterContext", 1);
            uint32_t* ptr = (uint32_t*)m_nativeImageToApplyFilter->data();
            size_t len = stride * height / 4;
            for (size_t i = 0; i < len; i++) {
                if (ptr[i]) {
                    imageStartYPosition = i * 4 / stride;
                    break;
                }
            }

            if (imageStartYPosition != height) {
                // give a room for blurred image
                auto offset = ceil(m_maxRadiusOffset / 2);
                if ((int)imageStartYPosition - offset > 0) {
                    imageStartYPosition -= offset;
                } else {
                    imageStartYPosition = 0;
                }
            }
        } else {
            // TODO
            imageStartYPosition = 0;
        }

        if (imageStartYPosition != height) {
            auto style = m_ownerStackingContext->owner()->style();
            if (style->hasAvailableFilter()) {
                for (auto filter : *style->filter()) {
                    filter->apply(webView,
                                  buffer + imageStartYPosition * stride, width,
                                  height - imageStartYPosition, stride);
                }
            }

            for (auto ancestor :
                 m_ownerStackingContext->ancestorsThatHasFilters()) {
                style = ancestor->owner()->style();
                for (auto filter : *style->filter()) {
                    filter->apply(webView,
                                  buffer + imageStartYPosition * stride, width,
                                  height - imageStartYPosition, stride);
                }
            }
        }

        if (!m_ownerStackingContext->needsGraphicsBuffer()) {
            if (imageStartYPosition != height) {
                Unit::Rect rect(0, 0, m_nativeImageToApplyFilter->width(),
                                m_nativeImageToApplyFilter->height());
                float offset = ceil(m_maxRadiusOffset / 2);
                m_originCanvas->save();
                if (m_ownerStackingContext->owner()
                        ->node()
                        ->webView()
                        ->needsComposite()) {
                    m_originCanvas->setMatrix(SkMatrix::I());
                }
                m_originCanvas->translate(-offset, -offset);
                m_originCanvas->drawImage(m_nativeImageToApplyFilter, rect);
                m_originCanvas->restore();
            }
            delete m_nativeImageToApplyFilter;
            (*m_origin) = m_originCanvas;
        }
    }

private:
    Canvas** m_origin;
    Canvas* m_originCanvas;
    StackingContext* m_ownerStackingContext;
    float m_maxRadiusOffset;

    NativeImageData* m_nativeImageToApplyFilter;
    Canvas* m_canvasToApplyFilter;
};

void StackingContext::fillGraphicsBufferContents(
    Canvas* canvas, PaintingStackingContextContext& ctx)
{
    canvas->save();

    if (isRootContext()) {
        m_owner->node()->document()->browsingContext()->paintWindowBackground(
            canvas);
    }

    if (owner()->shouldResetTextDecoration()) {
        canvas->resetTextDecorationData();
    } else {
        canvas->mergeTextDecorationData(owner()->style());
    }

    if (owner()->isAbsolutePositioned()) {
        RectData* rect = owner()->style()->clip();
        if (rect) {
            canvas->clip(Unit::Rect(
                rect->left().numberData(), rect->top().numberData(),
                rect->right().numberData(), rect->bottom().numberData()));
        }
    }

    bool canRejectPainting =
        canvas->canRejectPainting(StackingContext::visibleRect());
    bool needsComputeOverflow =
        !isRootContext() && !isIFrameStackingContext() &&
        m_owner->isFrameBlockBox() && m_owner->shouldApplyOverflow();
    bool canApplyScroll =
        !ctx.willCompositing ||
        ctx.paintingForCompositingStartingFrom.valueOrNull() != this;
    bool drawBorderAtAnotherPlaceDueToScroll =
        !canApplyScroll && needsRepaintingWhenScrolling();

    if (!canRejectPainting) {
        applyMask(canvas, ctx);
        if (drawBorderAtAnotherPlaceDueToScroll) {
            canvas->save();
            canvas->translate(m_owner->asFrameBlockBox()->scrollLeft(),
                              m_owner->asFrameBlockBox()->scrollTop());
        }
        m_owner->paintBackgroundAndBorders(canvas);
        if (drawBorderAtAnotherPlaceDueToScroll) {
            canvas->restore();
        }
    }
    if (needsComputeOverflow && canApplyScroll) {
        canvas->save();
        canvas->clip(m_owner->makeRect(BoxValue::PaddingBoxBoxValue));
        const LayoutRect rect(0, 0, m_owner->width(), m_owner->height());
        m_owner->applyBorderRadiusClippingIfNeeds(canvas, rect);
        if (canApplyScroll) {
            ctx.layerScrollX = m_owner->asFrameBlockBox()->scrollLeft();
            ctx.layerScrollY = m_owner->asFrameBlockBox()->scrollTop();
        }
    } else if (drawBorderAtAnotherPlaceDueToScroll) {
        canvas->save();
        auto rt = m_owner->makeRect(BoxValue::PaddingBoxBoxValue);
        rt.setX(rt.x() + m_owner->asFrameBlockBox()->scrollLeft());
        rt.setY(rt.y() + m_owner->asFrameBlockBox()->scrollTop());
        canvas->clip(rt);
    }

    // Within each stacking context, the following layers are painted in
    // back-to-front order:
    // the background and borders of the element forming the stacking context.
    if (isIFrameStackingContext()) {
        FrameBlockBox* document = m_owner->layoutParent()->asFrameBlockBox();
        FrameBox* iframeBox = m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->sourceElement()
                                  ->frame()
                                  ->asFrameBox();
        auto clipRect = iframeBox->makeRect(BoxValue::PaddingBoxBoxValue);
        HTMLIFrameElement* iframe =
            m_owner->node()->document()->browsingContext()->sourceElement();
        clipRect.setX(clipRect.x() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollX());
        clipRect.setY(clipRect.y() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollY());

        if (!needsGraphicsBuffer()) {
            canvas->translate(iframeBox->borderLeft() +
                                  iframeBox->paddingLeft(),
                              iframeBox->borderTop() + iframeBox->paddingTop());
        }

        if (needsGraphicsBuffer()) {
            canvas->save();
            canvas->resetMatrixAndClip();
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
            canvas->restore();
        } else {
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
        }
    }

    // the child stacking contexts with negative stack levels (most negative
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                break;
            }
            auto iter2 = child->begin();
            while (iter2 != child->end()) {
                StackingContext* sCtx = *iter2;
                ComputeOverflow<Canvas> r(canvas, sCtx, m_owner, ctx);
                sCtx->paintStackingContext(canvas, ctx);
                iter2++;
            }
            iter++;
        }
    }

    if (!canRejectPainting) {
        if (m_hasFilterEffect && !Compositor::supportsFilterEffect(
                                     m_owner->document()->starfish(),
                                     canvas->renderTargetInfo().m_width,
                                     canvas->renderTargetInfo().m_height)) {
            FilterContext filterContext(&canvas, this, ctx);
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
            filterContext.applyAllFilter(ctx);
        } else {
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
        }
    }

    // the child stacking contexts with positive stack levels (least positive
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                auto iter2 = child->begin();
                while (iter2 != child->end()) {
                    StackingContext* sCtx = *iter2;

                    ComputeOverflow<Canvas> r(canvas, sCtx, m_owner, ctx);
                    sCtx->paintStackingContext(canvas, ctx);
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (needsComputeOverflow && canApplyScroll) {
        canvas->restore();
    } else if (drawBorderAtAnotherPlaceDueToScroll) {
        canvas->restore();
    }

    paintScrollbar(canvas);

    canvas->restore();
}

LayoutRect StackingContext::visibleRect()
{
    if (!m_isVisibleRectComputedForNonGraphicsLayer) {
        ensureRareData()->m_visibleRect = m_owner->frameVisibleRect();
        SkMatrix l = SkMatrix::I();
        Frame::ComputeVisibleRectContext ctx(
            Frame::ComputeVisibleRectContext::GraphicsBufferBySelf, this, l,
            m_rareData->m_visibleRect);
        computeVisibleRect(this, this, ctx);
        m_isVisibleRectComputedForNonGraphicsLayer = true;
    }
    return m_rareData ? m_rareData->m_visibleRect : LayoutRect(0, 0, 0, 0);
}

float StackingContext::additionalPixelRatio()
{
    return m_rareData ? m_rareData->m_additionalPixelRatio : 1;
}

static LayoutRect computeScreenRect(StackingContext* ctx)
{
    LayoutRect screenRect(0, 0,
                          ctx->owner()
                              ->node()
                              ->webView()
                              ->mainBrowsingContext()
                              ->window()
                              ->innerWidth(),
                          ctx->owner()
                              ->node()
                              ->webView()
                              ->mainBrowsingContext()
                              ->window()
                              ->innerHeight());
    LayoutRect windowRect = screenRect;

    if (!ctx->owner()
             ->node()
             ->document()
             ->browsingContext()
             ->isTopLevelBrowsingContext()) {
        FrameBox* f = ctx->owner();

        while (f) {
            if (f->isFrameReplaced() &&
                f->asFrameReplaced()->isFrameReplacedIFrame()) {
                break;
            }
            f = f->layoutParent()->asFrameBox();
        }

        if (f) {
            windowRect = f->computeScreenExtent();
        }
    }

    return windowRect;
}

static LayoutRect computeWindowRectOnScreen(StackingContext* ctx)
{
    LayoutRect windowRect = computeScreenRect(ctx);

    if (!ctx->owner()
             ->node()
             ->document()
             ->browsingContext()
             ->isTopLevelBrowsingContext()) {
        FrameBox* f = ctx->owner();

        while (f) {
            if (f->isFrameReplaced() &&
                f->asFrameReplaced()->isFrameReplacedIFrame()) {
                break;
            }
            f = f->layoutParent()->asFrameBox();
        }

        if (f) {
            windowRect = f->computeScreenExtent();
        }
    }
    return windowRect;
}

bool canSkipFillGraphicsBufferDueToOpacityIsZero(
    StackingContext* stackingContext)
{
    STARFISH_ASSERT(stackingContext != nullptr);

    if (stackingContext->needsGraphicsBuffer() &&
        stackingContext->owner()->style()->opacity() == 0 &&
        !stackingContext->owner()->isRunningOpacityAnimation()) {
        return true;
    }

    return false;
}

bool StackingContext::fillGraphicsBufferContentsWithoutClipRect()
{
    if (needsGraphicsBuffer()) {
        if (canSkipFillGraphicsBufferDueToOpacityIsZero(this)) {
            return false;
        }

        LayoutRect visibleRect = StackingContext::visibleRect();
        LayoutUnit minX = visibleRect.x();
        LayoutUnit maxX = visibleRect.maxX();
        LayoutUnit minY = visibleRect.y();
        LayoutUnit maxY = visibleRect.maxY();
        size_t bufferWidth;
        size_t bufferHeight;
        computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY, bufferWidth,
                                         bufferHeight);

        if (bufferWidth && bufferHeight) {
            ensureRareData();
            if (!m_rareData->m_graphicsBufferHolder) {
                m_rareData->m_graphicsBufferHolder = new GraphicsBufferHolder(
                    bufferWidth, bufferHeight,
                    m_owner->node()->window()->innerWidth(),
                    m_owner->node()->window()->innerHeight(), this);
            }

            size_t wTileSize =
                m_rareData->m_graphicsBufferHolder->m_tileDataWidth;
            size_t hTileSize =
                m_rareData->m_graphicsBufferHolder->m_tileDataHeight;
            size_t wTextureCount =
                m_rareData->m_graphicsBufferHolder->m_horizontalTileCount;
            size_t hTextureCount =
                m_rareData->m_graphicsBufferHolder->m_verticalTileCount;

            size_t tileIndex = 0;
            size_t coveredRowsCount = 0;

            auto screenMatrix = m_owner->computeScreenMatrix(true);
            LayoutRect screenRect = computeScreenRect(this);
            LayoutRect windowRect = computeWindowRectOnScreen(this);
            Optional<LayoutRect> scrollRect;
            if (inScrollWithGraphicsBufferActive()) {
                scrollRect = computeBoxExtent(
                    LayoutRect(0, 0, m_owner->width(), m_owner->height()),
                    m_owner->computeScreenMatrix(false));
            }

            size_t hVisibleTextureStart = hTextureCount;
            size_t hVisibleTextureEnd = 0;
            size_t wVisibleTextureStart = wTextureCount;
            size_t wVisibleTextureEnd = 0;

            for (size_t y = 0; y < hTextureCount; y++) {
                size_t coveredColsCount = 0;
                for (size_t x = 0; x < wTextureCount; x++) {
                    size_t tileDataX = coveredColsCount;
                    size_t tileDataY = coveredRowsCount;
                    size_t tileDataWidth = std::min(
                        wTileSize,
                        m_rareData->m_graphicsBufferHolder->bufferWidth() -
                            coveredColsCount);
                    size_t tileDataHeight = std::min(
                        hTileSize,
                        m_rareData->m_graphicsBufferHolder->bufferHeight() -
                            coveredRowsCount);

                    LayoutRect tileExtent = computeBoxExtent(
                        LayoutRect(minX + (LayoutUnit)tileDataX,
                                   minY + (LayoutUnit)tileDataY, tileDataWidth,
                                   tileDataHeight),
                        screenMatrix);

                    bool willPaintOnScreen =
                        screenRect.intersects(tileExtent) &&
                        windowRect.intersects(tileExtent);

                    if (scrollRect) {
                        willPaintOnScreen =
                            willPaintOnScreen &&
                            scrollRect.value().intersects(tileExtent);
                    }

                    if (willPaintOnScreen) {
                        wVisibleTextureStart =
                            std::min(wVisibleTextureStart, x);
                        wVisibleTextureEnd =
                            std::max(wVisibleTextureEnd, x + 1);
                        hVisibleTextureStart =
                            std::min(hVisibleTextureStart, y);
                        hVisibleTextureEnd =
                            std::max(hVisibleTextureEnd, y + 1);
                    }

                    coveredColsCount += wTileSize;
                }
                coveredRowsCount += hTileSize;
            }

            Scrolling* scrolling = nullptr;
            if (m_owner->isRootElement()) {
                scrolling = m_owner->node()->window()->scrolling();
            } else {
                if (m_owner->node()->isElement() &&
                    m_owner->node()->asElement()->rareMembers()) {
                    scrolling = m_owner->node()
                                    ->asElement()
                                    ->rareMembers()
                                    ->asRareElementMembers()
                                    ->m_scrolling;
                }
            }

            size_t hEarlyPaintingTextureStart = hVisibleTextureStart;
            size_t hEarlyPaintingTextureEnd = hVisibleTextureEnd;
            size_t wEarlyPaintingTextureStart = wVisibleTextureStart;
            size_t wEarlyPaintingTextureEnd = wVisibleTextureEnd;

            if (scrolling) {
                if (scrolling->inVerticalScrollingDown()) {
                    hEarlyPaintingTextureEnd = hVisibleTextureEnd + 1;
                }
                if (hVisibleTextureStart != 0 &&
                    scrolling->inVerticalScrollingUp()) {
                    hEarlyPaintingTextureStart = hVisibleTextureStart - 1;
                }

                if (scrolling->inHorizontalScrollingRight()) {
                    wEarlyPaintingTextureEnd = wVisibleTextureEnd + 1;
                }
                if (wVisibleTextureStart != 0 &&
                    scrolling->inHorizontalScrollingLeft()) {
                    wEarlyPaintingTextureStart = wVisibleTextureStart - 1;
                }
            }

            if (m_owner->node()->isElement() &&
                m_owner->node()->isRunningTransformAnimation()) {
                struct Data {
                    Element* element;
                    size_t* hEarlyPaintingTextureStart;
                    size_t* hEarlyPaintingTextureEnd;
                    size_t* wEarlyPaintingTextureStart;
                    size_t* wEarlyPaintingTextureEnd;
                } d;
                d.element = m_owner->node()->asElement();
                d.hEarlyPaintingTextureStart = &hEarlyPaintingTextureStart;
                d.hEarlyPaintingTextureEnd = &hEarlyPaintingTextureEnd;
                d.wEarlyPaintingTextureStart = &wEarlyPaintingTextureStart;
                d.wEarlyPaintingTextureEnd = &wEarlyPaintingTextureEnd;

                m_owner->node()
                    ->document()
                    ->animationExecutor()
                    ->iterateAnimationTasks(
                        [](ActiveAnimationTask* task, void* data) {
                            Data* d = (Data*)data;
                            if (task->targetElement() == d->element &&
                                task->property() ==
                                    CSSStyleValuePair::KeyKind::Transform &&
                                task->fraction(d->element->document()
                                                   ->browsingContext()
                                                   ->styleResolveStartTick())) {
                                const auto& fromValue =
                                    ((ActiveTransformAnimationTask*)task)
                                        ->decomposedFrom();
                                const auto& toValue =
                                    ((ActiveTransformAnimationTask*)task)
                                        ->decomposedTo();

                                if (fromValue.translateX < toValue.translateX) {
                                    if (*d->wEarlyPaintingTextureStart != 0) {
                                        *d->wEarlyPaintingTextureStart =
                                            *d->wEarlyPaintingTextureStart - 1;
                                    }
                                }
                                if (fromValue.translateX > toValue.translateX) {
                                    *d->wEarlyPaintingTextureEnd =
                                        *d->wEarlyPaintingTextureEnd + 1;
                                }

                                if (fromValue.translateY < toValue.translateY) {
                                    if (*d->hEarlyPaintingTextureStart != 0) {
                                        *d->hEarlyPaintingTextureStart =
                                            *d->hEarlyPaintingTextureStart - 1;
                                    }
                                }
                                if (fromValue.translateY > toValue.translateY) {
                                    *d->hEarlyPaintingTextureEnd =
                                        *d->hEarlyPaintingTextureEnd + 1;
                                }
                            }
                        },
                        &d);
            }

            tileIndex = 0;
            coveredRowsCount = 0;

            for (size_t y = 0; y < hTextureCount; y++) {
                size_t coveredColsCount = 0;
                for (size_t x = 0; x < wTextureCount; x++) {
                    size_t tileDataX = coveredColsCount;
                    size_t tileDataY = coveredRowsCount;
                    size_t tileDataWidth = std::min(
                        wTileSize,
                        m_rareData->m_graphicsBufferHolder->tileBufferWidth() -
                            coveredColsCount);
                    size_t tileDataHeight = std::min(
                        hTileSize,
                        m_rareData->m_graphicsBufferHolder->tileBufferHeight() -
                            coveredRowsCount);

                    LayoutRect tileExtent = computeBoxExtent(
                        LayoutRect(minX + (LayoutUnit)tileDataX,
                                   minY + (LayoutUnit)tileDataY, tileDataWidth,
                                   tileDataHeight),
                        screenMatrix);

                    bool isVisible =
                        hVisibleTextureStart <= y && y < hVisibleTextureEnd &&
                        wVisibleTextureStart <= x && x < wVisibleTextureEnd;
                    bool isEarlyPainting = hEarlyPaintingTextureStart <= y &&
                                           y < hEarlyPaintingTextureEnd &&
                                           wEarlyPaintingTextureStart <= x &&
                                           x < wEarlyPaintingTextureEnd;

                    if (isVisible || isEarlyPainting) {
                        if (m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex] == nullptr) {
                            if (m_owner->document()
                                    ->webView()
                                    ->didFirstRenderingAfterWakeup() &&
                                !isVisible && isEarlyPainting) {
                                auto tick = longTickCount();
                                if (tick - m_owner->node()
                                               ->webView()
                                               ->lastRenderingTick() >
                                    (uint64_t)WebView::
                                            g_fillingGraphicsBufferTileFrameTimeLimitInMS *
                                        1000) {
                                    tileIndex++;
                                    continue;
                                }
                            }

                            CanvasSurface* canvasSurface =
                                CanvasSurface::create(
                                    m_owner->document()->webView()->renderer(),
                                    tileDataWidth, tileDataHeight,
                                    additionalPixelRatio(),
                                    computeSurfaceFlag(this));
                            Canvas* canvas = Canvas::create(
                                m_owner->node()->webView(), canvasSurface);

                            canvas->setTextDecorationData(
                                m_rareData->m_textDecorationData);

                            // give empty repaint region
                            // this stage. we will just filling empty tiles
                            // if
                            // needed
                            RepaintRegion rr;
                            StackingContext::PaintingStackingContextContext ctx(
                                true,
                                m_owner->node()
                                    ->webView()
                                    ->m_prevDrawnStackingContextInfo,
                                LayoutRect(0, 0, 0, 0), rr, 0, 0);
                            ctx.paintingForCompositingStartingFrom = this;

                            ctx.layerBaseX = tileDataX;
                            ctx.layerBaseY = tileDataY;

                            ctx.layerClipRect =
                                LayoutRect(0, 0, canvasSurface->width(),
                                           canvasSurface->height());

                            canvas->translate(-minX, -minY);
                            canvas->translate(-ctx.layerBaseX, -ctx.layerBaseY);

                            fillGraphicsBufferContents(canvas, ctx);

                            delete canvas;

                            canvasSurface->unmapBufferAndNotifyUpdatedRegion(
                                0, 0, canvasSurface->bufferWidth(),
                                canvasSurface->bufferHeight());

                            m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex] = canvasSurface;
                        }
                    } else {
                        if (m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex]) {
                            m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex]
                                ->detachNativeBuffer();
                            m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex] = nullptr;
                        }
                    }

                    tileIndex++;
                    coveredColsCount += wTileSize;
                }

                coveredRowsCount += hTileSize;
            }

            STARFISH_ASSERT(tileIndex == wTextureCount * hTextureCount);
        }
    }

    return false;
}

bool StackingContext::fillGraphicsBufferContents(
    PaintingStackingContextContext& globalCtx)
{
    if (!needsGraphicsBuffer()) {
        return false;
    }

    bool drawnSomething = false;
    LayoutRect visibleRect = StackingContext::visibleRect();
    LayoutUnit minX = visibleRect.x();
    LayoutUnit maxX = visibleRect.maxX();
    LayoutUnit minY = visibleRect.y();
    LayoutUnit maxY = visibleRect.maxY();

    size_t bufferWidth;
    size_t bufferHeight;
    computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY, bufferWidth,
                                     bufferHeight);

    if (canSkipFillGraphicsBufferDueToOpacityIsZero(this)) {
        return false;
    }

    if (bufferWidth == 0 || bufferHeight == 0) {
        return false;
    }

    LayoutRect deviceLayerClipRect;

    if (m_rareData->m_graphicsBufferHolder == nullptr ||
        m_rareData->m_graphicsBufferHolder->bufferWidth() != bufferWidth ||
        m_rareData->m_graphicsBufferHolder->bufferHeight() != bufferHeight ||
        m_rareData->m_graphicsBufferHolder->additionalPixelRatio() !=
            additionalPixelRatio()) {
        bool reuse = false;
        auto iter =
            globalCtx.prevDrawnStackingContextInfoMap.find(m_owner->node());
        if (iter != globalCtx.prevDrawnStackingContextInfoMap.end()) {
            if (iter->second.graphicsBufferHolder &&
                iter->second.graphicsBufferHolder->bufferWidth() ==
                    bufferWidth &&
                iter->second.graphicsBufferHolder->bufferHeight() ==
                    bufferHeight &&
                iter->second.graphicsBufferHolder->additionalPixelRatio() ==
                    additionalPixelRatio()) {
                reuse = true;
                m_rareData->m_graphicsBufferHolder =
                    iter->second.graphicsBufferHolder;
                iter.value().graphicsBufferHolder = nullptr;
            } else {
                if (iter->second.graphicsBufferHolder) {
                    iter->second.graphicsBufferHolder->flushSurfaces();
                    iter.value().graphicsBufferHolder = nullptr;
                }
            }
        }

        if (!reuse) {
            m_rareData->m_graphicsBufferHolder = new GraphicsBufferHolder(
                bufferWidth, bufferHeight,
                m_owner->node()->window()->innerWidth(),
                m_owner->node()->window()->innerHeight(), this);
        }
    } else {
        auto iter =
            globalCtx.prevDrawnStackingContextInfoMap.find(m_owner->node());
        if (iter != globalCtx.prevDrawnStackingContextInfoMap.end()) {
            iter.value().graphicsBufferHolder = nullptr;
        }
    }

    size_t wTileSize = m_rareData->m_graphicsBufferHolder->m_tileDataWidth;
    size_t hTileSize = m_rareData->m_graphicsBufferHolder->m_tileDataHeight;
    size_t wTextureCount =
        m_rareData->m_graphicsBufferHolder->m_horizontalTileCount;
    size_t hTextureCount =
        m_rareData->m_graphicsBufferHolder->m_verticalTileCount;

    auto screenMatrix = m_owner->computeScreenMatrix(true);

    LayoutRect screenRect = computeScreenRect(this);
    LayoutRect windowRect = computeWindowRectOnScreen(this);
    Optional<LayoutRect> scrollRect;
    if (inScrollWithGraphicsBufferActive()) {
        scrollRect = computeBoxExtent(
            LayoutRect(0, 0, m_owner->width(), m_owner->height()),
            m_owner->computeScreenMatrix(false));
    }
    size_t tileIndex = 0;
    size_t coveredRowsCount = 0;
    for (size_t y = 0; y < hTextureCount; y++) {
        size_t coveredColsCount = 0;
        for (size_t x = 0; x < wTextureCount; x++) {
            size_t tileDataX = coveredColsCount;
            size_t tileDataY = coveredRowsCount;
            size_t tileDataWidth =
                std::min(wTileSize,
                         m_rareData->m_graphicsBufferHolder->tileBufferWidth() -
                             coveredColsCount);
            size_t tileDataHeight = std::min(
                hTileSize,
                m_rareData->m_graphicsBufferHolder->tileBufferHeight() -
                    coveredRowsCount);

            LayoutRect tileExtent =
                computeBoxExtent(LayoutRect(minX + (LayoutUnit)tileDataX,
                                            minY + (LayoutUnit)tileDataY,
                                            tileDataWidth, tileDataHeight),
                                 screenMatrix);

            bool willPaintOnScreen = screenRect.intersects(tileExtent) &&
                                     windowRect.intersects(tileExtent);
            if (scrollRect) {
                willPaintOnScreen = willPaintOnScreen &&
                                    scrollRect.value().intersects(tileExtent);
            }

            LayoutRect layerClipRect = globalCtx.repaintRegion[owner()->node()];
            bool isOverlappedWithScreenClipRect =
                !layerClipRect.isEmpty() &&
                layerClipRect.intersects(LayoutRect(
                    (LayoutUnit)tileDataX + minX, (LayoutUnit)tileDataY + minY,
                    tileDataWidth, tileDataHeight));
            layerClipRect.setX(layerClipRect.x() - (LayoutUnit)tileDataX -
                               minX);
            layerClipRect.setY(layerClipRect.y() - (LayoutUnit)tileDataY -
                               minY);

            if (isOverlappedWithScreenClipRect && !willPaintOnScreen) {
                if (m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex]) {
                    m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex]
                        ->detachNativeBuffer();
                    m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex] =
                        nullptr;
                }
            } else if (willPaintOnScreen) {
                bool gotNewBuffer = false;
                if (!m_rareData->m_graphicsBufferHolder
                         ->m_surfaces[tileIndex]) {
                    m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex] =
                        CanvasSurface::create(
                            m_owner->document()->webView()->renderer(),
                            tileDataWidth, tileDataHeight,
                            additionalPixelRatio(), computeSurfaceFlag(this));
                    gotNewBuffer = true;
                }

                if (isOverlappedWithScreenClipRect || gotNewBuffer) {
                    CanvasSurface* canvasSurface =
                        m_rareData->m_graphicsBufferHolder
                            ->m_surfaces[tileIndex];
                    Canvas* canvas = Canvas::create(m_owner->node()->webView(),
                                                    canvasSurface);

                    canvas->setTextDecorationData(
                        m_rareData->m_textDecorationData);

                    StackingContext::PaintingStackingContextContext ctx(
                        true, globalCtx.prevDrawnStackingContextInfoMap,
                        globalCtx.screenClipRect, globalCtx.repaintRegion,
                        globalCtx.scrollX, globalCtx.scrollY);
                    ctx.paintingForCompositingStartingFrom = this;
                    ctx.layerBaseX = tileDataX;
                    ctx.layerBaseY = tileDataY;

                    bool needsInitialClip = false;
                    ctx.layerClipRect = LayoutRect(0, 0, canvasSurface->width(),
                                                   canvasSurface->height());

                    if (isOverlappedWithScreenClipRect && !gotNewBuffer) {
                        ctx.layerClipRect = layerClipRect;
                        needsInitialClip = true;

                        if (ctx.layerClipRect.x() <= 0 &&
                            ctx.layerClipRect.y() <= 0 &&
                            ctx.layerClipRect.maxX() >=
                                (int)canvasSurface->width() &&
                            ctx.layerClipRect.maxY() >=
                                (int)canvasSurface->height()) {
                            needsInitialClip = false;
                            ctx.layerClipRect =
                                LayoutRect(0, 0, canvasSurface->width(),
                                           canvasSurface->height());
                        }
                    } else if (isOverlappedWithScreenClipRect || gotNewBuffer) {
                    } else {
                        ctx.layerClipRect = LayoutRect(0, 0, 0, 0);
                        needsInitialClip = true;
                    }

                    if (needsInitialClip) {
                        deviceLayerClipRect =
                            canvas->pixelSnappedClip(ctx.layerClipRect);
                    } else {
                        deviceLayerClipRect =
                            LayoutRect(0, 0, canvasSurface->bufferWidth(),
                                       canvasSurface->bufferHeight());
                    }
                    canvas->translate(-minX, -minY);
                    canvas->translate(-ctx.layerBaseX, -ctx.layerBaseY);

                    ComputedStyle* cs = owner()->node()->style();
                    bool canSkipClear =
                        gotNewBuffer ||
                        (!isRootContext() && !isIFrameStackingContext() &&
                         !cs->backgroundColor()
                              .hasAlpha() && // if bg-color is solid
                         cs->outlineColor().isTransparent() && // no outline
                         !cs->boxShadow());                    // no shadow

                    if (!canSkipClear) {
                        if (needsInitialClip) {
                            canvas->clearColor(Unit::Color(0, 0, 0, 0));
                        } else if (!gotNewBuffer) {
                            memset(canvas->renderTargetInfo().m_buffer, 0,
                                   canvas->renderTargetInfo().m_stride *
                                       canvas->renderTargetInfo().m_height);
                        }
                    }

                    fillGraphicsBufferContents(canvas, ctx);

                    delete canvas;

                    deviceLayerClipRect = LayoutRect::overlappedRect(
                        deviceLayerClipRect,
                        LayoutRect(0, 0,
                                   (LayoutUnit)canvasSurface->bufferWidth(),
                                   (LayoutUnit)canvasSurface->bufferHeight()));

                    if ((bool)deviceLayerClipRect.width() ||
                        (bool)deviceLayerClipRect.height()) {
                        canvasSurface->unmapBufferAndNotifyUpdatedRegion(
                            (int)deviceLayerClipRect.x(),
                            (int)deviceLayerClipRect.y(),
                            (int)deviceLayerClipRect.width(),
                            (int)deviceLayerClipRect.height());
                    } else {
                        canvasSurface->unmapBufferAndNotifyUpdatedRegion(0, 0,
                                                                         0, 0);
                    }

                    drawnSomething = true;
                }
            }

            tileIndex++;
            coveredColsCount += wTileSize;
        }

        coveredRowsCount += hTileSize;
    }

    return drawnSomething;
}

void StackingContext::paintStackingContext(Canvas* canvas,
                                           PaintingStackingContextContext& ctx)
{
    if (needsGraphicsBuffer()) {
        ensureRareData()->m_textDecorationData = canvas->textDecorationData();
        return;
    }

    {
        // draw debug rect
        // canvas->save();
        // canvas->setFillColor(Color(0, 0, 255, 64));
        // canvas->drawRect(visibleRect);
        // canvas->restore();
    }

    float opacity = owner()->style()->opacity();

    if (opacity == 0) {
        // invisible from here
        return;
    }

    canvas->save();

    if (owner()->style()->mixBlendMode() != BlendMode::Normal) {
        canvas->setCompositeOperator(CanvasCompositeOperator::SourceOver,
                                     owner()->style()->mixBlendMode());
    }

    if (opacity != 1) {
        canvas->beginOpacityLayer(owner()->style()->opacity(), visibleRect());
    }

    {
        SkMatrix m = transformMatrix();

        if (!m.isIdentity()) {
            SkMatrix test;
            bool testResult = m_rareData->m_matrix.invert(&test);
            if (!testResult) {
                // ignorePaintingDueToInvalidMatrix
                if (owner()->style()->opacity() != 1) {
                    canvas->endOpacityLayer();
                }
                canvas->restore();
                return;
            }
            LayoutLocation to = transformOrigin();
            canvas->translate(to.x(), to.y());
            canvas->postMatrix(m);
            canvas->translate(-to.x(), -to.y());
        }
    }

    if (owner()->shouldResetTextDecoration()) {
        canvas->resetTextDecorationData();
    } else {
        canvas->mergeTextDecorationData(owner()->style());
    }

    if (owner()->style()->visibility() ==
        VisibilityValue::HiddenVisibilityValue) {
        canvas->setVisible(false);
    } else {
        canvas->setVisible(true);
    }

    if (owner()->isAbsolutePositioned()) {
        RectData* rect = owner()->style()->clip();
        if (rect) {
            canvas->clip(Unit::Rect(
                rect->left().numberData(), rect->top().numberData(),
                rect->right().numberData(), rect->bottom().numberData()));
        }
    }

    bool canRejectPainting;
    if (owner()->shouldApplyOverflow()) {
        canRejectPainting =
            canvas->canRejectPainting(m_owner->frameVisibleRect());
    } else {
        canRejectPainting =
            canvas->canRejectPainting(StackingContext::visibleRect());
    }

    applyMask(canvas, ctx);

    if (!canRejectPainting) {
        if (m_hasFilterEffect) {
            FilterContext filterContext(&canvas, this, ctx);
            m_owner->paintBackgroundAndBorders(canvas);
            filterContext.applyAllFilter(ctx);
        } else {
            m_owner->paintBackgroundAndBorders(canvas);
        }
    }

    // Within each stacking context, the following layers are painted in
    // back-to-front order:
    // the background and borders of the element forming the stacking context.
    if (isIFrameStackingContext()) {
        FrameBlockBox* document = m_owner->layoutParent()->asFrameBlockBox();
        FrameBox* iframeBox = m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->sourceElement()
                                  ->frame()
                                  ->asFrameBox();
        auto clipRect = iframeBox->makeRect(BoxValue::PaddingBoxBoxValue);
        HTMLIFrameElement* iframe =
            m_owner->node()->document()->browsingContext()->sourceElement();
        clipRect.setX(clipRect.x() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollX());
        clipRect.setY(clipRect.y() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollY());
        canvas->clip(clipRect);
        canvas->translate(iframeBox->borderLeft() + iframeBox->paddingLeft(),
                          iframeBox->borderTop() + iframeBox->paddingTop());

        if (needsGraphicsBuffer()) {
            canvas->save();
            canvas->resetMatrixAndClip();

            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
            canvas->restore();
        } else {
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
        }
    }

    bool wasTranslateAppliedDueToScroll = false;
    if (owner()->shouldApplyOverflow()) {
        canvas->clip(owner()->makeRect(BoxValue::PaddingBoxBoxValue));
        const LayoutRect rect(0, 0, m_owner->width(), m_owner->height());
        m_owner->applyBorderRadiusClippingIfNeeds(canvas, rect);
        if (m_owner->isFrameBlockBox()) {
            wasTranslateAppliedDueToScroll = true;
            canvas->save();
            canvas->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                              -m_owner->asFrameBlockBox()->scrollTop());
        }
    }

    // the child stacking contexts with negative stack levels (most negative
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                break;
            }
            auto iter2 = child->begin();
            while (iter2 != child->end()) {
                StackingContext* sCtx = *iter2;
                ComputeOverflow<Canvas> r(canvas, sCtx, m_owner, ctx);
                sCtx->paintStackingContext(canvas, ctx);
                iter2++;
            }
            iter++;
        }
    }

    if (!canRejectPainting) {
        if (m_hasFilterEffect) {
            FilterContext filterContext(&canvas, this, ctx);
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
            filterContext.applyAllFilter(ctx);
        } else {
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
        }
    }

    // the child stacking contexts with positive stack levels (least positive
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                auto iter2 = child->begin();
                while (iter2 != child->end()) {
                    StackingContext* sCtx = *iter2;
                    ComputeOverflow<Canvas> r(canvas, sCtx, m_owner, ctx);
                    sCtx->paintStackingContext(canvas, ctx);
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (wasTranslateAppliedDueToScroll) {
        canvas->restore();
    }

    if (opacity != 1) {
        canvas->endOpacityLayer();
    }

    paintScrollbar(canvas);

    canvas->restore();
}

void StackingContext::paintScrollbar(Canvas* canvas)
{
    if (isIFrameStackingContext()) {
        HTMLIFrameElement* iframe =
            m_owner->node()->document()->browsingContext()->sourceElement();
        if (!iframe->scrolling()->toASCIILower()->equals("no")) {
            canvas->save();
            canvas->translate(m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->window()
                                  ->scrollX(),
                              m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->window()
                                  ->scrollY());
            FrameBlockBox* document =
                m_owner->layoutParent()->asFrameBlockBox();
            if (!needsGraphicsBuffer()) {
                Scrolling::paintScrollbars<Canvas*>(
                    m_owner->node()
                        ->document()
                        ->browsingContext()
                        ->window()
                        ->scrolling(),
                    canvas, document, document->appliedOverflowX(),
                    document->appliedOverflowY());
            }
            canvas->restore();
        }
    } else if (!isRootContext()) {
        if (m_owner->shouldApplyOverflow() && m_owner->node() &&
            m_owner->node()->isElement() && m_owner->isFrameBlockBox() &&
            !needsComposite()) {
            Scrolling::paintScrollbars<Canvas*>(
                m_owner->node()->asElement()->rareMembers()
                    ? m_owner->node()->asElement()->rareMembers()->m_scrolling
                    : nullptr,
                canvas, m_owner->asFrameBlockBox(), m_owner->appliedOverflowX(),
                m_owner->appliedOverflowY());
        }
    }
}

void StackingContext::compositeScrollbar(Compositor* compositor)
{
    if (isIFrameStackingContextOwner()) {
        if (m_childContexts.size()) {
            StackingContext* childCtx = m_childContexts[0]->at(0);
            if (childCtx->needsGraphicsBuffer()) {
                auto bc = m_owner->node()
                              ->asHTMLIFrameElement()
                              ->contentDocument()
                              ->browsingContext();
                compositor->translate(
                    m_owner->borderLeft() + m_owner->paddingLeft(),
                    m_owner->borderTop() + m_owner->paddingTop());
                FrameBlockBox* mainFrame =
                    bc->document()->frame()->asFrameBlockBox();
                Scrolling::paintScrollbars<Compositor*>(
                    m_owner->node()
                        ->document()
                        ->browsingContext()
                        ->window()
                        ->scrolling(),
                    compositor, mainFrame, mainFrame->appliedOverflowX(),
                    mainFrame->appliedOverflowY());
            }
        }
    } else if (!isRootContext()) {
        if (needsToDrawScrollbar()) {
            Scrolling::paintScrollbars<Compositor*>(
                m_owner->node()->asElement()->rareMembers()
                    ? m_owner->node()->asElement()->rareMembers()->m_scrolling
                    : nullptr,
                compositor, m_owner->asFrameBlockBox(),
                m_owner->appliedOverflowX(), m_owner->appliedOverflowY());
        }
    }
}

void StackingContext::compositeStackingContext(Compositor* compositor)
{
    STARFISH_ASSERT(compositor != nullptr);
    STARFISH_ASSERT(needsComposite());

    LayoutRect visibleRect = StackingContext::visibleRect();
    LayoutUnit minX = visibleRect.x();
    LayoutUnit maxX = visibleRect.maxX();
    LayoutUnit minY = visibleRect.y();
    LayoutUnit maxY = visibleRect.maxY();

    auto screenMatrix = m_owner->computeScreenMatrix(true);
    LayoutRect screenRect = computeScreenRect(this);
    LayoutRect stackingContextExtent =
        computeBoxExtent(visibleRect, screenMatrix);

    if (!stackingContextExtent.intersects(screenRect)) {
        bool needsCompositeAnyWay = false;
        if (inScrollWithGraphicsBufferActive()) {
            auto clr = owner()->style()->backgroundColor();
            if (!clr.isTransparent()) {
                needsCompositeAnyWay = true;
            }
        }
        if (owner()->contentSurface()) {
            needsCompositeAnyWay = true;
        }
        if (!needsCompositeAnyWay) {
            return;
        }
    }

    size_t bufferWidth;
    size_t bufferHeight;
    computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY, bufferWidth,
                                     bufferHeight);

    bool thereIsNoBufferBecauseThereIsNoVisibleContent =
        !bufferWidth || !bufferHeight;

    ComputedStyle* ownerStyle = m_owner->style();
    STARFISH_ASSERT(ownerStyle != nullptr);

    FrameBox* parentBox = parent() ? parent()->owner() : nullptr;

    if (isIFrameStackingContextOwner()) {
        if (m_childContexts.size()) {
            StackingContext* childCtx = m_childContexts[0]->at(0);
            STARFISH_ASSERT(childCtx != nullptr);

            auto bc = m_owner->node()
                          ->asHTMLIFrameElement()
                          ->contentDocument()
                          ->browsingContext();
            auto bgColor = bc->hasWindowBackgroundColor();

            if (bgColor.first || visibleRect.isEmpty()) {
                if (bgColor.second.a()) {
                    ComputeOverflow<Compositor> r(compositor, this,
                                                  parent()->owner());
                    SkMatrix test;
                    if (r.canvasOrCompositor()->currentTransformMatrix().invert(
                            &test)) {
                        compositor->save();
                        compositor->setFillColor(bgColor.second);
                        compositor->drawRect(LayoutRect(
                            m_owner->borderLeft() + m_owner->paddingLeft(),
                            m_owner->borderTop() + m_owner->paddingTop(),
                            m_owner->contentWidth(), m_owner->contentHeight()));
                        compositor->restore();
                    }
                }
            }
        }
    }

    ComputeOverflow<Compositor> r(compositor, this, parentBox);

    // If current matrix is invalid, we could not composite StackckingContext
    SkMatrix test;
    if (!r.canvasOrCompositor()->currentTransformMatrix().invert(&test)) {
        return;
    }

    auto contentSurface = owner()->contentSurface();
    if (!thereIsNoBufferBecauseThereIsNoVisibleContent || contentSurface) {
        owner()->willCompsiteStackingContext(compositor);

        bool hasFilterEffect = false;
        if (Compositor::supportsFilterEffect(m_owner->document()->starfish(),
                                             bufferWidth, bufferHeight) &&
            m_hasFilterEffect) {
            hasFilterEffect = true;
            Length standardDeviation;
            float maxRadiusOffset = 0;
            if (ownerStyle->hasAvailableFilter() &&
                ownerStyle->filter()->getStandardDeviationOfBlurFilter(
                    standardDeviation)) {
                maxRadiusOffset =
                    std::max(maxRadiusOffset, (standardDeviation.numberData()));
            }

            for (auto ancestor : ancestorsThatHasFilters()) {
                auto s = ancestor->owner()->style();
                if (s->filter()->getStandardDeviationOfBlurFilter(
                        standardDeviation)) {
                    maxRadiusOffset = std::max(
                        maxRadiusOffset, (standardDeviation.numberData()));
                }
            }

            if (maxRadiusOffset) {
                compositor->save();
                compositor->enableBlurEffect(maxRadiusOffset);
            }
        }

        if (m_rareData->m_graphicsBufferHolder) {
            compositor->save();
            if (m_owner->isFrameBlockBox() && m_owner->shouldApplyOverflow()) {
                Unit::Rect fullRect;

                if (needsRepaintingWhenScrolling()) {
                    fullRect = m_owner->makeRect(BoxValue::BorderBoxBoxValue);
                } else {
                    fullRect = m_owner->makeRect(BoxValue::PaddingBoxBoxValue);
                }

                if (inScrollWithGraphicsBufferActive()) {
                    auto clr = owner()->style()->backgroundColor();
                    if (!clr.isTransparent()) {
                        compositor->save();
                        compositor->setFillColor(clr);
                        compositor->drawRect(fullRect);
                        compositor->restore();
                    }
                }

                if (needsRepaintingWhenScrolling()) {
                    auto outline = owner()->style()->outline();
                    if (outline && outline->isVisible()) {
                        fullRect.setX(fullRect.x() -
                                      owner()->outlineThickness());
                        fullRect.setY(fullRect.y() -
                                      owner()->outlineThickness());
                        fullRect.setWidth(fullRect.width() +
                                          owner()->outlineThickness() * 2);
                        fullRect.setHeight(fullRect.height() +
                                           owner()->outlineThickness() * 2);
                    }
                    if (owner()->style()->boxShadow()) {
                        CanvasShadowDataList list =
                            owner()
                                ->style()
                                ->boxShadow()
                                ->toCanvasShadowDataList(owner());
                        LayoutRect ownerRect = owner()->frameRect();
                        LayoutRect shadowRect;
                        ownerRect.setX(0);
                        ownerRect.setY(0);
                        for (auto shadow = list.rbegin(); shadow != list.rend();
                             shadow++) {
                            LayoutRect rect =
                                computeVisibleShadowRect(ownerRect, *shadow);
                            shadowRect.unite(rect);
                        }
                        fullRect.unite(Unit::Rect(
                            shadowRect.x(), shadowRect.y(), shadowRect.width(),
                            shadowRect.height()));
                    }
                }

                compositor->clip(fullRect);

                compositor->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                                      -m_owner->asFrameBlockBox()->scrollTop());
            }
            size_t wTileSize =
                m_rareData->m_graphicsBufferHolder->m_tileDataWidth;
            size_t hTileSize =
                m_rareData->m_graphicsBufferHolder->m_tileDataHeight;
            size_t wTextureCount =
                m_rareData->m_graphicsBufferHolder->m_horizontalTileCount;
            size_t hTextureCount =
                m_rareData->m_graphicsBufferHolder->m_verticalTileCount;

            size_t tileIndex = 0;
            size_t coveredRowsCount = 0;
            float additionalPixelRatio = this->additionalPixelRatio();

            compositor->translate(minX, minY);

            for (size_t y = 0; y < hTextureCount; y++) {
                size_t coveredColsCount = 0;
                for (size_t x = 0; x < wTextureCount; x++) {
                    size_t tileDataX = coveredColsCount;
                    size_t tileDataY = coveredRowsCount;
                    size_t tileDataWidth = std::min(
                        wTileSize,
                        m_rareData->m_graphicsBufferHolder->tileBufferWidth() -
                            coveredColsCount);
                    size_t tileDataHeight = std::min(
                        hTileSize,
                        m_rareData->m_graphicsBufferHolder->tileBufferHeight() -
                            coveredRowsCount);

                    LayoutRect tileExtent = computeBoxExtent(
                        LayoutRect(minX + (LayoutUnit)tileDataX,
                                   minY + (LayoutUnit)tileDataY, tileDataWidth,
                                   tileDataHeight),
                        screenMatrix);

                    bool willPaintOnScreen = screenRect.intersects(tileExtent);
                    if (!willPaintOnScreen) {
                        tileIndex++;
                        coveredColsCount += wTileSize;
                        continue;
                    }

                    float tx = tileDataX / additionalPixelRatio;
                    float ty = tileDataY / additionalPixelRatio;
                    float w = tileDataWidth / additionalPixelRatio;
                    float h = tileDataHeight / additionalPixelRatio;

                    if (tileIndex < m_rareData->m_graphicsBufferHolder
                                        ->m_surfaces.size() &&
                        m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex] != nullptr) {
                        compositor->drawSurface(
                            m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex],
                            Unit::Rect(tx, ty, w, h));
#ifdef STARFISH_ENABLE_TEST
                        if (UNLIKELY(owner()->node()->webView()->startUpFlag() &
                                     StarfishStartUpFlag::
                                         enableDebugGraphicsLayer)) {
                            compositor->save();
                            compositor->setFillColor(
                                Unit::Color(255, 64, 0, 64));
                            compositor->drawRect(Unit::Rect(tx, ty, 1, h));
                            compositor->drawRect(Unit::Rect(tx, ty, w, 1));
                            compositor->drawRect(
                                Unit::Rect(tx, ty + h - 1, w, 1));
                            compositor->drawRect(
                                Unit::Rect(tx + w - 1, ty, 1, h));
                            compositor->restore();
                        }
#endif
                    }
                    tileIndex++;
                    coveredColsCount += wTileSize;
                }

                coveredRowsCount += hTileSize;
            }

            compositor->restore();
        }

        if (contentSurface) {
            compositor->save();

            // there is only background-color on video or canvas element, we
            // should not make graphics buffer since we can draw background
            // color property with compositor
            if (thereIsNoBufferBecauseThereIsNoVisibleContent) {
                auto clr = owner()->style()->backgroundColor();
                if (!clr.isTransparent()) {
                    compositor->setFillColor(clr);
                    compositor->drawRect(
                        LayoutRect(0, 0, owner()->width(), owner()->height()));
                }
            }
            auto dx = owner()->borderLeft() + owner()->paddingLeft();
            auto dy = owner()->borderTop() + owner()->paddingTop();
            compositor->translate(dx, dy);

            compositor->drawSurface(contentSurface.getValue(),
                                    Unit::Rect(0, 0, owner()->contentWidth(),
                                               owner()->contentHeight()));
            compositor->restore();
        }

#ifdef STARFISH_ENABLE_TEST
        if (UNLIKELY(owner()->node()->webView()->startUpFlag() &
                     StarfishStartUpFlag::enableDebugGraphicsLayer)) {
            // debug compositing method
            switch (m_needsGraphicsBufferReason) {
            case NeedsGraphicsLayerReasonNone:
                compositor->setFillColor(Unit::Color(255, 64, 0, 64));
                break;
            case NeedsGraphicsLayerReasonBySelf:
                compositor->setFillColor(Unit::Color(255, 0, 0, 64));
                break;
            case NeedsGraphicsLayerReasonNotCoveredByParent:
                compositor->setFillColor(Unit::Color(0, 255, 0, 64));
                break;
            case NeedsGraphicsLayerReasonCollapsedWithSiblingLayer:
                compositor->setFillColor(Unit::Color(0, 0, 255, 64));
                break;
            case NeedsGraphicsLayerReasonSiblingLayerNeedsAnimation:
                compositor->setFillColor(Unit::Color(0, 255, 255, 64));
                break;
            case NeedsGraphicsLayerReasonNeedsScroll:
                compositor->setFillColor(Unit::Color(255, 0, 255, 64));
                break;
            default:
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
            auto rt = Unit::Rect(minX, minY, bufferWidth, bufferHeight);
            compositor->beginOpacityLayer(0.5, rt);
            compositor->drawRect(rt);
            compositor->endOpacityLayer();
        }

        if (UNLIKELY(owner()->node()->webView()->startUpFlag() &
                     StarfishStartUpFlag::enableDebugRepaintRegion)) {
            auto iter =
                owner()->node()->webView()->repaintRegionInRendering().find(
                    owner()->node());

            if (iter !=
                owner()->node()->webView()->repaintRegionInRendering().end()) {
                compositor->beginOpacityLayer(0.5, iter->second);
                compositor->setFillColor(Unit::Color(0, 255, 0, 64));
                compositor->drawRect(iter->second);
                compositor->endOpacityLayer();
            }
        }
#endif

        if (hasFilterEffect) {
            compositor->restore();
        }

        owner()->didCompsiteStackingContext(compositor);
    }

    compositeScrollbar(compositor);
}

LayoutLocation StackingContext::relativeLocation(StackingContext* sCtx)
{
    LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
    bool isFixed = sCtx->owner()->style()->position() == FixedPositionValue;

    if (isFixed) {
        Frame* parent = sCtx->owner()->layoutParent();
        while ((!parent->style() || !parent->style()->hasTransforms(parent)) &&
               !parent->isFrameDocument() &&
               (isFixed || !parent->isPositioned())) {
            parent = parent->layoutParent();
        }

        if (isFixed && parent->isFrameBlockBox()) {
            l.setX(l.x() + parent->asFrameBlockBox()->scrollLeft());
            l.setY(l.y() + parent->asFrameBlockBox()->scrollTop());
        }
    } else {
        FrameBox* box = sCtx->owner()->layoutParent()->asFrameBox();
        while (!box->stackingContext()) {
            if (box->isFrameBlockBox()) {
                l.setX(l.x() - box->asFrameBlockBox()->scrollLeft());
                l.setY(l.y() - box->asFrameBlockBox()->scrollTop());
            }

            box = box->layoutParent()->asFrameBox();
        }
    }

    return l;
}

Frame* StackingContext::hitTestStackingContext(LayoutUnit x, LayoutUnit y,
                                               BrowsingContext* from)
{
    SkMatrix m = transformMatrix();
    if (!m.isIdentity()) {
        SkMatrix invert;
        if (!m.invert(&invert)) {
            return nullptr;
        }

        auto to = transformOrigin();
        LayoutUnit ox = to.x();
        LayoutUnit oy = to.y();
        x -= ox;
        y -= oy;
        SkPoint pt = SkPoint::Make((float)x, (float)y);
        invert.mapPoints(&pt, 1);
        x = pt.x() + ox;
        y = pt.y() + oy;
    }

    if (!m_owner->isAnonymous() &&
        m_owner->node()->document()->browsingContext() != from) {
        if (m_owner->FrameBox::hitTest(x, y, HitTestStageEnd)) {
            return m_owner->node()
                ->document()
                ->browsingContext()
                ->sourceElement()
                ->frame();
        } else {
            return nullptr;
        }
    }

    if (owner()->style()->visibility() ==
        VisibilityValue::HiddenVisibilityValue) {
        return nullptr;
    }

    if (owner()->shouldApplyOverflow()) {
        LayoutRect fr = owner()->asFrameBox()->frameRect();
        if (owner()->style()->overflowX() == OverflowValue::HiddenOverflow &&
            (x < 0 || x >= fr.width())) {
            return nullptr;
        }
        if (owner()->style()->overflowY() == OverflowValue::HiddenOverflow &&
            (y < 0 || y >= fr.height())) {
            return nullptr;
        }
        if (owner()->isFrameReplaced()) {
            return owner()->hitTest(x, y, HitTestStageEnd);
        }
    }

    if (m_owner->isFrameBlockBox()) {
        x += m_owner->asFrameBlockBox()->scrollLeft();
        y += m_owner->asFrameBlockBox()->scrollTop();
    }

    Frame* result = nullptr;
    // the child stacking contexts with positive stack levels (least positive
    // first).
    {
        auto iter = childContexts().rbegin();
        while (iter != childContexts().rend()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                auto iter2 = child->rbegin();
                LayoutUnit oldX = x;
                LayoutUnit oldY = y;
                while (iter2 != child->rend()) {
                    StackingContext* sCtx = *iter2;
                    LayoutLocation l = relativeLocation(sCtx);
                    x -= l.x();
                    y -= l.y();
                    result = sCtx->hitTestStackingContext(x, y, from);
                    x = oldX;
                    y = oldY;
                    if (result) {
                        // TODO test overflow between sCtx and this
                        return result;
                    }

                    iter2++;
                }
            }
            iter++;
        }
    }

    // the child stacking contexts with stack level 0 and the positioned
    // descendants with stack level 0.
    result = m_owner->hitTestChildrenWith(x, y, HitTestPositionedElements);
    if (result) {
        return result;
    }

    // the in-flow, inline-level, non-positioned descendants, including inline
    // tables and inline blocks.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNormalFlowInline);
    if (result) {
        return result;
    }

    // the non-positioned float.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNonPositionedFloats);
    if (result) {
        return result;
    }

    // the in-flow, non-inline-level, non-positioned descendants.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNormalFlowBlock);
    if (result) {
        return result;
    }

    // the child stacking contexts with negative stack levels (most negative
    // first).
    {
        auto iter = childContexts().rbegin();
        while (iter != childContexts().rend()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num > 0) {
                break;
            }
            auto iter2 = child->rbegin();
            LayoutUnit oldX = x;
            LayoutUnit oldY = y;
            while (iter2 != child->rend()) {
                StackingContext* sCtx = *iter2;
                LayoutLocation l = relativeLocation(sCtx);
                x -= l.x();
                y -= l.y();
                result = sCtx->hitTestStackingContext(x, y, from);
                if (result) {
                    return result;
                }

                x = oldX;
                y = oldY;
                iter2++;
            }
            iter++;
        }
    }

    if (m_owner->isFrameBlockBox()) {
        x -= m_owner->asFrameBlockBox()->scrollLeft();
        y -= m_owner->asFrameBlockBox()->scrollTop();
    }

    // the background and borders of the element forming the stacking context.
    result = m_owner->FrameBox::hitTest(x, y, HitTestNormalFlowBlock);
    if (result) {
        return result;
    }

    return nullptr;
}

void StackingContext::applyMask(Canvas* canvas,
                                PaintingStackingContextContext& ctx)
{
    if (m_owner->isFrameSVGBox()) {
        return;
    }

    if (m_owner->style() == nullptr || m_owner->style()->maskLayerSize() == 0) {
        return;
    }

    auto style = m_owner->style();
    auto document = m_owner->document();
    for (uint32_t i = 0; i < m_owner->style()->maskLayerSize(); i++) {
        if (style->maskImage(i) == nullptr) {
            continue;
        }

        PositionedMaskData* maskStyle =
            style->rareComputedStyleData()->positionedMask();

        FrameBox* box = m_owner;
        auto type = style->maskImage(i)->type();
        NativeImageData* id = nullptr;
        float width = 0;
        float height = 0;
        unsigned int idx = i;
        if (type.isURL()) {
            STARFISH_ASSERT(maskStyle != nullptr);

            ImageResource* ir = maskStyle->imageResource(i);

            if (!ir) {
                return;
            }
            if (box->node() != nullptr) {
                box->node()->webView()->putURLIntoActiveImageURLsInRenderingSet(
                    ir->url()->urlString()->toUTF8NonGCString());
            }

            id = ir->imageData();
            if (id == nullptr || id->width() == 0 || id->height() == 0) {
                return;
            }

            if (id->isSVGNativeImageData() &&
                !id->asSVGNativeImageData()->hasViewport()) {
                id->asSVGNativeImageData()->updateContentSize(box);
            }

            width = id->width();
            height = id->height();
        } else if (type.isGradient()) {
            ImageValue* imageValue = style->maskImage(i);

            if (!imageValue->gradientValue()->isEffective()) {
                continue;
            }

            Unit::Rect rect;
            rect = box->makeRect(BoxValue::PaddingBoxBoxValue);
            width = rect.width();
            height = rect.height();
        } else {
            continue;
        }

        Unit::Rect paintingRect;
        Unit::Rect positioningRect;

        if (box->isFrameBlockBox()) {
            FrameBox scrollBox(box->node(), style);
            scrollBox.copyFrom(box,
                               FrameBox::BorderCopy | FrameBox::PaddingCopy);
            scrollBox.setWidth(box->asFrameBlockBox()->scrollWidth());
            scrollBox.setHeight(box->asFrameBlockBox()->scrollHeight());

            positioningRect = scrollBox.makeRect(BoxValue::PaddingBoxBoxValue);
            positioningRect.setX(positioningRect.x() -
                                 box->asFrameBlockBox()->scrollLeft());
            positioningRect.setY(positioningRect.x() -
                                 box->asFrameBlockBox()->scrollTop());
            paintingRect = scrollBox.makeRect(BoxValue::BorderBoxBoxValue);
        } else {
            positioningRect = box->makeRect(BoxValue::PaddingBoxBoxValue);
            paintingRect = box->makeRect(BoxValue::BorderBoxBoxValue);
        }
        canvas->translate(paintingRect.x(), paintingRect.y());
        canvas->clip(
            Unit::Rect(0, 0, paintingRect.width(), paintingRect.height()));

        float positionW = positioningRect.width();
        float positionH = positioningRect.height();
        float paintingW = paintingRect.width();
        float paintingH = paintingRect.height();
        float imgW = positionW;
        float imgH = positionH;

        float boxR = positionW / positionH;
        float imgR = width / height;
        float hasSpecifiedSize = false;

        if (maskStyle->maskSizeIsLength(i)) {
            LengthSize bgSize = maskStyle->maskSizeLengthValue(i);
            if (bgSize.width().isAuto() && bgSize.height().isAuto()) {
                imgW = width;
                imgH = height;
            } else if (bgSize.width().isAuto() && !bgSize.height().isAuto()) {
                hasSpecifiedSize = true;
                imgH = bgSize.height().specifiedValue(positionH, box);
                imgW = imgH * width / height;
            } else if (!bgSize.width().isAuto() && bgSize.height().isAuto()) {
                hasSpecifiedSize = true;
                imgW = bgSize.width().specifiedValue(positionW, box);
                imgH = imgW * height / width;
            } else {
                hasSpecifiedSize = true;
                imgW = bgSize.width().specifiedValue(positionW, box);
                imgH = bgSize.height().specifiedValue(positionH, box);
            }
        } else {
            BackgroundSizeValue bgSize = maskStyle->maskSizeTypeValue(i);
            if (bgSize == BackgroundSizeValue::CoverBackgroundSizeValue) {
                if (boxR < imgR) {
                    imgW = positionH * imgR;
                } else {
                    imgH = positionW / imgR;
                }
            } else {
                STARFISH_ASSERT(
                    bgSize == BackgroundSizeValue::ContainBackgroundSizeValue);
                if (boxR > imgR) {
                    imgW = positionH * imgR;
                } else {
                    imgH = positionW / imgR;
                }
            }
        }

        Length positionX = maskStyle->positionX(i);
        Length positionY = maskStyle->positionY(i);
        LayoutUnit x;
        LayoutUnit y;

        if (positionX.isSpecified()) {
            x = positionX.specifiedValue(positionW - imgW, box) +
                positioningRect.x() - paintingRect.x();
        }

        if (positionY.isSpecified()) {
            y = positionY.specifiedValue(positionH - imgH, box) +
                positioningRect.y() - paintingRect.y();
        }

        auto repeatX = maskStyle->repeatX(i);
        auto repeatY = maskStyle->repeatY(i);

        bool shouldApplyRepeat = type.isGradient() ? hasSpecifiedSize : true;

        Unit::Rect rect =
            m_owner->makeRect(BoxValue::BorderBoxBoxValue).snapSizeToPixel();
        auto imageData =
            BufferedNativeImageData::create(rect.width(), rect.height());
        Canvas* maskCanvas =
            Canvas::create(m_owner->node()->webView(), imageData);
        maskCanvas->clearColor(Unit::Color(0, 0, 0, 0));
        maskCanvas->save();

        if (shouldApplyRepeat &&
            (repeatX == RepeatStyleValue::RepeatRepeatValue &&
             repeatY == RepeatStyleValue::RepeatRepeatValue)) {
            if (type.isURL()) {
                if (positioningRect.x() == paintingRect.x() &&
                    positioningRect.y() == paintingRect.y() &&
                    paintingW == imgW && paintingH == imgH) {
                    maskCanvas->drawImage(id, Unit::Rect(x, y, imgW, imgH));
                } else {
                    maskCanvas->drawRepeatImage(
                        id, Unit::Rect(x, y, paintingW, paintingH), imgW, imgH,
                        true, true);
                }
            } else if (type.isGradient()) {
                FrameBox::paintGradient(
                    maskCanvas, box, style->maskImage(idx),
                    Unit::Rect(x, y, paintingW, paintingH), imgW, imgH, true,
                    true, ImageRenderingValue::ImageRenderingAutoValue);
            }
        } else if (shouldApplyRepeat &&
                   repeatX == RepeatStyleValue::NoRepeatRepeatValue &&
                   repeatY == RepeatStyleValue::RepeatRepeatValue) {
            if (type.isURL()) {
                maskCanvas->drawRepeatImage(id,
                                            Unit::Rect(x, y, imgW, paintingH),
                                            imgW, imgH, false, true);
            } else if (type.isGradient()) {
                FrameBox::paintGradient(
                    maskCanvas, box, style->maskImage(idx),
                    Unit::Rect(x, y, imgW, paintingH), imgW, imgH, false, true,
                    ImageRenderingValue::ImageRenderingAutoValue);
            }

        } else if (shouldApplyRepeat &&
                   repeatX == RepeatStyleValue::RepeatRepeatValue &&
                   repeatY == RepeatStyleValue::NoRepeatRepeatValue) {
            if (type.isURL()) {
                maskCanvas->drawRepeatImage(id,
                                            Unit::Rect(x, y, paintingW, imgH),
                                            imgW, imgH, true, false);
            } else if (type.isGradient()) {
                FrameBox::paintGradient(
                    maskCanvas, box, style->maskImage(idx),
                    Unit::Rect(x, y, paintingW, imgH), imgW, imgH, true, false,
                    ImageRenderingValue::ImageRenderingAutoValue);
            }
        } else {
            if (type.isURL()) {
                maskCanvas->drawImage(id, Unit::Rect(x, y, imgW, imgH));
            } else if (type.isGradient()) {
                FrameBox::paintGradient(
                    maskCanvas, box, style->maskImage(idx),
                    Unit::Rect(x, y, imgW, imgH), imgW, imgH, false, false,
                    ImageRenderingValue::ImageRenderingAutoValue);
            }
        }
        maskCanvas->fill();
        maskCanvas->restore();
        delete maskCanvas;
        canvas->maskNativeImage(imageData, rect);
    }
}

} // namespace Starfish
