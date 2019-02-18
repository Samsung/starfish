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

#include <EscargotPublic.h>
using namespace Escargot;

#include "StarfishConfig.h"
#include "core/xml/FormData.h"

namespace Starfish {

FormData::FormData(ScriptBindingInstance* instance,
                   HTMLFormElement* form /*= nullptr*/)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(instance)
{
    if (form) {
        m_list = form->createFormDataSet(nullptr);
    } else {
        m_list = new (GC) GCVector<FormDataSetItem*>();
    }
}

void FormData::append(String* name, String* value)
{
    m_list->push_back(new FormDataSetItem(name, value, String::emptyString));
}

void FormData::set(String* name, String* value)
{
    remove(name);
    append(name, value);
}

bool FormData::has(String* name)
{
    return (findByName(name) != m_list->end());
}

void FormData::remove(String* name)
{
    m_list->erase(std::remove_if(m_list->begin(), m_list->end(),
                                 [name](const FormDataSetItem* item) {
                                     return item->m_name->equals(name);
                                 }),
                  m_list->end());
}

Nullable<String*> FormData::get(String* name)
{
    auto it = findByName(name);

    if (it != m_list->end()) {
        return (*it)->m_value;
    }
    return nullptr;
}

GCVector<String*> FormData::getAll(String* name) const
{
    GCVector<String*> ret;

    std::for_each(m_list->begin(), m_list->end(),
                  [&](const FormDataSetItem* item) {
                      if (item->m_name->equals(name)) {
                          ret.push_back(item->m_value);
                      }
                  });

    return ret;
}

GCVector<FormDataSetItem*>::iterator FormData::findByName(String* name)
{
    auto f = [name](const FormDataSetItem* item) {
        return item->m_name->equals(name);
    };
    return std::find_if(m_list->begin(), m_list->end(), f);
}

class FormDataIterationSource
    : public IterationSource<Nullable<String*>, Nullable<FormDataEntryValue*>> {
public:
    FormDataIterationSource(GCVector<FormDataSetItem*>& source)
        : m_list(source)
    {
    }

    bool next(ExecutionStateRef* state, Nullable<String*>& key,
              Nullable<FormDataEntryValue*>& value)
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return true;
    }

private:
    GCVector<FormDataSetItem*> m_list;
};

IterationSource<Nullable<String*>, Nullable<FormDataEntryValue*>>*
FormData::startIteration(ExecutionStateRef* state)
{
    return new FormDataIterationSource(*m_list);
}
}
