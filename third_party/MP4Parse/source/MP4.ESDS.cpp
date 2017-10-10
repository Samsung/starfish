#include "MP4.ESDS.h"

using namespace MP4;
          
ESDS::ESDS()
    // : decoder_config_size(0)
{
}

static uint32_t getDescriptorLength(MP4::BinaryStream * stream)
{
    uint8_t tmp = stream->readUnsignedChar();
    uint32_t len = 0;
    while (tmp & 0x80) {
        len = ((len << 7) | (tmp & 0x7f));
        tmp = stream->readUnsignedChar();
    }
    len = ((len << 7) | (tmp & 0x7f));
    return len;
}

void ESDS::processData(MP4::BinaryStream * stream, size_t length )
{
    size_t start = stream->pos();
    uint8_t v = stream->readUnsignedChar();
    uint32_t f = (stream->readUnsignedChar() << 16) + (stream->readUnsignedChar() << 8) + (stream->readUnsignedChar() << 0);
    uint8_t tag1 = stream->readUnsignedChar();
    if (tag1 == 3) {
        getDescriptorLength(stream);
        stream->ignore(3);
        uint8_t tag2 = stream->readUnsignedChar();
        if (tag2 == 4) {
            getDescriptorLength(stream);
            stream->ignore(13);
            uint8_t tag3 = stream->readUnsignedChar();
            if (tag3 == 5) {
                decoder_config_size = getDescriptorLength(stream);
                decoder_config.resize(decoder_config_size);
                stream->read((char*)decoder_config.data(), decoder_config_size);
            }
        }
    }
    size_t end = stream->pos();
    if (end < start + length) {
        stream->ignore(start + length - end);
    }
}
