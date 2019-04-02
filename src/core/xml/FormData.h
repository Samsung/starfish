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

#ifndef __StarfishFormData__
#define __StarfishFormData__

#include "binding/ScriptWrappable.h"
#include "binding/IterationSource.h"
#include "core/dom/HTMLFormElement.h"

namespace Starfish {

typedef String FormDataEntryValue;

class FormData : public ScriptWrappable {
public:
    FormData(ExecutionContext* executionContext,
             HTMLFormElement* form = nullptr);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(FormData)

    void set(String* name, String* value);
    void append(String* name, String* value);
    bool has(String* name);
    void remove(String* name);
    Nullable<FormDataEntryValue*> get(String* name);
    GCVector<String*> getAll(String* name) const;

    IterationSource<Nullable<String*>, Nullable<FormDataEntryValue*>>*
    startIteration(ExecutionStateRef* state);

private:
    ExecutionContext* m_executionContext;
    GCVector<FormDataSetItem*>* m_list;
    GCVector<FormDataSetItem*>::iterator findByName(String* name);
};
}

#endif
