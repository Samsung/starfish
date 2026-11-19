
/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "core/style/CSSTokenValue.h"
#include "core/style/CSSParser.h"
#include "core/style/Style.h"
#include "core/style/FilterFunctions.h"
#include "core/style/ComputedStyle.h"
#include "core/modules/threading/ParallelJobExecutor.h"
#include "core/modules/canvas/ShadowBlur.h"

#define ENABLE_PARALLEL_BLUR 0

namespace Starfish {

FilterFunction* FilterFunction::create(const CSSFilterFunction& from)
{
    FilterFunctionType type = from.type();
    switch (type) {
    case FilterFunctionType::BlurFilterFunctionType:
        return new BlurFilterFunction(from);
    case FilterFunctionType::SVGUrlFilterFunctionType:
        return new SVGUrlFilterFunction(from);
    case FilterFunctionType::DropShadowFilterFunctionType:
        return new DropShadowFilterFunction(from);
    case FilterFunctionType::HueRotateFilterFunctionType:
    case FilterFunctionType::BrightnessFilterFunctionType:
    case FilterFunctionType::ContrastFilterFunctionType:
    case FilterFunctionType::GrayScaleFilterFunctionType:
    case FilterFunctionType::InvertFilterFunctionType:
    case FilterFunctionType::OpacityFilterFunctionType:
    case FilterFunctionType::SaturateFilterFunctionType:
    case FilterFunctionType::SepiaFilterFunctionType:
        return new ColorFilterFunction(from);
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

static float colorFilterAmount(const CSSFilterFunction& from)
{
    const CSSStyleValuePair& v = from.data();
    switch (v.valueKind()) {
    case CSSStyleValuePair::ValueKind::Angle:
        return v.angleValue().toDegreeValue();
    case CSSStyleValuePair::ValueKind::Percentage:
        return v.percentageValue();
    default:
        return v.numberValue();
    }
}

ColorFilterFunction::ColorFilterFunction(FilterFunctionType type, float amount)
    : FilterFunction(type)
    , m_amount(amount)
{
    switch (type) {
    case FilterFunctionType::GrayScaleFilterFunctionType:
    case FilterFunctionType::InvertFilterFunctionType:
    case FilterFunctionType::OpacityFilterFunctionType:
    case FilterFunctionType::SepiaFilterFunctionType:
        // Values over 100% are allowed but UAs must clamp them to 1.
        m_amount = std::min(m_amount, 1.f);
        break;
    default:
        break;
    }
}

ColorFilterFunction::ColorFilterFunction(const CSSFilterFunction& from)
    : ColorFilterFunction(from.type(), colorFilterAmount(from))
{
}

float ColorFilterFunction::identityAmount(FilterFunctionType type)
{
    switch (type) {
    case FilterFunctionType::BrightnessFilterFunctionType:
    case FilterFunctionType::ContrastFilterFunctionType:
    case FilterFunctionType::OpacityFilterFunctionType:
    case FilterFunctionType::SaturateFilterFunctionType:
        return 1;
    default:
        return 0;
    }
}

// The feColorMatrix / feComponentTransfer equivalents of Filter Effects 1
// "Supported Filter Functions", as one row-major 4x5 matrix.
void ColorFilterFunction::colorMatrix(float m[20]) const
{
    const float a = m_amount;
    const float k = 1 - a;
    std::fill(m, m + 20, 0.f);
    m[18] = 1;
    switch (m_type) {
    case FilterFunctionType::GrayScaleFilterFunctionType: {
        const float g[9] = { 0.2126f + 0.7874f * k, 0.7152f - 0.7152f * k,
                             0.0722f - 0.0722f * k, 0.2126f - 0.2126f * k,
                             0.7152f + 0.2848f * k, 0.0722f - 0.0722f * k,
                             0.2126f - 0.2126f * k, 0.7152f - 0.7152f * k,
                             0.0722f + 0.9278f * k };
        for (int r = 0; r < 3; r++) {
            std::copy(g + r * 3, g + r * 3 + 3, m + r * 5);
        }
    } break;
    case FilterFunctionType::SepiaFilterFunctionType: {
        const float s[9] = { 0.393f + 0.607f * k, 0.769f - 0.769f * k,
                             0.189f - 0.189f * k, 0.349f - 0.349f * k,
                             0.686f + 0.314f * k, 0.168f - 0.168f * k,
                             0.272f - 0.272f * k, 0.534f - 0.534f * k,
                             0.131f + 0.869f * k };
        for (int r = 0; r < 3; r++) {
            std::copy(s + r * 3, s + r * 3 + 3, m + r * 5);
        }
    } break;
    case FilterFunctionType::SaturateFilterFunctionType: {
        const float s[9] = { 0.213f + 0.787f * a, 0.715f - 0.715f * a,
                             0.072f - 0.072f * a, 0.213f - 0.213f * a,
                             0.715f + 0.285f * a, 0.072f - 0.072f * a,
                             0.213f - 0.213f * a, 0.715f - 0.715f * a,
                             0.072f + 0.928f * a };
        for (int r = 0; r < 3; r++) {
            std::copy(s + r * 3, s + r * 3 + 3, m + r * 5);
        }
    } break;
    case FilterFunctionType::HueRotateFilterFunctionType: {
        const float c = cosf(a * static_cast<float>(M_PI) / 180.f);
        const float s = sinf(a * static_cast<float>(M_PI) / 180.f);
        const float h[9] = {
            0.213f + c * 0.787f - s * 0.213f, 0.715f - c * 0.715f - s * 0.715f,
            0.072f - c * 0.072f + s * 0.928f, 0.213f - c * 0.213f + s * 0.143f,
            0.715f + c * 0.285f + s * 0.140f, 0.072f - c * 0.072f - s * 0.283f,
            0.213f - c * 0.213f - s * 0.787f, 0.715f - c * 0.715f + s * 0.715f,
            0.072f + c * 0.928f + s * 0.072f
        };
        for (int r = 0; r < 3; r++) {
            std::copy(h + r * 3, h + r * 3 + 3, m + r * 5);
        }
    } break;
    case FilterFunctionType::InvertFilterFunctionType:
        // feFuncX type="table" tableValues="[amount] (1 - [amount])".
        m[0] = m[6] = m[12] = 1 - 2 * a;
        m[4] = m[9] = m[14] = a;
        break;
    case FilterFunctionType::OpacityFilterFunctionType:
        m[0] = m[6] = m[12] = 1;
        m[18] = a;
        break;
    case FilterFunctionType::BrightnessFilterFunctionType:
        m[0] = m[6] = m[12] = a;
        break;
    case FilterFunctionType::ContrastFilterFunctionType:
        m[0] = m[6] = m[12] = a;
        m[4] = m[9] = m[14] = 0.5f - 0.5f * a;
        break;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

String* ColorFilterFunction::toString() const
{
    return toCSSFilterFunction()->toString();
}

CSSFilterFunction* ColorFilterFunction::toCSSFilterFunction() const
{
    CSSStyleValuePair data;
    if (m_type == FilterFunctionType::HueRotateFilterFunctionType) {
        data.setAngleValue(CSSAngle(CSSAngle::DEG, m_amount));
    } else {
        // The computed value turns a percentage into a number.
        data.setNumberValue(m_amount);
    }
    return new CSSFilterFunction(m_type, data);
}

// The primitives work on unpremultiplied colors and clamp their result.
void ColorFilterFunction::apply(WebView* webView, uint8_t* buffer, size_t width,
                                size_t height, size_t stride, float scale) const
{
    float m[20];
    colorMatrix(m);
    for (size_t y = 0; y < height; y++) {
        uint8_t* p = buffer + y * stride;
        for (size_t x = 0; x < width; x++, p += 4) {
            float a = p[STARFISH_PIXEL_A_INDEX] / 255.f;
            if (!a && !m[19]) {
                continue;
            }
            float in[4] = { a ? p[STARFISH_PIXEL_R_INDEX] / 255.f / a : 0,
                            a ? p[STARFISH_PIXEL_G_INDEX] / 255.f / a : 0,
                            a ? p[STARFISH_PIXEL_B_INDEX] / 255.f / a : 0, a };
            float out[4];
            for (int r = 0; r < 4; r++) {
                float v = m[r * 5] * in[0] + m[r * 5 + 1] * in[1] +
                          m[r * 5 + 2] * in[2] + m[r * 5 + 3] * in[3] +
                          m[r * 5 + 4];
                out[r] = std::min(std::max(v, 0.f), 1.f);
            }
            p[STARFISH_PIXEL_R_INDEX] = std::lround(out[0] * out[3] * 255);
            p[STARFISH_PIXEL_G_INDEX] = std::lround(out[1] * out[3] * 255);
            p[STARFISH_PIXEL_B_INDEX] = std::lround(out[2] * out[3] * 255);
            p[STARFISH_PIXEL_A_INDEX] = std::lround(out[3] * 255);
        }
    }
}

DropShadowFilterFunction::DropShadowFilterFunction(
    const CSSFilterFunction& from)
    : FilterFunction(FilterFunctionType::DropShadowFilterFunctionType)
    , m_hasColor(false)
{
    // Values are interpreted as for box-shadow, the optional third length
    // being the standard deviation instead of a blur radius.
    Length* lengths[3] = { &m_offsetX, &m_offsetY, &m_stdDeviation };
    size_t lengthCount = 0;
    m_stdDeviation = Length(Length::Fixed, 0);
    for (const CSSStyleValuePair& v : *from.data().multiValue()) {
        switch (v.valueKind()) {
        case CSSStyleValuePair::ValueKind::Length:
            *lengths[lengthCount++] = v.cssLengthValue().toLength();
            break;
        case CSSStyleValuePair::ValueKind::ColorValueKind:
            m_hasColor = true;
            m_color = v.colorValue();
            break;
        case CSSStyleValuePair::ValueKind::NamedColorValueKind:
            if (v.namedColorValue() !=
                NamedColor::NamedColorValue::currentColor) {
                m_hasColor = true;
                m_color = NamedColor::namedColorToColor(v.namedColorValue());
            }
            break;
        default:
            // shortcut: light-dark() and other unresolved colors fall back
            // to currentcolor; resolve them like the color property does.
            break;
        }
    }
}

void DropShadowFilterFunction::checkComputed(Length currentFS, Length rootFS,
                                             Font* font, LayoutSize windowSize,
                                             ComputedStyle* cs)
{
    for (Length* l : { &m_offsetX, &m_offsetY, &m_stdDeviation }) {
        l->changeToFixedIfNeeded(currentFS, rootFS, font, windowSize.width(),
                                 windowSize.height(), cs);
    }
    // css-color-4: currentcolor is used as the element's own color. A
    // keyframe has no style yet; its animation takes the color per sample.
    if (!m_hasColor && cs) {
        m_color = cs->color();
    }
}

// shortcut: a buffer that holds a filter's output (a box painted into its
// parent's layer, a layer filtered on the CPU) and the culling of a layer
// whose content is off screen take the ink at most this far out of the
// content, so a shadow beyond it is cut off; draw it apart like the GPU
// path's last drop-shadow() if pages put shadows farther.
static const float kMaxInkReach = 2048;

LayoutRect DropShadowFilterFunction::inkRect(const LayoutRect& input) const
{
    float extent = ShadowBlur::computeKernelSizeAtStdDeviation(stdDeviation());
    auto bound = [](float offset) {
        return std::max(-kMaxInkReach, std::min(offset, kMaxInkReach));
    };
    LayoutRect shadow(input.x() + bound(offsetX()) - extent,
                      input.y() + bound(offsetY()) - extent,
                      ceil(input.width() + extent * 2),
                      ceil(input.height() + extent * 2));
    shadow.unite(input);
    return shadow;
}

String* DropShadowFilterFunction::toString() const
{
    return toCSSFilterFunction()->toString();
}

CSSFilterFunction* DropShadowFilterFunction::toCSSFilterFunction() const
{
    // The computed value puts the color first.
    ValueList* list = new ValueList(Separator::SpaceSeparator);
    CSSStyleValuePair color;
    color.setColorValue(m_color);
    list->push_back(color);
    for (float v : { offsetX(), offsetY(), stdDeviation() }) {
        list->push_back(CSSStyleValuePair(CSSStyleValuePair::ValueKind::Length,
                                          CSSLength(v)));
    }
    CSSStyleValuePair data;
    data.setValueList(list);
    return new CSSFilterFunction(m_type, data);
}

bool DropShadowFilterFunction::compare(const FilterFunction* b) const
{
    if (!FilterFunction::compare(b)) {
        return false;
    }
    auto* o = static_cast<const DropShadowFilterFunction*>(b);
    return m_offsetX == o->m_offsetX && m_offsetY == o->m_offsetY &&
           m_stdDeviation == o->m_stdDeviation && m_hasColor == o->m_hasColor &&
           m_color == o->m_color;
}

// feGaussianBlur of the input alpha, offset, flooded with the color and
// merged under the input. Only an alpha plane is allocated.
void DropShadowFilterFunction::apply(WebView* webView, uint8_t* buffer,
                                     size_t width, size_t height, size_t stride,
                                     float scale) const
{
    std::unique_ptr<uint8_t[]> alpha(new (std::nothrow)
                                         uint8_t[width * height]);
    if (!alpha) {
        return;
    }
    // An offset past the buffer moves everything out of it.
    auto pixels = [scale](float offset, size_t size) {
        float bound = size;
        return std::lround(std::max(-bound, std::min(offset * scale, bound)));
    };
    const long dx = pixels(offsetX(), width);
    const long dy = pixels(offsetY(), height);
    for (long y = 0; y < (long)height; y++) {
        for (long x = 0; x < (long)width; x++) {
            long sx = x - dx, sy = y - dy;
            bool inside =
                sx >= 0 && sy >= 0 && sx < (long)width && sy < (long)height;
            alpha[y * width + x] =
                inside ? buffer[sy * stride + sx * 4 + STARFISH_PIXEL_A_INDEX]
                       : 0;
        }
    }
    ShadowBlur blur(alpha.get(), width, height, width, 1);
    blur.process(stdDeviation() * scale);

    const float cr = m_color.r() / 255.f, cg = m_color.g() / 255.f,
                cb = m_color.b() / 255.f, ca = m_color.a() / 255.f;
    for (size_t y = 0; y < height; y++) {
        uint8_t* p = buffer + y * stride;
        for (size_t x = 0; x < width; x++, p += 4) {
            // source-over: input + shadow * (1 - input alpha), premultiplied.
            float s = alpha[y * width + x] / 255.f * ca *
                      (1 - p[STARFISH_PIXEL_A_INDEX] / 255.f);
            if (!s) {
                continue;
            }
            auto over = [s](uint8_t& c, float v) {
                c = std::min(255l, c + std::lround(v * s * 255));
            };
            over(p[STARFISH_PIXEL_R_INDEX], cr);
            over(p[STARFISH_PIXEL_G_INDEX], cg);
            over(p[STARFISH_PIXEL_B_INDEX], cb);
            over(p[STARFISH_PIXEL_A_INDEX], 1);
        }
    }
}

BlurFilterFunction::BlurFilterFunction(const CSSFilterFunction& from)
    : BlurFilterFunction(from.data().cssLengthValue().toLength())
{
}

String* BlurFilterFunction::toString() const
{
    char container[100];
    auto valueString = m_stdDeviation.dumpString()->toUTF8NonGCString();
    snprintf(container, sizeof(container), "blur(%s) ", valueString.data());
    return String::fromUTF8(container, strnlen(container, sizeof(container)));
}

CSSFilterFunction* BlurFilterFunction::toCSSFilterFunction() const
{
    return new CSSFilterFunction(
        FilterFunctionType::BlurFilterFunctionType,
        CSSStyleValuePair(CSSStyleValuePair::ValueKind::Length,
                          CSSLength(m_stdDeviation.fixed())));
}

void BlurFilterFunction::apply(WebView* webView, uint8_t* buffer, size_t width,
                               size_t height, size_t stride, float scale) const
{
    float radius = m_stdDeviation.numberData() * scale;

#if ENABLE_PARALLEL_BLUR == 0
    // I disabled parallel blur temporarily
    // because the working time is not stable(~2x slower than single)
    // I think the reason is worker thread sometimes stall because OS
    // threading scheduling
    // and huge memory operation is pretty slow in ARM(parallel needs buffer
    // copy)
    // we can enable after resolve the issue above & finding heterogeneous
    // multi-core CPU in runtime

    ShadowBlur blur(buffer, width, height, stride);
    blur.process(radius);

#else
    size_t extraHeight = 3 * (radius * 0.5);
    size_t numberOfThreadsToRequest =
        (width * height) / (100 * 100 + extraHeight * width);
    struct Params {
        float sigma;
        uint8_t* fragmentedBuffer;
        size_t width;
        size_t height;
        size_t stride;
    };

    auto blurFitlerWorker = [](void* data) -> void* {
        auto params = (Params*)data;
        ShadowBlur blur(params->fragmentedBuffer, params->width, params->height,
                        params->stride);
        blur.process(params->sigma);
        return nullptr;
    };

    ParallelJobExecutor<Params>* parallelJobExecutor =
        new ParallelJobExecutor<Params>(webView, blurFitlerWorker,
                                        numberOfThreadsToRequest);

    size_t num = parallelJobExecutor->numberOfThread();

    const size_t blockHeight = height / num;
    const size_t jobsWithExtra = height % num;

    // parallel job executor needs `blockHeight > extraHeight`
    // if blockHeight < extraHeight
    // size_t startY = !i ? 0 : currentY - extraHeight;
    // startY got wrong value at i == 1
    if (num > 1 && blockHeight > extraHeight) {
        size_t currentY = 0;

        for (size_t i = 0; i < num; ++i) {
            auto& params = parallelJobExecutor->parameters(i);

            size_t startY = !i ? 0 : currentY - extraHeight;
            currentY += i < jobsWithExtra ? blockHeight + 1 : blockHeight;
            size_t endY = i == num - 1 ? currentY : currentY + extraHeight;
            size_t blockSize = (endY - startY) * stride;

            params.sigma = radius;
            params.width = width;
            params.height = endY - startY;
            params.stride = stride;
            if (i == 0) {
                params.fragmentedBuffer = buffer;
            } else {
                params.fragmentedBuffer = new uint8_t[blockSize];
                memcpy(params.fragmentedBuffer, buffer + startY * stride,
                       blockSize);
            }
        }

        parallelJobExecutor->execute();

        currentY = 0;
        for (size_t i = 1; i < num; ++i) {
            auto& params = parallelJobExecutor->parameters(i);
            size_t sourceOffset;
            size_t destinationOffset;
            size_t size;
            size_t adjustedBlockHeight =
                i < jobsWithExtra ? blockHeight + 1 : blockHeight;

            currentY += adjustedBlockHeight;
            sourceOffset = extraHeight * stride;
            destinationOffset = currentY * stride;
            size = adjustedBlockHeight * stride;

            memcpy(buffer + destinationOffset,
                   params.fragmentedBuffer + sourceOffset, size);
            delete[] params.fragmentedBuffer;
        }
    } else {
        // Fallback
        ShadowBlur blur(buffer, width, height, stride);
        blur.process(radius);
    }
#endif
    return;
}

SVGUrlFilterFunction::SVGUrlFilterFunction(const CSSFilterFunction& from)
    : SVGUrlFilterFunction(from.data().urlStringValue())
{
}

String* SVGUrlFilterFunction::toString() const
{
    char container[100];
    auto valueString = m_url->toUTF8NonGCString();
    snprintf(container, sizeof(container), "SVGUrl(%s) ", valueString.data());
    return String::fromUTF8(container, strnlen(container, sizeof(container)));
}

CSSFilterFunction* SVGUrlFilterFunction::toCSSFilterFunction() const
{
    return new CSSFilterFunction(
        FilterFunctionType::SVGUrlFilterFunctionType,
        CSSStyleValuePair(CSSStyleValuePair::ValueKind::UrlValueKind, m_url));
}

void SVGUrlFilterFunction::apply(WebView* webView, uint8_t* buffer,
                                 size_t width, size_t height, size_t stride,
                                 float scale) const
{
}

FilterFunctions* FilterFunctions::forInheritance()
{
    // A currentcolor shadow takes the color of each element it applies to,
    // so the inheriting element resolves a copy of it (css-color-4).
    auto isCurrentColor = [](FilterFunction* f) {
        return f->type() == FilterFunctionType::DropShadowFilterFunctionType &&
               static_cast<DropShadowFilterFunction*>(f)->isCurrentColor();
    };
    if (std::none_of(begin(), end(), isCurrentColor)) {
        return this;
    }
    FilterFunctions* copy = new FilterFunctions();
    for (auto f : *this) {
        copy->push_back(isCurrentColor(f)
                            ? new DropShadowFilterFunction(
                                  *static_cast<DropShadowFilterFunction*>(f))
                            : f);
    }
    return copy;
}

FilterFunctions* FilterFunctions::create(const CSSStyleValuePair& from)
{
    if (from.valueKind() != CSSStyleValuePair::ValueKind::ValueListKind) {
        return nullptr;
    }
    ValueList* list = from.multiValue();
    const size_t length = list->size();
    if (!length) {
        return nullptr;
    }
    FilterFunctions* result = new FilterFunctions();
    for (size_t i = 0; i < length; i++) {
        CSSStyleValuePair& item = list->at(i);
        STARFISH_ASSERT(item.valueKind() ==
                        CSSStyleValuePair::ValueKind::FilterFunctionValueKind);
        result->push_back(FilterFunction::create(*item.filterFunctionValue()));
    }
    return result;
}

void FilterFunctions::toCSSStyleValue(CSSStyleValuePair& result) const
{
    const size_t length = size();
    if (!length) {
        result.setValueKind(CSSStyleValuePair::ValueKind::None);
        return;
    }
    ValueList* list = new ValueList(Separator::SpaceSeparator);
    for (size_t i = 0; i < length; i++) {
        list->emplace_back(
            CSSStyleValuePair::ValueKind::FilterFunctionValueKind,
            at(i)->toCSSFilterFunction());
    }
    result.setValueList(list);
}

String* FilterFunctions::toString() const
{
    StringBuilder builder;
    const size_t length = size();
    for (size_t i = 0; i < length; i++) {
        builder.appendString(at(i)->toString());
        if (i < length - 1) {
            builder.appendChar(' ');
        }
    }
    return builder.finalize();
}

bool FilterFunctions::hasEffect() const
{
    for (auto filter : *this) {
        switch (filter->type()) {
        case FilterFunctionType::BlurFilterFunctionType:
            if (!static_cast<BlurFilterFunction*>(filter)
                     ->standardDeviation()
                     .isZero()) {
                return true;
            }
            break;
        case FilterFunctionType::DropShadowFilterFunctionType:
            // A transparent shadow draws nothing; any other shows through
            // the content's translucent pixels, even unmoved.
            if (static_cast<DropShadowFilterFunction*>(filter)->color().a()) {
                return true;
            }
            break;
        case FilterFunctionType::SVGUrlFilterFunctionType:
            // Rendered by the SVG filter path, not as a CSS filter.
            break;
        default:
            if (!static_cast<ColorFilterFunction*>(filter)->isIdentity()) {
                return true;
            }
            break;
        }
    }
    return false;
}

bool FilterFunctions::isColorOnly() const
{
    return std::none_of(begin(), end(), [](FilterFunction* f) {
        return f->type() == FilterFunctionType::BlurFilterFunctionType ||
               f->type() == FilterFunctionType::DropShadowFilterFunctionType ||
               f->type() == FilterFunctionType::SVGUrlFilterFunctionType;
    });
}

bool FilterFunctions::filtersLayersApart() const
{
    if (!isColorOnly()) {
        return false;
    }
    for (auto f : *this) {
        float m[20];
        static_cast<ColorFilterFunction*>(f)->colorMatrix(m);
        if (m[15] || m[16] || m[17] || m[18] != 1 || m[19]) {
            return false;
        }
        for (size_t r = 0; r < 3; r++) {
            float low = m[r * 5 + 4];
            float high = low;
            for (size_t k = 0; k < 4; k++) {
                (m[r * 5 + k] < 0 ? low : high) += m[r * 5 + k];
            }
            // Allows for the rounding of rows that sum to 1.
            const float kEpsilon = 1e-5f;
            if (low < -kEpsilon || high > 1 + kEpsilon) {
                return false;
            }
        }
    }
    return true;
}

Unit::Color FilterFunctions::filterColor(const Unit::Color& color) const
{
    // Filter primitives take unpremultiplied colors and clamp each result.
    float v[4] = { color.r() / 255.f, color.g() / 255.f, color.b() / 255.f,
                   color.a() / 255.f };
    for (auto f : *this) {
        if (f->type() == FilterFunctionType::BlurFilterFunctionType ||
            f->type() == FilterFunctionType::DropShadowFilterFunctionType ||
            f->type() == FilterFunctionType::SVGUrlFilterFunctionType) {
            continue;
        }
        float m[20];
        static_cast<ColorFilterFunction*>(f)->colorMatrix(m);
        float out[4];
        for (size_t r = 0; r < 4; r++) {
            float x = m[r * 5 + 4];
            for (size_t k = 0; k < 4; k++) {
                x += m[r * 5 + k] * v[k];
            }
            out[r] = std::max(0.f, std::min(x, 1.f));
        }
        std::copy(out, out + 4, v);
    }
    return Unit::Color(std::lround(v[0] * 255), std::lround(v[1] * 255),
                       std::lround(v[2] * 255), std::lround(v[3] * 255));
}

LayoutRect FilterFunctions::inkRect(const LayoutRect& input) const
{
    LayoutRect rect = input;
    for (auto filter : *this) {
        if (filter->type() == FilterFunctionType::BlurFilterFunctionType) {
            float sd = static_cast<BlurFilterFunction*>(filter)
                           ->standardDeviation()
                           .numberData();
            if (sd > 0) {
                float e = ShadowBlur::computeKernelSizeAtStdDeviation(sd);
                rect = LayoutRect(rect.x() - e, rect.y() - e,
                                  ceil(rect.width() + e * 2),
                                  ceil(rect.height() + e * 2));
            }
        } else if (filter->type() ==
                   FilterFunctionType::DropShadowFilterFunctionType) {
            rect =
                static_cast<DropShadowFilterFunction*>(filter)->inkRect(rect);
        }
    }
    LayoutUnit reach(kMaxInkReach);
    LayoutUnit x = std::max(rect.x(), input.x() - reach);
    LayoutUnit y = std::max(rect.y(), input.y() - reach);
    return LayoutRect(x, y, std::min(rect.maxX(), input.maxX() + reach) - x,
                      std::min(rect.maxY(), input.maxY() + reach) - y);
}

#undef ENABLE_PARALLEL_BLUR

} // namespace Starfish
