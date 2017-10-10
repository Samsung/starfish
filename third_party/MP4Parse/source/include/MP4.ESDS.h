#ifndef _MP4_ESDS_H_
#define _MP4_ESDS_H_
#pragma once

#include "mp4.h"
#include "MP4.DataAtom.h"
#include "MP4.BinaryStream.h"

namespace MP4
{
    class ESDS : public DataAtom
    {
        private:
            
            
        protected:
            
            
        public:
            ESDS( void );
            
            void processData( MP4::BinaryStream * stream, size_t length );
            virtual uint32_t getType( void )
            {
                return MP4_PARSER_DEFINE_TYPE_STRING("esds");
            }
            std::vector<uint8_t> decoder_config;
            uint32_t decoder_config_size;
    };
}

#endif /* _MP4_BXML_H_ */
