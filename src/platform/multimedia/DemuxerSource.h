/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
