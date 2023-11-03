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
#include "core/dom/DOMRectReadOnly.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

DOMRectReadOnly::DOMRectReadOnly(ExecutionContext* executionContext, double x,
                                 double y, double width, double height)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
{
}

DOMRectReadOnly::DOMRectReadOnly(ExecutionContext* executionContext,
                                 const DOMRectInit& init)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    if (init.hasX()) {
        m_x = init.x();
    }
    if (init.hasY()) {
        m_y = init.y();
    }
    if (init.hasWidth()) {
        m_width = init.width();
    }
    if (init.hasHeight()) {
        m_height = init.height();
    }
}

ScriptBindingInstance* DOMRectReadOnly::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

bool DOMRectReadOnly::isEmpty()
{
    return m_width <= 0 || m_height <= 0;
}

bool DOMRectReadOnly::equals(const DOMRectReadOnly* other) const
{
    return m_x == other->m_x && m_y == other->m_y &&
           m_width == other->m_width && m_height == other->m_height;
}

ScriptObject DOMRectReadOnly::toJSON()
{
    ScriptBindingInstance* instance = scriptBindingInstance();
    ScriptObject result = createEmptyScriptObject(instance);
    setScriptObjectProperty(instance,
                            createScriptValue(String::createASCIIString("x")),
                            createScriptValue(m_x), createScriptValue(result));
    setScriptObjectProperty(instance,
                            createScriptValue(String::createASCIIString("y")),
                            createScriptValue(m_y), createScriptValue(result));
    setScriptObjectProperty(
        instance, createScriptValue(String::createASCIIString("width")),
        createScriptValue(m_width), createScriptValue(result));
    setScriptObjectProperty(
        instance, createScriptValue(String::createASCIIString("height")),
        createScriptValue(m_height), createScriptValue(result));
    setScriptObjectProperty(
        instance, createScriptValue(String::createASCIIString("top")),
        createScriptValue(top()), createScriptValue(result));
    setScriptObjectProperty(
        instance, createScriptValue(String::createASCIIString("right")),
        createScriptValue(right()), createScriptValue(result));
    setScriptObjectProperty(
        instance, createScriptValue(String::createASCIIString("bottom")),
        createScriptValue(bottom()), createScriptValue(result));
    setScriptObjectProperty(
        instance, createScriptValue(String::createASCIIString("left")),
        createScriptValue(left()), createScriptValue(result));

    return result;
}
} // namespace Starfish
