/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "core/modules/serviceworker/cache/CachePolyfillLoader.h"
#include "core/modules/worker/host/WorkerScriptController.h"
#include "core/modules/serviceworker/util/Trace.h"

#include "binding/generated/Js2c_CacheStorage.h"
#include "core/util/GlobalOptions.h"

namespace Starfish {

class FileScope {
public:
    FileScope(const char* path, const char* mode)
    {
        m_file = std::fopen(path, mode);
    }
    ~FileScope()
    {
        if (m_file) {
            std::fclose(m_file);
        }
    }
    std::FILE* file()
    {
        return m_file;
    }

private:
    std::FILE* m_file{ nullptr };
};

static bool readStringFromFile(const std::string& filepath, std::string& output)
{
    FileScope fileScope(filepath.c_str(), "r");
    std::FILE* file = fileScope.file();

    if (file == nullptr) {
        return false;
    }

    std::fseek(file, 0, SEEK_END);
    size_t size = std::ftell(file);
    std::rewind(file);
    output.resize(size);

    if (std::fread(&output[0], sizeof(char), size, file) == 0) {
        return false;
    }

    return true;
}

bool CachePolyfillLoader::load(WorkerScriptController* controller)
{
    TRACE(CACHE, "Load: s_js2c_cache_min_js");

    String* text;

    if (GlobalOptions::instance().has("CACHE_MODULE_PATH") == false) {
        text = String::fromUTF8(s_js2c_cache_min_js.c_str(),
                                s_js2c_cache_min_js.size());
    } else {
        // Read the file from local storage.
        std::string js2c_cache_min_js;
        std::string filepath =
            GlobalOptions::instance().get("CACHE_MODULE_PATH");

        TRACE(CACHE, "Load file:", filepath);
        if (readStringFromFile(filepath, js2c_cache_min_js) == false) {
            STARFISH_LOG_ERROR("Fail to load script from file");
            return false;
        }
        text = String::fromUTF8(js2c_cache_min_js.c_str(),
                                js2c_cache_min_js.size());
    }

    if (!controller->evaluatefromString(text)) {
        STARFISH_LOG_ERROR("Fail to load global script: js2c_cache_min_js");
        return false;
    }
    return true;
}

} // namespace Starfish

#endif
