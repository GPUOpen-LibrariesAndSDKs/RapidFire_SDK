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

#include <RFTypes.h>

using namespace std;


RFEncoderSettings::RFEncoderSettings()
    : m_uiEncoderWidth(0)
    , m_uiEncoderHeight(0)
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


bool RFEncoderSettings::createSettings(unsigned int uiWidth, unsigned int uiHeight, RFEncodePreset preset)
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
    return createSettings(uiWidth, uiHeight, RF_PRESET_NONE);
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
    if (m_ParameterMap.size() > 0)
    {
        // Map already exists
        return false;
    }

    MapEntry Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // H.264 profile
    // Type : uint
    // possible values: 66 (Baseline), 77 (main), 100 (High)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Profile";
    Entry.Value.uiValue                           = 77;       
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = 77;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 77;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 77;

    m_ParameterMap[RF_ENCODER_PROFILE] = Entry;
   

    ////////////////////////////////////////////////////////////////////////////////////
    // H.264 profile level
    // Type : uint
    // possible values: 10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 4, 41, 42, 50, 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Profile level";
    Entry.Value.uiValue                           = 42;
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = 42;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 42;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 42;

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
    Entry.EntryType                               = RF_PARAMETER_INT;
    Entry.strParameterName                        = "Usage";
    Entry.Value.nValue                            = -1;
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = -1;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = -1;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = -1;

    m_ParameterMap[RF_ENCODER_USAGE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Common Low Latency Internal
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Common Low Latency Internal";
    Entry.Value.bValue                            = false;
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = false;
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = false;
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = false;

    m_ParameterMap[RF_ENCODER_COMMON_LOW_LATENCY_INTERNAL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Bitrate
    // Type : uint
    // possible values: 10 KBit/sec - 100 MBit/sec
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Target Bitrate";
    Entry.Value.uiValue                           =  6000000;     // 6 MBit/sec
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  6000000;     // 6 MBit/sec
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 10000000;     // 10 MBit/sec
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 20000000;     // 20 MBit/sec

    m_ParameterMap[RF_ENCODER_BITRATE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Peak Bitrate
    // Type : uint
    // possible values: 10 KBit/sec - 100 MBit/sec
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Peak Bitrate";
    Entry.Value.uiValue                           =  6000000;     // 10 MBit/sec
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  6000000;     // 6 MBit/sec
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 10000000;     // 10 MBit/sec
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 20000000;     // 20 MBit/sec

     m_ParameterMap[RF_ENCODER_PEAK_BITRATE] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // Frame Rate
    // Type : uint
    // possible values: 1*FrameRateDen - 120*FrameRateDen
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Frame Rate";
    Entry.Value.uiValue                           = 60;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = 60;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 60; 
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 30;

    m_ParameterMap[RF_ENCODER_FRAME_RATE] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Rate Control Method
    // 
    // Type : uint
    // possible values: 0 (Constrained QP), 1 (Constand Bitrate), 2 (Peak Constrained VBR), 3 (Latency Cosntrained VBR)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Rate Control Method";
    Entry.Value.uiValue                           = 3;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = 3;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 2; 
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 2;

    m_ParameterMap[RF_ENCODER_RATE_CONTROL_METHOD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Minimum Quantizer Parameter
    // 
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Minimum Quantizer Parameter";
    Entry.Value.uiValue                           = 22;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = 22;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 22; 
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 18;

    m_ParameterMap[RF_ENCODER_MIN_QP] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // Maximum Quantizer Parameter
    // 
    // Type : uint
    // possible values: 0 - 51
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Maximum Quantizer Parameter";
    Entry.Value.uiValue                           = 51;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     = 51;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue = 51; 
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 51;

    m_ParameterMap[RF_ENCODER_MAX_QP] = Entry;
 


    ////////////////////////////////////////////////////////////////////////////////////
    // VBV (Video Buffering Verifier) Buffer Size in Bits
    // 
    // Type : uint
    // possible values: 1 KBit - 100 MBit
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "VBV Buffer Size";
    Entry.Value.uiValue                           =   110000;    // 110 KBit
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =   110000;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  1000000;    // 1 MBit
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  = 20000000;    // 20 MBit

    m_ParameterMap[RF_ENCODER_VBV_BUFFER_SIZE] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // VBV Initial Fullness
    // 
    // Type : uint
    // possible values: 0 - 64
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "VBV Initial Fullness";
    Entry.Value.uiValue                           =  64; 
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  64;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  64;    
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  64; 

    m_ParameterMap[RF_ENCODER_VBV_BUFFER_FULLNESS] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Disables/enables constraints on QP variation within a picture to meet HRD requirement(s)
    // 
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Enforce HRD";
    Entry.Value.bValue                            =  true;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  true;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  true;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  false; 

    m_ParameterMap[RF_ENCODER_ENFORCE_HRD] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // Frame Rate Denominator
    // 
    // Type : uint
    // possible values: 1 - MaxInt
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Frame Rate Denominator";
    Entry.Value.bValue                            =  1;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  1;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  1;    
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  1; 

    m_ParameterMap[RF_ENCODER_FRAME_RATE_DEN] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // IDR period. IDRPeriod = 0 turns IDR off
    // 
    // Type : uint
    // possible values: 0 - 1000
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "IDR Period";
    Entry.Value.uiValue                           =  300;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  300;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  300;    
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =   30; 

    m_ParameterMap[RF_ENCODER_IDR_PERIOD] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Number of intra-refresh macro-blocks per slot
    // 
    // Type : uint
    // possible values: 0 - #MBs per frame
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Number of Intra-Refresh Macro-Blocks per Slot";
    Entry.Value.uiValue                           =  225;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  225;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  225;    
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =    0; 

    m_ParameterMap[RF_ENCODER_INTRA_REFRESH_NUM_MB] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // De-Blocking filter
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "De-Blocking Filter";
    Entry.Value.bValue                            =  false;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  false;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  false;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  true; 

    m_ParameterMap[RF_ENCODER_DEBLOCKING_FILTER] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Num slices per frame
    // 
    // Type : uint
    // possible values: 1 - #MBs per frame
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Num Slices per Frame";
    Entry.Value.uiValue                           =  1;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  1;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  1;    
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  1; 

    m_ParameterMap[RF_ENCODER_NUM_SLICES_PER_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Quality Preset
    // 
    // Type : uint
    // possible values: 0 (Balanced), 1 (Quality), 2 (Speed)
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Quality Preset";
    Entry.Value.uiValue                           =  2;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  2;       // Speed
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  2;       // Speed
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  0;       // Balanced

    m_ParameterMap[RF_ENCODER_QUALITY_PRESET] = Entry;



    ////////////////////////////////////////////////////////////////////////////////////
    // Half Pixel Motion Estimation
    // 
    // Type : uint
    // possible values: 0,1
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Half Pixel Motion Estimation";
    Entry.Value.uiValue                           =  1;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  1;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  1;    
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  1; 

    m_ParameterMap[RF_ENCODER_HALF_PIXEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Quarter Pixel Motion Estimation
    // 
    // Type : uint
    // possible values: 0,1
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_UINT;
    Entry.strParameterName                        = "Quarter Pixel Motion Estimation";
    Entry.Value.uiValue                           =  1;    
    Entry.PresetValue[RF_PRESET_FAST].uiValue     =  1;   
    Entry.PresetValue[RF_PRESET_BALANCED].uiValue =  1;    
    Entry.PresetValue[RF_PRESET_QUALITY].uiValue  =  1; 

    m_ParameterMap[RF_ENCODER_QUARTER_PIXEL] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Force Intra-Refresh Frames Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Intra-Refresh Frames Picture Type";
    Entry.Value.bValue                            =  false;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  false;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  false;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  false; 

    m_ParameterMap[RF_ENCODER_FORCE_INTRA_REFRESH] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Force I-Frames Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Force I-Frames Picture Type";
    Entry.Value.bValue                            =  false;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  false;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  false;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  false; 

    m_ParameterMap[RF_ENCODER_FORCE_I_FRAME] = Entry;

    ////////////////////////////////////////////////////////////////////////////////////
    // Force P-Frames Picture Type (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Force P-Frames Picture Type";
    Entry.Value.bValue                            =  false;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  false;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  false;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  false; 

    m_ParameterMap[RF_ENCODER_FORCE_P_FRAME] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Insert Sequence Parameter Set (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Insert SPS";
    Entry.Value.bValue                            =  false;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  false;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  false;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  false; 

    m_ParameterMap[RF_ENCODER_INSERT_SPS] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Insert Picture Parameter Set (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Insert PPS";
    Entry.Value.bValue                            =  false;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  false;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  false;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  false; 

    m_ParameterMap[RF_ENCODER_INSERT_PPS] = Entry;


    ////////////////////////////////////////////////////////////////////////////////////
    // Insert Access Unit Delimiter (pre Submission)
    //
    // Type : bool
    // possible values: true, false
    ////////////////////////////////////////////////////////////////////////////////////
    Entry.EntryType                               = RF_PARAMETER_BOOL;
    Entry.strParameterName                        = "Insert AUD";
    Entry.Value.bValue                            =  true;    
    Entry.PresetValue[RF_PRESET_FAST].bValue      =  true;   
    Entry.PresetValue[RF_PRESET_BALANCED].bValue  =  true;    
    Entry.PresetValue[RF_PRESET_QUALITY].bValue   =  true; 

    m_ParameterMap[RF_ENCODER_INSERT_AUD] = Entry;


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