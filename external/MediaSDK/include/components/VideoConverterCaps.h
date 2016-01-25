#pragma once

#include "ComponentCaps.h"

namespace amf
{
    /*  Video Converter Capabilities Interface
    */

    class AMFVideoConverterCaps : public virtual AMFCaps
    {
    public:
        AMF_DECLARE_IID(0xc2cf7c7d, 0x5c9a, 0x4060, 0xbc, 0x67, 0xef, 0xed, 0x10, 0xb1, 0xcb, 0x47)
    };

    typedef AMFInterfacePtr_T<AMFVideoConverterCaps>   AMFVideoConverterCapsPtr;
}

