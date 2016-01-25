/*****************************************************************************
* Copyright (C) 2013 Advanced Micro Devices, Inc.
* All rights reserved.
*
* This software is provided by the copyright holders and contributors "As is"
* And any express or implied warranties, including, but not limited to, the
* implied warranties of merchantability, non-infringement, and fitness for a
* particular purpose are disclaimed. In no event shall the copyright holder or
* contributors be liable for any direct, indirect, incidental, special,
* exemplary, or consequential damages (including, but not limited to,
* procurement of substitute goods or services; loss of use, data, or profits;
* or business interruption) however caused and on any theory of liability,
* whether in contract, strict liability, or tort (including negligence or
* otherwise) arising in any way out of the use of this software, even if
* advised of the possibility of such damage.
*****************************************************************************/

#include "RFError.h"
#include "RFSession.h"


RFStatus RAPIDFIRE_API rfCreateEncodeSession(RFEncodeSession* session, RFProperties* properties)
{
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


void RAPIDFIRE_API rfDeleteEncodeSession(RFEncodeSession* s)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(*s);

    if (pEncodeSession)
    {
        delete pEncodeSession;
    }

    *s = nullptr;
}


RFStatus RAPIDFIRE_API rfCreateEncoder(RFEncodeSession s, unsigned int uiWidth, unsigned int uiHeight, const RFEncodePreset p)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->createEncoder(uiWidth, uiHeight, p);
}


RFStatus RAPIDFIRE_API rfCreateEncoder2(RFEncodeSession s, unsigned int uiWidth, unsigned int uiHeight, const RFProperties* properties)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->createEncoder(uiWidth, uiHeight, properties);
}


RFStatus RAPIDFIRE_API rfSetEncodeParameter(RFEncodeSession s, const int param, RFProperties value)
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

    return pSession->getEncodeParameter(param, *value);
}


RFStatus RAPIDFIRE_API rfRegisterRenderTarget(RFEncodeSession s, RFRenderTarget rt, unsigned int uiRTWidth, unsigned int uiRTHeight, unsigned int* idx)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    RFTexture rfTex;
    rfTex.rfRT = rt;

    return pSession->registerRenderTarget(rfTex, uiRTWidth, uiRTHeight, *idx);
}


RFStatus RAPIDFIRE_API rfRemoveRenderTarget(RFEncodeSession s, unsigned int idx)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->removeRenderTarget(idx);
}


RFRenderTargetState RAPIDFIRE_API rfGetRenderTargetState(RFEncodeSession s, unsigned int idx)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATE_INVALID;
    }

    return pSession->getRenderTargetState(idx);
}


RFStatus RAPIDFIRE_API rfResizeSession(RFEncodeSession s, unsigned int uiWidth, unsigned int uiHeight)
{
    RFSession* pSession = reinterpret_cast<RFSession*>(s);

    if (!pSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pSession->resize(uiWidth, uiHeight);
}


RFStatus RAPIDFIRE_API rfEncodeFrame(RFEncodeSession s, unsigned int idx)
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

    return pEncodeSession->getEncodedFrame(*uiSize, *pBitStream);
}


RFStatus RAPIDFIRE_API rfGetSourceFrame(RFEncodeSession session, unsigned int* uiSize, void** pBitStream)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(session);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pEncodeSession->getSourceFrame(*uiSize, *pBitStream);
}


RFStatus RAPIDFIRE_API rfGetMouseData(RFEncodeSession s, bool bWaitForShapeChange, RFMouseData* md)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(s);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pEncodeSession->getMouseData(bWaitForShapeChange, *md);
}


RFStatus RAPIDFIRE_API rfReleaseEvent(RFEncodeSession s, RFNotification const rfNotification)
{
    RFSession* pEncodeSession = reinterpret_cast<RFSession*>(s);

    if (!pEncodeSession)
    {
        return RF_STATUS_INVALID_SESSION;
    }

    return pEncodeSession->releaseEvent(rfNotification);
}