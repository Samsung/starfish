/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishImageDecoder__
#define __StarFishImageDecoder__

#if defined(PORT_IMAGEDECODER_BACKEND_MISC)

namespace StarFish {

class ImageDecoder : public gc {
public:
    enum ImageFormat { PNG, JPG, GIF, ERROR };

    ImageDecoder(const char* filename);
    ~ImageDecoder();

    void* buffer();
    int width();
    int height();

private:
    FILE* m_fp;
    int m_width;
    int m_height;
    unsigned char* m_imageData;

    ImageFormat parseImageFormat();
    void readPNGFile();

protected:
    ImageDecoder()
    {
    }
};
}
#endif
#endif
