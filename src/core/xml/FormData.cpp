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
#include "core/dom/ExecutionContext.h"

namespace Starfish {

FormDataSetItem::FormDataSetItem(String* name, String* value, String* type)
    : m_name(name)
    , m_value(value)
    , m_type(type)
{
}

void* FormDataSetItem::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FormDataSetItem));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FormDataSetItem)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(FormDataSetItem, m_name));
        GC_set_bit(desc, GC_WORD_OFFSET(FormDataSetItem, m_value));
        GC_set_bit(desc, GC_WORD_OFFSET(FormDataSetItem, m_type));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FormDataSetItem));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* FormDataSetItem::toString()
{
    StringBuilder b;
    b.appendString(m_name);
    b.appendChar('=');
    b.appendString(m_value);
    return b.finalize();
}

FormSubmitData::FormSubmitData(GCVector<FormDataSetItem*>* formDataSet,
                               EncodeType enctype, String* method)
    : m_formDataSet(formDataSet)
    , m_enctype(enctype)
    , m_method(method)
{
}

void* FormSubmitData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FormSubmitData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FormSubmitData)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(FormSubmitData, m_formDataSet));
        GC_set_bit(desc, GC_WORD_OFFSET(FormSubmitData, m_method));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FormSubmitData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* FormSubmitData::toString()
{
    StringBuilder b;
    for (size_t i = 0; i < m_formDataSet->size(); i++) {
        FormDataSetItem* item = (*m_formDataSet)[i];
        b.appendString(item->toString());
        if (i < m_formDataSet->size() - 1) {
            b.appendChar('&');
        }
    }

    return b.finalize();
}

FormData::FormData(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    m_list = new (GC) GCVector<FormDataSetItem*>();
}

#if !defined(STARFISH_WEBWORKER_HOST)
FormData::FormData(ExecutionContext* executionContext, HTMLFormElement* form)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    if (form) {
        m_list = form->createFormDataSet(nullptr);
    } else {
        m_list = new (GC) GCVector<FormDataSetItem*>();
    }
}
#endif /* !defined(STARFISH_WEBWORKER_HOST) */

ScriptBindingInstance* FormData::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
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
