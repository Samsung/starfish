/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"

#include "URLSearchParams.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

URLSearchParams::URLSearchParams(ExecutionContext* executionContext,
                                 String* init)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    parse(init);
}

URLSearchParams::URLSearchParams(ExecutionContext* executionContext,
                                 GCVector<GCVector<String*>> init)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    for (size_t i = 0; i < init.size(); i++) {
        GCVector<String*> keyAndValue = init[i];
        if (keyAndValue.size() == 2) {
            URLParam* param = new URLParam(keyAndValue[0], keyAndValue[1]);
            m_list.push_back(param);
        } else {
            throw new DOMException(executionContext,
                                   DOMException::SCRIPT_TYPE_ERR,
                                   "Pair does not contain exactly two items");
        }
    }
}

ScriptBindingInstance* URLSearchParams::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

void URLSearchParams::parse(String* str)
{
    m_resourceUrl = new ResourceURL(str);
    if (!m_resourceUrl->isValid()) {
        STARFISH_LOG_WARN("URLSearchParams::%s: Invalid url\n", __func__);
        return;
    }

    String* search = m_resourceUrl->search();
    if (search->isEmpty() || !search->startsWith("?", 1)) {
        return;
    }

    search = search->substring(1, search->length() - 1);
    std::string searchStr = search->toUTF8NonGCString();
    std::vector<std::string> pairs = StringUtils::split(searchStr, '&');
    for (auto pair : pairs) {
        std::vector<std::string> keyAndValue = StringUtils::split(pair, '=');
        if (keyAndValue.size() == 2) {
            URLParam* param = new URLParam(keyAndValue[0], keyAndValue[1]);
            m_list.push_back(param);
        }
    }
}

void URLSearchParams::append(String* name, String* value)
{
    m_list.push_back(new URLParam(name, value));
}

void URLSearchParams::deleteParams(String* name)
{
    m_list.erase(std::remove_if(m_list.begin(), m_list.end(),
                                [name](URLParam* item) {
                                    return item->key->equals(name);
                                }),
                 m_list.end());
}

Nullable<String*> URLSearchParams::get(String* name)
{
    Nullable<String*> r;
    for (auto param : m_list) {
        if (param->key->equals(name)) {
            r = param->value;
            break;
        }
    }
    return r;
}

GCVector<String*> URLSearchParams::getAll(String* name)
{
    GCVector<String*> values;
    for (auto param : m_list) {
        if (param->key->equals(name)) {
            values.push_back(param->value);
        }
    }
    return values;
}

bool URLSearchParams::has(String* name)
{
    for (auto param : m_list) {
        if (param->key->equals(name)) {
            return true;
        }
    }
    return false;
}

void URLSearchParams::set(String* name, String* value)
{
    GCVector<URLParam*>::iterator itr =
        std::find_if(m_list.begin(), m_list.end(), [name](URLParam* item) {
            return item->key->equals(name);
        });

    if (itr != m_list.end()) {
        (*itr)->value = value;
        m_list.erase(std::remove_if(itr + 1, m_list.end(),
                                    [name](URLParam* item) {
                                        return item->key->equals(name);
                                    }),
                     m_list.end());
    } else {
        append(name, value);
    }
}

void URLSearchParams::sort()
{
    std::stable_sort(
        m_list.begin(), m_list.end(),
        [](URLParam* a, URLParam* b) { return a->key->compare(b->key) < 0; });
}

size_t URLSearchParams::length()
{
    return m_list.size();
}
URLParam* URLSearchParams::at(size_t i)
{
    if (i >= m_list.size()) {
        return nullptr;
    }

    return m_list[i];
}

String* URLSearchParams::toString()
{
    StringBuilder b;
    for (size_t i = 0; i < m_list.size(); i++) {
        URLParam* param = m_list[i];
        b.appendString(param->key);
        b.appendString("=");
        b.appendString(param->value);
        if (i < m_list.size() - 1) {
            b.appendString("&");
        }
    }

    return b.finalize();
}

class URLSearchParamsIterationSource final
    : public IterationSource<Nullable<String*>, Nullable<String*>> {
public:
    URLSearchParamsIterationSource(URLSearchParams* params)
    {
        m_params = params;
        m_index = 0;
    }

    bool next(ExecutionStateRef* state, Nullable<String*>& key,
              Nullable<String*>& value)
    {
        if (m_index >= m_params->length()) {
            return false;
        }

        key = m_params->at(m_index)->key;
        value = m_params->at(m_index)->value;

        m_index++;
        return true;
    }

private:
    URLSearchParams* m_params{ nullptr };
    size_t m_index{ 0 };
};

IterationSource<Nullable<String*>, Nullable<String*>>*
URLSearchParams::startIteration(ExecutionStateRef* state)
{
    return new URLSearchParamsIterationSource(this);
}
} // namespace Starfish
