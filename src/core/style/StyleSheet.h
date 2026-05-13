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

#ifndef __StarfishStyleSheet__
#define __StarfishStyleSheet__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class MediaList;
class ExecutionContext;
class ElementOrProcessingInstruction;

class StyleSheet : public ScriptWrappable {
public:
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(StyleSheet)

    /* DOM APIs */
    virtual String* type() const = 0;
    virtual String* href() const = 0;
    virtual Optional<ElementOrProcessingInstruction> ownerNode() const = 0;
    virtual StyleSheet* parentStyleSheet() const
    {
        return nullptr;
    }
    virtual String* title() const = 0;
    virtual MediaList* media() = 0;
    virtual bool disabled() = 0;
    virtual void setDisabled(bool disabled) = 0;

protected:
    StyleSheet(ExecutionContext* executionContext);
    ExecutionContext* m_executionContext;
};

} /* namespace Starfish */

#endif /* __StarfishStyleSheet__ */
