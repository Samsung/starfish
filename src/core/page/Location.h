/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishLocation__
#define __StarFishLocation__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class StarFish;
class ResourceURL;

class Location : public ScriptWrappable, public DocumentHoldable {
public:
    Location(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isLocation() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return DocumentHoldable::scriptBindingInstance();
    }

    ResourceURL* url();
    String* href();
    String* host();
    String* hostname();
    String* protocol();
    String* pathname();
    String* search();
    String* hash();

    void setHref(String* newURL);
    void setHost(String* newHost);
    void setHostname(String* newHostname);
    void setProtocol(String* newProtocol);
    void setPathname(String* newPath, bool needRemovingDots = true);
    void setSearch(String* search);
    void setHash(String* search);

    void close()
    {
    }

    void setLocation(String* newURL);
};
}

#endif
