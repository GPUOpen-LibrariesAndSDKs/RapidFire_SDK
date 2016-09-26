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

#include "RFTypes.h"

template <class T>
class RFParameterTrait;


template <>
class RFParameterTrait<int>
{
public:
    typedef int RF_ATTRIB_TYPE;

    enum { rfTypeId = RF_PARAMETER_INT };

    static void convert(RFProperties from, RF_ATTRIB_TYPE& to) { to = static_cast<RF_ATTRIB_TYPE>(from); }
};


template <>
class RFParameterTrait<unsigned int>
{
public:
    typedef unsigned int RF_ATTRIB_TYPE;

    enum { rfTypeId = RF_PARAMETER_UINT };

    static void convert(RFProperties from, RF_ATTRIB_TYPE& to) { to = static_cast<RF_ATTRIB_TYPE>(from); }
};


template <>
class RFParameterTrait<bool>
{
public:
    typedef bool RF_ATTRIB_TYPE;

    enum { rfTypeId = RF_PARAMETER_BOOL };

    static void convert(RFProperties from, RF_ATTRIB_TYPE& to) { to = (from != 0); }
};


template <>
class RFParameterTrait<void*>
{
public:
    typedef void* RF_ATTRIB_TYPE;

    enum { rfTypeId = RF_PARAMETER_PTR };

    static void convert(RFProperties from, RF_ATTRIB_TYPE& to) { to = reinterpret_cast<RF_ATTRIB_TYPE>(from); }
};


class RFParameterAttr
{
public:

    RFParameterAttr()
        : m_bProtected(false)
        , m_strName()
        , m_rfType(RF_PARAMETER_UNKNOWN)
        , m_value(0)
    {}


    RFParameterAttr(std::string strName, RFParameterType rfType, RFProperties v)
        : m_bProtected(false)
        , m_strName(strName)
        , m_rfType(rfType)
        , m_value(v)
    {}


    RFParameterAttr(const RFParameterAttr& other)
        : m_bProtected(false)
        , m_strName(other.m_strName)
        , m_rfType(other.m_rfType)
        , m_value(other.m_value)
    {}


    RFParameterAttr(RFParameterAttr&& other)
        : m_bProtected(false)
        , m_strName(std::move(other.m_strName))
        , m_rfType(other.m_rfType)
        , m_value(other.m_value)
    {
        other.m_rfType = RF_PARAMETER_UNKNOWN;
        other.m_value  = 0;
    }


    RFParameterAttr& operator=(RFParameterAttr&& other)
    {
        m_strName = std::move(other.m_strName);

        m_rfType = other.m_rfType;
        m_value  = other.m_value;

        other.m_rfType = RF_PARAMETER_UNKNOWN;
        other.m_value  = 0;

        return *this;
    }


    RFParameterAttr& operator=(const RFParameterAttr& other)
    {
        m_strName = std::move(other.m_strName);

        m_rfType = other.m_rfType;
        m_value  = other.m_value;

        return *this;
    }


    const std::string& getName() const { return m_strName; }
    RFParameterType    getType() const { return m_rfType;  }

    template<class T>
    bool getValue(T& v) const
    {
        typedef RFParameterTrait<T> ParamTrait;

        if (ParamTrait::rfTypeId != m_rfType)
        {
            return false;
        }

        ParamTrait::convert(m_value, v);

        return true;
    }

    RFProperties getRawValue() const
    {
        return m_value;
    }


    bool setValue(const RFProperties& v, bool bProtect)
    {
        if (!m_bProtected)
        {
            m_value = v;
            m_bProtected = bProtect;

            return true;
        }

        return false;
    }


    void forceValue(const RFProperties& v)
    {
        m_value = v;
    }

private:

    bool                    m_bProtected;       // Indicates that the m_value should not be overwritten. 
    std::string             m_strName;
    RFParameterType         m_rfType;
    RFProperties            m_value;
};


class RFParameterMap
{
public:

    RFParameterMap()
        : m_PropertyMap()
    {};

    // Add a parameter to the map.
    bool addParameter(int nParameterName, RFParameterAttr&& rfParam)
    {
        if (isValidParameter(nParameterName))
        {
            // Param already exists.
            return false;
        }

        m_PropertyMap[nParameterName] = std::forward<RFParameterAttr>(rfParam);

        return true;
    }


    // Set value of an existing parameter.
    bool setParameterValue(int nParameterName, const RFProperties& rfValue, bool bProtect = false)
    {
        auto itr = m_PropertyMap.find(nParameterName);

        if (itr == m_PropertyMap.end())
        {
            return false;
        }

        return itr->second.setValue(rfValue, bProtect);
    }


    // Set the value of an existing parameter.
    bool forceParameterValue(int nParameterName, const RFProperties& rfValue)
    {
        auto itr = m_PropertyMap.find(nParameterName);

        if (itr == m_PropertyMap.end())
        {
            return false;
        }

        itr->second.forceValue(rfValue);

        return true;
    }


    template <class T>
    bool getParameterValue(int nParameterName, T& value) const
    {
        auto citr = m_PropertyMap.find(nParameterName);

        if (citr == m_PropertyMap.end())
        {
            return false;
        }

        return citr->second.getValue<T>(value);
    }


    bool getParameterType(int nParameterName, RFParameterType& rfParamType) const
    {
        auto citr = m_PropertyMap.find(nParameterName);

        if (citr == m_PropertyMap.end())
        {
            return false;
        }

        rfParamType = citr->second.getType();

        return true;
    }


    bool getParameterName(int nParameterName, std::string& strParamName)
    {
        auto citr = m_PropertyMap.find(nParameterName);

        if (citr == m_PropertyMap.end())
        {
            return false;
        }

        strParamName = citr->second.getName();

        return true;
    }


    bool isValidParameter(int nParameterName) const
    {
        auto citr = m_PropertyMap.find(nParameterName);

        return (citr != m_PropertyMap.end());
    }

    typedef std::map<int, RFParameterAttr>::const_iterator const_iterator;

    const_iterator  begin() const { return m_PropertyMap.begin(); }
    const_iterator  end()   const { return m_PropertyMap.end();   }

private:

    std::map<int, RFParameterAttr> m_PropertyMap;
};