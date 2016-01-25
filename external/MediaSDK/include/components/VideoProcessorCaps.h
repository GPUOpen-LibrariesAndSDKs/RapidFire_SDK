#pragma once

#include "ComponentCaps.h"
#include "VideoProcessor.h"

namespace amf
{
    /*  Video Processor Capabilities Interface
    */

    class AMFVideoProcessorFeaturesCaps : public virtual AMFInterface
    {
    public:
        virtual AMF_RESULT  AMF_STD_CALL SetImageInfo(amf_int32 width, amf_int32 height, bool interlace, AMF_VIDEO_PROCESSOR_DEINTERLACING_MODE_ENUM interlaceMethod) = 0;
        virtual amf_bool    AMF_STD_CALL IsSupported(const wchar_t* filterID) const = 0;
        virtual amf_bool    AMF_STD_CALL IsRealtime(const wchar_t* filterID) const = 0;
    };
    typedef AMFInterfacePtr_T<AMFVideoProcessorFeaturesCaps> AMFVideoProcessorFeaturesCapsPtr;

    class AMFVideoProcessorCaps : public AMFCaps
    {
    public:
        AMF_DECLARE_IID(0xf6b69de2, 0xea38, 0x4fa9, 0x9a, 0x38, 0xd2, 0x10, 0x91, 0x69, 0x93, 0xdd)

        virtual AMF_RESULT  AMF_STD_CALL GetFeaturesCaps(AMFVideoProcessorFeaturesCaps** pFeaturesCaps) = 0;
    };

    typedef AMFInterfacePtr_T<AMFVideoProcessorCaps> AMFVideoProcessorCapsPtr;
}
