/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(__StarfishMessage__)
#define __StarfishMessage__

namespace Starfish {

class Message : public gc {
public:
    Message(const char* msgname = "");
    void addParam(Archivable* param);
    void archive(Archiver& arch);

    // getter
    Archivable* param(size_t index);
    std::string name();

    // statics
    static void init();
    static void archive(Archiver& arch, Archivable*& param);

private:
    std::string m_name;
    GCVector<Archivable*> m_params;
};

} // namespace Starfish

#endif
