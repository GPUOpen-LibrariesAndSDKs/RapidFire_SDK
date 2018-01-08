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

#include "RFEncoderIdentity.h"

#include <assert.h>

#include "RFEncoderSettings.h"
#include "RFError.h"
#include "RFUtils.h"


RFEncoderIdentity::RFEncoderIdentity()
    : RFEncoder()
    , m_nBufferSize(0)
    , m_pBuffer(nullptr)
    , m_pContext(nullptr)
{
    m_strEncoderName = "RF_ENCODER_IDENTITY";
}


RFStatus RFEncoderIdentity::init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig)
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

    m_uiWidth  = pConfig->getEncoderWidth();
    m_uiHeight = pConfig->getEncoderHeight();

    m_uiAlignedWidth  = m_uiWidth;
    m_uiAlignedHeight = m_uiHeight;

    m_uiOutputWidth  = m_uiAlignedWidth;
    m_uiOutputHeight = m_uiAlignedHeight;

    // Set alignment in CSC.
    if (m_format == RF_RGBA8 || m_format == RF_ARGB8 || m_format == RF_BGRA8)
    {
        m_nBufferSize = m_uiAlignedWidth * m_uiAlignedHeight * 4;
    }
    else if (m_format == RF_NV12)
    {
        m_nBufferSize = (m_uiAlignedWidth * m_uiAlignedHeight) + (m_uiAlignedWidth * m_uiAlignedHeight) / 2;
    }
    else
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    return RF_STATUS_OK;
}


RFStatus RFEncoderIdentity::resize(unsigned int uiWidth, unsigned int uiHeight)
{
    m_uiWidth  = uiWidth;
    m_uiHeight = uiHeight;

    m_uiAlignedWidth  = m_uiWidth;
    m_uiAlignedHeight = m_uiHeight;

    m_uiOutputWidth  = m_uiAlignedWidth;
    m_uiOutputHeight = m_uiAlignedHeight;

    // Set alignment in CSC.
    if (m_format == RF_RGBA8 || m_format == RF_ARGB8 || m_format == RF_BGRA8)
    {
        m_nBufferSize = m_uiAlignedWidth * m_uiAlignedHeight * 4;
    }
    else if (m_format == RF_NV12)
    {
        m_nBufferSize = (m_uiAlignedWidth * m_uiAlignedHeight) + (m_uiAlignedWidth * m_uiAlignedHeight) / 2;
    }
    else
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    return RF_STATUS_OK;
}


bool RFEncoderIdentity::isFormatSupported(RFFormat format) const
{
    if (format == RF_RGBA8 || format == RF_ARGB8 || format == RF_BGRA8 || format == RF_NV12)
    {
        return true;
    }

    return false;
}


RFStatus RFEncoderIdentity::getEncodedFrame(unsigned int& uiSize, void* &pBitStream)
{
    if (m_pBuffer)
    {
        uiSize = static_cast<unsigned int>(m_nBufferSize);
        pBitStream = m_pBuffer;
        m_pBuffer = nullptr;
    }
    else
    {
        return RF_STATUS_NO_ENCODED_FRAME;
    }

    return RF_STATUS_OK;
}


RFStatus RFEncoderIdentity::encode(unsigned int uiBufferIdx, bool bUseInputImage)
{
    assert(m_pContext);

    m_pContext->getResultBuffer(uiBufferIdx, m_pBuffer);

    if (!m_pBuffer)
    {
        RF_Error(RF_STATUS_INVALID_OPENCL_MEMOBJ, "Input pBuffer is invalid");
        return RF_STATUS_INVALID_OPENCL_MEMOBJ;
    }

    return RF_STATUS_OK;
}