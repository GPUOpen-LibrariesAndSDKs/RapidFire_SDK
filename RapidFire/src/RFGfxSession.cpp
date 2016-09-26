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

#include "RFGfxSession.h"

#include <d3d11.h>
#include <d3d9.h>
#include <GL/glew.h>

#include "RFEncoderSettings.h"
#include "RFError.h"


RFGLSession::RFGLSession(HDC hDC, HGLRC hGlrc, RFEncoderID rfEncoder)
    : RFSession(rfEncoder)
    , m_hDC(hDC)
    , m_hGlrc(hGlrc)
{
    if (m_hGlrc == NULL || m_hDC == NULL)
    {
         m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create GL session. Invalid context");

         throw std::runtime_error("Failed to create GL session. Invalid context");
    }

    try
    {
        // Add all know parameters to map.
        m_ParameterMap.addParameter(RF_GL_GRAPHICS_CTX,   RFParameterAttr("RF_GL_GRAPHICS_CTX",   RF_PARAMETER_PTR, 0));
        m_ParameterMap.addParameter(RF_GL_DEVICE_CTX,     RFParameterAttr("RF_GL_DEVICE_CTX",     RF_PARAMETER_PTR, 0));
    }
    catch(...)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create GL Parameters.");

        throw std::runtime_error("Failed to create GL Parameters.");
    }
}


RFStatus RFGLSession::createContextFromGfx()
{
    if (!m_pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    return m_pContextCL->createContext(m_hDC, m_hGlrc);
}


RFStatus RFGLSession::finalizeContext()
{
    // For GL we need to invert the image since the origin is in the lower left corner.
    // but the encoder expects it in the upper left
    m_Properties.bInvertInput = !m_Properties.bInvertInput;

    // Force update in parameter map.
    m_ParameterMap.forceParameterValue(RF_FLIP_SOURCE, m_Properties.bInvertInput);

    return RF_STATUS_OK;
}


RFStatus RFGLSession::registerTexture(RFTexture rt, unsigned int uiWidth,unsigned int uiHeight, unsigned int& idx) 
{
    GLuint uiTex = rt.uiGLTexName;

    if (uiTex == 0)
    {
        return RF_STATUS_INVALID_TEXTURE;
    }

    if (!m_pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    return m_pContextCL->setInputTexture(uiTex, uiWidth, uiHeight, idx);
}


RFDX9Session::RFDX9Session(IDirect3DDevice9* pDx9Dev, RFEncoderID rfEncoder)
    : RFSession(rfEncoder)
    , m_pDx9Device(pDx9Dev)
    , m_pDx9DeviceEX(nullptr)
{
    if (!m_pDx9Device)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create DX9 session. Invalid device");

        throw std::runtime_error("Failed to create DX9 session. Invalid device");
    }

    try
    {
        // Add all know parameters to map
        m_ParameterMap.addParameter(RF_D3D9_DEVICE,     RFParameterAttr("RF_D3D9_DEVICE",    RF_PARAMETER_PTR, 0));
        m_ParameterMap.addParameter(RF_D3D9EX_DEVICE,   RFParameterAttr("RF_D3D9EX_DEVICE",  RF_PARAMETER_PTR, 0));
    }
    catch(...)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create DX9 Parameters.");

        throw std::runtime_error("Failed to create DX9 Parameters.");
    }
}


RFDX9Session::RFDX9Session(IDirect3DDevice9Ex* pDx9DevEx, RFEncoderID rfEncoder)
    : RFSession(rfEncoder)
    , m_pDx9Device(nullptr), m_pDx9DeviceEX(pDx9DevEx)
{
    if (!m_pDx9DeviceEX)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create DX9 EX session. Invalid device");

        throw std::runtime_error("Failed to create DX9 EX session. Invalid device");
    }

    try
    {
        // Add all know parameters to map
        m_ParameterMap.addParameter(RF_D3D9_DEVICE,     RFParameterAttr("RF_D3D9_DEVICE",    RF_PARAMETER_PTR, 0));
        m_ParameterMap.addParameter(RF_D3D9EX_DEVICE,   RFParameterAttr("RF_D3D9EX_DEVICE",  RF_PARAMETER_PTR, 0));
    }
    catch(...)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create DX9 EX Parameters.");

        throw std::runtime_error("Failed to create DX9 EX Parameters. ");
    }
}


RFStatus RFDX9Session::createContextFromGfx()
{
    if (!m_pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    if (m_pDx9Device)
    {
        return m_pContextCL->createContext(m_pDx9Device);
    }

    if (m_pDx9DeviceEX)
    {
        return m_pContextCL->createContext(m_pDx9DeviceEX);
    }

    return RF_STATUS_INVALID_CONTEXT;
}


RFStatus RFDX9Session::finalizeContext()   
{
    if (m_Properties.EncoderId == RF_AMF && m_pDx9Device)
    {
        // For DX9 devices no OpenCL interop is available. AMF supports DX9 but only
        // with BGRA surfaces. We need to submit BGRA to AMF and run NO csc.
        m_pEncoderSettings->setFormat(RF_BGRA8);
    }

    return RF_STATUS_OK;
}


RFStatus RFDX9Session::registerTexture(RFTexture rt, unsigned int uiWidth,unsigned int uiHeight, unsigned int& idx) 
{
    IDirect3DSurface9* pDx9Surface = rt.pDX9TexPtr;

    if (pDx9Surface == nullptr)
    {
        return RF_STATUS_INVALID_TEXTURE;
    }

    if (!m_pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    return m_pContextCL->setInputTexture(pDx9Surface, uiWidth, uiHeight, idx);
}


RFDX11Session::RFDX11Session(ID3D11Device* pDx11Device, RFEncoderID rfEncoder) 
    : RFSession(rfEncoder)
    , m_pDX11Device(pDx11Device)
{
    if (m_pDX11Device == nullptr)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create DX11 session. Invalid device");

        throw std::runtime_error("Failed to create DX11 session. Invalid device");
    }

    try
    {
        // Add all know parameters to map.
        m_ParameterMap.addParameter(RF_D3D11_DEVICE, RFParameterAttr("RF_D3D11_DEVICE", RF_PARAMETER_PTR, 0));
    }
    catch(...)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create DX11 Parameters.");

        throw std::runtime_error("Failed to create DX11 Parameters. ");
    }
}


RFStatus RFDX11Session::createContextFromGfx()
{
    if (!m_pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    return m_pContextCL->createContext(m_pDX11Device);
}


RFStatus RFDX11Session::registerTexture(RFTexture rt, unsigned int uiWidth,unsigned int uiHeight, unsigned int& idx) 
{
    ID3D11Texture2D* pDX11Tex = rt.pDX11TexPtr;

    if (pDX11Tex == nullptr)
    {
        return RF_STATUS_INVALID_TEXTURE;
    }

    if (!m_pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    return m_pContextCL->setInputTexture(pDX11Tex, uiWidth, uiHeight, idx);
}