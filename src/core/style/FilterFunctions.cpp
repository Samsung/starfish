
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

#include "StarFishConfig.h"
#include "core/style/CSSTokenValue.h"
#include "core/style/CSSParser.h"
#include "core/style/Style.h"
#include "core/style/FilterFunctions.h"
#include "core/modules/threading/ParallelJobExecutor.h"
#include "core/modules/canvas/ShadowBlur.h"

namespace StarFish {

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
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
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

void UnsupportedFilterFunction::apply(StarFish* starfish, uint8_t* buffer,
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

void BlurFilterFunction::apply(StarFish* starfish, uint8_t* buffer,
                               size_t width, size_t height, size_t stride) const
{
    float sigma = m_stdDeviation.numberData();

    int extraHeight = 3 * (sigma * 0.5f);
    int numberOfThreadsToRequest =
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
        blur.process(params->sigma * 2);
        return nullptr;
    };

    ParallelJobExecutor<Params>* parallelJobExecutor =
        new ParallelJobExecutor<Params>(starfish, blurFitlerWorker,
                                        numberOfThreadsToRequest);

    int num = parallelJobExecutor->numberOfThread();

    if (num > 1) {
        const int blockHeight = height / num;
        const int jobsWithExtra = width % num;
        int currentY = 0;

        for (int i = 0; i < num; ++i) {
            auto& params = parallelJobExecutor->parameters(i);

            int startY = !i ? 0 : currentY - extraHeight;
            currentY += i < jobsWithExtra ? blockHeight + 1 : blockHeight;
            int endY = i == num - 1 ? currentY : currentY + extraHeight;
            int blockSize = (endY - startY) * stride;

            params.sigma = sigma;
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
        for (int i = 1; i < num; ++i) {
            auto& params = parallelJobExecutor->parameters(i);
            int sourceOffset;
            int destinationOffset;
            int size;
            int adjustedBlockHeight =
                i < jobsWithExtra ? blockHeight + 1 : blockHeight;

            currentY += adjustedBlockHeight;
            sourceOffset = extraHeight * stride;
            destinationOffset = currentY * stride;
            size = adjustedBlockHeight * stride;

            memcpy(buffer + destinationOffset,
                   params.fragmentedBuffer + sourceOffset, size);
            delete[] params.fragmentedBuffer;
        }
    }

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

} // namespace StarFish
