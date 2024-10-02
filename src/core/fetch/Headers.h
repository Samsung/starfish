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

#ifndef __StarfishHeaders__
#define __StarfishHeaders__

#include "binding/ScriptWrappable.h"
#include "binding/IterationSource.h"
#include "binding/Iterable.h"

namespace Starfish {

class ExecutionContext;
class HeadersData;
enum class Guard;

typedef ScriptValue HeadersInit;

class Headers : public ScriptWrappable, public Iterable<String*, String*> {
public:
    Headers(ExecutionContext* executionContext);
    Headers(ExecutionContext* executionContext, HeadersInit headerInit);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(Headers)

    void fill(HeadersInit headersInit);
    Optional<String*> get(String* name);
    void append(String* name, String* value);
    void set(String* name, String* value);
    void noCheckValidSet(const std::string& lowerCaseName,
                         const std::string& value);
    bool has(String* name);
    bool noCheckValidHas(const std::string& lowerCaseName);
    void deleteHeader(String* name);

    void setGuard(Guard guard);
    Guard guard();

    void copyHeaders(Headers* src);
    String* extractMIMEType();

    IterationSource<Optional<String*>, Optional<String*>>* startIteration(
        Escargot::ExecutionStateRef* state) override;

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    HeadersData* headersData()
    {
        return m_headersData;
    }

private:
    ExecutionContext* m_executionContext;
    HeadersData* m_headersData;

    void initHeadersFromHeaders(Headers* headers);
    void initHeadersFromArrayObject(ScriptObject object);
    void initHeadersFromObject(ScriptObject object);

    void setHeader(ScriptValue keyValue, ScriptValue scriptValue,
                   Escargot::ExecutionStateRef* state);

    void checkValidHeader(String* name);
    void checkValidHeader(String* name, String* value);
};
} // namespace Starfish

#endif
