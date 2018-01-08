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

#include <queue>

#include <CL/opencl.h>
#include <components/Component.h>
#include <core/Buffer.h>
#include <core/Context.h>

#include "RFEncoder.h"
#include "RFLock.h"

class RFContextCL;

class RFEncoderDM : public RFEncoder
{
public:

    RFEncoderDM();
    ~RFEncoderDM();

    virtual RFStatus            init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig)                           override;

    virtual RFStatus            resize(unsigned int uiWidth, unsigned int uiHeight)                                             override;

    virtual RFStatus            encode(unsigned int uiBufferIdx, bool bUseInputImages)                                          override;

    virtual RFStatus            getEncodedFrame(unsigned int& uiSize, void* &pBitStream)                                        override;

    virtual bool                isFormatSupported(RFFormat format) const                                                        override;

    virtual RFStatus            setParameter(unsigned int const uiParameterName, RFParameterType rfType, RFProperties value)    override;

    virtual RFParameterState    getParameter(unsigned int const uiParameterName, RFVideoCodec codec, RFProperties &value) const override;

    // Returns preferred format of the encoder.
    virtual RFFormat            getPreferredFormat() const override { return RF_RGBA8; }

    virtual RFVideoCodec        getPreferredVideoCodec() const override { return RF_VIDEO_CODEC_NONE; }

    virtual bool                isResizeSupported()  const override { return true; }

private:

    bool                      deleteBuffers();
    bool                      createBuffers();
    RFStatus                  GenerateCLProgramAndKernel();

    struct DMDiffMapBuffer
    {
        cl_mem              clGPUBuffer;
        cl_mem              clPageLockedBuffer;
        char*               pSysmemBuffer;

        cl_event            clDiffFinished;
        cl_event            clDMAFinished;
    };

    bool                                        m_bLockMappedBuffer;

    unsigned int                                m_uiPreviousBuffer;
    unsigned int                                m_uiDiffMapSize;

    unsigned int                                m_uiNumLocalPixels[2];
    unsigned int                                m_uiTotalBlockSize[2];

    const unsigned int                          m_uiNumTargetBuffers;
    unsigned int                                m_uiCurrentTargetBuffer;

    char*                                       m_pClearData;

    size_t                                      m_globalDim[2];
    size_t                                      m_localDim[2];

    cl_kernel                                   m_DiffMapImagekernel;
    cl_kernel                                   m_DiffMapBufferkernel;
    RFProgramCL                                 m_DiffMapProgram;

    const RFContextCL*                          m_pContext;

    // vector of buffers into which the diff kernel can write.
    std::vector<DMDiffMapBuffer>                m_TargetBuffers;

    // Queue that contains references to buffers that store a diff map which were not yet
    // read by calling getEncodedFrame
    RFLockedQueue<const DMDiffMapBuffer*>       m_ResultQueue;

    // Pointer to the buffer that was retrieved by calling getEncodedFrame
    const DMDiffMapBuffer*                      m_pMappedBuffer;
};