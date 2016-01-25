#pragma once

#include "ComponentCaps.h"

namespace amf
{
    /*  Decoder Codec Capabilities Interface
    */

    class AMFDecoderCaps : public virtual AMFCaps
    {
    public:
        AMF_DECLARE_IID(0x367247f4, 0x25a0, 0x47d1,  0x9d, 0x5b, 0x90, 0x1, 0x78, 0x97, 0xea, 0xdc)

        virtual amf_int32 AMF_STD_CALL GetMaxNumOfStreams() const = 0;
    };

    typedef AMFInterfacePtr_T<AMFDecoderCaps>   AMFDecoderCapsPtr;
}

