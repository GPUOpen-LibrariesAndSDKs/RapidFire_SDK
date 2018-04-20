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

#include "DisplayManager.h"
#include "RFSession.h"
#include "RFUtils.h"

#define DOPP_NUM_RT     3

class GLDOPPCapture;
class DOPPDrvInterface;
class RFMouseGrab;


class RFDOPPSession : public RFSession
{
public:

    explicit RFDOPPSession(RFEncoderID rfEncoder, HDC hDC = NULL, HGLRC hGlrc = NULL);
    ~RFDOPPSession();

private:

    virtual RFStatus    createContextFromGfx()  override;

    virtual RFStatus    finalizeContext()       override;

    virtual RFStatus    resizeResources(unsigned int w, unsigned int h)         override;

    virtual RFStatus    registerTexture(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx)    override;

    virtual RFStatus    preprocessFrame(unsigned int& idx)                      override;

    virtual RFStatus    releaseSessionEvents(RFNotification const rfEvent)      override;

    virtual RFStatus    getMouseData(int iWaitForShapeChange, RFMouseData& md) const override;

    virtual RFStatus    getMouseData2(int iWaitForShapeChange, RFMouseData2& md) const override;

    bool                createGLContext();
    void                dumpDspInfo(const DisplayManager& dpManager);


    HDC                                     m_hDC;
    HGLRC                                   m_hGlrc;
    bool                                    m_bDeleteContexts;

    bool                                    m_bMouseShapeData;
    bool                                    m_bBlockUntilChange;
    bool                                    m_bUpdateOnlyOnChange;

    unsigned int                            m_uiDisplayId;

    std::vector<unsigned int>               m_DesktopRTIndexList;
    unsigned int                            m_uiIdx;

    std::string                             m_strDisplayName;
    std::string                             m_strPrimaryDisplayName;

    std::unique_ptr<GLDOPPCapture>          m_pDeskotpCapture;
    std::unique_ptr<DOPPDrvInterface>       m_pDrvInterface;
    std::unique_ptr<RFMouseGrab>            m_pMouseGrab;

    Timer                                   m_doppTimer;
    unsigned int                            m_uiDoppTextureReinits;
};