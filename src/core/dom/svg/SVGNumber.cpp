/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "SVGNumber.h"
#include "SVGElement.h"
#include "core/dom/svg/SVGNumberList.h"
#include "core/dom/DOMException.h"

namespace Starfish {

SVGNumber::SVGNumber(SVGElement* sourceElement, QualifiedName targetAttribute,
                     float value)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_value(value)
    , m_readOnly(false)
{
}

SVGNumber::SVGNumber(SVGElement* sourceElement, SVGNumberList* targetList,
                     float value)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetList(targetList)
    , m_value(value)
    , m_readOnly(false)
{
}

ScriptBindingInstance* SVGNumber::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

float SVGNumber::value() const
{
    return m_value;
}

void SVGNumber::setValue(float value)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (std::isnan(value) || std::isinf(value)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    m_value = value;

    if (m_targetList) {
        m_targetList->updateAttributeByList();
    } else if (!m_targetAttribute.localName()->isEmpty()) {
        m_sourceElement->setAttribute(m_targetAttribute,
                                      String::fromFloat(m_value));
    }
}

bool SVGNumber::isReadOnly() const
{
    return m_readOnly;
}

void SVGNumber::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}
} // namespace Starfish
