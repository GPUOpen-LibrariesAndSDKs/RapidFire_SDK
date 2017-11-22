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

#pragma once

#include <map>
#include <string>
#include <vector>

#include "RapidFire.h"
#include "RFTypes.h"

class RFEncoderSettings
{
public:

    RFEncoderSettings();
    ~RFEncoderSettings();

    // Creates default settings.
    bool    createSettings(unsigned int uiWidth, unsigned int uiHeight);
    // Creates settings based on preset.
    bool    createSettings(unsigned int uiWidth, unsigned int uiHeight, RFVideoCodec codec, RFEncodePreset preset);
    // Sets the video codec for the encoder.
    bool    setVideoCodec(RFVideoCodec codec);
    // Stores the input format for the encoder.
    bool    setFormat(RFFormat format);
    // Stores dimension of the encoder frames.
    bool    setDimension(unsigned int uiWidth, unsigned int uiHeight);
    // Sets parameter state. rfParamStatus can be RF_PARAMETER_STATE_INVALID, RF_PARAMETER_STATE_READY, RF_PARAMETER_STATE_BLOCKED.
    void    setParameterState(const unsigned int uiParameterName, RFParameterState rfParamStatus);

    // Assigns a value to the paramter with the name uiParameterName.
    template <typename T>
    bool    setParameter(const unsigned int uiParameterName, T Value, RFParameterState rfParamStatus);

    // Gets the value of the paramter with the name uiParameterName.
    template <typename T>
    bool    getParameterValue(const unsigned int uiParameterName, T& Value) const;

    template <typename T>
    RFParameterState getValidatedParameterValue(const unsigned int uiParameterName, T& Value) const;

    // Returns the value of the paramter with the name uiParameterName.
    template <typename T>
    T       getParameterValue(const unsigned int uiParameterName) const;

    // Returns the type of the parameter of RF_PARAMETER_UNKNOWN.
    RFParameterType getParameterType(const unsigned int uiParameterName) const;

    bool    getParameterString(const unsigned int uiParameterName, std::string& strName) const;

    // Returns the parameter name that is stored at position uiParamIndex.
    bool    getParameterName(const unsigned int uiPramIndex, unsigned int& uiParamName) const;

    // Checks if the parameter name uiParameterName is valid.
    bool    checkParameter(const unsigned int uiParameterName);

    unsigned int    getEncoderWidth()       const { return m_uiEncoderWidth; }
    unsigned int    getEncoderHeight()      const { return m_uiEncoderHeight; }
    unsigned int    getNumSettings()        const { return static_cast<unsigned int>(m_ParameterNames.size()); }
    RFVideoCodec    getVideoCodec()         const { return m_rfVideoCodec; }
    RFFormat        getInputFormat()        const { return m_rfFormat; }
    RFEncodePreset  getEncoderPreset()      const { return m_rfPreset; }

private:

    union VALUE_TYPE
    {
        bool            bValue;
        int             nValue;
        unsigned int    uiValue;
    };

    struct MapEntry
    {
        RFParameterState    ParemeterState;         // Indicates if a parameter is used by the encoder and if it is accessible.
        RFParameterType     EntryType;              // Type of the parameter: RF_BOOL, RF_INT, RF_UINT ...
        std::string         strParameterName;       // String containing the name of the parameter

        VALUE_TYPE          Value;                  // Current value
        VALUE_TYPE          PresetValue[3];         // Preset values. 0: FAST  1: BALANCED  2: Quality

        MapEntry()
            : ParemeterState(RF_PARAMETER_STATE_INVALID)
            , EntryType(RF_PARAMETER_UNKNOWN)
        {
            memset(&Value, 0, sizeof(VALUE_TYPE));
            memset(PresetValue, 0, 3 * sizeof(VALUE_TYPE));
        };

        // Make sure class has a valid (default) copy constructor which is called when getting inserted into the map.
    };

    // Builds map that contains all paramters and their default values.
    bool                                    createParameterMap();

    unsigned int                            m_uiEncoderWidth;
    unsigned int                            m_uiEncoderHeight;
    RFVideoCodec                            m_rfVideoCodec;
    RFFormat                                m_rfFormat;
    RFEncodePreset                          m_rfPreset;

    std::map<unsigned int, MapEntry>        m_ParameterMap;
    std::vector<unsigned int>               m_ParameterNames;
};


// Disable performance warning on converting int to bool. Since the EntryType is tested this
// conversion will never happen.
#pragma warning( push )
#pragma warning( disable : 4800 )

template <typename T>
bool RFEncoderSettings::getParameterValue(const unsigned int uiParameterName, T& Value) const
{
    auto itr = m_ParameterMap.find(uiParameterName);

    if (itr == m_ParameterMap.end())
    {
        return false;
    }

    switch (itr->second.EntryType)
    {
        case RF_PARAMETER_BOOL:
            Value = itr->second.Value.bValue;
            return true;

        case RF_PARAMETER_INT:
            Value = itr->second.Value.nValue;
            return true;

        case RF_PARAMETER_UINT:
            Value = itr->second.Value.uiValue;
            return true;
    }

    return false;
}


template <typename T>
RFParameterState RFEncoderSettings::getValidatedParameterValue(const unsigned int uiParameterName, T& Value) const
{
    auto itr = m_ParameterMap.find(uiParameterName);

    if (itr == m_ParameterMap.end())
    {
        return RF_PARAMETER_STATE_INVALID;
    }

    switch (itr->second.EntryType)
    {
        case RF_PARAMETER_BOOL:
            Value = itr->second.Value.bValue;
            break;

        case RF_PARAMETER_INT:
            Value = itr->second.Value.nValue;
            break;

        case RF_PARAMETER_UINT:
            Value = itr->second.Value.uiValue;
            break;
    }

    return itr->second.ParemeterState;
}


template <typename T>
T RFEncoderSettings::getParameterValue(const unsigned int uiParameterName) const
{
    T value;

    if (getParameterValue(uiParameterName, value))
    {
        return value;
    }
    else
    {
        return static_cast<T>(0);
    }
}


template <typename T>
bool RFEncoderSettings::setParameter(const unsigned int uiParameterName, T Value, RFParameterState rfParamStatus)
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

    m_ParameterMap[uiParameterName].ParemeterState = rfParamStatus;

    switch (m_ParameterMap[uiParameterName].EntryType)
    {
        case RF_PARAMETER_BOOL:
            m_ParameterMap[uiParameterName].Value.bValue = static_cast<bool>(Value);
            return true;

        case RF_PARAMETER_UINT:
            m_ParameterMap[uiParameterName].Value.uiValue = static_cast<unsigned int>(Value);
            return true;

        case RF_PARAMETER_INT:
            m_ParameterMap[uiParameterName].Value.nValue = static_cast<int>(Value);
            return true;
    };

    return false;
}

#pragma warning ( pop )