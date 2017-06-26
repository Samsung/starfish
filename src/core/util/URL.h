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

#ifndef __StarFishURL__
#define __StarFishURL__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class Blob;
class MediaSource;

class URL : public ScriptWrappable {
public:
    URL(Window* window, String* url);
    URL(Window* window, String* url, String* baseURL);
    URL(ScriptBindingInstance* ins, String* url, String* baseURL);

    static void revokeObjectURL(Document* document, String* blobURLRef);
    static String* createObjectURL(Blob* blob);
#ifdef STARFISH_ENABLE_MULTIMEDIA
    static String* createObjectURL(MediaSource* mediaSource);
#endif

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isURL() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    String* origin();
    String* href();
    void setHref(String* newHref);
    String* protocol();
    void setProtocol(String* newProtocol);
    String* username();
    void setUsername(String* newPath);
    String* password();
    void setPassword(String* newPath);
    String* host();
    void setHost(String* newHost);
    String* hostname();
    void setHostname(String* newHostname);
    String* port();
    void setPort(String* newPort);
    String* pathname();
    void setPathname(String* newPath, bool needRemovingDots = true);
    String* search();
    void setSearch(String* newPath);
    String* hash();
    void setHash(String* newPath);

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    ResourceURL* m_resourceURL;
};
}

#endif
