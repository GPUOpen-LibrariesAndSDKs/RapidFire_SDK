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

#include <core/Buffer.h>
#include <components/Component.h>

#include "RFContext.h"

class RFContextAMF : public RFContextCL
{
public:

    RFContextAMF();
    ~RFContextAMF();

    // Creates OpenCL context.
    virtual RFStatus    createContext() override;

    // Creates OpenCL context based on an existing OpenGL context.
    virtual RFStatus    createContext(DeviceCtx hDC, GraphicsCtx hGLRC) override;
    // Creates OpenCL context based on an existing D3D9 Device.
    virtual RFStatus    createContext(ID3D11Device*         pD3DDevice) override;
    // Creates OpenCL context based on an existing D3D9 Ex Device.
    virtual RFStatus    createContext(IDirect3DDevice9*     pD3DDevice) override;
    // Creates OpenCL context based on an existing D3D11 Device.
    virtual RFStatus    createContext(IDirect3DDevice9Ex* pD3DDeviceEx) override;

    // Registers DX9 texture. DX9 is only supported with AMF.
    virtual RFStatus    setInputTexture(IDirect3DSurface9* pD3D9Texture, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx) override;

    virtual RFStatus    createBuffers(RFFormat format, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiAlignedWidth, unsigned int uiAlignedHeight, bool bUseAsyncCopy) override;

    virtual RFStatus    processBuffer(bool bRunCSC, bool bInvert, unsigned int uiSorceIdx, unsigned int uiDestIdx) override;

    amf::AMFContextPtr  getAMFContext() const { return m_amfContext; };

    amf::AMFSurfacePtr  getAMFSurface(unsigned int uiIdx) const;

    amf::AMF_SURFACE_FORMAT getAMFSurfaceFormat() const { return m_amfFormat; };

private:

    cl_mem              getImageBuffer(unsigned int uiBuffer, unsigned int uiPlaneId);

    RFStatus            createNV12InteropFromDX11(unsigned int idx);
    RFStatus            createNV12InteropFromDX9(unsigned int idx);
    RFStatus            acquireNV12Planes(cl_command_queue clQueue, unsigned int idx, unsigned int numEvents = 0, cl_event* eventsWait = nullptr, cl_event* eventReturned = nullptr);
    RFStatus            releaseNV12Planes(cl_command_queue clQueue, unsigned int idx, unsigned int numEvents = 0, cl_event* eventsWait = nullptr, cl_event* eventReturned = nullptr);
    void                releaseNV12Interop(unsigned int idx);

    // Performs common actions like compiling kernels, creating dma queue to finish context creation.
    RFStatus            finalizeContext();

    unsigned int                    m_uiPlaneCount;

    amf::AMFContextPtr              m_amfContext;
    amf::AMF_SURFACE_FORMAT         m_amfFormat;
    amf::AMFSurfacePtr*             m_pSurfaceList;
    amf::AMF_MEMORY_TYPE            m_amfMemory;
    cl_mem                          m_clNV12Planes[MAX_NUM_RENDER_TARGETS * 2];
    amf::AMF_MEMORY_TYPE            m_clNV12Memory[MAX_NUM_RENDER_TARGETS];

    IDirect3DSurface9*              m_pD3D9Surfaces[MAX_NUM_RENDER_TARGETS];
};