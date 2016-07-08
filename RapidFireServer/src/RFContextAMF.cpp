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

#include "RFContextAMF.h"

#include <CL/cl.h>
#include <components/VideoEncoderVCE.h>

#include "AMFWrapper.h"
#include "RFError.h"

using namespace amf;

#define CHECK_AMF_ERROR(a) if (a != AMF_OK) return RF_STATUS_AMF_FAIL;

RFContextAMF::RFContextAMF()
    : RFContextCL()
    , m_uiPlaneCount(0)
    , m_amfContext(NULL)
    , m_amfFormat(AMF_SURFACE_UNKNOWN)
    , m_amfMemory(AMF_MEMORY_UNKNOWN)
{
    m_pSurfaceList = new AMFSurfacePtr[NUM_RESULT_BUFFERS];

    for (int i = 0; i < MAX_NUM_RENDER_TARGETS; ++i)
    {
        m_pD3D9Surfaces[i] = NULL;
    }
}


RFContextAMF::~RFContextAMF()
{
    // The OpenCL context was created by AMF and it should be released by AMF
    // DO not call clRelease here and make sure it is called in RFContextCL!
    m_clCtx      = NULL;
    m_clCmdQueue = NULL;
    m_clDevId    = NULL;

    if (m_pSurfaceList)
    {
        delete[] m_pSurfaceList;
    }

    for (int i = 0; i < MAX_NUM_RENDER_TARGETS; ++i)
    {
        // Surfaces are created by the application, so no need to delete them here.
        m_pD3D9Surfaces[i] = NULL;
    }
}


RFStatus RFContextAMF::createContext()
{
    // No interop device is known, this config is not supported with AMF.
    return RF_STATUS_AMF_FAIL;
}


RFStatus RFContextAMF::createContext(DeviceCtx hDC, GraphicsCtx hGLRC)
{
    AMF_RESULT amfErr;

    // Check if we already have a valid context.
    if (m_bValid)
    {
        return RF_STATUS_FAIL;
    }

    amfErr = AMFWrapper::CreateContext(&m_amfContext);
    CHECK_AMF_ERROR(amfErr);

    // Pass OpenGL context to AMF. If a valuid DC is provided the HWND is not used.
    amfErr = m_amfContext->InitOpenGL(hGLRC, NULL, hDC);
    CHECK_AMF_ERROR(amfErr);

    amfErr = m_amfContext->InitDX9(nullptr);
    CHECK_AMF_ERROR(amfErr);

    amfErr = m_amfContext->InitOpenCL();
    CHECK_AMF_ERROR(amfErr);

    m_CtxType = RF_CTX_FROM_GL;

    return finalizeContext();
}


RFStatus RFContextAMF::createContext(ID3D11Device* pD3DDevice)
{
    AMF_RESULT amfErr;

    // Check if we already have an valid context.
    if (m_bValid)
    {
        return RF_STATUS_FAIL;
    }

    if (!pD3DDevice)
    {
        return RF_STATUS_INVALID_D3D_DEVICE;
    }

    amfErr = AMFWrapper::CreateContext(&m_amfContext);
    CHECK_AMF_ERROR(amfErr);

    amfErr = m_amfContext->InitDX11(pD3DDevice);
    CHECK_AMF_ERROR(amfErr);

    // We still need to init DX9 unless DX 11.1 is available.
    amfErr = m_amfContext->InitDX9(nullptr);
    CHECK_AMF_ERROR(amfErr);

    amfErr = m_amfContext->InitOpenCL();
    CHECK_AMF_ERROR(amfErr);

    m_CtxType = RF_CTX_FROM_DX11;

    return finalizeContext();
}


RFStatus RFContextAMF::createContext(IDirect3DDevice9Ex* pD3DDeviceEx)
{
    AMF_RESULT amfErr;

    // Check if we already have an valid context.
    if (m_bValid)
    {
        return RF_STATUS_FAIL;
    }

    if (!pD3DDeviceEx)
    {
        return RF_STATUS_INVALID_D3D_DEVICE;
    }

    amfErr = AMFWrapper::CreateContext(&m_amfContext);
    CHECK_AMF_ERROR(amfErr);

    amfErr = m_amfContext->InitDX9(pD3DDeviceEx);
    CHECK_AMF_ERROR(amfErr);

    amfErr = m_amfContext->InitOpenCL();
    CHECK_AMF_ERROR(amfErr);

    m_CtxType = RF_CTX_FROM_DX9EX;

    return finalizeContext();
}


RFStatus RFContextAMF::createContext(IDirect3DDevice9* pD3DDevice)
{
    AMF_RESULT amfErr;

    // Check if we already have an valid context.
    if (m_bValid)
    {
        return RF_STATUS_FAIL;
    }

    if (!pD3DDevice)
    {
        return RF_STATUS_INVALID_D3D_DEVICE;
    }

    amfErr = AMFWrapper::CreateContext(&m_amfContext);
    CHECK_AMF_ERROR(amfErr);

    amfErr = m_amfContext->InitDX9(pD3DDevice);
    CHECK_AMF_ERROR(amfErr);

    m_CtxType = RF_CTX_FROM_DX9;

    // In case of DX9 the CSC is done by AMF. The plane count is 1.
    m_uiPlaneCount = 1;

    // For DX9 no OpenCL interop is used -> no need to call finalizeContext().
    m_bValid = true;

    return RF_STATUS_OK;
}


RFStatus RFContextAMF::finalizeContext()
{
    cl_int nStatus;

    m_clCtx      = static_cast<cl_context>(m_amfContext->GetOpenCLContext());
    m_clCmdQueue = static_cast<cl_command_queue>(m_amfContext->GetOpenCLCommandQueue());
    m_clDevId    = static_cast<cl_device_id>(m_amfContext->GetOpenCLDeviceID());

    if (!m_clCtx || !m_clCmdQueue || !m_clDevId)
    {
        return RF_STATUS_AMF_FAIL;
    }

    // Create DMA queue that can be used for async readback. This queue is only used if the app requests
    // to readback the result buffer.
    m_clDMAQueue = clCreateCommandQueue(m_clCtx, m_clDevId, 0, &nStatus);
    SAFE_CALL_CL(nStatus);

    // create CSC kernel
    SAFE_CALL_RF(setupKernel());

    setMemAccessFunction();

    // Indicate that the context is ready.
    m_bValid = true;

    return RF_STATUS_OK;
}


RFStatus RFContextAMF::createBuffers(RFFormat format, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiAlignedWidth, unsigned int uiAlignedHeight, bool bUseAsyncCopy)
{
    AMF_RESULT amfErr;

    if (!m_bValid)
    {
        return RF_STATUS_INVALID_CONTEXT;
    }

    if (!validateDimensions(uiWidth, uiHeight))
    {
        // Scaling is not supported.
        return RF_STATUS_INVALID_DIMENSION;
    }

    m_amfFormat = AMF_SURFACE_NV12;
    m_amfMemory = AMF_MEMORY_OPENCL;

    m_TargetFormat = RF_NV12;
    m_uiCSCKernelIdx = RF_KERNEL_RGBA_TO_NV12_PLANES;

    m_uiOutputWidth = uiWidth;
    m_uiOutputHeight = uiHeight;
    m_uiAlignedOutputWidth = uiAlignedWidth;
    m_uiAlignedOutputHeight = uiAlignedHeight;

    // The AMF encoder is implemented to use NV12 as input. The RFContextAMD::processBuffers will transform the input buffer to
    // a NV12 buffer that is passed to the encoder.
    // EXCEPTION DX9: Since no CL-DX9 interop exists the DX9 BGRA surfaces are passed directly to the AMF encoder. The CSC
    // is done by AMF. For DX9 only BGRA is supported. Dx9Ex will work through the OpenCL CSC.
    if (m_CtxType == RF_CTX_FROM_DX9)
    {
        if (format == RF_BGRA8)
        {
            m_amfFormat = AMF_SURFACE_BGRA;
            m_amfMemory = AMF_MEMORY_DX9;

            m_uiCSCKernelIdx = RF_KERNEL_UNKNOWN;
        }
        else
        {
            return RF_STATUS_INVALID_FORMAT;
        }
    }
    else if (format != RF_NV12)
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    // Create surdfaces that are used as target for the CSC and as input for the VCE.
    for (unsigned int i = 0; i < NUM_RESULT_BUFFERS; ++i)
    {
        // For OpenCL it is recommended to allocate a DX9 surface and convert it.
        amfErr = m_amfContext->AllocSurface(AMF_MEMORY_DX9, m_amfFormat, m_uiAlignedOutputWidth, m_uiAlignedOutputHeight, &(m_pSurfaceList[i]));
        CHECK_AMF_ERROR(amfErr);

        if (m_amfMemory == AMF_MEMORY_OPENCL)
        {
            m_pSurfaceList[i]->Convert(AMF_MEMORY_OPENCL);
        }
    }

    if (m_CtxType == RF_CTX_FROM_DX9)
    {
        // We are done since DX9 has no CSC kernels.
        return RF_STATUS_OK;
    }

    if (!configureKernels())
    {
        return RF_STATUS_OPENCL_FAIL;
    }

    m_uiPlaneCount = static_cast<unsigned int>(m_pSurfaceList[0]->GetPlanesCount());

    return RF_STATUS_OK;
}


RFStatus RFContextAMF::processBuffer(bool bInvert, unsigned int uiSorceIdx, unsigned int uiDestIdx)
{
    if (!m_bValid)
    {
        return RF_STATUS_INVALID_CONTEXT;
    }

    if (m_CtxType == RF_CTX_FROM_DX9)
    {
        amf::AMFSurfacePtr pTmpSurface;
        AMF_RESULT amfErr = m_amfContext->CreateSurfaceFromDX9Native(m_pD3D9Surfaces[uiSorceIdx], &pTmpSurface, nullptr);

        amf::AMFDataPtr pDuplicated;

        pTmpSurface->Duplicate(pTmpSurface->GetMemoryType(), &pDuplicated);

        m_pSurfaceList[uiDestIdx] = AMFSurfacePtr(pDuplicated);

        if (amfErr != AMF_OK)
        {
            return RF_STATUS_AMF_FAIL;
        }

        return RF_STATUS_OK;
    }

    if (m_TargetFormat != RF_NV12 || m_uiCSCKernelIdx != RF_KERNEL_RGBA_TO_NV12_PLANES)
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    // Make sure OpenCL events get released. This is usually done when getAMFSurface is called. In case getAMFSurface
    // was not called for some reason, the event is released here.
    // No need to wait since the session will not allow to encode frames if no free surface is available.
    m_clCSCFinished[uiDestIdx].release();

    cl_mem clEncodeBufferY = static_cast<cl_mem>(getImageBuffer(uiDestIdx, 0));
    cl_mem clEncodeBufferUV = static_cast<cl_mem>(getImageBuffer(uiDestIdx, 1));

    if (!clEncodeBufferY || !clEncodeBufferY)
    {
        return RF_STATUS_INVALID_OPENCL_MEMOBJ;
    }

    // Acquire OpenCL object from OpenGl/D3D object.
    SAFE_CALL_RF(acquireCLMemObj(uiSorceIdx));

    // RGBA input buffer (src)
    SAFE_CALL_CL(clSetKernelArg(m_CSCKernels[m_uiCSCKernelIdx].kernel, 0, sizeof(cl_mem), static_cast<void*>(&(m_clInputImage[uiSorceIdx]))));

    // Y output buffer (dst)
    SAFE_CALL_CL(clSetKernelArg(m_CSCKernels[m_uiCSCKernelIdx].kernel, 1, sizeof(cl_mem), static_cast<void*>(&clEncodeBufferY)));

    // UV output buffer (dst)
    SAFE_CALL_CL(clSetKernelArg(m_CSCKernels[m_uiCSCKernelIdx].kernel, 4, sizeof(cl_mem), static_cast<void*>(&clEncodeBufferUV)));

    cl_int nInvert = (bInvert) ? 1 : 0;

    // Indicate if flipping is required
    SAFE_CALL_CL(clSetKernelArg(m_CSCKernels[m_uiCSCKernelIdx].kernel, 3, sizeof(cl_int), static_cast<void*>(&nInvert)));

    SAFE_CALL_CL(clEnqueueNDRangeKernel(m_clCmdQueue, m_CSCKernels[m_uiCSCKernelIdx].kernel, 2, nullptr,
                 m_CSCKernels[m_uiCSCKernelIdx].uiGlobalWorkSize, m_CSCKernels[m_uiCSCKernelIdx].uiLocalWorkSize, 0,
                 nullptr, &(m_clCSCFinished[uiDestIdx])));

    clFlush(m_clCmdQueue);

    // Release OpenCL object from OpenGL/D3D object.
    SAFE_CALL_RF(releaseCLMemObj(uiSorceIdx));

    return RF_STATUS_OK;
}


AMFSurfacePtr RFContextAMF::getAMFSurface(unsigned int uiIdx) const
{
    if (m_CtxType == RF_CTX_FROM_DX9)
    {
        return m_pSurfaceList[uiIdx];
    }

    if (uiIdx < NUM_RESULT_BUFFERS)
    {
        // We need to wait until CSC kernel finished.
        m_clCSCFinished[uiIdx].wait();

        return m_pSurfaceList[uiIdx];
    }

    return NULL;
}


void* RFContextAMF::getImageBuffer(unsigned int uiBuffer, unsigned int uiPlaneId)
{
    if (uiPlaneId >= m_uiPlaneCount)
    {
        return nullptr;
    }

    AMF_MEMORY_TYPE amfMemType = m_pSurfaceList[uiBuffer]->GetMemoryType();

    if (amfMemType != m_amfMemory)
    {
        m_pSurfaceList[uiBuffer]->Convert(m_amfMemory);
    }

    AMFPlane* pPlane = m_pSurfaceList[uiBuffer]->GetPlaneAt(uiPlaneId);

    if (!pPlane)
    {
        return nullptr;
    }

    return pPlane->GetNative();
}


RFStatus RFContextAMF::setInputTexture(IDirect3DSurface9* pD3D9Texture, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx)
{
    if (m_CtxType != RF_CTX_FROM_DX9)
    {
        return RFContextCL::setInputTexture(pD3D9Texture, uiWidth, uiHeight, idx);
    }

    unsigned int index;

    idx = 0xFF;

    if (!validateDimensions(uiWidth, uiHeight))
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    // Get index of a free slot.
    if (!getFreeRenderTargetIndex(index))
    {
        return RF_STATUS_RENDER_TARGET_FAIL;
    }

    m_pD3D9Surfaces[index] = pD3D9Texture;

    m_rtState[index] = RF_STATE_FREE;

    ++m_uiNumRegisteredRT;

    m_uiInputWidth = uiWidth;
    m_uiInputHeight = uiHeight;

    idx = index;

    return RF_STATUS_OK;
}