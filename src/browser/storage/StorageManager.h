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

#ifndef __StarfishStorageManager__
#define __StarfishStorageManager__

#include "rapidjson/document.h"

namespace Starfish {

class WebOrigin;

typedef rapidjson::GenericDocument<rapidjson::UTF8<>> JsonDocument;

class StorageManager : public gc {
public:
    StorageManager(String* localStoragePath);
    ~StorageManager()
    {
    }
    void load(GCUnorderedMap<String*, String*>& out, WebOrigin* webOrigin);
    Nullable<String*> getItem(WebOrigin* webOrigin, String* key);
    void setItem(WebOrigin* webOrigin, String* key, String* value);
    void removeItem(WebOrigin* webOrigin, String* key);
    void clear(WebOrigin* webOrigin);
    unsigned long size(WebOrigin* webOrigin);

private:
    void loadFromFileToJsonDocument();
    void writeJsonDocumentAsFile();
    String* m_localStoragePath;
    JsonDocument* m_jsonDocument;
};
} // namespace Starfish

#endif
