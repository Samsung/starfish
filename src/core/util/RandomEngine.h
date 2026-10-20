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

namespace Starfish {

class RandomEngine {
public:
    std::random_device& device()
    {
        return m_device;
    }

    static RandomEngine& instance();

private:
    RandomEngine();

    // Every supported host (libstdc++/libc++ on Linux, Tizen and Android;
    // MSVC on Windows) backs random_device with the OS CSPRNG, which is
    // also safe to call from worker threads.
    std::random_device m_device;
};

} // namespace Starfish
