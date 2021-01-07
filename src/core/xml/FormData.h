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

#ifndef __StarfishFormData__
#define __StarfishFormData__

#include "binding/ScriptWrappable.h"
#include "binding/IterationSource.h"

namespace Starfish {

class HTMLFormElement;
enum class EncodeType;

typedef String FormDataEntryValue;

class FormDataSetItem : public gc {
public:
    FormDataSetItem(String* name, String* value, String* type);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    String* toString();

    String* m_name;
    String* m_value;
    String* m_type;
};

class FormSubmitData : public gc {
public:
    FormSubmitData(GCVector<FormDataSetItem*>* formDataSet, EncodeType enctype,
                   String* method);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    String* toString();

    GCVector<FormDataSetItem*>* m_formDataSet;
    EncodeType m_enctype;
    String* m_method;
};

class FormData : public ScriptWrappable {
public:
    FormData(ExecutionContext* executionContext);

#if !defined(STARFISH_WEBWORKER_HOST)
    FormData(ExecutionContext* executionContext, HTMLFormElement* form);
#endif /* !defined(STARFISH_WEBWORKER_HOST) */

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
} // namespace Starfish

#endif
