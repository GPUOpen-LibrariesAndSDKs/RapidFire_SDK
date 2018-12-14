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

#include "RFError.h"
#include "RFSession.h"


RFStatus RAPIDFIRE_API rfCreateEncodeSession(RFEncodeSession* session, const RFProperties* properties)
{
    if (!session || !properties)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    RFSession* pSession = nullptr;
    RFStatus rfStatus = createRFSession(&pSession, properties);

    if (rfStatus != RF_STATUS_OK)
    {
        *session = nullptr;
        return rfStatus;
    }

    // Create OpenCL context, compile OpenCL kernels
    rfStatus = pSession->createContext();
    if (rfStatus != RF_STATUS_OK)
    {
        delete pSession;
        *session = nullptr;
        return rfStatus;
    }

    *session = static_cast<RFEncodeSession>(pSession);

    return RF_STATUS_OK;
}


RFStatus RAPIDFIRE_API rfDeleteEncodeSession(RFEncodeSession* s)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(*s);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    delete pEncodeSession;
    *s = nullptr;
    return RF_STATUS_OK;
}


RFStatus RAPIDFIRE_API rfCreateEncoder(RFEncodeSession s, const unsigned int uiWidth, const unsigned int uiHeight, const RFEncodePreset p)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    RFVideoCodec codec = RF_VIDEO_CODEC_AVC;
    RFEncodePreset preset = p;
    switch(preset)
    {
        case RF_PRESET_HEVC_FAST:
            codec = RF_VIDEO_CODEC_HEVC;
            preset = RF_PRESET_FAST;
            break;

        case RF_PRESET_HEVC_BALANCED:
            codec = RF_VIDEO_CODEC_HEVC;
            preset = RF_PRESET_BALANCED;
            break;

        case RF_PRESET_HEVC_QUALITY:
            codec = RF_VIDEO_CODEC_HEVC;
            preset = RF_PRESET_QUALITY;
            break;
    }

    return pSession->createEncoder(uiWidth, uiHeight, codec, preset);
}


RFStatus RAPIDFIRE_API rfCreateEncoder2(RFEncodeSession s, const unsigned int uiWidth, const unsigned int uiHeight, const RFProperties* properties)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    if (!properties)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    return pSession->createEncoder(uiWidth, uiHeight, properties);
}


RFStatus RAPIDFIRE_API rfSetEncodeParameter(RFEncodeSession s, const int param, const RFProperties value)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->setEncodeParameter(param, value);
}


RFStatus RAPIDFIRE_API rfGetEncodeParameter(RFEncodeSession s, const int param, RFProperties* value)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    if (!value)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    return pSession->getEncodeParameter(param, *value);
}


RFStatus RAPIDFIRE_API rfRegisterRenderTarget(RFEncodeSession s, const RFRenderTarget rt, const unsigned int uiRTWidth, const unsigned int uiRTHeight, unsigned int* idx)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    RFTexture rfTex;
    rfTex.rfRT = rt;

    if (!idx)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    return pSession->registerRenderTarget(rfTex, uiRTWidth, uiRTHeight, *idx);
}


RFStatus RAPIDFIRE_API rfRemoveRenderTarget(RFEncodeSession s, const unsigned int idx)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->removeRenderTarget(idx);
}


RFStatus RAPIDFIRE_API rfGetRenderTargetState(RFEncodeSession s, RFRenderTargetState* state, const unsigned int idx)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!state)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    if (!pSession)
    {
        *state = RF_STATE_INVALID;
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->getRenderTargetState(state, idx);
}


RFStatus RAPIDFIRE_API rfResizeSession(RFEncodeSession s, const unsigned int uiWidth, const unsigned int uiHeight)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->resize(uiWidth, uiHeight);
}


RFStatus RAPIDFIRE_API rfEncodeFrame(RFEncodeSession s, const unsigned int idx)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(s);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pEncodeSession->encodeFrame(idx);
}


RFStatus RAPIDFIRE_API rfGetEncodedFrame(RFEncodeSession session, unsigned int* uiSize, void** pBitStream)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(session);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    if (!uiSize || !pBitStream)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    return pEncodeSession->getEncodedFrame(*uiSize, *pBitStream);
}


RFStatus RAPIDFIRE_API rfGetSourceFrame(RFEncodeSession session, unsigned int* uiSize, void** pBitStream)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(session);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    if (!uiSize || !pBitStream)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    return pEncodeSession->getSourceFrame(*uiSize, *pBitStream);
}


RFStatus RAPIDFIRE_API rfGetMouseData(RFEncodeSession s, const int iWaitForShapeChange, RFMouseData* md)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(s);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    if (!md)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    return pEncodeSession->getMouseData(iWaitForShapeChange, *md);
}


RFStatus RAPIDFIRE_API rfGetMouseData2(RFEncodeSession s, const int iWaitForShapeChange, RFMouseData2* md)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(s);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    if (!md)
    {
        return RF_STATUS_INVALID_PARAMETER;
    }

    return pEncodeSession->getMouseData2(iWaitForShapeChange, *md);
}


RFStatus RAPIDFIRE_API rfReleaseEvent(RFEncodeSession s, const RFNotification rfNotification)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(s);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pEncodeSession->releaseEvent(rfNotification);
}