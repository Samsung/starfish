/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishStyleSheetList__
#define __StarFishStyleSheetList__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class Document;
class StyleSheet;

class StyleSheetList : public ScriptWrappable {
public:
    StyleSheetList(Document* document)
        : ScriptWrappable(this)
        , m_document(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isStyleSheetList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    /* DOM APIs */
    StyleSheet* item(unsigned long index);
    size_t length() const;

private:
    Document* m_document;
};
}

#endif
