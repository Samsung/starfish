#include "MP4.MP4A.h"

using namespace MP4;
          
MP4A::MP4A( void )
    : ContainerAtom(MP4_PARSER_DEFINE_TYPE_STRING("mp4a"))
    , channels(0)
    , sample_size(0)
    , sample_rate(1)
{

}

void MP4A::processData( MP4::BinaryStream * stream, size_t length )
{
    // Sample entry
    stream->ignore(8);
    // Audio sample entry
    stream->ignore(8);

    channels = stream->readBigEndianUnsignedShort();
    sample_size = stream->readBigEndianUnsignedShort();
    stream->ignore(4);
    sample_rate = stream->readBigEndianUnsignedInteger() >> 16;
}
