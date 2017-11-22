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

#include <RFEncoderSettings.h>

#include <components/VideoEncoderVCE.h>
#include <components/VideoEncoderHEVC.h>
#include <RFTypes.h>

using namespace std;


RFEncoderSettings::RFEncoderSettings()
    : m_uiEncoderWidth(0)
    , m_uiEncoderHeight(0)
    , m_rfVideoCodec(RF_VIDEO_CODEC_NONE)
    , m_rfFormat(RF_FORMAT_UNKNOWN)
    , m_rfPreset(RF_PRESET_NONE)
{
    m_ParameterMap.clear();

    // Create parameter map with default settings.
    createParameterMap();
}


RFEncoderSettings::~RFEncoderSettings()
{
    m_ParameterMap.clear();

    m_ParameterNames.clear();
}


bool RFEncoderSettings::createSettings(unsigned int uiWidth, unsigned int uiHeight, RFVideoCodec codec, RFEncodePreset preset)
{
    if (m_ParameterMap.size() == 0)
    {
        return false;
    }

    // Store the global preset used to create the parameters.
    m_rfPreset = preset;

    // Store encoding dimensions.
    m_uiEncoderWidth  = uiWidth;
    m_uiEncoderHeight = uiHeight;

    if (codec != RF_VIDEO_CODEC_NONE)
    {
        m_rfVideoCodec = codec;
    }

    if (preset != RF_PRESET_NONE)
    {
        // Only copy preset values int current value if a preset is defined. Otherwise use the default value.
        for (auto& p : m_ParameterMap)
        {
            p.second.Value = p.second.PresetValue[preset];
        }
    }

    return true;
}


bool RFEncoderSettings::createSettings(unsigned int uiWidth, unsigned int uiHeight)
{
    // Use fast preset as default.
    return createSettings(uiWidth, uiHeight, RF_VIDEO_CODEC_NONE, RF_PRESET_NONE);
}


bool RFEncoderSettings::setVideoCodec(RFVideoCodec codec)
{
    m_rfVideoCodec = codec;

    return true;
}


bool RFEncoderSettings::setFormat(RFFormat format)
{
    m_rfFormat = format;

    return true;
}


bool RFEncoderSettings::setDimension(unsigned int uiWidth, unsigned int uiHeight)
{
    m_uiEncoderWidth  = uiWidth;
    m_uiEncoderHeight = uiHeight;

    return true;
}


void RFEncoderSettings::setParameterState(const unsigned int uiParameterName, RFParameterState rfParamStatus)
{
    auto itr = m_ParameterMap.find(uiParameterName);

    if (itr != m_ParameterMap.end())
    {
        itr->second.ParemeterState = rfParamStatus;
    }
}


bool RFEncoderSettings::checkParameter(const unsigned int uiParameterName)
{
    if (m_ParameterMap.size() == 0)
    {
        return false;
    }

    auto itr = m_ParameterMap.find(uiParameterName);

    if (itr == m_ParameterMap.end())
    {
        return false;
    }

    return true;
}


RFParameterType RFEncoderSettings::getParameterType(const unsigned int uiParameterName) const
{
    if (m_ParameterMap.size() == 0)
    {
        return RF_PARAMETER_UNKNOWN;
    }

    auto itr = m_ParameterMap.find(uiParameterName);

    if (itr == m_ParameterMap.end())
    {
        return RF_PARAMETER_UNKNOWN;
    }

    return itr->second.EntryType;
}


bool RFEncoderSettings::getParameterString(const unsigned int uiParameterName, std::string& strName) const
{
    if (m_ParameterMap.size() == 0)
    {
        return false;
    }

    auto itr = m_ParameterMap.find(uiParameterName);

    if (itr == m_ParameterMap.end())
    {
        return false;
    }

    strName = itr->second.strParameterName;

    return true;
}


bool RFEncoderSettings::getParameterName(const unsigned int uiParamIdx, unsigned int& uiParamName) const
{
    if (uiParamIdx < m_ParameterNames.size())
    {
        uiParamName = m_ParameterNames[uiParamIdx];

        return true;
    }

    uiParamName = 0;

    return false;
}


bool RFEncoderSettings::createParameterMap()
{
    m_ParameterMap.clear();

    MapEntry Entry;

    ////////////////////////////////////////////////////////////////////////////////////
    // H.264 profile
    // Type : uint
    // possible values: 66 (Baseline), 77 (main), 100 (High)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Profile";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_PROFILE_MAIN;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_PROFILE_MAIN;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_PROFILE_MAIN;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_PROFILE_MAIN;

    m_ParameterMap[RF_ENCODER_PROFILE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.264 profile level
    // Type : uint
    // possible values: 10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 4, 41, 42, 50, 51, 52
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Profile Level";
    Entry.Value.uiValue = 42;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 42;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 42;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 42;

    m_ParameterMap[RF_ENCODER_LEVEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Encoder usage.
    // This parameter configures the whole parameter set to a preset according to the usage.
    // Type : int
    // possible values:
    //    -1 (not set)
    //     0 (AMF_VIDEO_ENCODER_USAGE_TRANSCONDING)
    //     1 (AMF_VIDEO_ENCODER_USAGE_ULTRA_LOW_LATENCY)
    //     2 (AMF_VIDEO_ENCODER_USAGE_LOW_LATENCY)
    //     3 (AMF_VIDEO_ENCODER_USAGE_WEBCAM)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_INT;
    Entry.strParameterName = "Usage";
    Entry.Value.nValue = -1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = -1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = -1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = -1;

    m_ParameterMap[RF_ENCODER_USAGE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Common Low Latency Internal
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Common Low Latency Internal";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = false;

    m_ParameterMap[RF_ENCODER_COMMON_LOW_LATENCY_INTERNAL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Target Bitrate
    // Type : uint
    // possible values: 10 KBit/sec - 100 MBit/sec
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Target Bitrate";
    Entry.Value.uiValue = 20000000;     // 20 MBit/sec
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 20000000;        // 20 MBit/sec
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 20000000;    // 20 MBit/sec
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 20000000;     // 20 MBit/sec

    m_ParameterMap[RF_ENCODER_BITRATE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Peak Bitrate
    // Type : uint
    // possible values: 10 KBit/sec - 100 MBit/sec
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Peak Bitrate";
    Entry.Value.uiValue = 30000000;     // 30 MBit/sec
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 30000000;        // 30 MBit/sec
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 30000000;    // 30 MBit/sec
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 30000000;     // 30 MBit/sec

    m_ParameterMap[RF_ENCODER_PEAK_BITRATE] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // Frame Rate
    // Type : uint
    // possible values: 1*FrameRateDen - 120*FrameRateDen
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Frame Rate";
    Entry.Value.uiValue = 30;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 30;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 30;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 30;

    m_ParameterMap[RF_ENCODER_FRAME_RATE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Rate Control Method
    //
    // Type : uint
    // possible values: 0 (Constant QP), 1 (Constant Bitrate), 2 (Peak Constrained VBR), 3 (Latency Cosntrained VBR)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Rate Control Method";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR;

    m_ParameterMap[RF_ENCODER_RATE_CONTROL_METHOD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Minimum Quantizer Parameter
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Minimum Quantizer Parameter";
    Entry.Value.uiValue = 22;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 22;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 22;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 18;

    m_ParameterMap[RF_ENCODER_MIN_QP] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // Maximum Quantizer Parameter
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Maximum Quantizer Parameter";
    Entry.Value.uiValue = 48;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 48;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 48;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 46;

    m_ParameterMap[RF_ENCODER_MAX_QP] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // VBV (Video Buffering Verifier) Buffer Size in Bits
    //
    // Type : uint
    // possible values: 1 KBit - 100 MBit
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "VBV Buffer Size";
    Entry.Value.uiValue = 735000;    // 735 KBit
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 735000;         // 735 KBit
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 4000000;    //   4 MBit
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 20000000;    //  20 MBit

    m_ParameterMap[RF_ENCODER_VBV_BUFFER_SIZE] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // VBV Initial Fullness
    //
    // Type : uint
    // possible values: 0 - 64
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Initial VBV Buffer Fullness";
    Entry.Value.uiValue = 64;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 64;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 64;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 64;

    m_ParameterMap[RF_ENCODER_VBV_BUFFER_FULLNESS] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Disables/enables constraints on QP variation within a picture to meet HRD requirement(s)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Enforce HRD";
    Entry.Value.bValue = true;
    Entry.PresetValue[RF_PRESET_FAST].bValue = true;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_ENFORCE_HRD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Disables/enables Variance Based Adaptive Quantization
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Enable Variance Based Adaptive Quantization";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_ENABLE_VBAQ] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Frame Rate Denominator
    //
    // Type : uint
    // possible values: 1 - MaxInt
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Frame Rate Denominator";
    Entry.Value.bValue = 1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 1;

    m_ParameterMap[RF_ENCODER_FRAME_RATE_DEN] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // IDR period. IDRPeriod = 0 turns IDR off
    //
    // Type : uint
    // possible values: 0 - 1000
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "IDR Period";
    Entry.Value.uiValue = 300;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 300;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 300;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 30;

    m_ParameterMap[RF_ENCODER_IDR_PERIOD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Number of intra-refresh macro-blocks per slot
    //
    // Type : uint
    // possible values: 0 - #MBs per frame
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Number of Intra-Refresh Macro-Blocks per Slot";
    Entry.Value.uiValue = 225;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 225;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 225;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 0;

    m_ParameterMap[RF_ENCODER_INTRA_REFRESH_NUM_MB] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // De-Blocking filter
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "De-Blocking Filter";
    Entry.Value.bValue = true;
    Entry.PresetValue[RF_PRESET_FAST].bValue = true;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = true;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = true;

    m_ParameterMap[RF_ENCODER_DEBLOCKING_FILTER] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Num slices per frame
    //
    // Type : uint
    // possible values: 1 - #MBs per frame
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Num Slices per Frame";
    Entry.Value.uiValue = 1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 1;

    m_ParameterMap[RF_ENCODER_NUM_SLICES_PER_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Quality Preset
    //
    // Type : uint
    // possible values: 0 (Balanced), 1 (Quality), 2 (Speed)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Quality Preset";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_QUALITY_PRESET_BALANCED;

    m_ParameterMap[RF_ENCODER_QUALITY_PRESET] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // Half Pixel Motion Estimation
    //
    // Type : uint
    // possible values: 0,1
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Half Pixel Motion Estimation";
    Entry.Value.uiValue = 1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 1;

    m_ParameterMap[RF_ENCODER_HALF_PIXEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Quarter Pixel Motion Estimation
    //
    // Type : uint
    // possible values: 0,1
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "Quarter Pixel Motion Estimation";
    Entry.Value.uiValue = 1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 1;

    m_ParameterMap[RF_ENCODER_QUARTER_PIXEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Force Intra-Refresh Frame Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Force Intra-Refresh Frames Picture Type";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_FORCE_INTRA_REFRESH] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Force I-Frame Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Force I-Frames Picture Type";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_FORCE_I_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Force P-Frame Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Force P-Frames Picture Type";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_FORCE_P_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Insert Sequence Parameter Set (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Insert SPS";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_INSERT_SPS] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Insert Picture Parameter Set (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Insert PPS";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_INSERT_PPS] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Insert Access Unit Delimiter (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "Insert AUD";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_INSERT_AUD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Encoder usage.
    // This parameter configures the whole parameter set to a preset according to the usage.
    // Type : int
    // possible values:
    //    -1 (not set)
    //     0 (AMF_VIDEO_ENCODER_HEVC_USAGE_TRANSCONDING)
    //     1 (AMF_VIDEO_ENCODER_HEVC_USAGE_ULTRA_LOW_LATENCY)
    //     2 (AMF_VIDEO_ENCODER_HEVC_USAGE_LOW_LATENCY)
    //     3 (AMF_VIDEO_ENCODER_HEVC_USAGE_WEBCAM)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_INT;
    Entry.strParameterName = "HEVC Usage";
    Entry.Value.nValue = -1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = -1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = -1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = -1;

    m_ParameterMap[RF_ENCODER_HEVC_USAGE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 profile
    // Type : uint
    // possible values: 1 (main)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Profile";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN;

    m_ParameterMap[RF_ENCODER_HEVC_PROFILE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 profile level
    // Type : uint
    // possible values: 30 (1), 60 (2), 63 (2.1), 90 (3), 93 (3.1), 120 (4), 123 (4.1),
    //                  150 (5), 153 (5.1), 156 (5.2), 180 (6), 183 (6.1), 186 (6.2)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Profile Level";
    Entry.Value.uiValue = AMF_LEVEL_6_2;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_LEVEL_6_2;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_LEVEL_6_2;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_LEVEL_6_2;

    m_ParameterMap[RF_ENCODER_HEVC_LEVEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 tier
    // Type : uint
    // possible values: 0 (main), 1 (high)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Tier";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_HEVC_TIER_MAIN;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_HEVC_TIER_MAIN;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_HEVC_TIER_MAIN;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_HEVC_TIER_MAIN;

    m_ParameterMap[RF_ENCODER_HEVC_TIER] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Rate Control Method
    //
    // Type : uint
    // possible values: 0 (Constant QP), 1 (Latency Constrained VBR), 2 (Peak Constrained VBR), 3 (Constant Bitrate)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Rate Control Method";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR;

    m_ParameterMap[RF_ENCODER_HEVC_RATE_CONTROL_METHOD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Frame Rate
    // Type : uint
    // possible values: 1*FrameRateDen - 60*FrameRateDen
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Framerate";
    Entry.Value.uiValue = 30;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 30;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 30;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 30;

    m_ParameterMap[RF_ENCODER_HEVC_FRAMERATE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Frame Rate Denominator
    //
    // Type : uint
    // possible values: 1 - MaxInt
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Frame Rate Denominator";
    Entry.Value.bValue = 1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 1;

    m_ParameterMap[RF_ENCODER_HEVC_FRAMERATE_DEN] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 VBV (Video Buffering Verifier) Buffer Size in Bits
    //
    // Type : uint
    // possible values: >0
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC VBV Buffer Size";
    Entry.Value.uiValue = 735000;    // 735 KBit
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 735000;         // 735 KBit
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 4000000;    //   4 MBit
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 20000000;    //  20 MBit

    m_ParameterMap[RF_ENCODER_HEVC_VBV_BUFFER_SIZE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 VBV Initial Fullness
    //
    // Type : uint
    // possible values: 0 - 64
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Initial VBV Buffer Fullness";
    Entry.Value.uiValue = 64;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 64;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 64;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 64;

    m_ParameterMap[RF_ENCODER_HEVC_INITIAL_VBV_BUFFER_FULLNESS] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 pre-analysis assisted rate control
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Pre-analysis Assisted Rate Control";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_RATE_CONTROL_PREANALYSIS_ENABLE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 disables/enables Variance Based Adaptive Quantization
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Enable Variance Based Adaptive Quantization";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_ENABLE_VBAQ] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Target Bitrate
    // Type : uint
    // possible values: >0
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Target Bitrate";
    Entry.Value.uiValue = 20000000;     // 20 MBit/sec
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 20000000;        // 20 MBit/sec
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 20000000;    // 20 MBit/sec
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 20000000;     // 20 MBit/sec

    m_ParameterMap[RF_ENCODER_HEVC_TARGET_BITRATE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Peak Bitrate
    // Type : uint
    // possible values: >= Target Bitrate
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Peak Bitrate";
    Entry.Value.uiValue = 30000000;     // 30 MBit/sec
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 30000000;        // 30 MBit/sec
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 30000000;    // 30 MBit/sec
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 30000000;     // 30 MBit/sec

    m_ParameterMap[RF_ENCODER_HEVC_PEAK_BITRATE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 minimum Quantizer Parameter for I frame
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Minimum Quantizer Parameter for I Frame";
    Entry.Value.uiValue = 22;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 22;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 22;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 18;

    m_ParameterMap[RF_ENCODER_HEVC_MIN_QP_I] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 maximum Quantizer Parameter for I frame
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Maximum Quantizer Parameter for I Frame";
    Entry.Value.uiValue = 48;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 48;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 48;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 46;

    m_ParameterMap[RF_ENCODER_HEVC_MAX_QP_I] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 minimum Quantizer Parameter for P frame
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Minimum Quantizer Parameter for P Frame";
    Entry.Value.uiValue = 22;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 22;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 22;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 18;

    m_ParameterMap[RF_ENCODER_HEVC_MIN_QP_P] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 maximum Quantizer Parameter for P frame
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Maximum Quantizer Parameter for P Frame";
    Entry.Value.uiValue = 48;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 48;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 48;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 46;

    m_ParameterMap[RF_ENCODER_HEVC_MAX_QP_P] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 constant Quantizer Parameter for I frame
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Constant Quantizer Parameter for I Frame";
    Entry.Value.uiValue = 26;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 26;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 26;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 26;

    m_ParameterMap[RF_ENCODER_HEVC_QP_I] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 constant Quantizer Parameter for P frame
    //
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Constant Quantizer Parameter for P Frame";
    Entry.Value.uiValue = 26;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 26;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 26;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 26;

    m_ParameterMap[RF_ENCODER_HEVC_QP_P] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 disables/enables constraints on QP variation within a picture to meet HRD requirement(s)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Enforce HRD";
    Entry.Value.bValue = true;
    Entry.PresetValue[RF_PRESET_FAST].bValue = true;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_ENFORCE_HRD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 maximum AU size in bits
    //
    // Type : uint
    // possible values: 0 - 100 MBit
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Maximum AU Size in Bits";
    Entry.Value.uiValue = 0;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 0;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 0;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 0;

    m_ParameterMap[RF_ENCODER_HEVC_MAX_AU_SIZE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 disables/enables filler data for CBR usage
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Enable Filler Data for CBR Usage";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_FILLER_DATA_ENABLE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 disables/enables skip frame for rate control
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Enable Skip Frame for Rate Control";
    Entry.Value.bValue = true;
    Entry.PresetValue[RF_PRESET_FAST].bValue = true;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = true;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_RATE_CONTROL_SKIP_FRAME_ENABLE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 headers insertion mode
    //
    // Type : uint
    // possible values: 0 (none), 1 (GOP aligned), 2 (IDR aligned)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Headers Insertion Mode";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_NONE;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_NONE;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_NONE;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_NONE;

    m_ParameterMap[RF_ENCODER_HEVC_HEADER_INSERTION_MODE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 the period to insert IDR / CRA in fixed size mode.
    //       0 means only insert the first IDR / CRA(infinite GOP size)
    //
    // Type : uint
    // possible values: 0 ... 1000
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC IDR / CRA Period";
    Entry.Value.uiValue = 300;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 300;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 300;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 30;

    m_ParameterMap[RF_ENCODER_HEVC_GOP_SIZE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Determines the frequency to insert IDR as start of a GOP.
    //       0 means no IDR will be inserted except for the first picture in the sequence.
    //
    // Type : uint
    // possible values: 0 - 65535
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC GOPS per IDR";
    Entry.Value.uiValue = 1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 1;

    m_ParameterMap[RF_ENCODER_HEVC_NUM_GOPS_PER_IDR] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 disables/enables the de-blocking filter
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Disable the De-Blocking Filter";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_DE_BLOCKING_FILTER_DISABLE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 number of slices per frame
    //
    // Type : uint
    // possible values: 1 - #CTBs per frame
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Slices per Frame";
    Entry.Value.uiValue = 1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = 1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = 1;

    m_ParameterMap[RF_ENCODER_HEVC_SLICES_PER_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Quality Preset
    //
    // Type : uint
    // possible values: 0 (Balanced), 5 (Quality), 10 (Speed)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_UINT;
    Entry.strParameterName = "HEVC Quality Preset";
    Entry.Value.uiValue = AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_SPEED;
    Entry.PresetValue[RF_PRESET_FAST].uiValue = AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_SPEED;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_SPEED;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue = AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_QUALITY;

    m_ParameterMap[RF_ENCODER_HEVC_QUALITY_PRESET] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 disables/enables half-pixel motion estimation
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Enable Half-Pixel Motion Estimation";
    Entry.Value.bValue = true;
    Entry.PresetValue[RF_PRESET_FAST].bValue = true;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = true;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = true;

    m_ParameterMap[RF_ENCODER_HEVC_MOTION_HALF_PIXEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 disables/enables quarter-pixel motion estimation
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Enable Quarter-Pixel Motion Estimation";
    Entry.Value.bValue = true;
    Entry.PresetValue[RF_PRESET_FAST].bValue = true;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = true;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = true;

    m_ParameterMap[RF_ENCODER_HEVC_MOTION_QUARTERPIXEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Force Intra-Refresh Frames Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Force Intra-Refresh Frames Picture Type";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_FORCE_INTRA_REFRESH] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Force I-Frame Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Force I-Frame Picture Type";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_FORCE_I_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Force P-Frame Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Force P-Frame Picture Type";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_FORCE_P_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Insert SPS, PPS and VPS (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Insert SPS, PPS and VPS";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_INSERT_HEADER] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.265 Insert Access Unit Delimiter (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType = RF_PARAMETER_BOOL;
    Entry.strParameterName = "HEVC Insert AUD";
    Entry.Value.bValue = false;
    Entry.PresetValue[RF_PRESET_FAST].bValue = false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue = false;

    m_ParameterMap[RF_ENCODER_HEVC_INSERT_AUD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Block Size for Difference Encoder
    //
    // Type : unsigend int
    // possible values: BLOCK_SIZE_S * BLOCK_SIZE_T == 64
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Block Size S";
    Entry.Value.uiValue                           =  16;
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  16;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  16;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  16;

    m_ParameterMap[RF_DIFF_ENCODER_BLOCK_S] = Entry;

    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Block Size T";
    Entry.Value.uiValue                           =  16;
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  16;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  16;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  16;

    m_ParameterMap[RF_DIFF_ENCODER_BLOCK_T] = Entry;

    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Lock Encoded Buffer";
    Entry.Value.bValue                            =  false;
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  false;
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  false;
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  false;

    m_ParameterMap[RF_DIFF_ENCODER_LOCK_BUFFER] = Entry;

    // Store all names in m_ParameterNames.
    map<unsigned int, MapEntry>::const_iterator itr;

    for (auto& p : m_ParameterMap)
    {
        m_ParameterNames.push_back(p.first);
    }

    return true;
}