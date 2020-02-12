/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishURLSearchParams__
#define __StarfishURLSearchParams__

#include "binding/ScriptWrappable.h"
#include "binding/IterationSource.h"

namespace Starfish {

struct URLParam : public gc {
    URLParam()
    {
    }

    URLParam(String* k, String* v)
        : key(k)
        , value(v)
    {
    }

    URLParam(std::string& k, std::string& v)
    {
        key = String::fromUTF8(k.data(), k.length());
        value = String::fromUTF8(v.data(), v.length());
    }

    String* key{ nullptr };
    String* value{ nullptr };
};

class URLSearchParams : public ScriptWrappable {
public:
    URLSearchParams(ExecutionContext* executionContext,
                    String* init = String::emptyString);
    URLSearchParams(ExecutionContext* executionContext,
                    GCVector<GCVector<String*>> init);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(URLSearchParams);

    void append(String* name, String* value);
    void deleteParams(String* name);
    Nullable<String*> get(String* name);
    GCVector<String*> getAll(String* name);
    bool has(String* name);
    void set(String* name, String* value);

    void sort();

    size_t length();
    URLParam* at(size_t i);
    String* toString();
    IterationSource<Nullable<String*>, Nullable<String*>>* startIteration(
        ExecutionStateRef* state);

private:
    ExecutionContext* m_executionContext{ nullptr };
    ResourceURL* m_resourceUrl{ nullptr };
    GCVector<URLParam*> m_list;

    void parse(String* str);
};
} // namespace Starfish

#endif
