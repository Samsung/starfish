/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishProcess__
#define __StarfishProcess__

namespace Starfish {

class ProcessUtil {
public:
    static PID getCurrentProcId();
    static bool launchProcess(const std::vector<std::string>& argv,
                              PID* process_handle);
    static bool launchProcessOnDoubleFork(const std::vector<std::string>& argv,
                                          PID* process_handle);
    static bool killProcess(PID pid, bool isWait);
};

} // namespace Starfish

#endif
