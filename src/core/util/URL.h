/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
    void setUsername(String* newUsername);
    String* password();
    void setPassword(String* newPassword);
    String* host();
    void setHost(String* newHost);
    String* hostname();
    void setHostname(String* newHostname);
    String* port();
    void setPort(String* newPort);
    String* pathname();
    void setPathname(String* newPath, bool needRemovingDots = true);
    String* search();
    void setSearch(String* newSearch);
    String* hash();
    void setHash(String* newHash);

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    ResourceURL* m_resourceURL;
};
}

#endif
