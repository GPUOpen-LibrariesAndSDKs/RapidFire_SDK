//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "RFEncoderAMF.h"

#include <assert.h>

#include <iostream>

#include <components/VideoConverter.h>
#include <components/VideoEncoderVCE.h>
#include <core/Platform.h>
#include <core/PropertyStorageEx.h>
#include <core/Surface.h>

#include "RFEncoderSettings.h"
#include "RFError.h"
#include "RFTypes.h"

#define CHECK_AMF_ERROR(a) if (a != AMF_OK) return RF_STATUS_AMF_FAIL

// Max supported dimensions of VCE with H.264 level 5.1
#define VCE_MAX_WIDTH   (1920*2)
#define VCE_MAX_HEIGHT  (1080*2)

using namespace std;
using namespace amf;


struct MAPPING_ENTRY
{
    const unsigned int   RFPropertyName;
    const wchar_t*       AMFPropertyName;
};


// Table that contains the mapping of RF names to AMF names
// Since AMF 1.22 FRAMERATE and FRAMERATE_DEN are passed as AMFRate structure. 
// The AMF Param name is AMF_VIDEO_ENCODER_FRAMERATE. 
// Therefore both entries point to AMF_VIDEO_ENCODER_FRAMERATE
static struct MAPPING_ENTRY g_PropertyNameMap[] = { {RF_ENCODER_PROFILE,                 AMF_VIDEO_ENCODER_PROFILE},
                                                    {RF_ENCODER_LEVEL,                   AMF_VIDEO_ENCODER_PROFILE_LEVEL},
                                                    {RF_ENCODER_BITRATE,                 AMF_VIDEO_ENCODER_TARGET_BITRATE},
                                                    {RF_ENCODER_PEAK_BITRATE,            AMF_VIDEO_ENCODER_PEAK_BITRATE},
                                                    {RF_ENCODER_FRAME_RATE,              AMF_VIDEO_ENCODER_FRAMERATE},
                                                    {RF_ENCODER_FRAME_RATE_DEN,          AMF_VIDEO_ENCODER_FRAMERATE},
                                                    {RF_ENCODER_RATE_CONTROL_METHOD,     AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD},
                                                    {RF_ENCODER_MIN_QP,                  AMF_VIDEO_ENCODER_MIN_QP},
                                                    {RF_ENCODER_MAX_QP,                  AMF_VIDEO_ENCODER_MAX_QP },
                                                    {RF_ENCODER_GOP_SIZE,                AMF_VIDEO_ENCODER_GOP_SIZE},
                                                    {RF_ENCODER_VBV_BUFFER_SIZE,         AMF_VIDEO_ENCODER_VBV_BUFFER_SIZE},
                                                    {RF_ENCODER_VBV_BUFFER_FULLNESS,     AMF_VIDEO_ENCODER_INITIAL_VBV_BUFFER_FULLNESS},
                                                    {RF_ENCODER_ENFORCE_HRD,             AMF_VIDEO_ENCODER_ENFORCE_HRD},
                                                    {RF_ENCODER_IDR_PERIOD,              AMF_VIDEO_ENCODER_IDR_PERIOD},
                                                    {RF_ENCODER_INTRA_REFRESH_NUM_MB,    AMF_VIDEO_ENCODER_INTRA_REFRESH_NUM_MBS_PER_SLOT},
                                                    {RF_ENCODER_DEBLOCKING_FILTER,       AMF_VIDEO_ENCODER_DE_BLOCKING_FILTER},
                                                    {RF_ENCODER_NUM_SLICES_PER_FRAME,    AMF_VIDEO_ENCODER_SLICES_PER_FRAME},
                                                    {RF_ENCODER_QUALITY_PRESET,          AMF_VIDEO_ENCODER_QUALITY_PRESET},
                                                    {RF_ENCODER_HALF_PIXEL,              AMF_VIDEO_ENCODER_MOTION_HALF_PIXEL},
                                                    {RF_ENCODER_QUARTER_PIXEL,           AMF_VIDEO_ENCODER_MOTION_QUARTERPIXEL} };


RFEncoderAMF::RFEncoderAMF()
    : RFEncoder()
    , m_bBlock(false)
    , m_uiPendingFrames(0)
    , m_amfEncodedFrame(NULL)
    , m_pContext(nullptr)
{
    m_pPreSubmitSettings.clear();

    m_strEncoderName = "RF_ENCODER_AMF";
}


RFEncoderAMF::~RFEncoderAMF()
{
    m_amfEncodedFrame = NULL;
    m_pPreSubmitSettings.clear();
}


bool RFEncoderAMF::isFormatSupported(RFFormat format) const
{
    return ((format == RF_NV12) || (format == RF_BGRA8));
}


RFStatus RFEncoderAMF::init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig)
{
    AMF_RESULT amfErr;

    if (!pConfig)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    // Get OpenCL context.
    m_pContext = dynamic_cast<const RFContextAMF*>(pContextCL);

    if (!m_pContext)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    m_format = pConfig->getInputFormat();

    if (!isFormatSupported(m_format))
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    // Only NV12 (and BGRA for DX9 only) are supported.
    AMF_SURFACE_FORMAT amfFormat = AMF_SURFACE_NV12;

    if (m_format == RF_BGRA8)
    {
        amfFormat = AMF_SURFACE_BGRA;
    }

    m_uiWidth  = pConfig->getEncoderWidth();
    m_uiHeight = pConfig->getEncoderHeight();

    if (m_uiWidth > VCE_MAX_WIDTH || m_uiHeight > VCE_MAX_HEIGHT)
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    m_uiAlignedWidth  = m_uiWidth;
    m_uiAlignedHeight = m_uiHeight;

    m_uiOutputWidth  = m_uiAlignedWidth;
    m_uiOutputHeight = m_uiAlignedHeight;

    m_amfContext = m_pContext->getAMFContext();

    if (!m_amfContext)
    {
        return RF_STATUS_AMF_FAIL;
    }

    amfErr = AMFCreateComponent(m_amfContext, AMFVideoEncoderVCE_AVC, &m_amfEncoder);

    if (amfErr != AMF_OK)
    {
        RF_Error(RF_STATUS_AMF_FAIL, "Failed to create HW encoder");
        return RF_STATUS_AMF_FAIL;
    }

    // Apply preset if defined in the configuration.
    if (pConfig->getEncoderPreset() != RF_PRESET_NONE)
    {
        if (!applyPreset(pConfig))
        {
            return RF_STATUS_INVALID_CONFIG;
        }

        // Disable B-Frames. We do not check for errors when applying this parameter since on some GPUs
        // B Frames won't be available.
        m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_B_PIC_PATTERN, 0);
    }
    else
    {
        m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_B_PIC_PATTERN, 0);

        if (!applyConfiguration(pConfig))
        {
            return RF_STATUS_INVALID_CONFIG;
        }
    }

    amfErr = m_amfEncoder->Init(amfFormat, m_uiAlignedWidth, m_uiAlignedHeight);
    CHECK_AMF_ERROR(amfErr);

    return RF_STATUS_OK;
}


RFStatus RFEncoderAMF::resize(unsigned int uiWidth, unsigned int uiHeight)
{
    AMF_RESULT amfErr;

    m_uiAlignedWidth  = m_uiWidth,
    m_uiAlignedHeight = m_uiHeight;

    m_uiOutputWidth  = m_uiAlignedWidth;
    m_uiOutputHeight = m_uiAlignedHeight;

    amfErr = m_amfEncoder->ReInit(m_uiAlignedWidth, m_uiAlignedHeight);
    CHECK_AMF_ERROR(amfErr);

    return RF_STATUS_OK;
}


RFStatus RFEncoderAMF::encode(unsigned int uiBufferIdx)
{
    AMF_RESULT amfErr;

    assert(m_pContext);

    amf::AMFDataPtr amfSurface = m_pContext->getAMFSurface(uiBufferIdx);

    // Set default picture type in case that a presubmit parameter had changed it in previous frame.
    amfErr = amfSurface->SetProperty(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_PICTURE_TYPE_NONE);

    // Need to submit new settings
    for (size_t i = m_pPreSubmitSettings.size(); i > 0; --i)
    {
        amfErr = amfSurface->SetProperty(m_pPreSubmitSettings[i - 1].first, m_pPreSubmitSettings[i - 1].second);
        m_pPreSubmitSettings.pop_back();

        // We dont abort if an error occurs when applying the properties. The frame will still be encoded.
    }

    amfErr = amfSurface->SetProperty(AMF_VIDEO_ENCODER_PICTURE_STRUCTURE, AMF_VIDEO_ENCODER_PICTURE_STRUCTURE_FRAME);
    CHECK_AMF_ERROR(amfErr);

    unsigned int uiFailedSubmitCount = 0;

    do
    {
        amfErr = m_amfEncoder->SubmitInput(amfSurface);

        // AMF_REPEAT is returned if the queue is full. We have too many pending frames and need 
        // to free space by calling QueryOutput. If a separate thread reads the data we will wait
        // and retry. It might be the case that the read thread is too slow and data is encoded faster
        // than read.
        // In a non multithreaded app this should not happen unless the app does not call getEncodedFrame.
        // ATTENTION: The RFSession manages a queue as well that will prevent to submit too many frames without
        // retreiving the result. The queue used by RFSession might be greater than the AMF queue and we still 
        // might run into the situation that we get an AMF_REPEAT.
        if (amfErr == AMF_REPEAT)
        {
            Sleep(1); // provide some time to the reader thread
            ++uiFailedSubmitCount;

            if (uiFailedSubmitCount > 2)
            {
                Sleep(1);
            }
        }
    } while (amfErr == AMF_REPEAT && uiFailedSubmitCount < 10);

    if (amfErr != AMF_OK)
    {
        return RF_STATUS_AMF_FAIL;
    }

    CHECK_AMF_ERROR(amfErr);

    ++m_uiPendingFrames;

    return RF_STATUS_OK;
}


RFStatus RFEncoderAMF::getEncodedFrame(unsigned int& uiSize, void* &pBitStream)
{
    bool            bBlocking = false;
    AMFDataPtr      pData;
    AMF_RESULT      amfErr = AMF_OK;
    amf_int32       picStruct = AMF_VIDEO_ENCODER_PICTURE_STRUCTURE_NONE;

    pBitStream = nullptr;

    if (m_uiPendingFrames == 0)
    {
        return RF_STATUS_NO_ENCODED_FRAME;
    }

    if (m_uiPendingFrames > 2 || m_bBlock)
    {
        // Block to make sure queue gets freed otherwise following calls to
        // encode frame would fail.
        bBlocking = true;
    }

    do
    {
        amfErr = m_amfEncoder->QueryOutput(&pData);

        if (amfErr == AMF_OK)
        {
            AMFBufferPtr pBuffer(pData);

            m_amfEncodedFrame = pBuffer;

            amfErr = pData->GetProperty(AMF_VIDEO_ENCODER_PICTURE_STRUCTURE, &picStruct);
            CHECK_AMF_ERROR(amfErr);

            pBitStream = m_amfEncodedFrame->GetNative();
            uiSize = static_cast<unsigned int>(m_amfEncodedFrame->GetSize());

            --m_uiPendingFrames;
        }
        else if (bBlocking && amfErr == AMF_REPEAT)
        {
            Sleep(1);
        }

    } while (bBlocking && amfErr == AMF_REPEAT);

    if (amfErr == AMF_REPEAT)
    {
        return RF_STATUS_NO_ENCODED_FRAME;
    }

    CHECK_AMF_ERROR(amfErr);

    return RF_STATUS_OK;
}


void RFEncoderAMF::setBlockingRead(bool block)
{
    m_bBlock = block;
}


RFStatus RFEncoderAMF::setParameter(const unsigned int uiParameterName, RFParameterType rfType, RFProperties value)
{
    RFStatus rfStatus = RF_STATUS_OK;

    // Check for presubmit paramters.
    unsigned int uiValue = static_cast<unsigned int>(value);

    if (uiValue && uiParameterName == RF_ENCODER_FORCE_INTRA_REFRESH)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_PICTURE_TYPE_IDR));
    }
    else if (uiValue && uiParameterName == RF_ENCODER_FORCE_I_FRAME)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_PICTURE_TYPE_I));
    }
    else if (uiValue && uiParameterName == RF_ENCODER_FORCE_P_FRAME)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_PICTURE_TYPE_P));
    }
    else if (uiValue && uiParameterName == RF_ENCODER_INSERT_SPS)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_INSERT_SPS, true));
    }
    else if (uiValue && uiParameterName == RF_ENCODER_INSERT_PPS)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_INSERT_PPS, true));
    }
    else
    {
        // Check if parameter is supported by encoder. Presubmit parameters are not part of the map.
        unsigned int uiNumSupportedProperties = sizeof(g_PropertyNameMap) / sizeof(struct MAPPING_ENTRY);

        unsigned int uiParameterIdx = 0;

        for (uiParameterIdx = 0; uiParameterIdx < uiNumSupportedProperties; ++uiParameterIdx)
        {
            if (g_PropertyNameMap[uiParameterIdx].RFPropertyName == uiParameterName)
            {
                break;
            }
        }

        if (uiParameterIdx == uiNumSupportedProperties)
        {
            return RF_STATUS_INVALID_ENCODER_PARAMETER;
        }

        rfStatus = setAMFProperty(uiParameterIdx, rfType, value);
    }

    return rfStatus;
}


RFParameterState RFEncoderAMF::getParameter(const unsigned int uiParameterName, RFProperties& value) const
{
    AMF_RESULT  amfErr;

    value = 0;

    if ((uiParameterName == RF_ENCODER_FORCE_INTRA_REFRESH) || (uiParameterName == RF_ENCODER_FORCE_I_FRAME) || (uiParameterName == RF_ENCODER_FORCE_P_FRAME) ||
        (uiParameterName == RF_ENCODER_INSERT_SPS) || (uiParameterName == RF_ENCODER_INSERT_PPS))
    {
        // Presubmit var has no permanent value. Indicate that parameters are ready to use.
        return RF_PARAMETER_STATE_READY;
    }
    else
    {
        // Check if parameter is supporetd by encoder. Presubmit parameters are not part of the map.
        unsigned int uiNumSupportedProperties = sizeof(g_PropertyNameMap) / sizeof(struct MAPPING_ENTRY);

        unsigned int uiParameterIdx = 0;

        for (uiParameterIdx = 0; uiParameterIdx < uiNumSupportedProperties; ++uiParameterIdx)
        {
            if (g_PropertyNameMap[uiParameterIdx].RFPropertyName == uiParameterName)
            {
                break;
            }
        }

        if (uiParameterIdx == uiNumSupportedProperties)
        {
            return RF_PARAMETER_STATE_INVALID;
        }

        const AMFPropertyInfo* pParamInfo;
        amfErr = m_amfEncoder->GetPropertyInfo(g_PropertyNameMap[uiParameterIdx].AMFPropertyName, &pParamInfo);

        if (amfErr != AMF_OK)
        {
            return RF_PARAMETER_STATE_INVALID;
        }

        if (pParamInfo->accessType == AMF_PROPERTY_ACCESS_PRIVATE)
        {
            return RF_PARAMETER_STATE_BLOCKED;
        }

        unsigned int uiAmfValue;

        // Handle FRAMERATE as special case. Since 1.1.22 the framerate is passed as AMFRate.
        if (uiParameterName == RF_ENCODER_FRAME_RATE || uiParameterName == RF_ENCODER_FRAME_RATE_DEN)
        {
            AMFRate amfFrameRate = { 30, 1 };

            amfErr = m_amfEncoder->GetProperty<AMFRate>(AMF_VIDEO_ENCODER_FRAMERATE, &amfFrameRate);

            if (uiParameterName == RF_ENCODER_FRAME_RATE)
            {
                uiAmfValue = static_cast<unsigned int>(amfFrameRate.num);
            }
            else
            {
                uiAmfValue = static_cast<unsigned int>(amfFrameRate.den);
            }
        }
        else
        {
            amfErr = m_amfEncoder->GetProperty(g_PropertyNameMap[uiParameterIdx].AMFPropertyName, &uiAmfValue);
        }

        if (amfErr == AMF_OK)
        {
            value = uiAmfValue;

            return (pParamInfo->accessType == AMF_PROPERTY_ACCESS_READ) ? RF_PARAMETER_STATE_BLOCKED : RF_PARAMETER_STATE_READY;
        }
    }

    return RF_PARAMETER_STATE_INVALID;
}


bool RFEncoderAMF::applyPreset(const RFEncoderSettings* pConfig)
{
    AMF_RESULT amfErr;

    if (!pConfig)
    {
        return false;
    }

    RFEncodePreset                  rfPreset = pConfig->getEncoderPreset();
    AMF_VIDEO_ENCODER_USAGE_ENUM    amfUsage = AMF_VIDEO_ENCODER_USAGE_ULTRA_LOW_LATENCY;

    switch (rfPreset)
    {
        case RF_PRESET_FAST:
            amfUsage = AMF_VIDEO_ENCODER_USAGE_ULTRA_LOW_LATENCY;
            break;

        case RF_PRESET_BALANCED:
            amfUsage = AMF_VIDEO_ENCODER_USAGE_LOW_LATENCY;
            break;

        case RF_PRESET_QUALITY:
            amfUsage = AMF_VIDEO_ENCODER_USAGE_TRANSCONDING;
            break;

        default:
            return false;
    }

    // Apply usage (preset that sets all parameters). 
    amfErr = m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_USAGE, amfUsage);


    if (amfErr != AMF_OK)
    {
        return false;
    }

    return true;
}


bool RFEncoderAMF::applyConfiguration(const RFEncoderSettings* pConfig)
{
    RFStatus rfStatus = RF_STATUS_OK;

    if (!pConfig)
    {
        return false;
    }

    unsigned int uiNumSupportedProperties = sizeof(g_PropertyNameMap) / sizeof(struct MAPPING_ENTRY);
    bool retValue = true;

    for (unsigned int i = 0; i < uiNumSupportedProperties; ++i)
    {
        RFStatus r = setAMFProperty(i, pConfig->getParameterType(g_PropertyNameMap[i].RFPropertyName), pConfig->getParameterValue<int>(g_PropertyNameMap[i].RFPropertyName));
        if (r != RF_STATUS_OK)
        {
            std::cout << g_PropertyNameMap[i].RFPropertyName << std::endl;
        }
        retValue &= (rfStatus == RF_STATUS_OK);
    }

    return retValue;
}


RFStatus RFEncoderAMF::setAMFProperty(unsigned int uiParameterIdx, RFParameterType rfType, RFProperties value)
{
    AMF_RESULT amfErr;

    unsigned int uiNumSupportedProperties = sizeof(g_PropertyNameMap) / sizeof(struct MAPPING_ENTRY);

    if (uiParameterIdx >= uiNumSupportedProperties)
    {
        return RF_STATUS_INVALID_ENCODER_PARAMETER;
    }

    const AMFPropertyInfo* pParamInfo;
    amfErr = m_amfEncoder->GetPropertyInfo(g_PropertyNameMap[uiParameterIdx].AMFPropertyName, &pParamInfo);

    if (!pParamInfo->AllowedWrite())
    {
        return RF_STATUS_PARAM_ACCESS_DENIED;
    }

    // Handle FRAMERATE as special case. Since 1.1.22 the framerate is passed as AMFRate.
    if (g_PropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_FRAME_RATE || g_PropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_FRAME_RATE_DEN)
    {
        AMFRate amfFrameRate = { 30, 1 };

        amfErr = m_amfEncoder->GetProperty<AMFRate>(AMF_VIDEO_ENCODER_FRAMERATE, &amfFrameRate);

        if (g_PropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_FRAME_RATE)
        {
            // Change framerate and leave DEN unchanged.
            amfFrameRate.num = static_cast<unsigned int>(value);
        }
        else
        {
            // Change DEN only
            amfFrameRate.den = static_cast<unsigned int>(value);
        }

        amfErr = m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_FRAMERATE, amfFrameRate);

    }
    else if (rfType == RF_PARAMETER_INT)
    {
        amfErr = m_amfEncoder->SetProperty(g_PropertyNameMap[uiParameterIdx].AMFPropertyName, static_cast<int>(value));
    }
    else if (rfType == RF_PARAMETER_UINT || rfType == RF_PARAMETER_BOOL)
    {
        // Cast bool and uint to an unsigned int to avoid performance warning when casting to bool.
        amfErr = m_amfEncoder->SetProperty(g_PropertyNameMap[uiParameterIdx].AMFPropertyName, static_cast<unsigned int>(value));
    }
    else
    {
        return RF_STATUS_INVALID_ENCODER_PARAMETER;
    }

    if (amfErr != AMF_OK)
    {
        return RF_STATUS_INVALID_ENCODER_PARAMETER;
    }

    return RF_STATUS_OK;
}