/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishMediaSourceClient__)
#define __StarFishMediaSourceClient__

class AVFormatContext;
class AVIOContext;
namespace StarFish {

struct MediaRawData {
    char* memory;
    size_t size;
};

class AVIOContextWrapper : public gc {
public:
    AVIOContextWrapper(char* videoData, const int videoLen);
    AVIOContextWrapper(MediaRawData data);
    ~AVIOContextWrapper();

    void pushMediaData(MediaRawData data);

    static int read(void *opaque, unsigned char *buf, int buf_size);

    static int write(void *opaque, unsigned char *buf, int buf_size)
    {
        // TODO
        return 0;
    }

    static int64_t seek(void *opaque, int64_t offset, int whence)
    {
        // TODO
        return 0;
    }

    AVIOContext* get_avio()
    {
        return m_avioctx;
    }

    int pos()
    {
        return m_pos;
    }

    void clearPosition()
    {
        m_pos = 0;
    }

    void increasePosition(int size)
    {
        m_pos += size;
    }

    int dataListSize()
    {
        return m_rawDataList.size();
    }

    bool hasNextDataIdx()
    {
        return (m_curListIdx + 1 < dataListSize());
    }

    bool moveToNextDataIfPossible()
    {
        if (!hasNextDataIdx())
            return false;
        m_curListIdx++;
        clearPosition();
        return true;
    }

    int curDataListIdx()
    {
        return m_curListIdx;
    }

    int dataSize()
    {
        return m_rawDataList[m_curListIdx].size;
    }

    int totalSize()
    {
        return m_totalSize;
    }

    char* rawData()
    {
        return m_rawDataList[m_curListIdx].memory;
    }

private:
    // Output buffer
    int m_bufferSize;
    char* m_buffer;

    // Internal buffer
    int m_pos;
    char* m_rawData;
    int m_dataSize;
    int m_totalSize;
    std::vector<MediaRawData> m_rawDataList;
    int m_curListIdx;
    AVIOContext* m_avioctx;
};


class VideoPlayer;
class Mutex;

class MediaSourceClient : public gc {
public:
    struct MediaPacket {
        uint8_t* data;
        int size;
        int64_t pts;
    };
    MediaSourceClient();
    void registerMediaPlayer(VideoPlayer* player);
    void setFormat(String* type);
    void appendBuffer(MediaRawData buffer);
    AVFormatContext* formatContext()
    {
        return m_formatContext;
    }

    int videoStreamIdx()
    {
        return m_videoStreamIdx;
    }

    int audioStreamIdx()
    {
        return m_audioStreamIdx;
    }

    VideoPlayer* videoPlayer()
    {
        return m_player;
    }

    bool isReady()
    {
        return m_isReady;
    }

    void setReady()
    {
        m_isReady = true;
    }

protected:
    VideoPlayer* m_player;
    AVFormatContext* m_formatContext;
    AVIOContextWrapper* m_ioContext;
    int m_videoStreamIdx;
    int m_audioStreamIdx;
    int m_subtitleStreamIdx;
    bool m_isWebm;
    bool m_isReady;
    Mutex* m_mutex;
};

}

#endif
