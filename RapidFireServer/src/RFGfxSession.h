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

#include "RFSession.h"

class RFGLSession : public RFSession
{
public:

    RFGLSession(HDC hDC, HGLRC hGlrc, RFEncoderID rfEncoder);

private:

    virtual RFStatus    createContextFromGfx()  override;

    virtual RFStatus    finalizeContext()       override;

    virtual RFStatus    registerTexture(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx) override;

    HDC            m_hDC;
    HGLRC          m_hGlrc;
};


class RFDX9Session : public RFSession
{
public:

    RFDX9Session(IDirect3DDevice9*   pDx9Dev, RFEncoderID rfEncoder);
    RFDX9Session(IDirect3DDevice9Ex* pDx9Dev, RFEncoderID rfEncoder);

private:

    virtual RFStatus    createContextFromGfx()  override;

    virtual RFStatus    finalizeContext()       override;

    virtual RFStatus    registerTexture(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx) override;

    IDirect3DDevice9*       m_pDx9Device;
    IDirect3DDevice9Ex*     m_pDx9DeviceEX;
};


class RFDX11Session : public RFSession
{
public:

    RFDX11Session(ID3D11Device* pDx11Device, RFEncoderID rfEncoder);

private:

    virtual RFStatus    createContextFromGfx()  override;

    virtual RFStatus    registerTexture(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx) override;

    ID3D11Device*       m_pDX11Device;
};