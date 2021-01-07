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

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "EscargotPublic.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/Document.h"
#include "core/dom/canvas/TextMetrics.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {
TextMetrics::TextMetrics(ExecutionContext* ownerExecutionContext, double width,
                         double actualBoundingBoxLeft,
                         double actualBoundingBoxRight,
                         double fontBoundingBoxAscent,
                         double fontBoundingBoxDescent,
                         double actualBoundingBoxAscent,
                         double actualBoundingBoxDescent, double emHeightAscent,
                         double emHeightDescent, double hangingBaseline,
                         double alphabeticBaseline, double ideographicBaseline)
    : ScriptWrappable(this)
    , m_executionContext(ownerExecutionContext)
    , m_width(width)
    , m_actualBoundingBoxLeft(actualBoundingBoxLeft)
    , m_actualBoundingBoxRight(actualBoundingBoxRight)
    , m_fontBoundingBoxAscent(fontBoundingBoxAscent)
    , m_fontBoundingBoxDescent(fontBoundingBoxDescent)
    , m_actualBoundingBoxAscent(actualBoundingBoxAscent)
    , m_actualBoundingBoxDescent(actualBoundingBoxDescent)
    , m_emHeightAscent(emHeightAscent)
    , m_emHeightDescent(emHeightDescent)
    , m_hangingBaseline(hangingBaseline)
    , m_alphabeticBaseline(alphabeticBaseline)
    , m_ideographicBaseline(ideographicBaseline)
{
    STARFISH_ASSERT(ownerExecutionContext != nullptr);
}

ScriptBindingInstance* TextMetrics::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}
} // namespace Starfish

#endif
