#pragma once

#include "ComponentCaps.h"

namespace amf
{
    /*  Encoder Capabilities Interface
        All codec-specific encoder caps interface should be derived from it.
    */
    class AMFEncoderCaps : public virtual AMFCaps
    {
    public:
        AMF_DECLARE_IID(0x92ea1672, 0x7311, 0x46e6, 0xa6, 0x2, 0x93, 0xa9, 0xc3, 0x6c, 0x6e, 0x69)

        //  Get maximum supported bitrate:
        virtual amf_uint32 GetMaxBitrate() const = 0;
        virtual amf_int32 AMF_STD_CALL GetMaxNumOfStreams() const = 0;

    };
    typedef AMFInterfacePtr_T<AMFEncoderCaps>   AMFEncoderCapsPtr;
}
