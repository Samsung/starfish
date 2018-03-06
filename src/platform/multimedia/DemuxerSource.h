/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishDemuxerSource__)
#define __StarFishDemuxerSource__

namespace StarFish {
class DemuxerSource : public gc {
public:
    enum SeekWhence {
        SeekWhenceSet,
        SeekWhenceCurrent,
        SeekWhenceEnd,
        SeekWhenceLookSize,
    };
    virtual ~DemuxerSource()
    {
    }
    virtual int64_t onSeek(int64_t position, SeekWhence whence) = 0;
    virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead,
                        int& errorCode, uint8_t* buffer) = 0;
};
}
#endif
