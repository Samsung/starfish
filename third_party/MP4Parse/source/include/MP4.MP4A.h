#ifndef _MP4_MP4A_H_
#define _MP4_MP4A_H_
#pragma once

#include "mp4.h"
#include "MP4.ContainerAtom.h"
#include "MP4.BinaryStream.h"

namespace MP4
{
    class MP4A : public ContainerAtom
    {
        private:
            
            
        protected:
            
            
        public:
            
            MP4A( void );
            
            void processData( MP4::BinaryStream * stream, size_t length ) override;

            uint16_t channels;
            uint16_t sample_size;
            uint32_t sample_rate;
    };
}

#endif /* _MP4_TRUN_H_ */
