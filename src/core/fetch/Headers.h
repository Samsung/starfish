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

#ifndef __StarfishHeaders__
#define __StarfishHeaders__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"
#include "binding/IterationSource.h"
#include "core/fetch/HeadersData.h"

namespace Starfish {

class Document;
class HeadersData;

typedef ScriptValue HeadersInit;

class Headers : public ScriptWrappable, public DocumentHoldable {
public:
    Headers(Document* document);
    Headers(Document* document, HeadersInit headerInit);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_instance;
    }
    virtual bool isHeaders() const override;

    void fill(HeadersInit headersInit);
    Nullable<String*> get(String* name);
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

    IterationSource<Nullable<String*>, Nullable<String*>>* startIteration(
        ExecutionStateRef* state);

private:
    ScriptBindingInstance* m_instance;
    HeadersData* m_headersData;

    void initHeadersFromHeaders(Headers* headers);
    void initHeadersFromArrayObject(ScriptObject object);
    void initHeadersFromObject(ScriptObject object);

    void setHeader(ScriptValue keyValue, ScriptValue scriptValue,
                   Escargot::ExecutionStateRef* state);

    void checkValidHeader(String* name);
    void checkValidHeader(String* name, String* value);
};
}

#endif
