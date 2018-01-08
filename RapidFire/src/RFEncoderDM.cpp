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

#include "RFEncoderDM.h"

#include <assert.h>
#include <math.h>

#include <fstream>
#include <sstream>
#include <string>

#include "RFContext.h"
#include "RFEncoderSettings.h"
#include "RFError.h"
#include "RFUtils.h"

using namespace std;

#define DIFF_KERNEL_NAME "rfDiffMapKernel.cl"

#define MULTI_LINE_STR(a) #a

const char* str_cl_DiffMapkernels = MULTI_LINE_STR(     __constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

                                                        __kernel void DiffMap_Image(__read_only image2d_t Image1, __read_only image2d_t Image2, __global unsigned char* DiffMap,
                                                                                    unsigned int DomainSizeX, unsigned int DomainSizeY, const unsigned int uiLocalPxX, const unsigned int uiLocalPxY)
                                                        {
                                                            __local unsigned int result;
                                                            result = 0;
                                                            barrier(CLK_LOCAL_MEM_FENCE);
                                                            short groupX = get_group_id(0);
                                                            short groupY = get_group_id(1);
                                                            short groupIndex = groupX + get_num_groups(0) * groupY;
                                                            short groupSize = get_local_size(0) * get_local_size(1);
                                                            short localIndex = get_local_id(0) + get_local_size(0) * get_local_id(1);

                                                            // Offset into the image
                                                            unsigned int x_offset = groupX * uiLocalPxX;
                                                            unsigned int y_offset = groupY * uiLocalPxY;

                                                            // Limit size to xDimension
                                                            unsigned int localBlockSize = uiLocalPxX * uiLocalPxY;

                                                            float4 pixels1;
                                                            float4 pixels2;

                                                            int2 pos = (int2)(x_offset, y_offset);

                                                            for (; localIndex < localBlockSize; localIndex += groupSize)
                                                            {
                                                                if (result != 0)
                                                                {
                                                                    return;
                                                                }

                                                                unsigned int x = localIndex % uiLocalPxX;
                                                                unsigned int y = localIndex / uiLocalPxX;

                                                                pixels1 = read_imagef(Image1, sampler, pos + (int2)(x, y));
                                                                pixels2 = read_imagef(Image2, sampler, pos + (int2)(x, y));

                                                                if (amd_sad4(as_uint4(pixels1), as_uint4(pixels2), 0) != 0)
                                                                {
                                                                    result = 1;
                                                                    DiffMap[groupIndex] = 1;
                                                                    return;
                                                                }
                                                            }
                                                        };


                                                        __kernel void DiffMap_Buffer(__global unsigned int* Image1, __global unsigned int* Image2, __global unsigned char* DiffMap,
                                                                                     unsigned int DomainSizeX, unsigned int DomainSizeY, const unsigned int uiLocalPxX, const unsigned int uiLocalPxY)
                                                        {
                                                            __local unsigned int result;
                                                            result = 0;
                                                            barrier(CLK_LOCAL_MEM_FENCE);
                                                            short groupX = get_group_id(0);
                                                            short groupY = get_group_id(1);
                                                            short groupIndex = groupX + get_num_groups(0) * groupY;
                                                            short groupSize = get_local_size(0) * get_local_size(1);
                                                            short localIndex = get_local_id(0) + get_local_size(0) * get_local_id(1);

                                                            // Offset into the image
                                                            unsigned int x_offset = groupX * uiLocalPxX;
                                                            unsigned int y_offset = groupY * uiLocalPxY;

                                                            // Offset into linear buffer
                                                            unsigned int idx = x_offset + (DomainSizeX * y_offset);

                                                            // Limit size to xDimension
                                                            unsigned int localBlockSize = uiLocalPxX * uiLocalPxY;

                                                            uint4 pixels1;
                                                            uint4 pixels2;

                                                            for (; localIndex < localBlockSize; localIndex += 4 * groupSize)
                                                            {
                                                                if(result != 0)
                                                                {
                                                                    return;
                                                                }

                                                                for (unsigned int i = 0; i < 4; ++i)
                                                                {
                                                                    unsigned int localIndex_ = localIndex + i * groupSize;
                                                                    unsigned int x = localIndex_ % uiLocalPxX;
                                                                    unsigned int y = localIndex_ / uiLocalPxX;
                                                                    if (x_offset + x < DomainSizeX && y_offset + y < DomainSizeY)
                                                                    {
                                                                        ((unsigned int*)&(pixels1))[i] = Image1[idx + x + y * DomainSizeX];
                                                                        ((unsigned int*)&(pixels2))[i] = Image2[idx + x + y * DomainSizeX];
                                                                    }
                                                                    else
                                                                    {
                                                                        ((unsigned int*)&(pixels1))[i] = 0;
                                                                        ((unsigned int*)&(pixels2))[i] = 0;
                                                                    }
                                                                }
                                                                if (amd_sad4(pixels1, pixels2, 0) != 0)
                                                                {
                                                                    result = 1;
                                                                    DiffMap[groupIndex] = 1;
                                                                    return;
                                                                }
                                                            }
                                                        };
                                                    );


// ATTENTION: The difference encoder needs two frames (ResultBuffers) to create a difference map. In order not to override
// the result buffer of the previous frame, the RFEncoderDM::encode function needs to fail before the result buffer gets written.
// Therefor the diff encoder can only store NUM_RESULT_BUFFERS - 1 frames. This way the RFEncoderDM::m_ResultQueue is full
// but the RFContext::m_clResultBuffer has still one slot left which contains the previous frame.
RFEncoderDM::RFEncoderDM()
    : RFEncoder()
    , m_uiNumTargetBuffers(NUM_RESULT_BUFFERS - 1)
    , m_bLockMappedBuffer(false)
    , m_uiPreviousBuffer(0)
    , m_uiCurrentTargetBuffer(0)
    , m_pClearData(nullptr)
    , m_uiDiffMapSize(0)
    , m_DiffMapImagekernel(NULL)
    , m_DiffMapBufferkernel(NULL)
    , m_pContext(nullptr)
    , m_pMappedBuffer(nullptr)
{
    m_uiTotalBlockSize[0] = 16;
    m_uiTotalBlockSize[1] = 16;

    m_uiNumLocalPixels[0] = 1;
    m_uiNumLocalPixels[1] = 1;

    m_globalDim[0] = 0;
    m_globalDim[1] = 0;

    m_localDim[0] = m_uiTotalBlockSize[0];
    m_localDim[1] = m_uiTotalBlockSize[1];

    m_strEncoderName = "RF_ENCODER_DIFFERENCE";
}


RFEncoderDM::~RFEncoderDM()
{
    if (m_DiffMapImagekernel != NULL)
    {
        clReleaseKernel(m_DiffMapImagekernel);
    }

    if (m_DiffMapBufferkernel != NULL)
    {
        clReleaseKernel(m_DiffMapBufferkernel);
    }

	m_DiffMapProgram.Release();

    deleteBuffers();
}


bool RFEncoderDM::isFormatSupported(RFFormat format) const
{
    return (format == RF_RGBA8 || format == RF_ARGB8 || format == RF_BGRA8);
}


RFStatus RFEncoderDM::init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig)
{
    if (!pConfig)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    if (!pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    m_pContext = pContextCL;

    m_format = pConfig->getInputFormat();

    if (!isFormatSupported(m_format))
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    if (!pConfig->getParameterValue(RF_DIFF_ENCODER_BLOCK_S, m_uiTotalBlockSize[0]))
    {
        m_uiTotalBlockSize[0] = 16;
    }

    if (!pConfig->getParameterValue(RF_DIFF_ENCODER_BLOCK_T, m_uiTotalBlockSize[1]))
    {
        m_uiTotalBlockSize[1] = 16;
    }

    if (!pConfig->getParameterValue<bool>(RF_DIFF_ENCODER_LOCK_BUFFER, m_bLockMappedBuffer))
    {
        m_bLockMappedBuffer = false;
    }

    // For now only a block size of 64 is supported.
    if ((m_uiTotalBlockSize[0] % 8) || (m_uiTotalBlockSize[1] % 8) || (m_uiTotalBlockSize[0] * m_uiTotalBlockSize[1] == 0))
    {
        return RF_STATUS_INVALID_ENCODER_PARAMETER;
    }

    m_uiWidth = pConfig->getEncoderWidth();
    m_uiHeight = pConfig->getEncoderHeight();

    m_uiAlignedWidth = m_uiWidth;
    m_uiAlignedHeight = m_uiHeight;

    if (!createBuffers())
    {
        return RF_STATUS_OPENCL_FAIL;
    }

    // m_uiPreviousBuffer will strore the index of the previously encoded buffer. For the first frame we set it to the maximum.
    // RFSession will use as first buffer the ResultBuffer with index 0. It is expected that the first encoded frame shows a
    // difference on all blocks.
    m_uiPreviousBuffer = m_pContext->getNumResultBuffers() - 1;

    return GenerateCLProgramAndKernel();
}


RFStatus RFEncoderDM::resize(unsigned int uiWidth, unsigned int uiHeight)
{
    if (!deleteBuffers())
    {
        return RF_STATUS_OPENCL_FAIL;
    }

    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;

    m_uiAlignedWidth = m_uiWidth;
    m_uiAlignedHeight = m_uiHeight;

    if (!createBuffers())
    {
        return RF_STATUS_OPENCL_FAIL;
    }

    return RF_STATUS_OK;
}


bool RFEncoderDM::createBuffers()
{
    cl_int nStatus;

    m_uiNumLocalPixels[0] = m_uiTotalBlockSize[0] / static_cast<unsigned int>(m_localDim[0]);
    m_uiNumLocalPixels[1] = m_uiTotalBlockSize[1] / static_cast<unsigned int>(m_localDim[1]);

    unsigned int uiAlignedWidth = static_cast<unsigned int>(ceil(static_cast<float>(m_uiWidth) / static_cast<float>(m_uiTotalBlockSize[0]))) * m_uiTotalBlockSize[0];
    unsigned int uiAlignedHeight = static_cast<unsigned int>(ceil(static_cast<float>(m_uiHeight) / static_cast<float>(m_uiTotalBlockSize[1]))) * m_uiTotalBlockSize[1];

    // Define the dimension of the diff map output buffer.
    m_uiOutputWidth = uiAlignedWidth / m_uiTotalBlockSize[0];
    m_uiOutputHeight = uiAlignedHeight / m_uiTotalBlockSize[1];

    // Create buffers to store diff map.
    m_uiDiffMapSize = (m_uiOutputWidth * m_uiOutputHeight);

    m_globalDim[0] = uiAlignedWidth / m_uiNumLocalPixels[0];
    m_globalDim[1] = uiAlignedHeight / m_uiNumLocalPixels[1];

    m_localDim[0] = m_localDim[0];
    m_localDim[1] = m_localDim[1];

    for (unsigned int i = 0; i < m_uiNumTargetBuffers; ++i)
    {
        DMDiffMapBuffer  TargetBuffer;

        // Create pinned OpenCL buffers that can be accessed by the application to retreive the diff map.
        TargetBuffer.clPageLockedBuffer = clCreateBuffer(m_pContext->getContext(), CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, m_uiDiffMapSize, nullptr, &nStatus);
        if (nStatus != CL_SUCCESS)
        {
            break;
        }

        // Get address of pinned OpenCL buffers.
        TargetBuffer.pSysmemBuffer = static_cast<char*>(clEnqueueMapBuffer(m_pContext->getCmdQueue(), TargetBuffer.clPageLockedBuffer, CL_TRUE, CL_MAP_READ, 0, m_uiDiffMapSize,
                                                        0, nullptr, nullptr, &nStatus));
        if (nStatus != CL_SUCCESS)
        {
            break;
        }

        // Create buffer in GPU mem that will store the diff map computed by the kernel.
        TargetBuffer.clGPUBuffer = clCreateBuffer(m_pContext->getContext(), CL_MEM_READ_WRITE, m_uiDiffMapSize, nullptr, &nStatus);
        if (nStatus != CL_SUCCESS)
        {
            break;
        }

        char cPattern = 0;
        nStatus = clEnqueueFillBuffer(m_pContext->getCmdQueue(), TargetBuffer.clGPUBuffer, &cPattern, sizeof(cPattern), 0, m_uiDiffMapSize, 0, nullptr, nullptr);
        if (nStatus != CL_SUCCESS)
        {
            break;
        }

        m_TargetBuffers.push_back(TargetBuffer);
    }

    if (nStatus != CL_SUCCESS)
    {
        return false;
    }

    clFinish(m_pContext->getCmdQueue());

    return true;
}


bool RFEncoderDM::deleteBuffers()
{
    // Release events that are still in use.
    while (m_ResultQueue.size() > 0)
    {
        const DMDiffMapBuffer* pElem = m_ResultQueue.pop();

        clReleaseEvent(pElem->clDiffFinished);
        clReleaseEvent(pElem->clDMAFinished);
    }

    if (!m_pContext)
    {
        return false;
    }

    if (m_pContext->getCmdQueue())
    {
        clFinish(m_pContext->getCmdQueue());
    }

    if (m_pContext->getDMAQueue())
    {
        clFinish(m_pContext->getDMAQueue());
    }

    cl_int nStatus = CL_SUCCESS;

    for (auto& tb : m_TargetBuffers)
    {
        if (tb.pSysmemBuffer)
        {
            nStatus |= clEnqueueUnmapMemObject(m_pContext->getCmdQueue(), tb.clPageLockedBuffer, tb.pSysmemBuffer, 0, nullptr, nullptr);
            clFinish(m_pContext->getCmdQueue());

            tb.pSysmemBuffer = nullptr;
        }

        if (tb.clPageLockedBuffer)
        {
            nStatus |= clReleaseMemObject(tb.clPageLockedBuffer);
            tb.clPageLockedBuffer = NULL;
        }

        if (tb.clGPUBuffer)
        {
            nStatus |= clReleaseMemObject(tb.clGPUBuffer);
            tb.clGPUBuffer = NULL;
        }
    }

    m_TargetBuffers.clear();

    clFinish(m_pContext->getCmdQueue());

    return (nStatus == CL_SUCCESS);
}


RFStatus RFEncoderDM::encode(unsigned int uiBufferIdx, bool bUseInputImages)
{
    cl_mem          clCurrentImage;
    cl_mem          clPrevImage;
    unsigned int    uiFailCount = 0;

    DMDiffMapBuffer* pCurrentBuffer = &m_TargetBuffers[m_uiCurrentTargetBuffer];

    // m_bLockMappedBuffer should only be set if a separate reader thread is used. In this case a call to RFEncoderDM::encode
    // is possible while the reader thread is still working on the buffer returned by RFEncoderDM::getEncodedFrame.
    // In the single threading case RFEncoderDM::encode is only called after the procession on the buffer returned by RFEncoderDM::getEncodedFrame
    // has finished.
    // This differentiation needs to be done to allow the single threading case to submit 2 frames before calling RFEncoderDM::getEncodedFrame.
    // This enables RFEncoderDM::getEncodedFrame to return without waiting for the current encode task since it can return the
    // result of the previously submitted task.
    while ((m_bLockMappedBuffer && (pCurrentBuffer == m_pMappedBuffer)) || m_ResultQueue.size() >= m_uiNumTargetBuffers)
    {
        // Give other threads the chance to run.
        Sleep(0);
        ++uiFailCount;

        if (uiFailCount > 100)
        {
            return RF_STATUS_QUEUE_FULL;
        }
    }

    cl_kernel diffMapKernel;
    if (bUseInputImages)
    {
        m_pContext->getInputImage(uiBufferIdx, &clCurrentImage);
        m_pContext->getInputImage(m_uiPreviousBuffer, &clPrevImage);
        diffMapKernel = m_DiffMapImagekernel;
    }
    else
    {
        m_pContext->getResultBuffer(uiBufferIdx, &clCurrentImage);
        m_pContext->getResultBuffer(m_uiPreviousBuffer, &clPrevImage);
        diffMapKernel = m_DiffMapBufferkernel;
    }

    SAFE_CALL_CL(clSetKernelArg(diffMapKernel, 0, sizeof(cl_mem),       &clCurrentImage));
    SAFE_CALL_CL(clSetKernelArg(diffMapKernel, 1, sizeof(cl_mem),       &clPrevImage));
    SAFE_CALL_CL(clSetKernelArg(diffMapKernel, 2, sizeof(cl_mem),       &(pCurrentBuffer->clGPUBuffer)));
    SAFE_CALL_CL(clSetKernelArg(diffMapKernel, 3, sizeof(unsigned int), &m_uiWidth));
    SAFE_CALL_CL(clSetKernelArg(diffMapKernel, 4, sizeof(unsigned int), &m_uiHeight));
    SAFE_CALL_CL(clSetKernelArg(diffMapKernel, 5, sizeof(unsigned int), &m_uiTotalBlockSize[0]));
    SAFE_CALL_CL(clSetKernelArg(diffMapKernel, 6, sizeof(unsigned int), &m_uiTotalBlockSize[1]));

    char cPattern = 0;
    SAFE_CALL_CL(clEnqueueFillBuffer(m_pContext->getCmdQueue(), pCurrentBuffer->clGPUBuffer, &cPattern, sizeof(cPattern), 0, m_uiDiffMapSize, 0, nullptr, nullptr));
    SAFE_CALL_CL(clEnqueueNDRangeKernel(m_pContext->getCmdQueue(), diffMapKernel, 2, nullptr, m_globalDim, m_localDim, 0, nullptr, &(pCurrentBuffer->clDiffFinished)));
    SAFE_CALL_CL(clEnqueueCopyBuffer(m_pContext->getCmdQueue(), pCurrentBuffer->clGPUBuffer, pCurrentBuffer->clPageLockedBuffer, 0, 0, m_uiDiffMapSize, 0, nullptr, &pCurrentBuffer->clDMAFinished));

    // Now we can be sure to get a Diff Map -> Store buffer in queue to be retrieved by getEncodedFrame.
    m_ResultQueue.push(pCurrentBuffer);

    clFlush(m_pContext->getCmdQueue());

    if (bUseInputImages)
    {
        const_cast<RFContextCL*>(m_pContext)->releaseCLMemObj(m_pContext->getDMAQueue(), m_uiPreviousBuffer, 1, &(pCurrentBuffer->clDiffFinished));
    }

    m_uiPreviousBuffer = uiBufferIdx;

    m_uiCurrentTargetBuffer = (m_uiCurrentTargetBuffer + 1) % m_uiNumTargetBuffers;

    return RF_STATUS_OK;
}


RFStatus RFEncoderDM::getEncodedFrame(unsigned int& uiSize, void* &pBitStream)
{
    if (m_ResultQueue.size() == 0)
    {
        return RF_STATUS_NO_ENCODED_FRAME;
    }

    const DMDiffMapBuffer* pEncodedBuffer = m_ResultQueue.pop();

    m_pMappedBuffer = pEncodedBuffer;

    // Wait until transfer has completed.
    clWaitForEvents(1, &pEncodedBuffer->clDMAFinished);
    clReleaseEvent(pEncodedBuffer->clDMAFinished);

    // Just release event, no sync is required sine m_clDMAFinished can only finish if m_clDiffFinished is finished.
    clReleaseEvent(pEncodedBuffer->clDiffFinished);

    if (pEncodedBuffer->pSysmemBuffer)
    {
        pBitStream = pEncodedBuffer->pSysmemBuffer;
        uiSize = m_uiDiffMapSize;

        return RF_STATUS_OK;
    }

    return RF_STATUS_NO_ENCODED_FRAME;
}


RFStatus RFEncoderDM::setParameter(const unsigned int uiParameterName, RFParameterType rfType, RFProperties value)
{
    if (uiParameterName == RF_DIFF_ENCODER_LOCK_BUFFER)
    {
        m_bLockMappedBuffer = (value != 0);
    }

    return RF_STATUS_FAIL;
}


RFParameterState RFEncoderDM::getParameter(const unsigned int uiParameterName, RFVideoCodec codec, RFProperties& value) const
{
    if (codec != RF_VIDEO_CODEC_NONE)
    {
        return RF_PARAMETER_STATE_INVALID;
    }

    value = 0;

    if (uiParameterName == RF_DIFF_ENCODER_BLOCK_S)
    {
        value = m_uiTotalBlockSize[0];

        return RF_PARAMETER_STATE_BLOCKED;
    }
    else if (uiParameterName == RF_DIFF_ENCODER_BLOCK_T)
    {
        value = m_uiTotalBlockSize[1];

        return RF_PARAMETER_STATE_BLOCKED;
    }
    else if (uiParameterName == RF_DIFF_ENCODER_LOCK_BUFFER)
    {
        value = m_bLockMappedBuffer;

        return RF_PARAMETER_STATE_READY;
    }

    return RF_PARAMETER_STATE_INVALID;
}

RFStatus RFEncoderDM::GenerateCLProgramAndKernel()
{
    assert(m_pContext);

	m_DiffMapProgram.Create(m_pContext->getContext(), m_pContext->getDeviceId(), DIFF_KERNEL_NAME, str_cl_DiffMapkernels);

    if (m_DiffMapProgram)
    {
        cl_int nStatus;
        m_DiffMapImagekernel = clCreateKernel(m_DiffMapProgram, "DiffMap_Image", &nStatus);
		SAFE_CALL_CL(nStatus);
        m_DiffMapBufferkernel = clCreateKernel(m_DiffMapProgram, "DiffMap_Buffer", &nStatus);
        SAFE_CALL_CL(nStatus);

        return RF_STATUS_OK;
    }
	else
	{
		RF_Error(RF_STATUS_OPENCL_FAIL, m_DiffMapProgram.GetBuildLog().c_str());
	}

    return RF_STATUS_OPENCL_FAIL;
}