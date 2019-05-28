/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishTextMetrics__
#define __StarfishTextMetrics__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/ScriptWrappable.h"

namespace Starfish {
class TextMetrics : public ScriptWrappable {
public:
    TextMetrics(ExecutionContext* ownerExecutionContext, double width,
                double actualBoundingBoxLeft, double actualBoundingBoxRight,
                double fontBoundingBoxAscent, double fontBoundingBoxDescent,
                double actualBoundingBoxAscent, double actualBoundingBoxDescent,
                double emHeightAscent, double emHeightDescent,
                double hangingBaseline, double alphabeticBaseline,
                double ideographicBaseline);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTextMetrics() const override;
    virtual ScriptBindingInstance* scriptBindingInstance();

    DEFINE_GETTER(double, width);
    DEFINE_GETTER(double, actualBoundingBoxLeft);
    DEFINE_GETTER(double, actualBoundingBoxRight);
    DEFINE_GETTER(double, fontBoundingBoxAscent);
    DEFINE_GETTER(double, fontBoundingBoxDescent);
    DEFINE_GETTER(double, actualBoundingBoxAscent);
    DEFINE_GETTER(double, actualBoundingBoxDescent);
    DEFINE_GETTER(double, emHeightAscent);
    DEFINE_GETTER(double, emHeightDescent);
    DEFINE_GETTER(double, hangingBaseline);
    DEFINE_GETTER(double, alphabeticBaseline);
    DEFINE_GETTER(double, ideographicBaseline);

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

private:
    TextMetrics(ExecutionContext* ownerExecutionContext);
    ExecutionContext* m_executionContext;
    double m_width;
    double m_actualBoundingBoxLeft;
    double m_actualBoundingBoxRight;
    double m_fontBoundingBoxAscent;
    double m_fontBoundingBoxDescent;
    double m_actualBoundingBoxAscent;
    double m_actualBoundingBoxDescent;
    double m_emHeightAscent;
    double m_emHeightDescent;
    double m_hangingBaseline;
    double m_alphabeticBaseline;
    double m_ideographicBaseline;
};
}
#endif
#endif
