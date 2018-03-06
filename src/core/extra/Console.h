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

#ifndef __StarFishConsole__
#define __StarFishConsole__

namespace StarFish {

class StarFish;

#define CONSOLE_APIS(F) \
    F(log)              \
    F(info)             \
    F(error)            \
    F(warn)             \
    F(debug)

class Console : public gc {
public:
    Console(StarFish* starFish);
    void log(String* m);
    void info(String* m);
    void error(String* m);
    void warn(String* m);
    void debug(String* m);

protected:
    StarFish* m_starFish;
};
}

#endif
