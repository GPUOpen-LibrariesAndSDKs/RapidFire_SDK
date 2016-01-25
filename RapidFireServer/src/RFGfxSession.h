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