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

/*****************************************************************************
* RFWrapper.hpp
* * File Version 1.0.0 (CL 36199) Feb 12th 2015
* * File Version 1.0.1 (CL 36735) September 17th 2015
* * File Version 1.1.0.1          January 25th 2016
* * File Version 1.1.0.17         September 23rd 2016
*****************************************************************************/

#pragma once

#include <tchar.h>

#include <Windows.h>

#include "RapidFireServer.h"

typedef RFStatus		    (RAPIDFIRE_API *RF_CREATE_ENCODE_SESSION)     (RFEncodeSession* s, RFProperties* properties);
typedef RFStatus            (RAPIDFIRE_API *RF_DELETE_ENCODE_SESSION)     (RFEncodeSession* s);
typedef RFStatus            (RAPIDFIRE_API *RF_CREATE_ENCODER)            (RFEncodeSession s, unsigned int uiWidth, unsigned int uiHeight, const RFEncodePreset p);
typedef RFStatus            (RAPIDFIRE_API *RF_CREATE_ENCODER2)           (RFEncodeSession s, unsigned int uiWidth, unsigned int uiHeight, const RFProperties* properties);
typedef RFStatus            (RAPIDFIRE_API *RF_REGISTER_RENDERTARGET)     (RFEncodeSession s, RFRenderTarget rt, unsigned int uiRTWidth, unsigned int uiRTHeight, unsigned int* idx);
typedef RFStatus            (RAPIDFIRE_API *RF_REMOVE_RENDERTARGET)       (RFEncodeSession s, unsigned int idx);
typedef RFStatus            (RAPIDFIRE_API *RF_GET_RENDERTARGET_STATE)    (RFEncodeSession s, RFRenderTargetState* state, unsigned int idx);
typedef RFStatus            (RAPIDFIRE_API *RF_RESIZE_SESSION)            (RFEncodeSession s, unsigned int uiWidth, unsigned int uiHeight);
typedef RFStatus            (RAPIDFIRE_API *RF_ENCODE_FRAME)              (RFEncodeSession s, unsigned int idx);
typedef RFStatus            (RAPIDFIRE_API *RF_GET_ENCODED_FRAME)         (RFEncodeSession s, unsigned int* uiSize, void** pBitStream);
typedef RFStatus            (RAPIDFIRE_API *RF_GET_SOURCE_FRAME)          (RFEncodeSession s, unsigned int* uiSize, void** pBitStream);
typedef RFStatus            (RAPIDFIRE_API *RF_SET_ENCODE_PARAMETER)      (RFEncodeSession s, const int param, RFProperties value);
typedef RFStatus            (RAPIDFIRE_API *RF_GET_ENCODE_PARAMETER)      (RFEncodeSession s, const int param, RFProperties* value);
typedef RFStatus            (RAPIDFIRE_API *RF_GET_MOUSEDATA)             (RFEncodeSession s, int iWaitForShapeChange, RFMouseData* md);
typedef RFStatus            (RAPIDFIRE_API *RF_RELEASE_EVENT)             (RFEncodeSession s, RFNotification const rfNotification);


#define GET_RF_PROC(xx)                                          \
    {                                                            \
        void** x = reinterpret_cast<void**>(&rfFunc.xx);         \
        *x = static_cast<void*>(GetProcAddress(m_hDLL, #xx));    \
        if (*x == nullptr)                                       \
        {                                                        \
            return false;                                        \
        }                                                        \
    }


class RFWrapper
{
public:

    static const RFWrapper& getInstance()
    {
        static RFWrapper g_RFWrapper;
        return g_RFWrapper;
    }

    operator bool() const { return m_bRFFuncLoaded; }

    struct RFFunctions
    {
        RF_CREATE_ENCODE_SESSION    rfCreateEncodeSession;
        RF_DELETE_ENCODE_SESSION    rfDeleteEncodeSession;
        RF_CREATE_ENCODER           rfCreateEncoder;
        RF_CREATE_ENCODER2          rfCreateEncoder2;
        RF_REGISTER_RENDERTARGET    rfRegisterRenderTarget;
        RF_REMOVE_RENDERTARGET      rfRemoveRenderTarget;
        RF_GET_RENDERTARGET_STATE   rfGetRenderTargetState;
        RF_RESIZE_SESSION           rfResizeSession;
        RF_ENCODE_FRAME             rfEncodeFrame;
        RF_GET_ENCODED_FRAME        rfGetEncodedFrame;
        RF_GET_SOURCE_FRAME         rfGetSourceFrame;
        RF_SET_ENCODE_PARAMETER     rfSetEncodeParameter;
        RF_GET_ENCODE_PARAMETER     rfGetEncodeParameter;
        RF_GET_MOUSEDATA            rfGetMouseData;
        RF_RELEASE_EVENT            rfReleaseEvent;
    };

    RFFunctions rfFunc;
    HMODULE m_hDLL;

private:

    RFWrapper();
    RFWrapper(const RFWrapper&);

    ~RFWrapper();

    RFWrapper& operator=(const RFWrapper&);

    bool loadFunctions();

    bool m_bRFFuncLoaded;
};

inline RFWrapper::RFWrapper()
    : m_hDLL(NULL)
    , m_bRFFuncLoaded(false)
{
    memset(&rfFunc, 0, sizeof(RFFunctions));
    m_bRFFuncLoaded = loadFunctions();
}

inline RFWrapper::~RFWrapper()
{
    if (m_hDLL)
    {
        FreeLibrary(m_hDLL);
    }

    m_bRFFuncLoaded = false;
    memset(&rfFunc, 0, sizeof(RFFunctions));
}

inline bool RFWrapper::loadFunctions()
{
    m_hDLL = LoadLibrary(_T("RapidFireServer.dll"));

    if (!m_hDLL)
    {
        m_hDLL = LoadLibrary(_T("RapidFireServer64.dll"));
    }

    if (!m_hDLL)
    {
        return false;
    }

    GET_RF_PROC(rfCreateEncodeSession);
    GET_RF_PROC(rfDeleteEncodeSession);
    GET_RF_PROC(rfCreateEncoder);
    GET_RF_PROC(rfCreateEncoder2);
    GET_RF_PROC(rfRegisterRenderTarget);
    GET_RF_PROC(rfRemoveRenderTarget);
    GET_RF_PROC(rfGetRenderTargetState);
    GET_RF_PROC(rfResizeSession);
    GET_RF_PROC(rfEncodeFrame);
    GET_RF_PROC(rfGetEncodedFrame);
    GET_RF_PROC(rfGetSourceFrame);
    GET_RF_PROC(rfSetEncodeParameter);
    GET_RF_PROC(rfGetEncodeParameter);
    GET_RF_PROC(rfGetMouseData);
    GET_RF_PROC(rfReleaseEvent);

    return true;
}