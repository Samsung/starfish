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

class AVIOContextWrapper : public gc {
public:
    AVIOContextWrapper(char* videoData, const int videoLen);
    ~AVIOContextWrapper();

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

    void increasePosition(int size)
    {
        m_pos += size;
    }

    int dataSize()
    {
        return m_dataSize;
    }

    char* rawData()
    {
        return m_rawData;
    }

private:
    // Output buffer
    int m_bufferSize;
    char* m_buffer;

    // Internal buffer
    int m_pos;
    char* m_rawData;
    int m_dataSize;
    AVIOContext* m_avioctx;
};


class VideoPlayer;

class MediaSourceClient : public gc {
public:
    struct MediaRawBuffer {
        char* memory;
        size_t size;
    };
    struct MediaPacket {
        uint8_t* data;
        int size;
        int64_t pts;
    };
    MediaSourceClient();
    void registerMediaPlayer(VideoPlayer* player);
    void setFormat(String* type);
    void appendBuffer(MediaRawBuffer buffer);
    AVFormatContext* formatContext()
    {
        return m_formatContext;
    }

protected:
    VideoPlayer* m_player;
    AVFormatContext* m_formatContext;
    bool m_isWebm;
};

}

#endif
