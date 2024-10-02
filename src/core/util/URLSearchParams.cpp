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
#include "URL.h"

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
                                 URL* sourceURL)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_sourceUrl(sourceURL)
{
    parse(sourceURL->search());
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
    m_list.clear();

    String* search = str;
    if (search->isEmpty()) {
        return;
    }

    if (search->startsWith("?", 1)) {
        search = search->substring(1, search->length() - 1);
    }

    std::string searchStr = search->toUTF8NonGCString();
    std::vector<std::string> pairs = StringUtils::split(searchStr, '&');
    for (auto& pair : pairs) {
        std::vector<std::string> keyAndValue = StringUtils::split(pair, '=');
        if (!keyAndValue.size()) {
            continue;
        }
        String* key = ResourceURL::createPercentDecodingString(
            String::fromUTF8(keyAndValue[0].data(), keyAndValue[0].length()),
            true);
        URLParam* param;
        if (keyAndValue.size() == 2) {
            param =
                new URLParam(key, ResourceURL::createPercentDecodingString(
                                      String::fromUTF8(keyAndValue[1].data(),
                                                       keyAndValue[1].length()),
                                      true));
        } else {
            param = new URLParam(key, String::emptyString);
        }
        m_list.push_back(param);
    }
}

void URLSearchParams::updateSourceUrlIfNeeds()
{
    if (m_sourceUrl) {
        m_sourceUrl->setSearch(toString(), false);
    }
}

void URLSearchParams::append(String* name, String* value)
{
    m_list.push_back(new URLParam(name, value));

    updateSourceUrlIfNeeds();
}

void URLSearchParams::deleteParams(String* name)
{
    m_list.erase(std::remove_if(m_list.begin(), m_list.end(),
                                [name](URLParam* item) {
                                    return item->key->equals(name);
                                }),
                 m_list.end());

    updateSourceUrlIfNeeds();
}

Optional<String*> URLSearchParams::get(String* name)
{
    Optional<String*> r;
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

    updateSourceUrlIfNeeds();
}

void URLSearchParams::sort()
{
    std::stable_sort(
        m_list.begin(), m_list.end(),
        [](URLParam* a, URLParam* b) { return a->key->compare(b->key) < 0; });

    updateSourceUrlIfNeeds();
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
        b.appendString(
            ResourceURL::createPercentEncodingString(param->key, true));
        b.appendString("=");
        b.appendString(
            ResourceURL::createPercentEncodingString(param->value, true));
        if (i < m_list.size() - 1) {
            b.appendString("&");
        }
    }

    return b.finalize();
}

class URLSearchParamsIterationSource final
    : public IterationSource<Optional<String*>, Optional<String*>> {
public:
    URLSearchParamsIterationSource(URLSearchParams* params)
    {
        m_params = params;
        m_index = 0;
    }

    virtual bool next(Escargot::ExecutionStateRef* state,
                      Optional<String*>& key, Optional<String*>& value) override
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

IterationSource<Optional<String*>, Optional<String*>>*
URLSearchParams::startIteration(Escargot::ExecutionStateRef* state)
{
    return new URLSearchParamsIterationSource(this);
}
} // namespace Starfish
