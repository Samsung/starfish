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

#include "StarfishConfig.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLInputElement.h"
#include "core/fileapi/File.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

// `files` is not backed by a real FileList object. It returns a fresh JS Array
// of File objects each call, which is enough for Array.from(input.files) and
// input.files.length. Returns null when no files have been selected.
ValueRef* filesHTMLInputElementGetterFunction(ExecutionStateRef* state,
                                              ValueRef* thisValue, size_t argc,
                                              ValueRef** argv,
                                              bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLInputElement);

    GCVector<File*>* files = originalObj->selectedFiles();
    if (!files) {
        return ValueRef::createNull();
    }
    ValueVectorRef* elements = ValueVectorRef::create();
    for (size_t i = 0; i < files->size(); i++) {
        elements->pushBack((*files)[i]->scriptValue());
    }
    return ArrayObjectRef::create(state, elements);
}

ValueRef* sizeHTMLInputElementSetterFunction(ExecutionStateRef* state,
                                             ValueRef* thisValue, size_t argc,
                                             ValueRef** argv,
                                             bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(HTMLInputElement);

    ValueRef* arg0 = argv[0];
    String* value0 = toBrowserString(state, arg0);

    try {
        originalObj->setSize(value0);
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return scriptUndefined();
}
} // namespace Starfish
