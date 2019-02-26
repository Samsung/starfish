
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

#include "StarfishConfig.h"
#include "core/style/CSSTokenValue.h"
#include "core/style/CSSParser.h"
#include "core/style/Style.h"
#include "core/style/FilterFunctions.h"
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
    case FilterFunctionType::DropShadowFilterFunctionType:
    case FilterFunctionType::HueRotateFilterFunctionType:
    case FilterFunctionType::BrightnessFilterFunctionType:
    case FilterFunctionType::ContrastFilterFunctionType:
    case FilterFunctionType::GrayScaleFilterFunctionType:
    case FilterFunctionType::InvertFilterFunctionType:
    case FilterFunctionType::OpacityFilterFunctionType:
    case FilterFunctionType::SaturateFilterFunctionType:
    case FilterFunctionType::SepiaFilterFunctionType:
    case FilterFunctionType::SVGUrlFilterFunctionType:
        return new UnsupportedFilterFunction(type);
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

String* UnsupportedFilterFunction::toString() const
{
    char container[100];
    const char* name = CSSFilterFunction::typeToString(m_type);
    snprintf(container, sizeof(container), "%s(not supported) ", name);
    return String::fromUTF8(container);
}

CSSFilterFunction* UnsupportedFilterFunction::toCSSFilterFunction() const
{
    if (m_type == FilterFunctionType::SVGUrlFilterFunctionType) {
        return new CSSFilterFunction(
            m_type,
            CSSStyleValuePair(CSSStyleValuePair::ValueKind::UrlValueKind,
                              String::createASCIIString("unsupported")));
    }
    return new CSSFilterFunction(
        m_type,
        CSSStyleValuePair(CSSStyleValuePair::ValueKind::KeywordValueKind,
                          String::createASCIIString("unsupported")));
}

void UnsupportedFilterFunction::apply(WebView* webView, uint8_t* buffer,
                                      size_t width, size_t height,
                                      size_t stride) const
{
    STARFISH_LOG_INFO("%s is unsupported\n",
                      CSSFilterFunction::typeToString(m_type));
    return;
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
    return String::fromUTF8(container);
}

CSSFilterFunction* BlurFilterFunction::toCSSFilterFunction() const
{
    return new CSSFilterFunction(
        FilterFunctionType::BlurFilterFunctionType,
        CSSStyleValuePair(CSSStyleValuePair::ValueKind::Length,
                          CSSLength(m_stdDeviation.fixed())));
}

void BlurFilterFunction::apply(WebView* webView, uint8_t* buffer, size_t width,
                               size_t height, size_t stride) const
{
    float radius = m_stdDeviation.numberData() * 2;

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
    ValueList* list = new ValueList(ValueList::SpaceSeparator);
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

bool FilterFunctions::getStandardDeviationOfBlurFilter(Length& out)
{
    for (auto filter : *this) {
        if (filter->type() == FilterFunctionType::BlurFilterFunctionType) {
            out = static_cast<BlurFilterFunction*>(filter)->standardDeviation();
            return true;
        }
    }
    return false;
}

#undef ENABLE_PARALLEL_BLUR

} // namespace Starfish
