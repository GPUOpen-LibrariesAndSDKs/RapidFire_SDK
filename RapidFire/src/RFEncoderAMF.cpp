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
#include <components/VideoEncoderHEVC.h>
#include <core/Platform.h>
#include <core/PropertyStorageEx.h>
#include <core/Surface.h>

#include "AMFWrapper.h"
#include "RFEncoderSettings.h"
#include "RFError.h"
#include "RFTypes.h"

#define CHECK_AMF_ERROR(a) if (a != AMF_OK) return RF_STATUS_AMF_FAIL

// Max supported dimensions of VCE with H.264 level 5.1
#define VCE_MAX_WIDTH   (1920*2)
#define VCE_MAX_HEIGHT  (1080*2)

using namespace std;
using namespace amf;


// Table that contains the mapping of RF names to AMF names
// Since AMF 1.22 FRAMERATE and FRAMERATE_DEN are passed as AMFRate structure.
// The AMF Param name is AMF_VIDEO_ENCODER_FRAMERATE.
// Therefore both entries point to AMF_VIDEO_ENCODER_FRAMERATE
static const RFEncoderAMF::MAPPING_ENTRY g_AVCPropertyNameMap[] = { {RF_ENCODER_PROFILE,                     AMF_VIDEO_ENCODER_PROFILE},
                                                                    {RF_ENCODER_LEVEL,                       AMF_VIDEO_ENCODER_PROFILE_LEVEL},
                                                                    {RF_ENCODER_USAGE,                       AMF_VIDEO_ENCODER_USAGE},
                                                                    {RF_ENCODER_COMMON_LOW_LATENCY_INTERNAL, L"CommonLowLatencyInternal"},
                                                                    {RF_ENCODER_BITRATE,                     AMF_VIDEO_ENCODER_TARGET_BITRATE},
                                                                    {RF_ENCODER_PEAK_BITRATE,                AMF_VIDEO_ENCODER_PEAK_BITRATE},
                                                                    {RF_ENCODER_FRAME_RATE,                  AMF_VIDEO_ENCODER_FRAMERATE},
                                                                    {RF_ENCODER_FRAME_RATE_DEN,              AMF_VIDEO_ENCODER_FRAMERATE},
                                                                    {RF_ENCODER_RATE_CONTROL_METHOD,         AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD},
                                                                    {RF_ENCODER_MIN_QP,                      AMF_VIDEO_ENCODER_MIN_QP},
                                                                    {RF_ENCODER_MAX_QP,                      AMF_VIDEO_ENCODER_MAX_QP },
                                                                    {RF_ENCODER_VBV_BUFFER_SIZE,             AMF_VIDEO_ENCODER_VBV_BUFFER_SIZE},
                                                                    {RF_ENCODER_VBV_BUFFER_FULLNESS,         AMF_VIDEO_ENCODER_INITIAL_VBV_BUFFER_FULLNESS},
                                                                    {RF_ENCODER_ENFORCE_HRD,                 AMF_VIDEO_ENCODER_ENFORCE_HRD},
                                                                    {RF_ENCODER_ENABLE_VBAQ,                 AMF_VIDEO_ENCODER_ENABLE_VBAQ},
                                                                    {RF_ENCODER_IDR_PERIOD,                  AMF_VIDEO_ENCODER_IDR_PERIOD},
                                                                    {RF_ENCODER_INTRA_REFRESH_NUM_MB,        AMF_VIDEO_ENCODER_INTRA_REFRESH_NUM_MBS_PER_SLOT},
                                                                    {RF_ENCODER_DEBLOCKING_FILTER,           AMF_VIDEO_ENCODER_DE_BLOCKING_FILTER},
                                                                    {RF_ENCODER_NUM_SLICES_PER_FRAME,        AMF_VIDEO_ENCODER_SLICES_PER_FRAME},
                                                                    {RF_ENCODER_QUALITY_PRESET,              AMF_VIDEO_ENCODER_QUALITY_PRESET},
                                                                    {RF_ENCODER_HALF_PIXEL,                  AMF_VIDEO_ENCODER_MOTION_HALF_PIXEL},
                                                                    {RF_ENCODER_QUARTER_PIXEL,               AMF_VIDEO_ENCODER_MOTION_QUARTERPIXEL}};


static const RFEncoderAMF::MAPPING_ENTRY g_HEVCPropertyNameMap[] = { {RF_ENCODER_HEVC_USAGE,                           AMF_VIDEO_ENCODER_HEVC_USAGE},
                                                                     {RF_ENCODER_HEVC_PROFILE,                         AMF_VIDEO_ENCODER_HEVC_PROFILE},
                                                                     {RF_ENCODER_HEVC_LEVEL,                           AMF_VIDEO_ENCODER_HEVC_PROFILE_LEVEL},
                                                                     {RF_ENCODER_HEVC_TIER,                            AMF_VIDEO_ENCODER_HEVC_TIER},
                                                                     {RF_ENCODER_HEVC_RATE_CONTROL_METHOD,             AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD},
                                                                     {RF_ENCODER_HEVC_FRAMERATE,                       AMF_VIDEO_ENCODER_HEVC_FRAMERATE},
                                                                     {RF_ENCODER_HEVC_FRAMERATE_DEN,                   AMF_VIDEO_ENCODER_HEVC_FRAMERATE},
                                                                     {RF_ENCODER_HEVC_VBV_BUFFER_SIZE,                 AMF_VIDEO_ENCODER_HEVC_VBV_BUFFER_SIZE},
                                                                     {RF_ENCODER_HEVC_INITIAL_VBV_BUFFER_FULLNESS,     AMF_VIDEO_ENCODER_HEVC_INITIAL_VBV_BUFFER_FULLNESS},
                                                                     {RF_ENCODER_HEVC_RATE_CONTROL_PREANALYSIS_ENABLE, AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_PREANALYSIS_ENABLE},
                                                                     {RF_ENCODER_HEVC_ENABLE_VBAQ,                     AMF_VIDEO_ENCODER_HEVC_ENABLE_VBAQ},
                                                                     {RF_ENCODER_HEVC_TARGET_BITRATE,                  AMF_VIDEO_ENCODER_HEVC_TARGET_BITRATE},
                                                                     {RF_ENCODER_HEVC_PEAK_BITRATE,                    AMF_VIDEO_ENCODER_HEVC_PEAK_BITRATE},
                                                                     {RF_ENCODER_HEVC_MIN_QP_I,                        AMF_VIDEO_ENCODER_HEVC_MIN_QP_I},
                                                                     {RF_ENCODER_HEVC_MAX_QP_I,                        AMF_VIDEO_ENCODER_HEVC_MAX_QP_I},
                                                                     {RF_ENCODER_HEVC_MIN_QP_P,                        AMF_VIDEO_ENCODER_HEVC_MIN_QP_P},
                                                                     {RF_ENCODER_HEVC_MAX_QP_P,                        AMF_VIDEO_ENCODER_HEVC_MAX_QP_P},
                                                                     {RF_ENCODER_HEVC_QP_I,                            AMF_VIDEO_ENCODER_HEVC_QP_I},
                                                                     {RF_ENCODER_HEVC_QP_P,                            AMF_VIDEO_ENCODER_HEVC_QP_P},
                                                                     {RF_ENCODER_HEVC_ENFORCE_HRD,                     AMF_VIDEO_ENCODER_HEVC_ENFORCE_HRD},
                                                                     {RF_ENCODER_HEVC_MAX_AU_SIZE,                     AMF_VIDEO_ENCODER_HEVC_MAX_AU_SIZE},
                                                                     {RF_ENCODER_HEVC_FILLER_DATA_ENABLE,              AMF_VIDEO_ENCODER_HEVC_FILLER_DATA_ENABLE},
                                                                     {RF_ENCODER_HEVC_RATE_CONTROL_SKIP_FRAME_ENABLE,  AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_SKIP_FRAME_ENABLE},
                                                                     {RF_ENCODER_HEVC_HEADER_INSERTION_MODE,           AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE},
                                                                     {RF_ENCODER_HEVC_GOP_SIZE,                        AMF_VIDEO_ENCODER_HEVC_GOP_SIZE},
                                                                     {RF_ENCODER_HEVC_NUM_GOPS_PER_IDR,                AMF_VIDEO_ENCODER_HEVC_NUM_GOPS_PER_IDR},
                                                                     {RF_ENCODER_HEVC_DE_BLOCKING_FILTER_DISABLE,      AMF_VIDEO_ENCODER_HEVC_DE_BLOCKING_FILTER_DISABLE},
                                                                     {RF_ENCODER_HEVC_SLICES_PER_FRAME,                AMF_VIDEO_ENCODER_HEVC_SLICES_PER_FRAME},
                                                                     {RF_ENCODER_HEVC_QUALITY_PRESET,                  AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET},
                                                                     {RF_ENCODER_HEVC_MOTION_HALF_PIXEL,               AMF_VIDEO_ENCODER_HEVC_MOTION_HALF_PIXEL},
                                                                     {RF_ENCODER_HEVC_MOTION_QUARTERPIXEL,             AMF_VIDEO_ENCODER_HEVC_MOTION_QUARTERPIXEL} };


RFEncoderAMF::RFEncoderAMF()
    : RFEncoder()
    , m_bBlock(false)
    , m_uiPendingFrames(0)
    , m_amfEncodedFrame(NULL)
    , m_pContext(nullptr)
    , m_videoCodec(RF_VIDEO_CODEC_NONE)
    , m_pPropertyNameMap(nullptr)
    , m_uiPropertyNameMapCount(0)
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

    if (pConfig->getVideoCodec() == RF_VIDEO_CODEC_AVC)
    {
        m_videoCodec = RF_VIDEO_CODEC_AVC;
        m_pPropertyNameMap = g_AVCPropertyNameMap;
        m_uiPropertyNameMapCount = sizeof(g_AVCPropertyNameMap) / sizeof(MAPPING_ENTRY);
        amfErr = AMFWrapper::CreateComponent(m_amfContext, AMFVideoEncoderVCE_AVC, &m_amfEncoder);
    }
    else if (pConfig->getVideoCodec() == RF_VIDEO_CODEC_HEVC)
    {
        m_videoCodec = RF_VIDEO_CODEC_HEVC;
        m_pPropertyNameMap = g_HEVCPropertyNameMap;
        m_uiPropertyNameMapCount = sizeof(g_HEVCPropertyNameMap) / sizeof(MAPPING_ENTRY);
        amfErr = AMFWrapper::CreateComponent(m_amfContext, AMFVideoEncoder_HEVC, &m_amfEncoder);
    }
    else
    {
        RF_Error(RF_STATUS_AMF_FAIL, "No video codec set for the HW encoder");
        return RF_STATUS_AMF_FAIL;
    }

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

        // Disable B-Frames for avc codec. We do not check for errors when applying this parameter since on some GPUs
        // B Frames won't be available.
        if (pConfig->getVideoCodec() == RF_VIDEO_CODEC_AVC)
        {
            m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_B_PIC_PATTERN, 0);
        }
    }
    else
    {
        if (pConfig->getVideoCodec() == RF_VIDEO_CODEC_AVC)
        {
            m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_B_PIC_PATTERN, 0);
        }

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

    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;

    m_uiAlignedWidth  = m_uiWidth,
    m_uiAlignedHeight = m_uiHeight;

    m_uiOutputWidth  = m_uiAlignedWidth;
    m_uiOutputHeight = m_uiAlignedHeight;

    amfErr = m_amfEncoder->ReInit(m_uiAlignedWidth, m_uiAlignedHeight);
    CHECK_AMF_ERROR(amfErr);

    return RF_STATUS_OK;
}


RFStatus RFEncoderAMF::encode(unsigned int uiBufferIdx, bool bUseInputImage)
{
    AMF_RESULT amfErr;

    assert(m_pContext);

    amf::AMFDataPtr amfSurface = m_pContext->getAMFSurface(uiBufferIdx);

    // Set default pre submission settings in case it was changed in previous frame.
    if (m_videoCodec == RF_VIDEO_CODEC_AVC)
    {
        amfSurface->SetProperty(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_PICTURE_TYPE_NONE);
        amfSurface->SetProperty(AMF_VIDEO_ENCODER_INSERT_SPS, false);
        amfSurface->SetProperty(AMF_VIDEO_ENCODER_INSERT_PPS, false);
        amfSurface->SetProperty(AMF_VIDEO_ENCODER_INSERT_AUD, false);
    }
    else if (m_videoCodec == RF_VIDEO_CODEC_HEVC)
    {
        amfSurface->SetProperty(AMF_VIDEO_ENCODER_HEVC_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_HEVC_PICTURE_TYPE_NONE);
        amfSurface->SetProperty(AMF_VIDEO_ENCODER_HEVC_INSERT_HEADER, false);
        amfSurface->SetProperty(AMF_VIDEO_ENCODER_HEVC_INSERT_AUD, false);
    }

    // Need to submit new settings
    for (const auto& pss : m_pPreSubmitSettings)
    {
        amfSurface->SetProperty(pss.first, pss.second);
        // We dont abort if an error occurs when applying the properties. The frame will still be encoded.
    }
    m_pPreSubmitSettings.clear();

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
    else if (uiParameterName == RF_ENCODER_INSERT_SPS)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_INSERT_SPS, uiValue ? true : false));
    }
    else if (uiParameterName == RF_ENCODER_INSERT_PPS)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_INSERT_PPS, uiValue ? true : false));
    }
    else if (uiParameterName == RF_ENCODER_INSERT_AUD)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_INSERT_AUD, uiValue ? true : false));
    }
    else if (uiValue && uiParameterName == RF_ENCODER_HEVC_FORCE_INTRA_REFRESH)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_HEVC_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_HEVC_PICTURE_TYPE_IDR));
    }
    else if (uiValue && uiParameterName == RF_ENCODER_HEVC_FORCE_I_FRAME)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_HEVC_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_HEVC_PICTURE_TYPE_I));
    }
    else if (uiValue && uiParameterName == RF_ENCODER_HEVC_FORCE_P_FRAME)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_HEVC_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_HEVC_PICTURE_TYPE_P));
    }
    else if (uiParameterName == RF_ENCODER_HEVC_INSERT_HEADER)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_HEVC_INSERT_HEADER, uiValue ? true : false));
    }
    else if (uiParameterName == RF_ENCODER_HEVC_INSERT_AUD)
    {
        m_pPreSubmitSettings.push_back(make_pair(AMF_VIDEO_ENCODER_HEVC_INSERT_AUD, uiValue ? true : false));
    }
    else
    {
        unsigned int uiParameterIdx = 0;
        for (uiParameterIdx = 0; uiParameterIdx < m_uiPropertyNameMapCount; ++uiParameterIdx)
        {
            if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == uiParameterName)
            {
                break;
            }
        }

        if (uiParameterIdx == m_uiPropertyNameMapCount)
        {
            return RF_STATUS_INVALID_ENCODER_PARAMETER;
        }

        rfStatus = setAMFProperty(uiParameterIdx, rfType, value);
    }

    return rfStatus;
}


RFParameterState RFEncoderAMF::getParameter(const unsigned int uiParameterName, RFVideoCodec codec, RFProperties& value) const
{
    if (codec == RF_VIDEO_CODEC_NONE)
    {
        return RF_PARAMETER_STATE_INVALID;
    }

    AMF_RESULT amfErr;

    value = 0;

    bool isAVCPreSubmissionParameter = (uiParameterName == RF_ENCODER_FORCE_INTRA_REFRESH) || (uiParameterName == RF_ENCODER_FORCE_I_FRAME) || (uiParameterName == RF_ENCODER_FORCE_P_FRAME) ||
                                       (uiParameterName == RF_ENCODER_INSERT_SPS) || (uiParameterName == RF_ENCODER_INSERT_PPS) || (uiParameterName == RF_ENCODER_INSERT_AUD);
    bool isHEVCPreSubmissionParameter = (uiParameterName == RF_ENCODER_HEVC_FORCE_INTRA_REFRESH) || (uiParameterName == RF_ENCODER_HEVC_FORCE_I_FRAME) ||
                                        (uiParameterName == RF_ENCODER_HEVC_FORCE_P_FRAME) || (uiParameterName == RF_ENCODER_HEVC_INSERT_HEADER) || (uiParameterName == RF_ENCODER_HEVC_INSERT_AUD);

    if (isAVCPreSubmissionParameter || isHEVCPreSubmissionParameter)
    {
        if ((isAVCPreSubmissionParameter && (codec == RF_VIDEO_CODEC_AVC)) || (isHEVCPreSubmissionParameter && (codec == RF_VIDEO_CODEC_HEVC)))
        {
            // Presubmit var has no permanent value. Indicate that parameters are ready to use.
            return RF_PARAMETER_STATE_READY;
        }

        return RF_PARAMETER_STATE_INVALID;
    }
    else
    {
        unsigned int uiParameterIdx = 0;
        for (uiParameterIdx = 0; uiParameterIdx < m_uiPropertyNameMapCount; ++uiParameterIdx)
        {
            if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == uiParameterName)
            {
                break;
            }
        }

        if (uiParameterIdx == m_uiPropertyNameMapCount)
        {
            return RF_PARAMETER_STATE_INVALID;
        }

        const AMFPropertyInfo* pParamInfo;
        amfErr = m_amfEncoder->GetPropertyInfo(m_pPropertyNameMap[uiParameterIdx].AMFPropertyName, &pParamInfo);

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
            AMFRate amfFrameRate = {30, 1};

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
        else if (uiParameterName == RF_ENCODER_HEVC_FRAMERATE || uiParameterName == RF_ENCODER_HEVC_FRAMERATE_DEN)
        {
            AMFRate amfFrameRate = {30, 1};

            amfErr = m_amfEncoder->GetProperty<AMFRate>(AMF_VIDEO_ENCODER_HEVC_FRAMERATE, &amfFrameRate);

            if (uiParameterName == RF_ENCODER_HEVC_FRAMERATE)
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
            amfErr = m_amfEncoder->GetProperty(m_pPropertyNameMap[uiParameterIdx].AMFPropertyName, &uiAmfValue);
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

    RFEncodePreset rfPreset = pConfig->getEncoderPreset();
    if (pConfig->getVideoCodec() == RF_VIDEO_CODEC_AVC)
    {
        AMF_VIDEO_ENCODER_USAGE_ENUM amfUsage = AMF_VIDEO_ENCODER_USAGE_ULTRA_LOW_LATENCY;
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
    }
    else if (pConfig->getVideoCodec() == RF_VIDEO_CODEC_HEVC)
    {
        AMF_VIDEO_ENCODER_HEVC_USAGE_ENUM amfUsage = AMF_VIDEO_ENCODER_HEVC_USAGE_ULTRA_LOW_LATENCY;
        switch (rfPreset)
        {
            case RF_PRESET_FAST:
                amfUsage = AMF_VIDEO_ENCODER_HEVC_USAGE_ULTRA_LOW_LATENCY;
                break;

            case RF_PRESET_BALANCED:
                amfUsage = AMF_VIDEO_ENCODER_HEVC_USAGE_LOW_LATENCY;
                break;

            case RF_PRESET_QUALITY:
                amfUsage = AMF_VIDEO_ENCODER_HEVC_USAGE_TRANSCONDING;
                break;

            default:
                return false;
        }

        // Apply usage (preset that sets all parameters).
        amfErr = m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_USAGE, amfUsage);
    }
    else
    {
        return false;
    }

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

    bool retValue = true;

    for (unsigned int i = 0; i < m_uiPropertyNameMapCount; ++i)
    {
        RFStatus rfStatus = setAMFProperty(i, pConfig->getParameterType(m_pPropertyNameMap[i].RFPropertyName), pConfig->getParameterValue<int>(m_pPropertyNameMap[i].RFPropertyName));
        if (rfStatus != RF_STATUS_OK && rfStatus != RF_STATUS_PARAM_ACCESS_DENIED)
        {
            retValue = false;
        }
    }

    return retValue;
}


RFStatus RFEncoderAMF::setAMFProperty(unsigned int uiParameterIdx, RFParameterType rfType, RFProperties value)
{
    AMF_RESULT amfErr;

    if (uiParameterIdx >= m_uiPropertyNameMapCount)
    {
        return RF_STATUS_INVALID_ENCODER_PARAMETER;
    }

    const AMFPropertyInfo* pParamInfo;
    amfErr = m_amfEncoder->GetPropertyInfo(m_pPropertyNameMap[uiParameterIdx].AMFPropertyName, &pParamInfo);

    if (!pParamInfo && amfErr != AMF_NOT_FOUND)
    {
        return RF_STATUS_AMF_FAIL;
    }

    if (amfErr == AMF_NOT_FOUND || !pParamInfo->AllowedWrite())
    {
        return RF_STATUS_PARAM_ACCESS_DENIED;
    }

    // Handle FRAMERATE as special case. Since 1.1.22 the framerate is passed as AMFRate.
    if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_FRAME_RATE || m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_FRAME_RATE_DEN)
    {
        AMFRate amfFrameRate = {30, 1};

        amfErr = m_amfEncoder->GetProperty<AMFRate>(AMF_VIDEO_ENCODER_FRAMERATE, &amfFrameRate);

        if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_FRAME_RATE)
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
    else if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_HEVC_FRAMERATE || m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_HEVC_FRAMERATE_DEN)
    {
        AMFRate amfFrameRate = {30, 1};

        amfErr = m_amfEncoder->GetProperty<AMFRate>(AMF_VIDEO_ENCODER_HEVC_FRAMERATE, &amfFrameRate);

        if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_HEVC_FRAMERATE)
        {
            // Change framerate and leave DEN unchanged.
            amfFrameRate.num = static_cast<unsigned int>(value);
        }
        else
        {
            // Change DEN only
            amfFrameRate.den = static_cast<unsigned int>(value);
        }

        amfErr = m_amfEncoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_FRAMERATE, amfFrameRate);
    }
    // Handle USAGE as special case: Don't set this paramtere if it was not specified by the user
    else if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_USAGE && value == -1)
    {
        return RF_STATUS_OK;
    }
    else if (m_pPropertyNameMap[uiParameterIdx].RFPropertyName == RF_ENCODER_HEVC_USAGE && value == -1)
    {
        return RF_STATUS_OK;
    }
    else if (rfType == RF_PARAMETER_INT)
    {
        amfErr = m_amfEncoder->SetProperty(m_pPropertyNameMap[uiParameterIdx].AMFPropertyName, static_cast<int>(value));
    }
    else if (rfType == RF_PARAMETER_UINT)
    {
        // Cast bool and uint to an unsigned int to avoid performance warning when casting to bool.
        amfErr = m_amfEncoder->SetProperty(m_pPropertyNameMap[uiParameterIdx].AMFPropertyName, static_cast<unsigned int>(value));
    }
    else if (rfType == RF_PARAMETER_BOOL)
    {
        amfErr = m_amfEncoder->SetProperty(m_pPropertyNameMap[uiParameterIdx].AMFPropertyName, (value != 0));
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